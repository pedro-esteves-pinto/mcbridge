#pragma once

#include <functional>
#include <memory>
#include <stdint.h>

// fwds
namespace mcbridge {
struct EndPoint;
struct Message;
} // namespace mcbridge
namespace asio {
class io_context;
}

namespace mcbridge {

using SubID = uint64_t;

class GroupManager {
 public:
   using OnMessage = std::function<void(Message const &)>;

   GroupManager(asio::io_context &, uint32_t interface);
   ~GroupManager();

   SubID add_subscriber(EndPoint, OnMessage const &);
   void remove_subscriber(SubID);

 private:
   struct PImpl;
   struct Subscriber;
   struct Group;
   std::unique_ptr<PImpl> me;
};

} // namespace mcbridge
