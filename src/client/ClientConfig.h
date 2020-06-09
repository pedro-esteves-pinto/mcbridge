#pragma once

#include "../common/common.h"
#include <set>
#include <string>
#include <vector>

namespace mcbridge {

class ClientConfig {
 public:
   bool auto_discover_groups;
   NetMask auto_discover_mask;
   std::set<EndPoint> groups_to_join;
   uint32_t outbound_interface;
   EndPoint server_address;
   uint32_t max_in_flight_datagrams_per_group = 1000;
};

} // namespace mcbridge
