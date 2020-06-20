#pragma once

#include "../common/common.h"
#include <asio/io_service.hpp>
#include <functional>
#include <memory>

#include <string>

namespace mcbridge {

// Utility to receive multicast traffic for a specific group
// on a specific interface

class MCastReceiver : public std::enable_shared_from_this<MCastReceiver> {
 public:
   MCastReceiver(asio::io_service &io, EndPoint mc_group,
                 uint32_t interface_ip = 0);
   ~MCastReceiver();
   void start();
   void stop();
   std::function<void(std::string_view const &)> &on_receive();

 private:
   struct PImpl;
   std::unique_ptr<PImpl> me;
   void receive();
};

} // namespace mcbridge
