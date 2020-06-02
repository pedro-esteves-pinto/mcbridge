#include "Server.h"
#include "../common/common.h"
#include "GroupManager.h"
#include "ServerConfig.h"
#include "ServerConnection.h"

#include <asio.hpp>

namespace mcbridge {

struct Server::PImpl {
   PImpl(ServerConfig const &cfg)
       : cfg(cfg), io_service(1),
         acceptor(io_service,
                  asio::ip::tcp::endpoint(asio::ip::tcp::v4(), cfg.port)),
         group_manager(io_service) {}

   ServerConfig cfg;
   asio::io_service io_service;
   asio::ip::tcp::acceptor acceptor;
   GroupManager group_manager;
};

Server::Server(ServerConfig const &cfg) : me(new PImpl(cfg)) {}
Server::~Server() {}

int Server::run() {
   LOG(info) << "Listening on " << me->cfg.port;
   accept();
   return me->io_service.run();
}

void Server::accept() {
   me->acceptor.async_accept([this](auto ec, auto socket) {
      if (!ec) {
         auto c = std::make_shared<ServerConnection>(
             me->io_service, std::move(socket), me->group_manager);
         c->start();
      }
      accept();
   });
}

} // namespace mcbridge
