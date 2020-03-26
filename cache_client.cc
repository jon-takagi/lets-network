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
    std::string port_;
    beast::tcp_stream stream_;
};

Cache::Cache(std::string host, std::string port) : pImpl_(new Impl()){
    pImpl_->host_ = host;
    pImpl_->port_ = port;
    net::io_context ioc;
    tcp::resolver resolver(ioc);
    pImpl_->stream_(ioc);
    auto const results = resolver.resolve(host, port);
    pImpl_->stream_.connect(results)
}
Cache::~Cache() {
    reset();
    beast::error_code ec;
    pImpl_->stream_.socket().shutdown(tcp::socket::shutdown_both, ec);
    if(ec && ec != beast::errc::not_connected)
        throw beast::system_error{ec};
}

void Cache::set(key_type key, val_type val, size_type size) {
    std::cout << key << ": " << val << "; " << size << std::endl;
    // PUT /key/val
    http::request<http::string_body> req;
    req.method(http::verb::put);
    std::string val_str(val);
    req.target("/" + key + "/" + val_str);
    req.version(11);
    req.set(http::field::host, pImpl_->host_);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    try{
        auto const results = resolver.resolve(pImpl_->host_, pImpl_->port_);
        stream.connect(results);
        http::write(stream, req);
        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);
        std::cout << res << std::endl;
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        if(ec && ec != beast::errc::not_connected)
            throw beast::system_error{ec};
    }
    catch(std::exception const& e){
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

Cache::val_type Cache::get(key_type key, size_type& val_size) {
    //GET /key
    //set val_size
    http::request<http::string_body> req;
    req.method(http::verb::get);
    req.target("/" + key);
    req.version(11);
    req.set(http::field::host, pImpl_->host_);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    //try and CATCH_CONFIG_MAIN
    //return the val back

}
bool Cache::del(key_type key) {
    //DELETE /key
    http::request<http::string_body> req;
    req.method(http::verb::delete_);
    req.target("/" + key);
    req.version(11);
    req.set(http::field::host, pImpl_->host_);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    //try catch
    //error checking
    //else
    return true;
}
Cache::size_type Cache::space_used() const{

    http::request<http::string_body> req;
    req.method(http::verb::head);
    req.target("/");
    req.version(11);
    req.set(http::field::host, pImpl_->host_);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    try
    {

        http::write(pImpl_->stream_, req);
        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(pImpl_->stream_, buffer, res);
        std::cout << res << std::endl;
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;
}
void Cache::reset() {
    // POST /reset
    http::request<http::string_body> req{http::verb::post};
    req.target("/");
    req.version(11);
    req.set(http::field::host, pImpl_->host_);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    try{
        auto const results = resolver.resolve(pImpl_->host_, pImpl_->port_);
        stream.connect(results);
        http::write(stream, req);
        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);
        std::cout << res << std::endl;
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        if(ec && ec != beast::errc::not_connected)
            throw beast::system_error{ec};
    }
    catch(std::exception const& e){
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

int main() {
    Cache c("127.0.0.1", "42069");
    c.space_used();
    return 0;
}
