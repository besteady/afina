#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    auto found_key_node_pair = _lru_index.find(key);
    if (found_key_node_pair != _lru_index.end()) {
        lru_node *to_delete = &found_key_node_pair->second.get();
        _lru_index.erase(to_delete->key);
        _delete_node(to_delete);
    } else {
        if (_free_size < key.size() + value.size()) {
            _lru_index.erase(_lru_head->key);
            _pop_front();
        }
    }
    bool res = _push_back(key, value);
    if (res) {
        _lru_index.insert(std::make_pair(rw_string(_lru_tail->key),
                    rw_lru_node(*_lru_tail)));
    }
    return res;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) { 
    auto found_key_node_pair = _lru_index.find(key);
    if (found_key_node_pair != _lru_index.end()) {
        return false;
    } else {
        if (_free_size < key.size() + value.size()) {
            _lru_index.erase(_lru_head->key);
            _pop_front();
        }
    }
    bool res = _push_back(key, value);
    if (res) {
        _lru_index.insert(std::make_pair(rw_string(_lru_tail->key),
                    rw_lru_node(*_lru_tail)));
    }
    return res;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    auto found_key_node_pair = _lru_index.find(key);
    if (found_key_node_pair != _lru_index.end()) {
        lru_node *to_delete = &found_key_node_pair->second.get();
        _lru_index.erase(to_delete->key);
        _delete_node(to_delete);
    } else {
        return false;
    }
    bool res = _push_back(key, value);
    if (res) {
        _lru_index.insert(std::make_pair(rw_string(_lru_tail->key),
                    rw_lru_node(*_lru_tail)));
    }
    return res;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto found_key_node_pair = _lru_index.find(key);
    if (found_key_node_pair == _lru_index.end()) {
        return false;
    }
    return _delete_node(&found_key_node_pair->second.get());
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {
    auto found_key_node_pair = _lru_index.find(key);
    if (found_key_node_pair == _lru_index.end()) {
        return false;
    }
    lru_node *to_move = &found_key_node_pair->second.get();
    value = to_move->value;
    return true;
}


// -------------------------------------------

bool SimpleLRU::_push_back(const std::string &key, const std::string &value) {
    if (key.size() + value.size() > _free_size) { 
        return false;
    } else {
        _free_size -= key.size() + value.size();
    }

    std::unique_ptr<lru_node> new_node(new lru_node(key, value));

    if (_lru_head == nullptr) {
        _lru_head = std::move(new_node);
        _lru_tail = _lru_head.get();
    } else {
        assert(_lru_tail->next == nullptr);
        new_node->prev = _lru_tail;
        _lru_tail->next = std::move(new_node);
        _lru_tail = _lru_tail->next.get();
    }

    return true;
}

bool SimpleLRU::_pop_front() {
    if (_lru_head) {
        _free_size += _lru_head->key.size() + _lru_head->value.size();

        if (_lru_head->next) {
            _lru_head = std::move(_lru_head->next);
            _lru_head->prev = nullptr;
        } else {
            _lru_head.reset(nullptr);
            _lru_tail = nullptr;
        }
    }

    return true;
}

bool SimpleLRU::_delete_node(lru_node *node) {
    if (node->prev) {
        _free_size += node->key.size() + node->value.size();

        if (node->next) {
            node->prev->next = std::move(node->next);
            node->next->prev = node->prev;

            delete node;
        } else {
            _lru_tail = _lru_tail->prev;
            _lru_tail->next.reset(nullptr);
        }
    } else {
        _pop_front();
    }

    return true;
}

bool SimpleLRU::_move_node_to_tail(lru_node *node) {
    if (node->next) {
        if (node->prev) {
            node->next->prev = nullptr;
            _lru_head = std::move(node->next);
        } else {
            node->next->prev = node->prev;
            node->prev->next = std::move(node->next);
        }
        _lru_tail->next.reset(node);
        node->prev = _lru_tail;
        _lru_tail = node;
    }

    return true;
}

} // namespace Backend
} // namespace Afina
