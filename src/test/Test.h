#pragma once

#include <memory>

namespace mcbridge {

struct EndPoint;

class Test {
public:
   static void send(EndPoint const&ep);
   static void receive(EndPoint const&ep);
};

}
