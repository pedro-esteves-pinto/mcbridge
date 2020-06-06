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

uint32_t resolve_interface_ip(std::string const &interface_name) {
   auto ip = get_interface_ip(interface_name);
   if (ip)
      return ip.value();
   else {
      LOG(fatal) << "Invalid interface name: " << interface_name;
      exit(-1);
   }
}

std::set<EndPoint> read_groups_from_file(std::string const &groups_file) {
   std::set<EndPoint> result;
   std::ifstream f(groups_file);
   while (!f.eof()) {
      std::string ip_port;
      f >> std::skipws >> ip_port;
      if (ip_port.size()) {
         EndPoint ep(ip_port);
         result.insert(ep);
      }
   }
   return result;
}

int main(int argc, char **argv) {
   using namespace mcbridge;
   CLI::App app{"Bridges multicast traffic over a TCP/IP connection"};
   app.require_subcommand(1);

   // common options
   mcbridge::log logging_verbosity = mcbridge::log::info;
   app.add_option("-l", "Logging verbosity (0 to 4)");

   uint16_t port = 30000;
   std::string interface;

   // client
   auto client_cmd = app.add_subcommand("client", "Acts a client");
   std::string server_addr;
   client_cmd->add_option("-a", server_addr, "Server address")->required();
   std::string groups_file;
   client_cmd
       ->add_option("-f", groups_file,
                    "Read multicast groups to join from file")
       ->check(CLI::ExistingFile);
   std::vector<std::string> groups;
   client_cmd->add_option("-g", groups,
                          "Multicast groups to join in the form ip:port");
   client_cmd->add_option("-p", port, "Server port", 30000);
   client_cmd->add_option("-i", interface,
                          "Interface to use for outbound multicast");

   // server
   auto server_cmd = app.add_subcommand("server", "Acts as a server");
   server_cmd->add_option("-p", port, "Port to bind to", 30000);
   server_cmd->add_option("-i", interface,
                          "Interface to use for inbound multicast");

   // test_send
   auto test_send = app.add_subcommand(
       "test_send", "Send a small multicast packet every few seconds");
   std::string group_s = "224.0.255.255:30000";
   test_send->add_option("-g", group_s, "Multicast group to send to",
                         "224.0.255.255:30000");
   test_send->add_option("-i", interface,
                         "Interface to use for outbound multicast");

   auto test_recv = app.add_subcommand(
       "test_recv", "Receive multicast and print a few bytes to screen");
   std::string group_c = "224.0.255.255:30000";
   test_recv->add_option("-g", group_c, "Multicast group to join",
                         "224.0.255.255:30000");
   test_recv->add_option("-i", interface,
                         "Interface to use for inbound multicast");

   CLI11_PARSE(app, argc, argv);

   set_log_level(logging_verbosity);

   if (*client_cmd) {
      ClientConfig cfg;
      if (interface.size())
         cfg.outbound_interface = resolve_interface_ip(interface);
      if (groups_file.size())
         cfg.joined_groups = read_groups_from_file(groups_file);
      for (auto &g : groups)
         cfg.joined_groups.insert({g});
      cfg.server_address = {resolve_host_name(server_addr), port};
      return Client{cfg}.run();
   } else if (*server_cmd) {
      ServerConfig cfg;
      cfg.port = port;
      if (interface.size())
         cfg.inbound_interface = resolve_interface_ip(interface);
      return Server{cfg}.run();
   } else if (*test_send) {
      auto interface_ip = 0;
      if (interface.size())
         interface_ip = resolve_interface_ip(interface);
      return Test::send(group_s, interface_ip);
   } else if (*test_recv) {
      auto interface_ip = 0;
      if (interface.size())
         interface_ip = resolve_interface_ip(interface);
      return Test::recv(group_c, interface_ip);
   }

   return -1;
}
