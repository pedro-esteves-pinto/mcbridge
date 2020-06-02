#pragma once

#include "ClientConfig.h"
#include <memory>

namespace mcbridge {
struct Message;

class Client {
 public:
   explicit Client(ClientConfig const &);
   ~Client();
   int run();

 private:
   enum class State;
   struct ConnectionRec;

   void update_joined_groups();
   void schedule_timer();
   void pause();
   void connect();
   void on_timer();
   void on_msg(Message const &);
   void on_disconnect();
   std::set<EndPoint> get_current_groups();

   struct PImpl;
   std::unique_ptr<PImpl> me;
};

} // namespace mcbridge
