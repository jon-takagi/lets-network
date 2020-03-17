#include <unordered_map>
#include <tuple>
#include "cache.hh"

#include <iostream>

using entry_type = std::pair<Cache::val_type, Cache::size_type>; //To better track size for entries

class Cache::Impl{
    public:
        std::unordered_map<key_type, entry_type, hash_func> dict_;//might be an error here; might be able to use default hash type
        size_type size_;
        size_type maxmem_;
        Evictor* evictor_;
};

Cache::Cache(size_type maxmem,float max_load_factor, Evictor* evictor, hash_func hasher)
 : pImpl_(new Impl()) {
    std::unordered_map<key_type, entry_type, hash_func> dict(4,hasher);//set to 4 buckets at start
    pImpl_->dict_ = dict;//hope this works..
    pImpl_->dict_.max_load_factor(max_load_factor);
    pImpl_->size_ = 0;
    pImpl_->maxmem_ = maxmem;
    pImpl_->evictor_ = evictor;
}

Cache::~Cache() {
  //call reset on cache
  reset();
}
// Basic version: Allows setting of key only when there's enough size, otherwise rejects
// Then copies by iterating over val to make new entry to put into pImpl_ dict
void Cache::set(key_type key, val_type val, size_type size) {
// TODO: if we overwrite an existing value, free the old memory first by del-ing the old value

    if(pImpl_->dict_.find(key) != pImpl_->dict_.end()) {
        // std::cout << "overwriting existing key" << std::endl;
        del(key);
    }

    if(space_used() + size > pImpl_->maxmem_ && pImpl_->evictor_ == nullptr) {
        return;
    } else {
        while(space_used() + size > pImpl_->maxmem_) {
            del(pImpl_->evictor_->evict());
        }
    }
    byte_type *copy = new byte_type[size];
    int i = 0;
    while(val[i] != '\0'){ //Searching for null terminator
        copy[i] = val[i];
        i++;
    }
    val_type entry_val = copy;
    pImpl_->dict_.insert(std::make_pair(key, std::make_pair(entry_val, size)));
    // need to free copy
    // delete[] pImpl_->dict_[key].second.first
    pImpl_->size_ += size;
    if(pImpl_->evictor_ != nullptr) {
        pImpl_->evictor_->touch_key(key);
    }
}

//Just assign the size to the reference that was passed, then return the value, both from the entry pair
Cache::val_type Cache::get(key_type key, size_type& val_size) const {
    if(pImpl_->evictor_ != nullptr) {
        pImpl_->evictor_->touch_key(key);
    }
    val_size = pImpl_->dict_[key].second;
    return pImpl_->dict_[key].first;
}
//frees the memory used when a key is deleted from memory.
bool Cache::del(key_type key) {
    if(pImpl_->dict_.find(key) == pImpl_->dict_.end()) {
        return false;
    }
    pImpl_->size_ -= pImpl_->dict_[key].second;
    delete[] pImpl_->dict_[key].first;
    pImpl_->dict_.erase(key);
    return true;
}

Cache::size_type Cache::space_used() const{
    return pImpl_->size_;
}

//del-s all elements, since del frees the memory used
void Cache::reset() {
    for (auto i: pImpl_->dict_) {
        delete[] i.second.first;
    }
    pImpl_->dict_.clear();
    pImpl_->size_ = 0;
}
