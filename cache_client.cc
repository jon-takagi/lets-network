#include "cache.hh"
class Cache::Impl{
    public:
        std::string host_;
        int port_;
};
Cache::Cache(std::string host, std::string port):pImpl_(new Impl()){
    pImpl_->host_ = host;
    pImpl_->port_ = std::stoi(port);
}

Cache::~Cache() {
    reset();
}

void Cache::set(key_type key, val_type val, size_type size) {
    // PUT /key/val
}

val_type Cache::get(key_type key, size_type& val_size) {
    // GET /key
    // set val_size
}
bool Cache::del(key_type key) {
    // DELETE /key
}
size_type Cache::space_used() {
    // HEAD
}
void Cache::reset() {
    // POST /reset
}
