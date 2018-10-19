#pragma once

#include <sol.hpp>

#include <list>
#include <unordered_map>

namespace effil {

template<typename Key, typename Value, typename Hash = std::hash<Key>>
class LruCache {
public:
    LruCache(size_t size) : capacity_(size)
    {}

    void put(const Key& key, const Value& value) {
        if (itemsMap_.find(key) != itemsMap_.end())
            return;

        auto stat = itemsMap_.emplace(key, itemsList_.end());
        if (stat.second) { // was inserted
            itemsList_.emplace_front(std::make_pair(key, value));
            stat.first->second = itemsList_.begin();
        }

        if (capacity_ > 0 && itemsList_.size() > capacity_)
            removeLast();
    }

    sol::optional<Value> get(const Key& key) {
        const auto mapIter = itemsMap_.find(key);
        if (mapIter != itemsMap_.end()) {
            itemsList_.splice(itemsList_.begin(), itemsList_, mapIter->second);
            return mapIter->second->second;
        }
        return sol::nullopt;
    }

    void clear() {
        itemsMap_.clear();
        itemsList_.clear();
    }

    bool remove(const Key& key) {
        const auto iter = itemsMap_.find(key);
        if (iter == itemsMap_.end())
                return false;

        itemsList_.erase(iter->second);
        itemsMap_.erase(iter);
        return true;
    }

    size_t size() {
        return itemsList_.size();
    }

    void setCapacity(size_t capacity) {
        capacity_ = capacity;

        while (capacity > 0 && itemsList_.size() > capacity)
            removeLast();
    }

    size_t capacity() {
        return capacity_;
    }

private:
    void removeLast() {
        const auto& pair = itemsList_.back();
        itemsMap_.erase(pair.first);
        itemsList_.pop_back();
    }

    typedef std::list<std::pair<Key, Value>> ItemsList;

    size_t capacity_;
    ItemsList itemsList_;
    std::unordered_map<Key, typename ItemsList::iterator, Hash> itemsMap_;
};

} // namespace effil
