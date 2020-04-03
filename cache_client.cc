#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <cassert>
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
    beast::tcp_stream* stream_;

    http::request<http::string_body> prep_req(http::verb method, std::string target) {
        http::request<http::string_body> req;
        req.method(method);
        req.target(target);
        req.version(11);
        req.set(http::field::host, host_+":"+port_);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.prepare_payload();
        return req;
    }
    http::response<http::dynamic_body> send(http::request<http::string_body> req) {
        try{
            //beast::tcp_stream stream(ioc_);
            // std::cout << "connecting...";
            //stream.connect(results_);
            // std::cout << "done" << std::endl;
            // std::cout << "sending...";
            http::write(*stream_, req);
            // std::cout << "done" << std::endl;
            // std::cout << "waiting for response...";
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;
            http::read(*stream_, buffer, res);
            // std::cout << "done" << std::endl;
            //beast::error_code ec;
            //stream.socket().shutdown(tcp::socket::shutdown_both, ec);
            //if(ec && ec != beast::errc::not_connected)
            //    throw beast::system_error{ec};
            return res;
        }
        catch(std::exception const& e){
            std::cerr << "Error: " << e.what() << std::endl;
            http::response<http::dynamic_body> res;
            res.result(499);
            res.insert("error: ", e.what());
            return res;
        }
    }
};

Cache::Cache(std::string host, std::string port) : pImpl_(new Impl()){
    pImpl_->host_ = host;
    pImpl_->port_ = port;
    tcp::resolver resolver(pImpl_->ioc_);
    pImpl_->results_= resolver.resolve(host, port);
    try{
        pImpl_->stream_ = new beast::tcp_stream(pImpl_->ioc_);
        //beast::tcp_stream *stream_ptr = stream;
        // std::cout << "connecting...";
        pImpl_->stream_->connect(pImpl_->results_);
        // std::cout << "done" << std::endl;
        // std::cout << "sending...";
        //http::write(stream, req);
        // std::cout << "done" << std::endl;
        // std::cout << "waiting for response...";
        //beast::flat_buffer buffer;
        //http::response<http::dynamic_body> res;
        //http::read(stream, buffer, res);
        // std::cout << "done" << std::endl;
        //beast::error_code ec;
        //stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        //if(ec && ec != beast::errc::not_connected)
        //    throw beast::system_error{ec};
        //return res;
    }
    catch(std::exception const& e){
        std::cerr << "Error: " << e.what() << std::endl;
        //http::response<http::dynamic_body> res;
        //res.result(499);
        //res.insert("error: ", e.what());
        assert(false);
    }
}
Cache::~Cache() {
    reset();
    beast::error_code ec;
    pImpl_->stream_->socket().shutdown(tcp::socket::shutdown_both, ec);
    delete pImpl_->stream_;
    //if(ec && ec != beast::errc::not_connected) throw beast::system_error{ec};
}

void Cache::set(key_type key, val_type val, size_type size) {
    // PUT /key/val
    size += 1;
    pImpl_->send(pImpl_->prep_req(http::verb::put, "/" + key + "/" + std::string(val)));
}

Cache::val_type Cache::get(key_type key, size_type& val_size) const{
    //GET /key
    http::request<http::string_body> req = pImpl_->prep_req(http::verb::get, "/"+key);
    http::response<http::dynamic_body> res = pImpl_->send(req);
    if(res.result() == http::status::not_found){
        return nullptr;
    }

    std::string data_str = boost::beast::buffers_to_string(res.body().data());
    kv_json kv(data_str);
    //Here's the hacky workaround to make sure we don't lose the memory our return val points to
    //val_type val = kv.value_ would lose the data once kv was destroyed
    std::string string_holder(kv.value_);
    val_type val = const_cast<char*>(string_holder.c_str());

    val_size = strlen(val)+1;//we count the null terminator as well
    return val;
}
bool Cache::del(key_type key) {
    //DELETE /key
    http::request<http::string_body> req = pImpl_->prep_req(http::verb::delete_, "/"+key);

    http::response<http::dynamic_body> res = pImpl_->send(req);
    return res.result() == http::status::ok;
}
Cache::size_type Cache::space_used() const{
    http::response<http::dynamic_body> res = pImpl_->send(pImpl_->prep_req(http::verb::head, "/"));
    return std::stoi(std::string(res["Space-Used"]));
}
void Cache::reset() {
    // POST /reset
    pImpl_->send(pImpl_->prep_req(http::verb::post, "/reset"));
}
