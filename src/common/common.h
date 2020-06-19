#pragma once

#include <array>
#include <chrono>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <stdint.h>
#include <vector>
#include <cstring>
#include <iomanip>
#include <ctime>
 
namespace mcbridge {

inline uint16_t default_port() { return 30000; }

using Timer = std::chrono::system_clock;
using TimeStamp = Timer::time_point;
using Duration = Timer::duration;

inline int64_t sec_diff(TimeStamp t1, TimeStamp t2) {
   return std::chrono::duration_cast<std::chrono::seconds>(t1 - t2).count();
}

enum class MessageType { JOIN, LEAVE, HB, MC_DATAGRAM };


struct NetMask {
   uint32_t ip = 0;
   uint32_t mask = 0;
   bool is_match(uint32_t address) {return (address & mask) == ip;}
};

std::optional<NetMask> parse_net_mask(std::string const&);

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
std::optional<uint32_t> get_interface_ip(std::string const &interface);

std::set<EndPoint> get_joined_groups(NetMask filter);
std::string to_quad(uint32_t ip);
uint32_t from_quad(std::string const &);

std::vector<std::string> split(const std::string &str, char delim);

struct MessageHeader {
   MessageType type;
   EndPoint end_point;
   size_t sequence_number;
   uint16_t payload_size;
};

struct Message {
   MessageHeader header;
   std::array<char, std::numeric_limits<uint16_t>::max()> payload;
   Message &operator=(Message const&other) {
      header = other.header;
      if (header.type == MessageType::MC_DATAGRAM) {
         memcpy(payload.data(), other.payload.data(), other.header.payload_size);
      }
      return *this;
   }
};

enum class log {
   fatal =0,
   error,
   warn,
   info,
   diag
};

void set_log_level(log);
log get_log_level();

class Logger {
public:
   Logger() : _msg() {}
   ~Logger() {
      tm localTime;
      auto t = Timer::now();
      time_t now = Timer::to_time_t(t);
      localtime_r(&now, &localTime);

      const std::chrono::duration<double> tse = t.time_since_epoch();
      std::chrono::seconds::rep milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(tse).count() % 1000;

      std::cout << std::setfill('0') << std::setw(2) << localTime.tm_hour << ':'
                << std::setfill('0') << std::setw(2) << localTime.tm_min << ':'
                << std::setfill('0') << std::setw(2) << localTime.tm_sec << '.'
                << std::setfill('0') << std::setw(3) << milliseconds
                << ": " << _msg.str() << std::endl;
   }

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

#define LOG(LEVEL) (log::LEVEL <= get_log_level()) && Logger::dummy == Logger()

} // namespace mcbridge
