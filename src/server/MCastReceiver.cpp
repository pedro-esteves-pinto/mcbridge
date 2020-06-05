#include "MCastReceiver.h"

#include "../common/common.h"
#include <asio.hpp>

namespace mcbridge {

struct MCastReceiver::PImpl {
   PImpl(asio::io_service &io, EndPoint group)
       : socket(io), group(group), shutdown(false) {}
   asio::ip::udp::socket socket;
   asio::ip::udp::endpoint sender_endpoint;
   std::function<void(std::string_view const &)> on_bytes = [](auto &) {};
   std::array<char, (1 << 16) - 1> buffer;
   EndPoint group;
   bool shutdown;
   size_t n_packets;
};

MCastReceiver::MCastReceiver(asio::io_service &io, uint32_t listen_ip,
                             EndPoint group)
    : me(new PImpl(io, group)) {
   LOG(info) << "Starting multicast receiver for " << me->group;
   using namespace asio;

   // The interface (NIC) we are going to use
   auto listen_addr = ip::address_v4(listen_ip);
   auto listen_endpoint = ip::udp::endpoint(listen_addr, group.port);

   // The multicast group we are going to use
   auto mc_addr = ip::address_v4(group.ip);

   // Open the socket
   me->socket.open(listen_endpoint.protocol());
   me->socket.set_option(ip::udp::socket::reuse_address(true));
   me->socket.bind(ip::udp::endpoint(ip::address_v4::any(), group.port));

   // Join the group
   me->socket.set_option(ip::multicast::join_group(mc_addr, listen_addr));
}

void MCastReceiver::start() { receive(); }

void MCastReceiver::stop() {
   me->shutdown = true;
   me->on_bytes = [](auto) {};
}

void MCastReceiver::receive() {
   if (me->shutdown)
      return;
   auto self = shared_from_this();
   me->socket.async_receive_from(
       asio::buffer(me->buffer.data(), me->buffer.size()), me->sender_endpoint,
       [self, this](auto ec, auto bytes_recvd) {
          if (!ec) {
             LOG(info) << "Sending packet " << me->n_packets++ << " to " << me->group
                       << " first 64: " << (uint64_t*) me->buffer.data();
             me->on_bytes({me->buffer.data(), bytes_recvd});
             receive();
          } else
             LOG(info) << "Error reading multicast socket " << me->group;
       });
}

std::function<void(std::string_view const &)> &MCastReceiver::on_receive() {
   return me->on_bytes;
}

MCastReceiver::~MCastReceiver() {
   LOG(info) << "Destroying multicast receiver for " << me->group;
}

} // namespace mcbridge
