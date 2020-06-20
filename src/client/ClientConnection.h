#pragma once

#include <asio/ip/tcp.hpp>
#include <functional>
#include <memory>

namespace mcbridge {
struct EndPoint;
struct Message;

// Manages the interaction with a mcbridge server, encoding join/leave
// requests and decoding and forwarding received multicast datagrams.

class ClientConnection : public std::enable_shared_from_this<ClientConnection> {
 public:
   using OnMCDatagram = std::function<void(Message const &)>;
   using OnDisconnect = std::function<void()>;

   ClientConnection(asio::ip::tcp::socket &, OnMCDatagram const &,
                    OnDisconnect const &);
   ~ClientConnection();
   void start();
   void on_timer();
   void join_group(EndPoint);
   void leave_group(EndPoint);

 private:
   void shutdown(asio::error_code);
   void read_header();
   void read_payload();
   struct PImpl;
   std::unique_ptr<PImpl> me;
};

} // namespace mcbridge
