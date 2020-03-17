#include "evictor.hh"
#include <queue>        // std::queue
#include <list>         // std::list
class FifoEvictor:public Evictor {
    private:
        std::queue<key_type, std::list<key_type>> contents_;
    public:
        void touch_key(const key_type&);
        const key_type evict();
        ~FifoEvictor();
        FifoEvictor();
};
