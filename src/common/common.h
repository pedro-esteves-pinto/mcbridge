#pragma once

#include <array>
#include <chrono>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <stdint.h>
#include <vector>

namespace mcbridge {

inline uint16_t default_port() { return 30000; }

using Timer = std::chrono::system_clock;
using TimeStamp = Timer::time_point;
using Duration = Timer::duration;

inline int64_t sec_diff(TimeStamp t1, TimeStamp t2) {
   return std::chrono::duration_cast<std::chrono::seconds>(t1 - t2).count();
}

enum class MessageType { JOIN, LEAVE, HB, MC_DATAGRAM };


struct EndPoint {
   uint32_t ip;
   uint16_t port;

   EndPoint(std::string const&);
   EndPoint (uint32_t ip, uint16_t port) : ip(ip), port(port) {}
   EndPoint () : ip(0), port(0) {}
   
   bool operator<(EndPoint const &rhs) const {
      if (ip != rhs.ip)
         return ip < rhs.ip;
      else
         return port < rhs.port;
   }
};

std::ostream &operator<<(std::ostream &out, EndPoint const &);
std::istream &operator>>(std::istream &in, EndPoint &);

uint32_t resolve_host_name(std::string const &);
std::set<EndPoint> get_joined_groups();
std::string to_quad(uint32_t ip);
uint32_t from_quad(std::string const &);

std::vector<std::string> split(const std::string &str, char delim);

struct MessageHeader {
   MessageType type;
   EndPoint end_point;
   uint16_t payload_size;
};

struct Message {
   MessageHeader header;
   std::array<char, std::numeric_limits<uint16_t>::max()> payload;
};

enum class log { diag, info, warn, error, fatal };

void set_log_level(log);
log get_log_level();

class Logger {
 public:
   Logger() : _msg() {}
   ~Logger() { std::cout << _msg.str() << std::endl; }

   template <class T> Logger &operator<<(const T &v) {
      _msg << v;
      return *this;
   }

   // used to support LOG macro
   struct DummyType {
      bool operator==(const DummyType &) const { return false; }
   };
   operator DummyType() { return dummy; }
   static DummyType dummy;

 private:
   std::ostringstream _msg;
};

#define LOG(LEVEL) (log::LEVEL >= get_log_level()) && Logger::dummy == Logger()

} // namespace mcbridge
