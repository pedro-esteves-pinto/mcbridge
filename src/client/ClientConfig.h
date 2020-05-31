#pragma once

#include <vector>
#include <string>
#include <set>
#include "../common/common.h"

namespace mcbridge {

class ClientConfig {
 public:
   ClientConfig (std::vector<std::string> const&); 
   const bool poll_joined_groups;
   const std::set<EndPoint> joined_groups;
   const EndPoint server_address;
};

}
