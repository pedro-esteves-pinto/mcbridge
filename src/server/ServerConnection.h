#pragma once

#include <asio/io_service.hpp>
#include <asio/ip/tcp.hpp>
#include <memory.h>

// fwds
namespace mcbridge {
struct MessageHeader;
struct Message;
struct EndPoint;
class GroupManager;
} // namespace mcbridge

namespace mcbridge {

// Manages a connection to a specific client, listening to join/leave
// requests and heartbeats as well as forwarding multicas traffic over
// the connection.

class ServerConnection : public std::enable_shared_from_this<ServerConnection> {
 public:
   ServerConnection(asio::io_service &, asio::ip::tcp::socket &&,
                    GroupManager &, uint32_t max_in_flight);
   ~ServerConnection();
   void start();

 private:
   void read();
   bool on_msg(MessageHeader const &);
   bool join(EndPoint const &);
   bool leave(EndPoint const &);
   void on_datagram(Message const &);
   void schedule_timer();
   void on_timer();
   void shutdown(asio::error_code);
   struct PImpl;
   std::unique_ptr<PImpl> me;
};

} // namespace mcbridge
