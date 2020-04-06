#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include "cache.hh"
#include <iostream>
#include "request_processor.hh"
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class tcp_listener : public std::enable_shared_from_this<tcp_listener> {
public:
    tcp_listener(net::io_context& ioc, tcp::endpoint endpoint, Cache* cache, request_processor rp);
    void run();
private:
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    Cache* server_cache_;
    request_processor processor_;
    void do_accept();
    void on_accept(beast::error_code ec, tcp::socket socket);
};
