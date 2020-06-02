#pragma once

#include "IGroupSubscriptionMonitor.h"

namespace mcbridge {

class StaticGroupSubscriptionMonitor : public IGroupSubscriptionMonitor {
 public:
   StaticGroupSubscriptionMonitor(std::set<EndPoint> const &g) : groups(g) {}
   std::set<EndPoint> get_subscribed_groups() override { return groups; }

 private:
   std::set<EndPoint> groups;
};

} // namespace mcbridge
