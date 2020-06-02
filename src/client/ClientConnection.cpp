#include "ClientConnection.h"
#include "../common/common.h"

namespace mcbridge {

struct ClientConnection::PImpl {
   asio::ip::tcp::socket &socket;
   OnMessage on_message;
   OnDisconnect on_disconnect;
   Message buffer;
   TimeStamp last_sent_hb;
   TimeStamp last_rcvd_hb;
};

ClientConnection::ClientConnection(asio::ip::tcp::socket &s,
                                   OnMessage const &on_message,
                                   OnDisconnect const &on_disconnect)

    : me(new PImpl{
          s, on_message, on_disconnect, {}, Timer::now(), Timer::now()}) {}

ClientConnection::~ClientConnection() = default;

void ClientConnection::start() { read_header(); }

void ClientConnection::on_timer() {
   if (me->socket.is_open()) {
      if (sec_diff(Timer::now(), me->last_rcvd_hb) > 15) {
         LOG(info) << "Closing idle connection to "
                   << me->socket.remote_endpoint();
         shutdown();
      } else if (sec_diff(Timer::now(), me->last_sent_hb) > 5) {
         Message m;
         m.header.end_point = {};
         m.header.type = MessageType::HB;
         m.header.payload_size = 0;
         me->last_sent_hb = Timer::now();
         LOG(diag) << "Sending HB to " << me->socket.remote_endpoint();
         auto self = shared_from_this();
         me->socket.async_send(asio::buffer(&m, sizeof(MessageHeader)),
                               [this, self](auto ec, auto) {
                                  if (ec)
                                     shutdown();
                               });
      }
   }
}

void ClientConnection::join_group(EndPoint ep) {
   Message m;
   m.header.end_point = ep;
   m.header.type = MessageType::JOIN;
   me->last_sent_hb = Timer::now();
   LOG(info) << "Joining group " << ep;
   auto self = shared_from_this();
   me->socket.async_send(asio::buffer(&m, sizeof(MessageHeader)),
                         [this, self](auto ec, auto) {
                            if (ec)
                               shutdown();
                         });
}

void ClientConnection::leave_group(EndPoint ep) {
   Message m;
   m.header.end_point = ep;
   m.header.type = MessageType::LEAVE;
   me->last_sent_hb = Timer::now();
   LOG(info) << "Leaving group " << ep;
   auto self = shared_from_this();
   me->socket.async_send(asio::buffer(&m, sizeof(MessageHeader)),
                         [this, self](auto ec, auto) {
                            if (ec)
                               shutdown();
                         });
}

void ClientConnection::read_header() {
   auto self = shared_from_this();
   me->socket.async_receive(
       asio::buffer(&me->buffer.header, sizeof(MessageHeader)),
       [this, self](auto ec, auto) {
          if (!ec) {
             me->last_rcvd_hb = Timer::now();
             if (me->buffer.header.type == MessageType::MC_DATAGRAM)
                read_payload();
             else
                read_header();
          } else {
             LOG(diag) << "Error reading header " << ec;
             shutdown();
          }
       });
}

void ClientConnection::read_payload() {
   auto self = shared_from_this();
   me->socket.async_receive(
       asio::buffer(&me->buffer.payload, me->buffer.header.payload_size),
       [this, self](auto ec, auto count) {
          if (!ec) {
             LOG(diag) << "Received " << count << " bytes datagram for "
                       << me->buffer.header.end_point
                       << " expected: " << me->buffer.header.payload_size;
             me->on_message(me->buffer);
             read_header();
          } else {
             shutdown();
          }
       });
}

void ClientConnection::shutdown() {
   LOG(info) << "Shutting down connection";
   me->socket.close();
   me->on_disconnect();
}

} // namespace mcbridge
