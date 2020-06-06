#pragma once

#include "../common/common.h"
#include <set>
#include <string>
#include <vector>

namespace mcbridge {

class ClientConfig {
 public:
   bool poll_joined_groups;
   std::set<EndPoint> joined_groups;
   uint32_t outbound_interface;
   EndPoint server_address;
};

} // namespace mcbridge
