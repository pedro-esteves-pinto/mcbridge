#pragma once

#include <stdint.h>
#include <vector>
#include <string>

namespace mcbridge {

class ServerConfig {
public:
   ServerConfig(std::vector<std::string> const&) ;
   uint16_t port; 
};

}
