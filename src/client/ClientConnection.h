#pragma once

#include <asio/ip/tcp.hpp>
#include <functional>
#include <memory>

namespace mcbridge {
struct EndPoint;
struct Message;

class ClientConnection : public std::enable_shared_from_this<ClientConnection> {
 public:
   using OnMessage = std::function<void(Message const &)>;
   using OnDisconnect = std::function<void()>;

   ClientConnection(asio::ip::tcp::socket &, OnMessage const &,
                    OnDisconnect const &);
   ~ClientConnection();
   void start();
   void on_timer();
   void join_group(EndPoint);
   void leave_group(EndPoint);
   void shutdown();

 private:
   void read_header();
   void read_payload();
   struct PImpl;
   std::unique_ptr<PImpl> me;
};

} // namespace mcbridge
