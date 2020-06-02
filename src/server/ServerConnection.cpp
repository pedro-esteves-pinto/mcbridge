#include "ServerConnection.h"

#include "../common/common.h"
#include "GroupManager.h"
#include <asio.hpp>
#include <map>

namespace mcbridge {

struct ServerConnection::PImpl {
   PImpl(asio::io_service &io, asio::ip::tcp::socket &&socket,
         GroupManager &group_manager)
       : io_service(io), socket(std::move(socket)), buffer(),
         group_manager(group_manager), last_rcvd_hb(Timer::now()),
         last_sent_hb(Timer::now()), timer(io_service) {
      std::stringstream str;
      str << this->socket.remote_endpoint();
      remote_endpoint = str.str();
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
};

ServerConnection::ServerConnection(asio::io_service &io,
                                   asio::ip::tcp::socket &&socket,
                                   GroupManager &group_manager)
    : me(new PImpl(io, std::move(socket), group_manager)) {
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
         shutdown();
      } else {
         if (sec_diff(now, me->last_sent_hb) > 5) {
            me->last_sent_hb = Timer::now();
            Message m;
            m.header.end_point = {};
            m.header.payload_size = 0;
            m.header.type = MessageType::HB;
            auto self = shared_from_this();
            me->socket.async_send(asio::buffer(&m, sizeof(MessageHeader)),
                                  [this, self](auto ec, auto) {
                                     if (ec) {
                                        LOG(info) << "Error sending HB to"
                                                  << me->remote_endpoint;
                                        shutdown();
                                     }
                                  });
         }
         schedule_timer();
      }
   }
}

void ServerConnection::read() {
   auto self = shared_from_this();

   me->socket.async_receive(
       asio::buffer(&me->buffer.header, sizeof(MessageHeader)),
       [this, self](auto ec, auto) {
          if (!ec && on_msg(me->buffer.header)) {
             me->last_rcvd_hb = Timer::now();
             read();
          } else {
             LOG(info) << "Error reading from " << me->remote_endpoint;
             shutdown();
          }
       });
}

bool ServerConnection::on_msg(MessageHeader const &header) {
   if (header.payload_size != 0)
      return false;
   switch (header.type) {
   case MessageType::JOIN:
      return join(header.end_point);
   case MessageType::LEAVE:
      return leave(header.end_point);
   case MessageType::MC_DATAGRAM:
      return false;
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
      LOG(diag) << "Forwarding multicast datagram on " << m.header.end_point
                << " to " << me->remote_endpoint;
      auto self = shared_from_this();
      me->socket.async_send(
          asio::buffer(&m, sizeof(MessageHeader) + m.header.payload_size),
          [this, self](auto ec, auto) {
             if (ec) {
                LOG(info) << "Error sending datagram to "
                          << me->remote_endpoint;
                shutdown();
             }
          });
   }
}

void ServerConnection::shutdown() {
   if (me->socket.is_open()) {
      LOG(info) << "Closing connection to " << me->remote_endpoint;
      me->socket.close();
      for (auto &p : me->subscriptions)
         me->group_manager.remove_subscriber(p.second);
      me->subscriptions.clear();
   }
}

} // namespace mcbridge
