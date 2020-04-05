#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include "udp_handler.hh"
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using udp = boost::asio::ip::udp;       // from <boost/asio/ip/udp.hpp>

udp_handler::udp_handler(net::io_context& ioc, Cache* cache, unsigned short udp_port):
    socket_(ioc, udp::endpoint(udp::v4(), udp_port))
{
    server_cache_ = cache;
}
void udp_handler::run() {
    start_recieve();
}

void udp_handler::start_recieve() {
    socket_.async_receive_from(
        boost::asio::buffer(recv_buffer_), remote_endpoint_,
        boost::bind(&udp_handler::handle_receive, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}
void udp_handler::handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred) {
    if(!error) {
        std::cout << "recieved message" << std::endl;
        boost::shared_ptr<std::string> message (new std::string("world"));

        socket_.async_send_to(boost::asio::buffer(*message), remote_endpoint_,
            boost::bind(&udp_handler::handle_send, shared_from_this(), message,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    } else {
        std::cerr << "error recieving: code " << error.value() << std::endl;
        std::cerr << error.value() << std::endl;
        std::cerr << "total bytes recieved: " << bytes_transferred << std::endl;
        std::cerr << error.category().name() << std::endl;
    }
}
void udp_handler::handle_send(boost::shared_ptr<std::string> message, const boost::system::error_code& error, std::size_t bytes_transferred){
    if(error) {
        std::cerr << "error code " << error.value() << std::endl;
        std::cerr << "after " << bytes_transferred << " bytes were bytes_transferred" << std::endl;
    } else {
        std::cout << message << std::endl;
    }
}
