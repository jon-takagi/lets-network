#include <iostream>
#include <boost/beast.hpp>

int main()
{
    std::string s = "PUT /key_one/val_one HTTP/1.1\r\nContent-Length: 5\r\n\r\nabcde";
    boost::system::error_code ec;
    boost::beast::http::request_parser<boost::beast::http::string_body> p;
    p.eager(true);
    p.put(boost::asio::buffer(s), ec);
    boost::beast::http::request<boost::beast::http::string_body> req = p.get();

    std::cout << "\t request method: " << req.method() << std::endl;
    std::cout << "\t request target: " << req.target() << std::endl;
}
