#pragma once

#include <memory>

namespace mcbridge {

struct EndPoint;

class Test {
public:
   static int send(EndPoint const&ep,uint32_t interface);
   static int recv(EndPoint const&ep,uint32_t interface);
};

}
