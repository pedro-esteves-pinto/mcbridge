#include "Test.h"
#include "../common/common.h"
#include <asio.hpp>
#include "../client/MCastSender.h"
#include "../server/MCastReceiver.h"

namespace mcbridge {

struct TestSender {
   TestSender(EndPoint const&ep,uint32_t interface) :
      io_service(1), timer(io_service),
      sender(io_service,ep,interface)
   {
      schedule_timer();
   }

   void schedule_timer() {
      timer.expires_from_now(std::chrono::seconds(1));
      timer.async_wait([this](auto) { on_timer(); });
   }

   void on_timer() {
      n_packets++;
      n_packets_buffer = n_packets;
      LOG(info) << "Sending test packet. First 64 bytes: " << n_packets;
      sender.send_bytes ({(char*) &n_packets_buffer,sizeof (n_packets_buffer)});
      schedule_timer();
   }

   int run() {
      return io_service.run();
   }

   asio::io_service io_service;
   asio::steady_timer timer;
   MCastSender sender;
   size_t n_packets = 0;
   size_t n_packets_buffer = n_packets;
};

struct TestReceiver {
   TestReceiver(EndPoint const&ep,uint32_t interface) :
      io_service(1),
      receiver(std::make_shared<MCastReceiver> (io_service,ep,interface))
   {
      receiver->on_receive() = [] (auto const&data) {
         assert(data.size() == sizeof(size_t));
         LOG(info) << "Received test packet. First 64 bytes: " << *((uint64_t*) data.data());
      };
   }

   int run () {
      receiver->start();
      return io_service.run();
   }
   
   asio::io_service io_service;
   std::shared_ptr<MCastReceiver> receiver;
};


int Test::send(EndPoint const&ep, uint32_t interface) {
   TestSender sender(ep, interface);
   return sender.run();
}

int Test::recv(EndPoint const&ep,uint32_t interface) {
   TestReceiver recv(ep,interface);
   return recv.run();
}

}
