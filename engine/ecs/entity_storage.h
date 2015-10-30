#pragma once

#include <unordered_map>
#include <functional>
#include <memory>
#include "entity.h"

using engine::ecs::entity;
using std::unordered_map;
using std::function;

namespace engine {  
namespace ecs_detail {
  
class entity_storage {
private:
  unordered_map<int, entity> entities;
  int next_handle = 1;
public:
  std::size_t size() { return entities.size(); }
  
  void for_each(std::function<void(entity *)> func) {
    for (auto it=entities.begin(); it != entities.end(); ++it) {
      func(&it->second);
    }
  }
  
  int get_next_handle() {
    int result = next_handle;
    ++next_handle;
    return result;
  }
  
  void add_entity(const ecs::entity &target) {
    entities[target.handle] = target;
  }
  
  ecs::entity * find_by_handle(const int &handle) {
    auto finder = entities.find(handle);
    if (finder != entities.end()) {
      return &finder->second;
    }
    return nullptr;
  }  
};
  
}
}