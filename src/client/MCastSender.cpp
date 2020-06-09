#include "MCastSender.h"

#include <asio.hpp>

namespace mcbridge {

struct MCastSender::PImpl {
   PImpl(asio::io_service &io, uint32_t max_in_flight)
      : socket(io), max_in_flight(max_in_flight) {

      // Setup our ougoing buffers ahead of time
      for (size_t i = 0; i < max_in_flight; i++) {
         all_buffers.push_back(std::make_unique<std::string>(1500, 0));
         free_buffers.push_back(all_buffers.back().get());
      }
   }

   asio::ip::udp::socket socket;
   uint32_t max_in_flight;
   asio::ip::udp::endpoint endpoint;

   std::vector<std::string *> free_buffers;
   std::vector<std::unique_ptr<std::string>> all_buffers;
};

MCastSender::MCastSender(asio::io_service &io, EndPoint mc_group,
                         uint32_t interface_ip, uint32_t max_in_flight)
    : me(new PImpl(io, max_in_flight)) {
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
   if (me->free_buffers.empty()) {
      LOG(warn) << "Cannot send multicast to " << me->endpoint << " fast enough. "
                << " Maximum of " << me->all_buffers.size() << " messages in flight reached,"
                << " dropping current message";
      return;
   }

   auto buffer = me->free_buffers.back();
   me->free_buffers.pop_back();
   *buffer = bytes;
   
   me->socket.async_send_to(
      asio::buffer(*buffer),
      me->endpoint, [this,buffer](auto es, auto) {
         me->free_buffers.push_back(buffer);
          if (es)
             LOG(error) << "Error sending to " << me->endpoint
                        << " error: " << es.message();
       });
}

} // namespace mcbridge
