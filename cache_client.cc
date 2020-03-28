#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "cache.hh"
#include "kv_json.hh"
//
namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>


class Cache::Impl {
public:
    std::string host_;
    std::string port_;
    // beast::tcp_stream* stream_ = nullptr;
    net::ip::basic_resolver<tcp>::results_type results_;
    net::io_context ioc_;

    http::response<http::dynamic_body> send(http::request<http::string_body> req) {
        try{
            beast::tcp_stream stream(ioc_);
            std::cout << "connecting...";
            stream.connect(results_);
            std::cout << "done" << std::endl;
            std::cout << "sending...";
            http::write(stream, req);
            std::cout << "done" << std::endl;
            std::cout << "waiting for response...";
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;
            http::read(stream, buffer, res);
            std::cout << "done" << std::endl;
            beast::error_code ec;
            stream.socket().shutdown(tcp::socket::shutdown_both, ec);
            if(ec && ec != beast::errc::not_connected)
                throw beast::system_error{ec};
            return res;
        }
        catch(std::exception const& e){
            std::cerr << "Error: " << e.what() << std::endl;
            http::response<http::dynamic_body> res;
            res.result(420);
            return res;
        }
    }
};

Cache::Cache(std::string host, std::string port) : pImpl_(new Impl()){
    pImpl_->host_ = host;
    pImpl_->port_ = port;
    tcp::resolver resolver(pImpl_->ioc_);
    pImpl_->results_= resolver.resolve(host, port);
}
Cache::~Cache() {
    reset();
}

void Cache::set(key_type key, val_type val, size_type size) {
    // PUT /key/val

    http::request<http::string_body> req;
    req.method(http::verb::put);
    std::string val_str(val);
    req.target("/" + key + "/" + val_str);
    req.version(11);
    req.set(http::field::host, pImpl_->host_);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.content_length(size);
    req.prepare_payload();

    http::response<http::dynamic_body> res = pImpl_->send(req);
}

Cache::val_type Cache::get(key_type key, size_type& val_size) const{
    //GET /key
    //set val_size
    http::request<http::string_body> req;
    req.method(http::verb::get);
    req.target("/" + key);
    req.version(11);
    req.set(http::field::host, pImpl_->host_);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.prepare_payload();

    http::response<http::dynamic_body> res = pImpl_->send(req);
    std::string json = boost::beast::buffers_to_string(res.body().data());
    boost::property_tree::ptree tree;
    boost::property_tree::read_json(json);
    
    std::cout << json << std::endl;
    //try and CATCH_CONFIG_MAIN
    //return the val back
    val_size = -1;
    return 0;
}
bool Cache::del(key_type key) {
    //DELETE /key
    http::request<http::string_body> req;
    req.method(http::verb::delete_);
    req.target("/" + key);
    req.version(11);
    req.set(http::field::host, pImpl_->host_);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    req.prepare_payload();

    http::response<http::dynamic_body> res = pImpl_->send(req);
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
    req.prepare_payload();

    http::response<http::dynamic_body> res = pImpl_->send(req);
    // try
    // {
    //     http::write(pImpl_->stream_, req);
    //     beast::flat_buffer buffer;
    //     http::response<http::dynamic_body> res;
    //     http::read(pImpl_->stream_, buffer, res);
    //     std::cout << res << std::endl;
    // }
    // catch(std::exception const& e)
    // {
    //     std::cerr << "Error: " << e.what() << std::endl;
    //     return EXIT_FAILURE;
    // }
    return 0;
}
void Cache::reset() {
    // POST /reset
    http::request<http::string_body> req;
    req.method(http::verb::post);
    req.target("/");
    req.version(11);
    req.set(http::field::host, pImpl_->host_);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.prepare_payload();

    http::response<http::dynamic_body> res = pImpl_->send(req);

    // try{
    //     http::write(pImpl_->stream_, req);
    //     beast::flat_buffer buffer;
    //     http::response<http::dynamic_body> res;
    //     http::read(stream, buffer, res);
    //     std::cout << res << std::endl;
    //     beast::error_code ec;
    //     stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    //     if(ec && ec != beast::errc::not_connected)
    //         throw beast::system_error{ec};
    // }
    // catch(std::exception const& e){
    //     std::cerr << "Error: " << e.what() << std::endl;
    //     return EXIT_FAILURE;
    // }
}

int main() {
    Cache c("127.0.0.1", "42069");
    c.set("key", "val", 4);
    Cache::size_type size;
    c.get("key", size);
    return 0;
}
