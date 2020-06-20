#pragma once

#include "ServerConfig.h"
#include <memory>

namespace mcbridge {

// Listens on the server port, spawning a ServerConnection to handle
// each connection.
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
