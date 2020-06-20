#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace mcbridge {

class ServerConfig {
 public:
   // port server listens on
   uint16_t port;

   // network interface to use for multicast
   uint32_t inbound_interface;

   // The maximumm number of multicast datagrams the server will push
   // through a single connection before starting to drop messages.
   uint32_t max_in_flight_datagrams_per_connection = 1000;
};

} // namespace mcbridge
