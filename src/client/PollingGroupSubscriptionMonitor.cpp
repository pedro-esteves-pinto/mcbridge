#include "PollingGroupSubscriptionMonitor.h"
#include "common/common.h"

#include <algorithm>

namespace mcbridge {

std::set<EndPoint> PollingGroupSubscriptionMonitor::get_subscribed_groups() {
   return get_joined_groups();
}

} // namespace mcbridge
