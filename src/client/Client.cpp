#include "Client.h"
#include "../common/common.h"
#include "ClientConnection.h"
#include "MCastSender.h"

#include <algorithm>
#include <asio.hpp>
#include <cassert>
#include <map>

namespace mcbridge {

enum class Client::State { PAUSED, CONNECTING, RUNNING };

struct Client::ConnectionRec {
   ConnectionRec(asio::ip::tcp::socket &s,
                 ClientConnection::OnMessage const &on_msg,
                 ClientConnection::OnDisconnect const &on_disc)
       : last_joined_group_scan(),
         connection(std::make_shared<ClientConnection>(s, on_msg, on_disc)),
         senders() {
      connection->start();
   }

   TimeStamp last_joined_group_scan;
   std::shared_ptr<ClientConnection> connection;
   std::map<EndPoint, MCastSender> senders;
};

struct Client::PImpl {
   PImpl(ClientConfig const &cfg)
       : cfg(cfg), state(State::PAUSED), start_of_pause(), io_service(1),
         socket(io_service), connection(), timer(io_service) {}

   ClientConfig cfg;
   State state;
   TimeStamp start_of_pause;
   asio::io_service io_service;
   asio::ip::tcp::socket socket;
   std::optional<ConnectionRec> connection;
   asio::steady_timer timer;
};

Client::Client(ClientConfig const &cfg) : me(new PImpl{cfg}) {}
Client::~Client() = default;

int Client::run() {
   me->state = State::PAUSED;
   me->start_of_pause = {};
   schedule_timer();
   return me->io_service.run();
}

void Client::schedule_timer() {
   me->timer.expires_from_now(std::chrono::seconds(1));
   me->timer.async_wait([this](auto) { on_timer(); });
}

void Client::on_timer() {
   schedule_timer();

   switch (me->state) {
   case State::CONNECTING:
      break;
   case State::PAUSED:
      if (sec_diff(Timer::now(), me->start_of_pause) > 10) {
         connect();
      }
      break;
   case State::RUNNING:
      if (me->cfg.auto_discover_groups &&
          sec_diff(Timer::now(),
                   me->connection.value().last_joined_group_scan) > 10) {
         me->connection.value().last_joined_group_scan = Timer::now();
         scan_for_new_joined_groups();
      }
      me->connection.value().connection->on_timer();
      break;
   }
}

void Client::connect() {
   me->start_of_pause = Timer::now();
   me->state = State::CONNECTING;

   using namespace asio;

   auto addr = ip::tcp::endpoint(ip::address_v4(me->cfg.server_address.ip),
                                 me->cfg.server_address.port);

   LOG(info) << "Connecting to " << addr;
   me->socket.async_connect(addr, [this](auto ec) {
      if (!ec) {
         LOG(info) << "Connection succeeded";
         me->state = State::RUNNING;
         me->connection.emplace(
             me->socket, [this](auto const &m) { on_msg(m); },
             [this]() { on_disconnect(); });
         auto &senders = me->connection.value().senders;
         for (auto ep : me->cfg.groups_to_join) {
            senders[ep] =
                MCastSender{me->io_service, ep, me->cfg.outbound_interface,
                            me->cfg.max_in_flight_datagrams_per_group};
            me->connection.value().connection->join_group(ep);
         }
      } else {
         LOG(info) << "Connection failed";
         me->start_of_pause = Timer::now();
         me->state = State::PAUSED;
      }
   });
}

std::set<EndPoint> Client::get_current_groups() {
   std::set<EndPoint> result;
   for (auto &p : me->connection.value().senders)
      result.insert(p.first);
   return result;
}

void Client::scan_for_new_joined_groups() {
   assert(me->state == State::RUNNING);

   auto new_groups = get_joined_groups(me->cfg.auto_discover_mask);
   auto old_groups = get_current_groups();

   std::set<EndPoint> to_add;
   std::set_difference(new_groups.begin(), new_groups.end(), old_groups.begin(),
                       old_groups.end(), std::inserter(to_add, to_add.begin()));

   auto &connection = me->connection.value().connection;
   auto &senders = me->connection.value().senders;
   for (auto ep : to_add) {
      senders[ep] = MCastSender{me->io_service, ep, me->cfg.outbound_interface,
                                me->cfg.max_in_flight_datagrams_per_group};
      connection->join_group(ep);
   }
}

void Client::on_msg(Message const &m) {
   switch (me->state) {
   case State::CONNECTING:
      assert(false);
      break;
   case State::PAUSED:
      break;
   case State::RUNNING: {
      auto &senders = me->connection.value().senders;
      auto it = senders.find(m.header.end_point);
      if (it != senders.end()) {
         it->second.send_bytes({m.payload.data(), m.header.payload_size});
      }
      else
         LOG(warn) << "Received message for unknown endpoint: " << m.header.end_point;
      break;
   }
   }
}

void Client::on_disconnect() {
   switch (me->state) {
   case State::CONNECTING:
      assert(false);
      break;
   case State::PAUSED:
      break;
   case State::RUNNING:
      me->connection.reset();
      me->state = State::PAUSED;
      me->start_of_pause = Timer::now();
      break;
   }
}

} // namespace mcbridge
