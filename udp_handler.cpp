#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/beast/http/parser.hpp>
#include "udp_handler.hh"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using udp = boost::asio::ip::udp;       // from <boost/asio/ip/udp.hpp>

udp_handler::udp_handler(net::io_context& ioc, udp::endpoint endpoint, Cache* cache, request_processor rp):
    ioc_(ioc),
    socket_(ioc, endpoint)
{
    processor_ = rp;
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
        std::cout << "recieved request:" << std::endl;
        std::string data(recv_buffer_.begin(), recv_buffer_.end());
        std::cout << data << std::endl;
        std::string s = "GET /key_one/ HTTP/1.1";
        std::cout << s.compare(data) << std::endl;
        boost::system::error_code ec;
        http::request_parser<http::string_body> parser;
        parser.eager(true);
        std::cout << "sending data to parser...";
        parser.put(boost::asio::buffer(data), ec);
        // parser.put_eof(ec);
        std::cout << "done" << std::endl;

        std::cout << "getting object from parser..." ;
        http::request<http::string_body> req = parser.get();
        std::cout << "done" << std::endl;

        std::cout << "processing response" << std::endl;
        http::response<http::string_body> res = processor_.handle_request(req, server_cache_);
        std::cout << "done" << std::endl;
        std::ostringstream oss;
        oss << res;
        std::string response_as_string = oss.str();
        std::cout << response_as_string << std::endl;

        boost::shared_ptr<std::string> message(new std::string(response_as_string));
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
        std::cout << *message << std::endl;
    }
    start_recieve();
}
