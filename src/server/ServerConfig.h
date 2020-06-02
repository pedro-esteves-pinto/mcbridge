#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace mcbridge {

class ServerConfig {
 public:
   ServerConfig(std::vector<std::string> const &);
   uint16_t port;
};

} // namespace mcbridge
