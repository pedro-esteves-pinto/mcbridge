#pragma once

#include "ClientConfig.h"
#include <memory>

namespace mcbridge {
struct Message;

// Orchestrates the connection to a mcbridge server, the creation of
// new multicast senders and possibly the discovery of new joined
// groups.

class Client {
 public:
   explicit Client(ClientConfig const &);
   ~Client();
   int run();

 private:
   enum class State;
   struct ConnectionRec;

   void schedule_timer();
   void pause();
   void connect();
   void on_timer();
   void on_msg(Message const &);
   void on_disconnect();
   void scan_for_new_joined_groups();
   std::set<EndPoint> get_current_groups();

   struct PImpl;
   std::unique_ptr<PImpl> me;
};

} // namespace mcbridge
