#include "client/Client.h"
#include "client/ClientConfig.h"
#include "common/common.h"
#include "server/Server.h"

#include <iostream>
#include <vector>
#include <string>


int help() {
   std::cerr << "mcbridge client|server\n";
   return 1;
}

int main(int argc, char **argv) {
   using namespace mcbridge;

   set_log_level(log::diag);
   std::vector<std::string> args;
   std::copy(argv, argv + argc, std::back_inserter(args));

   if (argc < 2)
      return help();
   else if (args[1] == "client") {
      Client client(ClientConfig{args});
      return client.run();
   } else if (args[1] == "server") {
      Server server(ServerConfig{args});
      return server.run();
   } else
      return help();

   std::string s;
}
