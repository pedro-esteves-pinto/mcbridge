#pragma once

#include "../common/common.h"
#include <asio/io_service.hpp>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace mcbridge {

class MCastSender {
 public:
   MCastSender();
   MCastSender(MCastSender &&);
   MCastSender &operator=(MCastSender &&);
   MCastSender(asio::io_service &io, EndPoint destination, uint32_t interface, uint32_t max_in_flight);
   ~MCastSender();
   void send_bytes(std::string_view const &bytes);

 private:
   struct PImpl;
   std::unique_ptr<PImpl> me;
};

} // namespace mcbridge
