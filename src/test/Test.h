#pragma once

#include <memory>

namespace mcbridge {

struct EndPoint;

class Test {
public:
   static int send(EndPoint const&ep);
   static int recv(EndPoint const&ep);
};

}
