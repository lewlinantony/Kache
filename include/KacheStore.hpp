#pragma once 

#include <string>
#include <unordered_map>
#include <mutex>
#include <optional>

class KacheStore {
public:
    KacheStore();

    // SET key value
    void set(const std::string& key, const std::string& value);

    // GET key
    std::optional<std::string> get(const std::string& key); //optional is safer cause of the bool inside it

    // DELETE key
    bool del(const std::string& key);

    // EXISTS key
    bool exists(const std::string& key);

private:
    std::unordered_map<std::string, std::string> map_;
    std::mutex mutex_; // The mutex that protects the map
};
