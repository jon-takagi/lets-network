#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/array.hpp>
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
using udp = net::ip::udp;


class Cache::Impl {
public:
    std::string host_;
    std::string tcp_port_;
    std::string udp_port_;
    net::ip::basic_resolver<tcp>::results_type tcp_results_;
    net::io_context ioc_;
    beast::tcp_stream* tcp_stream_;
    udp::endpoint sender_endpoint_;
    udp::endpoint receiver_endpoint_;
    udp::socket* udp_socket_;

    http::request<http::string_body> prep_req(http::verb method, std::string target) {
        http::request<http::string_body> req;
        req.method(method);
        req.target(target);
        req.version(11);
        req.set(http::field::host, host_+":"+port_);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.keep_alive(true);
        req.prepare_payload();
        return req;
    }

    http::response<http::dynamic_body> send_tcp(http::request<http::string_body> req) {
        try{
            http::write(*tcp_stream_, req);
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;
            http::read(*tcp_stream_, buffer, res);
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


    http::response<http::dynamic_body> send_udp(http::request<http::string_body> req) {
        try{
            std::ostringstream oss;
            oss << req;
            std::string request_string = oss.str();
            udp::endpoint endpoint_(net::ip::address::from_string(host_), udp_port_);
            std::cout << "opening...";
            socket_ -> open(udp::v4());
            std::cout << "done" << std::endl;
            std::cout << "sending...";
            std::cout << request_string << std::endl;
            socket_ -> send_to(boost::asio::buffer(request_string, request_string.length()), endpoint_);
            std::cout << "done" << std::endl;
            boost::array<char, 128> recv_buff;
            size_t len = socket_ -> receive_from(boost::asio::buffer(recv_buff), endpoint_);
            std::cout.write(recv_buff.data(), len);
            http::response<http::dynamic_body> res;
            return res;

            boost::array<char, 1> send_buf  = {{ 0 }};
            socket_->send_to(boost::asio::buffer(send_buf), receiver_endpoint_);
            boost::array<char, 128> recv_buf;
            socket_->receive_from(boost::asio::buffer(recv_buf), sender_endpoint_);
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;
            http::read(recv_buf.data(), buffer, res);
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
    pImpl_->port_ = port;//FIX THIS!!
    try{
        //udp
        udp::resolver udp_resolver(pImpl_->ioc_);
        udp::endpoint receiver_endpoint = *udp_resolver.resolve(udp::v4(), host, udp_port).begin();
        pImpl_->receiver_endpoint_ = receiver_endpoint;
        pImpl_->socket_ = new udp::socket(pImpl_->ioc_);
        pImpl_->socket_->open(udp::v4());
        udp::endpoint sender_endpoint;
        pImpl_->sender_endpoint_ = sender_endpoint;
        //tcp
        tcp::resolver tcp_resolver(pImpl_->ioc_);
        net::ip::basic_resolver<tcp>::results_type results = tcp_resolver.resolve(host, tcp_port);
        pImpl_->tcp_stream_ = new beast::tcp_stream(pImpl_->ioc_);
        pImpl_->tcp_stream_->connect(results);
    }
    catch(std::exception const& e){
        std::cerr << "Error: " << e.what() << std::endl;
        assert(false);
    }
}

Cache::~Cache() {
    reset();
    beast::error_code ec;
    pImpl_->stream_->socket().shutdown(tcp::socket::shutdown_both, ec);
    //delete pImpl_->stream_;
    delete pImpl_->socket_;
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
