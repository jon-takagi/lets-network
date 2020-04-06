#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include "tcp_listener.hh"
#include "session.hh"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

tcp_listener::tcp_listener(net::io_context& ioc, tcp::endpoint endpoint, Cache* cache, request_processor rp):
    ioc_(ioc),
    acceptor_(net::make_strand(ioc))
{
    server_cache_ = cache;
    beast::error_code ec;
    processor_ = rp;
    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if(ec) {
        processor_.fail(ec, "open");
        return;
    }

    // Allow address reuse
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if(ec) {
        processor_.fail(ec, "set_option");
        return;
    }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if(ec) {
        processor_.fail(ec, "bind");
        return;
    }

    // Start listening for connections
    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if(ec) {
        processor_.fail(ec, "listen");
        return;
    }
}

    // Start accepting incoming connections
void tcp_listener::run() {
    do_accept();
}

void tcp_listener::do_accept() {
        // The new connection gets its own strand
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(
            &tcp_listener::on_accept,
            shared_from_this()));
}

void tcp_listener::on_accept(beast::error_code ec, tcp::socket socket) {
    if(ec)
    {
        processor_.fail(ec, "accept");
    }
    else
    {
        // Create the session and run it
        std::make_shared<session>(
            std::move(socket), server_cache_, processor_)->run();
    }

    // Accept another connection
    do_accept();
}
