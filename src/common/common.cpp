#include "common.h"
#include <sstream>
#include <asio.hpp>
#include <fstream>

namespace mcbridge {

static enum log level = log::error;
enum log get_log_level() {return level;}
void set_log_level(log l) { level = l; }

Logger::DummyType Logger::dummy;

std::string to_quad(uint32_t ip) {
   std::stringstream str;
   str << ((ip & 0xff000000) >> 24) << "."
       << ((ip & 0x00ff0000) >> 16) << "."
       << ((ip & 0x0000ff00) >> 8) << "."
       << ((ip & 0x000000ff));
   return str.str();
}
uint32_t from_quad(std::string const&ip) {
   auto quads = split(ip,'.');
   return
      (std::stoi(quads[0]) << 24) |
      (std::stoi(quads[1]) << 16) |
      (std::stoi(quads[2]) << 8) |
      std::stoi(quads[3]);
}

std::ostream &operator<<(std::ostream &out, EndPoint const &ep) {
   out << to_quad(ep.ip) << ":" << ep.port;
   return out;
}

std::vector<std::string> split(const std::string& str, char delim)
{
   std::vector<std::string> result;
   enum class State {DELIM, TOKEN};
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
            state= State::DELIM;
         }
      }
      else {
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

uint32_t resolve_host_name(std::string const &host) {
   using namespace boost;
   asio::io_service ios;
   asio::ip::tcp::resolver::query resolver_query(host, "",
                                                 asio::ip::tcp::resolver::query::numeric_service);
  asio::ip::tcp::resolver resolver(ios);
  asio::error_code ec;
  asio::ip::tcp::resolver::iterator it =
    resolver.resolve(resolver_query, ec);
  if (ec) 
     return 0;
  else
     return it->endpoint().address().to_v4().to_uint();
}

std::set<EndPoint> get_joined_groups() {
   std::ifstream f("/proc/net/udp");
   std::string line;
   std::set<EndPoint> result;
   size_t line_c = 0;
   while (std::getline(f, line)) {
      if (line_c++) {
         auto fields = split(line, ' ');
         auto ip_port = split(fields[1],':');
         auto ip = std::stoul(ip_port[0],nullptr, 16);
         uint16_t port = std::stoul(ip_port[1],nullptr, 16);
         uint32_t network_order_ip = 0;
         network_order_ip = ((ip & 0x000000ff)<< 24);
         network_order_ip |= ((ip & 0x0000ff00) << 8);
         network_order_ip |= ((ip & 0x00ff0000) >> 8);
         network_order_ip |= ((ip & 0xff000000) >> 24);

         /*
         224.0.2.0 to 224.0.255.255	AD-HOC block 1
         224.3.0.0 to 224.4.255.255	AD-HOC block 2
         233.252.0.0 to 233.255.255.255	AD-HOC block 3
         */         
         if ((network_order_ip >= from_quad("224.0.2.0") &&
              network_order_ip <= from_quad("224.0.255.255")) ||
             (network_order_ip >= from_quad("224.3.0.0") &&
              network_order_ip <= from_quad("224.4.255.255")) ||
             (network_order_ip >= from_quad("233.252.0.0") &&
              network_order_ip <= from_quad("233.255.255.255"))) {
            result.insert({network_order_ip,port});
         }
      }
   }
   return result;
}

}

