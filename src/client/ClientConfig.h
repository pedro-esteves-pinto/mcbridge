#pragma once

#include "../common/common.h"
#include <set>
#include <string>
#include <vector>

namespace mcbridge {

class ClientConfig {
 public:
   // Wheter we should scan /proc/net/udp to automatically discover
   // new multicast joins
   bool auto_discover_groups;

   // If auto discovering joins, filter out groups that do not match
   // this network mask.
   NetMask auto_discover_mask;

   // Static list of multicast groups to join
   std::set<EndPoint> groups_to_join;

   // Interface to use to publish multicast
   uint32_t outbound_interface;

   // Address of our server
   EndPoint server_address;

   // Maximum number of multicast messages we will allow in flight per
   // multicast group before we start dropping.
   uint32_t max_in_flight_datagrams_per_group = 1000;
};

} // namespace mcbridge
