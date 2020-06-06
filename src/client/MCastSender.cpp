#include "MCastSender.h"

#include <asio.hpp>

namespace mcbridge {

struct MCastSender::PImpl {
   PImpl(asio::io_service &io) : socket(io) {}
   asio::ip::udp::socket socket;
   asio::ip::udp::endpoint endpoint;
};

MCastSender::MCastSender(asio::io_service &io, EndPoint mc_group,
                         uint32_t interface_ip)
    : me(new PImpl(io)) {
   using namespace asio;
   me->endpoint = ip::udp::endpoint(ip::address_v4(mc_group.ip), mc_group.port);
   me->socket.open(me->endpoint.protocol());
   if (interface_ip) {
      asio::ip::multicast::outbound_interface option{
          asio::ip::address_v4(interface_ip)};
      me->socket.set_option(option);
   }
}

MCastSender::MCastSender() = default;
MCastSender::~MCastSender() = default;

MCastSender::MCastSender(MCastSender &&src) { me = std::move(src.me); }

MCastSender &MCastSender::operator=(MCastSender &&src) {
   me = std::move(src.me);
   return *this;
}

void MCastSender::send_bytes(std::string_view const &bytes) {
   me->socket.async_send_to(
       asio::buffer(bytes), me->endpoint, [this](auto es, auto) {
          if (es)
             LOG(error) << "Error sending to " << me->endpoint
                        << " error: " << es;
       });
}

} // namespace mcbridge
