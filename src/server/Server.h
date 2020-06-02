#pragma once

#include "ServerConfig.h"
#include <memory>

namespace mcbridge {

class Server {
 public:
   Server(ServerConfig const &);
   ~Server();
   int run();

 private:
   void accept();

   struct PImpl;
   std::unique_ptr<PImpl> me;
};

} // namespace mcbridge
