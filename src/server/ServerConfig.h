#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace mcbridge {

class ServerConfig {
 public:
   uint16_t port;
   uint32_t inbound_interface;
};

} // namespace mcbridge
