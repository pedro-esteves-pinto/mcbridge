#pragma once

#include "../common/common.h"
#include <set>

namespace mcbridge {

class IGroupSubscriptionMonitor {
public:
   virtual std::set<EndPoint> get_subscribed_groups() = 0;
   virtual ~IGroupSubscriptionMonitor() {}
};

}
