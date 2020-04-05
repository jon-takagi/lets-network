#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include "cache.hh"
#include <iostream>
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using boost::asio::ip::udp;

class udp_handler : public std::enable_shared_from_this<udp_handler> {
public:
    udp_handler(net::io_context& ioc, Cache* cache, unsigned short udp_port);
    void run();
private:
    Cache* server_cache_;
    udp::endpoint remote_endpoint_;
    udp::socket socket_;
    boost::array<char, 1> recv_buffer_;
    void start_recieve();
    void handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred);
    void handle_send(boost::shared_ptr<std::string> message, const boost::system::error_code& error, std::size_t bytes_transferred);
};
