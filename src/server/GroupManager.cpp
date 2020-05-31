#include "GroupManager.h"
#include "../common/common.h"
#include "MCastReceiver.h"

#include <map>
#include <vector>

namespace mcbridge {

struct GroupManager::Subscriber {
   SubID sub_id;
   OnMessage callback;
};

struct GroupManager::Group {
   Group(asio::io_context &ctx, EndPoint ep)
      : receiver(new MCastReceiver(ctx, 0, ep)) {
      receiver->start();
   }
   std::shared_ptr<MCastReceiver> receiver;
   std::vector<GroupManager::Subscriber> subscribers;
};

struct GroupManager::PImpl {
   asio::io_context &ctx;
   std::map<EndPoint, std::unique_ptr<Group>> groups;
   size_t next_sub_id = 1;
};

GroupManager::GroupManager(asio::io_context &ctx)
    : me(new PImpl{ctx, {}}) {}

GroupManager::~GroupManager() {}

SubID GroupManager::add_subscriber(EndPoint ep, OnMessage const &cb) {
   auto &g = me->groups[ep];
   if (!g) {
      g = std::make_unique<Group>(me->ctx, ep);
      g->receiver->on_receive() = [&subs = g->subscribers, ep](auto &bytes) {
         Message m;
         m.header.type = MessageType::MC_DATAGRAM;
         m.header.end_point = ep;
         m.header.payload_size = bytes.size();
         assert(bytes.size() < sizeof(m.payload));
         memcpy(m.payload.data(), bytes.data(), bytes.size());
         for (auto &sub : subs)
            sub.callback(m);
      };
   }
   g->subscribers.push_back({me->next_sub_id, cb});
   return me->next_sub_id++;
}

void GroupManager::remove_subscriber(SubID id) {
   for (auto &[end_point, group] : me->groups) {
      for (auto sub_it = group->subscribers.begin();
           sub_it != group->subscribers.end(); sub_it++) {
         if (sub_it->sub_id == id) {
            group->subscribers.erase(sub_it);
            if (group->subscribers.empty()) {
               group->receiver->on_receive() = [] (auto const&) {};
               me->groups.erase(end_point);
               return;
            }
         }
      }
   }
}

} // namespace mcbridge
