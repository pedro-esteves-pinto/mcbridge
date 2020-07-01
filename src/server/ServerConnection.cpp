#include "ServerConnection.h"

#include "../common/common.h"
#include "GroupManager.h"
#include <asio.hpp>
#include <map>

namespace mcbridge {

struct ServerConnection::PImpl {
   PImpl(asio::io_service &io, asio::ip::tcp::socket &&socket,
         GroupManager &group_manager, uint32_t max_in_flight_msgs)
       : io_service(io), socket(std::move(socket)), buffer(),
         group_manager(group_manager), last_rcvd_hb(Timer::now()),
         last_sent_hb(Timer::now()), timer(io_service) {

      // We print out our peer end point a lot when logging, this just
      // makes it easier to do so.
      std::stringstream str;
      str << this->socket.remote_endpoint();
      remote_endpoint = str.str();

      // Setup our ougoing buffers ahead of time
      for (size_t i = 0; i < max_in_flight_msgs; i++) {
         all_buffers.push_back(std::make_unique<Message>());
         free_buffers.push_back(all_buffers.back().get());
      }
   }

   asio::io_service &io_service;
   asio::ip::tcp::socket socket;
   Message buffer;
   GroupManager &group_manager;
   std::map<EndPoint, SubID> subscriptions;
   TimeStamp last_rcvd_hb;
   TimeStamp last_sent_hb;
   std::string remote_endpoint;
   asio::steady_timer timer;
   size_t sequence_number = 1;

   std::vector<Message *> free_buffers;
   std::vector<std::unique_ptr<Message>> all_buffers;
};

ServerConnection::ServerConnection(asio::io_service &io,
                                   asio::ip::tcp::socket &&socket,
                                   GroupManager &group_manager,
                                   uint32_t max_in_flight_msgs)
    : me(new PImpl(io, std::move(socket), group_manager, max_in_flight_msgs)) {
   LOG(info) << "Accepted connection from " << me->remote_endpoint;
}

ServerConnection::~ServerConnection() = default;

void ServerConnection::start() {
   schedule_timer();
   read();
}

void ServerConnection::schedule_timer() {
   auto self = shared_from_this();
   me->timer.expires_from_now(std::chrono::seconds(1));
   me->timer.async_wait([this, self](auto) { on_timer(); });
}

void ServerConnection::on_timer() {
   if (me->socket.is_open()) {
      auto now = Timer::now();
      if (sec_diff(now, me->last_rcvd_hb) > 15) {
         LOG(info) << "Connection " << me->remote_endpoint
                   << " has been idle for " << sec_diff(now, me->last_rcvd_hb)
                   << "s";
         shutdown({});
      } else {
         if (sec_diff(now, me->last_sent_hb) > 5) {
            me->last_sent_hb = Timer::now();
            auto m = std::make_unique<Message>();
            m->header.end_point = {};
            m->header.payload_size = 0;
            m->header.type = MessageType::HB;
            m->header.sequence_number = me->sequence_number++;
            auto ptr = m.get();
            auto self = shared_from_this();
            LOG(diag) << "Sending HB to " << me->remote_endpoint;
            me->socket.async_send(
                asio::buffer(ptr, sizeof(MessageHeader)),
                [this, self, msg = std::move(m)](auto ec, auto) {
                   if (ec) {
                      LOG(info)
                          << "Error sending HB to " << me->remote_endpoint;
                      shutdown(ec);
                   }
                });
         }
         schedule_timer();
      }
   }
}

void ServerConnection::read() {
   auto self = shared_from_this();

   asio::async_read(me->socket,
                    asio::buffer(&me->buffer.header, sizeof(MessageHeader)),
                    [this, self](auto ec, auto) {
                       if (!ec && on_msg(me->buffer.header)) {
                          me->last_rcvd_hb = Timer::now();
                          read();
                       } else {
                          LOG(info)
                              << "Error reading from " << me->remote_endpoint
                              << " error: " << ec.message();
                          shutdown(ec);
                       }
                    });
}

bool ServerConnection::on_msg(MessageHeader const &header) {
   if (header.payload_size != 0) {
      LOG(error) << "Received malformed message from " << me->remote_endpoint;
      return false;
   }
   switch (header.type) {
   case MessageType::JOIN:
      return join(header.end_point);
   case MessageType::LEAVE:
      return leave(header.end_point);
   case MessageType::MC_DATAGRAM: {
      LOG(error) << "Received unexpected message from " << me->remote_endpoint;
      return false;
   }
   case MessageType::HB:
      LOG(diag) << "Received HB from " << me->remote_endpoint;
      return true;
   }
   return false;
}

bool ServerConnection::join(EndPoint const &end_point) {
   if (me->subscriptions.count(end_point)) {
      LOG(info) << "Already joined " << end_point << " for "
                << me->remote_endpoint;
      return false;
   } else {
      LOG(info) << "Joining " << end_point << " for " << me->remote_endpoint;
      me->subscriptions[end_point] = me->group_manager.add_subscriber(
          end_point, [this](auto const &m) { on_datagram(m); });
      return true;
   }
}

bool ServerConnection::leave(EndPoint const &end_point) {
   auto it = me->subscriptions.find(end_point);
   if (it == me->subscriptions.end()) {
      LOG(info) << "Already left " << end_point << " for "
                << me->remote_endpoint;
      return false;
   } else {
      LOG(info) << "Leaving " << end_point << " for " << me->remote_endpoint;
      me->group_manager.remove_subscriber(it->second);
      me->subscriptions.erase(it);
      return true;
   }
}

void ServerConnection::on_datagram(Message const &m) {
   if (me->socket.is_open()) {

      if (me->free_buffers.empty()) {
         LOG(warn) << "Cannot forward multicast to " << me->remote_endpoint
                   << " fast enough. "
                   << " Maximum of " << me->all_buffers.size()
                   << " messages in flight reached,"
                   << " dropping current message";
         return;
      }

      auto buffer = me->free_buffers.back();
      me->free_buffers.pop_back();
      *buffer = m;
      buffer->header.sequence_number = me->sequence_number++;

      me->last_sent_hb = Timer::now();
      auto self = shared_from_this();

      LOG(diag) << "About to forward msg " << buffer->header.sequence_number
                << " to " << me->remote_endpoint
                << " group: " << buffer->header.end_point
                << " size: " << buffer->header.payload_size
                << " first 64: " << *(uint64_t *)buffer->payload.data();

      me->socket.async_send(
          asio::buffer(buffer,
                       sizeof(MessageHeader) + buffer->header.payload_size),
          [this, self, buffer](auto ec, auto) {
             me->free_buffers.push_back(buffer);

             LOG(diag) << "Fisnished forwarding datagram "
                       << buffer->header.sequence_number << " to "
                       << me->remote_endpoint
                       << " group: " << buffer->header.end_point
                       << " size: " << buffer->header.payload_size
                       << " first 64: " << *(uint64_t *)buffer->payload.data();
             if (ec) {
                LOG(info) << "Error sending datagram to "
                          << me->remote_endpoint;
                shutdown(ec);
             }
          });
   }
}

void ServerConnection::shutdown(asio::error_code ec) {
   if (me->socket.is_open()) {
      LOG(info) << "Closing connection to " << me->remote_endpoint
                << " error: " << ec.message();
      me->socket.close();
      for (auto &p : me->subscriptions)
         me->group_manager.remove_subscriber(p.second);
      me->subscriptions.clear();
   }
}

} // namespace mcbridge
