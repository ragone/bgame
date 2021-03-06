#include "stdafx.h"
#include "ecs.hpp"
#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/xml.hpp>

namespace bengine {

std::size_t impl::base_component_t::type_counter = 1;
std::size_t entity_t::entity_counter{1}; // Not using zero since it is used as null so often
ecs default_ecs;

entity_t * ecs::entity(const std::size_t id) noexcept {
	entity_t * result = nullptr;
	auto finder = entity_store.find(id);
	if (finder == entity_store.end()) return result;
	if (finder->second.deleted) return result;
	result = &finder->second;
	return result;
}

entity_t * ecs::create_entity() {
    entity_t new_entity;
    while (entity_store.find(new_entity.id) != entity_store.end()) {
        ++entity_t::entity_counter;
        new_entity.id = entity_t::entity_counter;
    }
    //std::cout << "New Entity ID#: " << new_entity.id << "\n";

    entity_store.emplace(new_entity.id, new_entity);
    return entity(new_entity.id);
}

entity_t * ecs::create_entity(const std::size_t new_id) {
	entity_t new_entity(new_id);
    if (entity_store.find(new_entity.id) != entity_store.end()) {
        throw std::runtime_error("WARNING: Duplicate entity ID. Odd things will happen\n");
    }
	entity_store.emplace(new_entity.id, new_entity);
	return entity(new_entity.id);
}

void ecs::each(std::function<void(entity_t &)> &&func) {
	for (auto it=entity_store.begin(); it!=entity_store.end(); ++it) {
		if (!it->second.deleted) {
			func(it->second);
		}
	}
}

void ecs::ecs_save(std::unique_ptr<std::ofstream> &lbfile) {
    cereal::XMLOutputArchive oarchive(*lbfile);
    oarchive(*this);
}

void ecs::ecs_load(std::unique_ptr<std::ifstream> &lbfile) {
	entity_store.clear();
	component_store.clear();
    cereal::XMLInputArchive iarchive(*lbfile);
    iarchive(*this);
    std::cout << "Loaded " << entity_store.size() << " entities, and " << component_store.size() << " component types.\n";
}

}
