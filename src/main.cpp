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

   mcbridge::log logging_verbosity = mcbridge::log::info;
   uint16_t port = 30000;
   std::string interface;
   uint32_t max_in_flight_datagrams = 1000;

   // client
   auto client_cmd = app.add_subcommand("client", "Acts a client");
   std::string server_addr;
   client_cmd->add_option("-a,--server-address", server_addr, "Server address")
       ->required();

   client_cmd->add_option("-l,--logging-verbosity", logging_verbosity,
                          "Logging verbosity (0 to 4)");

   bool auto_discover_groups = false;
   client_cmd->add_flag("-d,--discover-groups", auto_discover_groups,
                        "Automatically discover joined groups");

   std::string auto_discover_mask;
   client_cmd->add_option("-m,--discover-network-mask", auto_discover_mask,
                          "Filter out discovered groups that do not match "
                          "network mask. Masks are of the form ip/mask.");

   std::string groups_file;
   client_cmd
       ->add_option("-f,--groups-file", groups_file,
                    "Read multicast groups to join from file")
       ->check(CLI::ExistingFile);
   std::vector<std::string> groups;
   client_cmd->add_option("-g,--group", groups,
                          "Multicast groups to join in the form ip:port");
   client_cmd->add_option("-p,--server-port", port, "Server port", 30000);
   client_cmd->add_option("-i,--interface", interface,
                          "Interface to use for outbound multicast");
   client_cmd->add_option(
       "-x,--max-in-flight-datagrams", max_in_flight_datagrams,
       "Maximum number of inflight datagrams per multicast group", 1000);

   // server
   auto server_cmd = app.add_subcommand("server", "Acts as a server");
   server_cmd->add_option("-l,--logging-verbosity", logging_verbosity,
                          "Logging verbosity (0 to 4)");
   server_cmd->add_option("-p,--port", port, "Port to bind to", 30000);
   server_cmd->add_option("-i,--interface", interface,
                          "Interface to use for inbound multicast");
   server_cmd->add_option(
       "-x,--max-in-flight-datagrams", max_in_flight_datagrams,
       "Maximum number of inflight datagrams per client", 5000);
   // test_send
   auto test_send = app.add_subcommand(
       "test_send", "Send a small multicast packet every few seconds");
   std::string group_s = "224.0.255.255:30000";
   test_send->add_option("-g,--group", group_s, "Multicast group to send to",
                         "224.0.255.255:30000");
   test_send->add_option("-i,--interface", interface,
                         "Interface to use for outbound multicast");

   auto test_recv = app.add_subcommand(
       "test_recv", "Receive multicast and print a few bytes to screen");
   std::string group_c = "224.0.255.255:30000";
   test_recv->add_option("-g,--group", group_c, "Multicast group to join",
                         "224.0.255.255:30000");
   test_recv->add_option("-i,--interface", interface,
                         "Interface to use for inbound multicast");

   CLI11_PARSE(app, argc, argv);

   set_log_level(logging_verbosity);

   auto interface_ip = 0;
   if (interface.size())
      interface_ip = resolve_interface_ip(interface);

   if (*client_cmd) {
      ClientConfig cfg;
      cfg.auto_discover_groups = auto_discover_groups;
      if (auto_discover_mask.size()) {
         auto nm = parse_net_mask(auto_discover_mask);
         if (!nm) {
            LOG(fatal) << "Invalid network mask: " << auto_discover_mask;
            exit(-1);
         } else
            cfg.auto_discover_mask = nm.value();
      }

      cfg.outbound_interface = interface_ip;
      if (groups_file.size())
         cfg.groups_to_join = read_groups_from_file(groups_file);
      for (auto &g : groups)
         cfg.groups_to_join.insert({g});
      cfg.server_address = {resolve_host_name(server_addr), port};
      cfg.max_in_flight_datagrams_per_group = max_in_flight_datagrams;
      return Client{cfg}.run();
   } else if (*server_cmd) {
      ServerConfig cfg;
      cfg.port = port;
      cfg.inbound_interface = interface_ip;
      cfg.max_in_flight_datagrams_per_connection = max_in_flight_datagrams;
      return Server{cfg}.run();
   } else if (*test_send)
      return Test::send(group_s, interface_ip);
   else if (*test_recv)
      return Test::recv(group_c, interface_ip);

   return -1;
}
