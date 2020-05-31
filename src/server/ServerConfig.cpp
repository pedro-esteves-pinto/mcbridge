#include "ServerConfig.h"
#include "../common/common.h"

namespace mcbridge {

ServerConfig::ServerConfig(std::vector<std::string> const&args) {
   if (args.size() > 2)
      port = std::stoi(args[2]);
   else
      port = default_port();
}

}
