#include "common.h"
#include <asio.hpp>
#include <fstream>
#include <sstream>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
namespace mcbridge {

static enum log level = log::error;
enum log get_log_level() { return level; }
void set_log_level(log l) { level = l; }

Logger::DummyType Logger::dummy;

std::string to_quad(uint32_t ip) {
   std::stringstream str;
   str << ((ip & 0xff000000) >> 24) << "." << ((ip & 0x00ff0000) >> 16) << "."
       << ((ip & 0x0000ff00) >> 8) << "." << ((ip & 0x000000ff));
   return str.str();
}
uint32_t from_quad(std::string const &ip) {
   auto quads = split(ip, '.');
   if (quads.size()!=4)
      throw std::runtime_error("Invalid ip");
   return (std::stoi(quads[0]) << 24) | (std::stoi(quads[1]) << 16) |
          (std::stoi(quads[2]) << 8) | std::stoi(quads[3]);
}

std::ostream &operator<<(std::ostream &out, EndPoint const &ep) {
   out << to_quad(ep.ip) << ":" << ep.port;
   return out;
}

std::istream &operator>>(std::istream &in, EndPoint &ep) {
   std::string ip_port;
   in >> std::skipws >> ip_port;
   if (in.bad())
      throw std::runtime_error("Invalid endpoint");
   
   auto fields = split(ip_port, ':');
   if (fields.size() != 2)
      throw std::runtime_error("Invalid endpoint");
   ep.ip = from_quad(fields[0]);
   ep.port = std::stoi(fields[1]);
   return in;
}

EndPoint::EndPoint(std::string const &str) {
   std::stringstream s(str);
   s >> *this;
}

std::vector<std::string> split(const std::string &str, char delim) {
   std::vector<std::string> result;
   enum class State { DELIM, TOKEN };
   auto state = State::DELIM;

   for (auto c : str) {
      if (c == delim) {
         if (state == State::DELIM)
            // Was consuming a delimiter and got another one, discard
            // it
            continue;
         else {
            // Was consuming a token and just spotted a delimiter,
            // switch states.
            state = State::DELIM;
         }
      } else {
         if (state == State::DELIM) {
            // was consuming delimiters and just spotted  the
            // beggining of a token, create a new entry
            // in the token vector and switch states
            result.push_back({});
            state = State::TOKEN;
         }
         result.back().push_back(c);
      }
   }
   return result;
}

std::optional<NetMask> parse_net_mask(std::string const &mask) {
   NetMask result;
   auto ip_mask = split(mask, '/');
   if (ip_mask.size() != 2)
      return {};
   else {
      uint32_t mask_bits = atoi(ip_mask[1].c_str());
      result.mask = 0;
      for (uint32_t i = 0; i < mask_bits; i++) {
         uint32_t this_bit = (1 << (31 - i));
         result.mask |= this_bit;
      }
      uint32_t address = from_quad(ip_mask[0]);
      result.ip = (address & result.mask);
      return result;
   }
}

uint32_t resolve_host_name(std::string const &host) {
   using namespace boost;
   asio::io_service ios;
   asio::ip::tcp::resolver::query resolver_query(
       host, "", asio::ip::tcp::resolver::query::numeric_service);
   asio::ip::tcp::resolver resolver(ios);
   asio::error_code ec;
   asio::ip::tcp::resolver::iterator it = resolver.resolve(resolver_query, ec);
   if (ec)
      return 0;
   else
      return it->endpoint().address().to_v4().to_uint();
}

std::set<EndPoint> get_joined_groups(NetMask filter) {
   std::ifstream f("/proc/net/udp");
   std::string line;
   std::set<EndPoint> result;
   size_t line_c = 0;
   while (std::getline(f, line)) {
      if (line_c++) {
         auto fields = split(line, ' ');
         auto ip_port = split(fields[1], ':');
         auto ip = std::stoul(ip_port[0], nullptr, 16);
         uint16_t port = std::stoul(ip_port[1], nullptr, 16);
         uint32_t network_order_ip = 0;
         network_order_ip = ((ip & 0x000000ff) << 24);
         network_order_ip |= ((ip & 0x0000ff00) << 8);
         network_order_ip |= ((ip & 0x00ff0000) >> 8);
         network_order_ip |= ((ip & 0xff000000) >> 24);
         /*
         224.0.2.0 to 224.0.255.255	AD-HOC block 1
         224.3.0.0 to 224.4.255.255	AD-HOC block 2
         233.0.0.0 to 233.251.255.255	GLOP addressing
         233.252.0.0 to 233.255.255.255	AD-HOC block 3
         */
         if (filter.is_match(network_order_ip) &&

             ((network_order_ip >= from_quad("224.0.2.0") &&
               network_order_ip <= from_quad("224.0.255.255")) ||
              (network_order_ip >= from_quad("224.3.0.0") &&
               network_order_ip <= from_quad("224.4.255.255")) ||
              (network_order_ip >= from_quad("233.0.0.0") &&
               network_order_ip <= from_quad("233.251.255.255")) ||
              (network_order_ip >= from_quad("233.252.0.0") &&
               network_order_ip <= from_quad("233.255.255.255"))))

            result.insert({network_order_ip, port});
      }
   }
   return result;
}

std::optional<uint32_t> get_interface_ip(std::string const &interface) {
   struct ifaddrs *ifaddr, *ifa;
   int family, s;
   char host[NI_MAXHOST];

   std::optional<uint32_t> result;

   if (getifaddrs(&ifaddr) == -1)
      return {};

   for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
      if (ifa->ifa_addr == NULL)
         continue;

      family = ifa->ifa_addr->sa_family;

      if (family == AF_INET) {
         s = getnameinfo(ifa->ifa_addr,
                         (family == AF_INET) ? sizeof(struct sockaddr_in)
                                             : sizeof(struct sockaddr_in6),
                         host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
         if (s != 0)
            break;

         if (interface == ifa->ifa_name) {
            result = from_quad(std::string{host});
            break;
         }
      }
   }

   freeifaddrs(ifaddr);
   return result;
}

} // namespace mcbridge
