#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace mcbridge {

class ServerConfig {
 public:
   uint16_t port;
   uint32_t inbound_interface;
   uint32_t max_in_flight_datagrams_per_connection =1000;
};

} // namespace mcbridge
