#include "fifo_evictor.h"
#include "evictor.hh"
#include <list>           // std::list
#include <queue>
FifoEvictor::FifoEvictor() {

}
FifoEvictor::~FifoEvictor() {

}
void FifoEvictor::touch_key(const key_type& key) {
    contents_.push(key);
}
const key_type FifoEvictor::evict() {
    key_type last = "";
    if (!contents_.empty() ){
        last = contents_.front();
        contents_.pop();
    }
    return last;
}
