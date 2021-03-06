#ifndef AFINA_STORAGE_SIMPLE_LRU_H
#define AFINA_STORAGE_SIMPLE_LRU_H

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <iostream>
#include <cassert>

#include <afina/Storage.h>

namespace Afina {
namespace Backend {

/**
 * # Map based implementation
 * That is NOT thread safe implementaiton!!
 */
class SimpleLRU : public Afina::Storage {
public:
    SimpleLRU(size_t max_size = 1024) 
        : _free_size(max_size)
        , _max_size(max_size)
        , _lru_head(nullptr)
        , _lru_tail(nullptr) 
    {}

    ~SimpleLRU() {
        _lru_index.clear();

        if (_lru_tail) {
            lru_node *cur = _lru_tail;
            while (cur->prev) {
                cur = cur->prev;
                cur->next.reset(nullptr);
            }
        }
        
        _lru_head.reset(nullptr);
    }

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value) override;

private:
    // LRU cache node
    using lru_node = struct lru_node {
        const std::string key;
        std::string value;
        lru_node *prev;
        std::unique_ptr<lru_node> next;

        lru_node(const std::string &_key, const std::string &_value)
            : key(_key)
            , value(_value)
            , prev(nullptr)
            , next(nullptr)
        { }

    };

    // Maximum number of bytes could be stored in this cache.
    // i.e all (keys+values) must be less the _max_size
    std::size_t _free_size;
    std::size_t _max_size;
    
    // Main storage of lru_nodes, elements in this list ordered descending by "freshness": in the head
    // element that wasn't used for longest time.
    //
    // List owns all nodes
    std::unique_ptr<lru_node> _lru_head;
    lru_node *_lru_tail;

    // Index of nodes from list above, allows fast random access to elements by lru_node#key
    std::map<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>, std::less<std::string>> _lru_index;
    using rw_string = std::reference_wrapper<const std::string>;
    using rw_lru_node = std::reference_wrapper<lru_node>;

    bool free_space(std::size_t needed_space);
    bool push_back(const std::string &key, const std::string &value);
    bool pop_front();
    bool delete_node(lru_node *node);
    bool move_node_to_tail(lru_node *node);

};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_SIMPLE_LRU_H
