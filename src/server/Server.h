#pragma once

#include <memory>
#include "ServerConfig.h"

namespace mcbridge {

class Server {
public:
   Server(ServerConfig const&);
   ~Server();
   int run();
private:
   void accept();
   
   struct PImpl;
   std::unique_ptr<PImpl> me;
};

}
