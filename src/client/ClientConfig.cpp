#include "ClientConfig.h"
#include "../common/common.h"
#include <assert.h>

namespace mcbridge {

EndPoint get_server_endpoint(std::vector<std::string> const &args) {
   assert(args.size() > 2);
   auto ip = resolve_host_name(args[2]);
   uint16_t port = default_port();
   if (args.size() > 3)
      port = std::stoi(args[3]);
   return {ip, port};
}

ClientConfig::ClientConfig(std::vector<std::string> const &args)
    : poll_joined_groups(true), joined_groups(),
      server_address(get_server_endpoint(args)) {
}

} // namespace mcbridge
