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

class ServerConnection : public std::enable_shared_from_this<ServerConnection> {
 public:
   ServerConnection(asio::io_service &, asio::ip::tcp::socket &&,
                    GroupManager &);
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
   void shutdown();
   struct PImpl;
   std::unique_ptr<PImpl> me;
};

} // namespace mcbridge
