#include "ClientConnection.h"
#include "../common/common.h"
#include <asio/read.hpp>

namespace mcbridge {

struct ClientConnection::PImpl {
   asio::ip::tcp::socket &socket;
   OnMessage on_message;
   OnDisconnect on_disconnect;
   Message buffer;
   TimeStamp last_sent_hb;
   TimeStamp last_rcvd_hb;
   size_t sequence_number = 0;
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
         shutdown({});
      } else if (sec_diff(Timer::now(), me->last_sent_hb) > 5) {
         me->last_sent_hb = Timer::now();
         auto m = std::make_unique<Message>(); 
         m->header.end_point = {};
         m->header.type = MessageType::HB;
         m->header.payload_size = 0;
         m->header.sequence_number = me->sequence_number++;
         auto ptr = m.get();
         LOG(diag) << "Sending HB to " << me->socket.remote_endpoint();
         auto self = shared_from_this();
         me->socket.async_send(asio::buffer(ptr,sizeof(MessageHeader)),
                               [this, self, msg = std::move(m)](auto ec, auto) {
                                  if (ec)
                                     shutdown(ec);
                               });
      }
   }
}

void ClientConnection::join_group(EndPoint ep) {
   LOG(info) << "Joining " << ep;
   me->last_sent_hb = Timer::now();
   auto m = std::make_unique<Message>(); 
   m->header.end_point = ep;
   m->header.type = MessageType::JOIN;
   m->header.payload_size = 0;
   m->header.sequence_number = me->sequence_number++;
   me->last_sent_hb = Timer::now();
   auto ptr = m.get();
   auto self = shared_from_this();
   me->socket.async_send(asio::buffer(ptr,sizeof(MessageHeader)),
                         [this, self, msg = std::move(m)](auto ec, auto) {
                            if (ec)
                               shutdown(ec);
                         });
}

void ClientConnection::leave_group(EndPoint ep) {
   me->last_sent_hb = Timer::now();
   auto m = std::make_unique<Message>(); 
   m->header.end_point = ep;
   m->header.type = MessageType::LEAVE;
   m->header.payload_size = 0;
   m->header.sequence_number = me->sequence_number++;
   me->last_sent_hb = Timer::now();
   me->last_sent_hb = Timer::now();
   auto ptr = m.get();
   auto self = shared_from_this();
   me->socket.async_send(asio::buffer(ptr,sizeof(MessageHeader)),
                         [this, self, msg = std::move(m)](auto ec, auto) {
                            if (ec)
                               shutdown(ec);
                         });
}

void ClientConnection::read_header() {
   auto self = shared_from_this();
   asio::async_read(
      me->socket,
       asio::buffer(&me->buffer.header, sizeof(MessageHeader)),
       [this, self](auto ec, auto) {
          if (!ec) {
             LOG(diag) << "Received header for msg " << me->buffer.header.sequence_number
                       << " type: " << (int) me->buffer.header.type
                       << " payload_size: " << me->buffer.header.payload_size;
             me->last_rcvd_hb = Timer::now();
             if (me->buffer.header.type == MessageType::HB) {
                LOG(diag) << "Received HB";
                read_header();
             }
             else if (me->buffer.header.type == MessageType::MC_DATAGRAM) 
                read_payload();
             else
                read_header();
          } else {
             LOG(diag) << "Error reading header " << ec.message();
             shutdown(ec);
          }
       });
}

void ClientConnection::read_payload() {
   auto self = shared_from_this();
   asio::async_read (me->socket,
                     asio::buffer(&me->buffer.payload, me->buffer.header.payload_size),
       [this, self](auto ec, auto bytes_read) {
          if (!ec) {
             LOG(diag) << "Received msg " << me->buffer.header.sequence_number
                       << " for "
                       << " read: " << bytes_read << " of: " << me->buffer.header.payload_size << " "
                       << me->buffer.header.end_point << " first 64 bytes: "
                       << *(uint64_t *)me->buffer.payload.data();
             me->on_message(me->buffer);
             read_header();
          } else {
             LOG(info) << "Error reading paylod for "
                       << me->buffer.header.end_point
                       << " error: " << ec.message();
             shutdown(ec);
          }
       });
}

void ClientConnection::shutdown(asio::error_code ec) {
   LOG(info) << "Shutting down connection: " << ec.message();
   me->socket.close();
   me->on_disconnect();
}

} // namespace mcbridge
