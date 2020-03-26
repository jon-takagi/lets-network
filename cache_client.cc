#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
//
namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>
#include "cache.hh"


class Cache::Impl {
public:
    std::string host_;
    int port_;
    tcp::resolver resolver_;
    beast::tcp_stream stream_;
};

Cache::Cache(std::string host, std::string port) : pImpl_(new Impl()) {
    pImpl_->host_ = host;
    pImpl_->port_ = std::stoi(port);
    net::io_context ioc;
    pImpl_->resolver_(ioc);
    pImpl_->stream_(ioc);
}
Cache::~Cache() {
    reset();
}

int main() {
    Cache c("127.0.0.1", "42069");
    // c.del("blah");
    return 0;
}
void Cache::set(key_type key, val_type val, size_type size) {
    // PUT /key/val
}
//
// Cache::val_type get(key_type key, size_type& val_size) {
//     // GET /key
//     // set val_size
//     return "";
// }
// bool del(key_type key) {
//     // DELETE /key
//     return true;
// }
// Cache::size_type space_used() {
// //     http::request<http::string_body> req{http::verb::head};
// //     req.target("/");
// //     req.version(11);
// //     req.set(http::field::host, pImpl_->host_);
// //     req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
// //
// //     // try
// //     // {
// //     //     auto const results = resolver.resolve(pImpl_->host_, pImpl_->port_);
// //     //     stream.connect(results);
// //     //
// //     //     http::write(stream, req);
// //     //     beast::flat_buffer buffer;
// //     //     http::response<http::dynamic_body> res;
// //     //     http::read(stream, buffer, res);
// //     //     std::cout << res << std::endl;
// //     //     beast::error_code ec;
// //     //     stream.socket().shutdown(tcp::socket::shutdown_both, ec);
// //     //     if(ec && ec != beast::errc::not_connected)
// //     //         throw beast::system_error{ec};
// //     // }
// //     // catch(std::exception const& e)
// //     // {
// //     //     std::cerr << "Error: " << e.what() << std::endl;
// //     //     return EXIT_FAILURE;
// //     // }
// //     // HEAD
//     return 0;
// }
void Cache::reset() {
    // POST /reset
}
