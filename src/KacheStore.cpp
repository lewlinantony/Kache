#include "KacheStore.hpp"

KacheStore::KacheStore(){    
}

void KacheStore::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> guard(mutex_); // Lock is acquired here
    map_[key] = value;
} // Lock is automatically released when 'guard' goes out of scope

std::optional<std::string> KacheStore::get(const std::string& key){
    std::lock_guard<std::mutex> guard(mutex_);
    auto it = map_.find(key); // directly using indexing will result it returning a default value
    if (it != map_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool KacheStore::del(const std::string& key){ // delete key and return if it was in the map or not
    std::lock_guard<std::mutex> guard(mutex_);
    return map_.erase(key) > 0;
}

bool KacheStore::exists(const std::string& key){
    std::lock_guard<std::mutex> guard(mutex_);
    return map_.find(key) != map_.end();
}

