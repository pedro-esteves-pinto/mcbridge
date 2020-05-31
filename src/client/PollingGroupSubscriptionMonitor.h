#pragma once

#include "IGroupSubscriptionMonitor.h"

namespace mcbridge {

class PollingGroupSubscriptionMonitor : public IGroupSubscriptionMonitor {
public:
   std::set<EndPoint> get_subscribed_groups() override ;
};

}
