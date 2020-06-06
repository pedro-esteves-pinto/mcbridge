#include "client/Client.h"
#include "client/ClientConfig.h"
#include "common/common.h"
#include "server/Server.h"
#include "test/Test.h"

#include <CLI11/CLI11.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace mcbridge;

int main(int argc, char **argv) {
   CLI::App app{"Bridges multicast traffic over a TCP/IP connection"};
   app.require_subcommand(1);

   // common options
   mcbridge::log logging_verbosity = mcbridge::log::info;
   app.add_option("-l", logging_verbosity, "Logging verbosity (0 to 4)");

   // client
   auto client_cmd = app.add_subcommand("client", "Acts a client");
   std::string server_addr;
   client_cmd->add_option("-a", server_addr, "Server address")->required();
   std::string groups_file;
   client_cmd
       ->add_option("-f", groups_file,
                    "File containing multicast groups to join")
       ->check(CLI::ExistingFile);
   std::vector<std::string> groups;
   client_cmd->add_option("-g", groups, "Multicast groups to join");
   uint16_t server_port_c = 30000;
   client_cmd->add_option("-p", server_port_c, "Server port");

   // server
   auto server_cmd = app.add_subcommand("server", "Acts as a server");
   uint16_t server_port_s = 30000;
   server_cmd->add_option("-p", server_port_s, "Port to bind to");

   // test_send
   auto test_send = app.add_subcommand(
       "test_send", "Send a small multicast packet every few seconds");
   std::string group_s = "224.0.255.255:30000";
   test_send->add_option("-g", group_s, "Multicast group to send to");

   auto test_recv = app.add_subcommand(
       "test_recv", "Receive multicast and print a few bytes to screen");
   std::string group_c = "224.0.255.255:30000";
   test_recv->add_option("-g", group_c, "Multicast group to join");

   CLI11_PARSE(app, argc, argv);

   set_log_level(logging_verbosity);
   if (*client_cmd) {
      ClientConfig cfg;
      for (auto &g : groups)
         cfg.joined_groups.insert({g});
      if (groups_file.size()) {
         std::ifstream f(groups_file);
         while (!f.eof()) {
            std::string ip_port;
            f >> std::skipws >> ip_port;
            if (ip_port.size()) {
               EndPoint ep (ip_port);
               cfg.joined_groups.insert(ep);
            }
         }
      }
      cfg.server_address = {resolve_host_name(server_addr), server_port_c};
      return Client{cfg}.run();
   } else if (*server_cmd) {
      ServerConfig cfg;
      cfg.port = server_port_s;
      return Server{cfg}.run();
   } else if (*test_send) {
      return Test::send(group_s);
   } else if (*test_recv) {
      return Test::recv(group_c);
   }

   return -1;
}
