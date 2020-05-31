#pragma once

#include "../common/common.h"
#include <asio/io_service.hpp>
#include <functional>
#include <memory>
#include <string>

namespace mcbridge {

class MCastReceiver : public std::enable_shared_from_this<MCastReceiver> {
 public:
   MCastReceiver(asio::io_service &io, uint32_t listen_ip,
                 EndPoint mc_group);
   ~MCastReceiver();
   void start();
   std::function<void(std::string_view const &)> &on_receive();

 private:
   struct PImpl;
   std::unique_ptr<PImpl> me;
   void receive();
};

} // namespace mcbridge
