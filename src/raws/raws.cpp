#include "stdafx.h"
#include "raws.hpp"
#include "lua_bridge.hpp"
#include "../components/position.hpp"
#include "../components/renderable.hpp"
#include "../components/items/item.hpp"
#include "../components/items/item_stored.hpp"
#include "string_table.hpp"
#include "creatures.hpp"
#include "species.hpp"
#include "biomes.hpp"
#include "plants.hpp"
#include "life_events.hpp"
#include "profession.hpp"
#include "buildings.hpp"
#include "reactions.hpp"
#include "clothing.hpp"
#include "materials.hpp"
#include "../global_assets/spatial_db.hpp"
#include "graphviz.hpp"
#include "../global_assets/game_config.hpp"
#include "defs/item_def_t.hpp"
#include "defs/material_def_t.hpp"
#include "defs/clothing_t.hpp"
#include "items.hpp"
#include "../render_engine/vox/renderables.hpp"
#include "../components/item_tags/item_ammo_t.hpp"
#include "../components/item_tags/item_bone_t.hpp"
#include "../components/item_tags/item_chopping_t.hpp"
#include "../components/item_tags/item_digging_t.hpp"
#include "../components/item_tags/item_drink_t.hpp"
#include "../components/item_tags/item_food_t.hpp"
#include "../components/item_tags/item_hide_t.hpp"
#include "../components/item_tags/item_leather_t.hpp"
#include "../components/item_tags/item_melee_t.hpp"
#include "../components/item_tags/item_ranged_t.hpp"
#include "../components/item_tags/item_skull_t.hpp"
#include "../components/item_tags/item_spice_t.hpp"
#include "../components/items/item_quality.hpp"
#include "../components/items/item_wear.hpp"
#include "../components/items/item_creator.hpp"

std::unique_ptr<lua_lifecycle> lua_handle;

void sanity_check_raws() {
    sanity_check_materials();
    sanity_check_clothing();
    sanity_check_professions();
    sanity_check_items();
    sanity_check_buildings();
    sanity_check_reactions();
    sanity_check_plants();
    sanity_check_biomes();
    sanity_check_species();
    sanity_check_creatures();
    sanity_check_stockpiles();
}

void build_tech_tree_files() {
    std::cout << "Building DOT files\n";

    graphviz_t mats("material_tree.gv");
    build_material_acquisition_tech_tree(&mats);

    graphviz_t master("tech_tree.gv");
    build_material_tech_tree(&master);
    build_reaction_tree(&master);
    make_building_tree(&master);

    graphviz_t civs("civ_tree.gv");
    make_civ_tree(&civs);

    std::ofstream script(get_save_path() + std::string("/build-tech-tree.sh"));
    script << "#!/bin/bash\n";
    script << "dot -Tpng material_tree.gv -o material_tree.png\n";
    script << "dot -Tpng tech_tree.gv -o tech_tree.png\n";
    script << "dot -Tpng civ_tree.gv -o civ_tree.png\n";
    script.close();
}

void load_game_tables()
{
    read_material_types();
    read_material_textures();
	read_voxel_models();
    read_clothing();
    read_life_events();
    read_professions();
    read_stockpiles();
    read_items();
    read_buildings();
    read_reactions();
    read_plant_types();
    read_biome_types();
    read_biome_textures();
    read_species_types();
    read_creature_types();

    sanity_check_raws();

    //if (game_config.build_tech_trees) {
    //    build_tech_tree_files();
    //}
}

void load_raws() {
    using namespace string_tables;

	// Load string tables for first names and last names
	load_string_table(FIRST_NAMES_MALE, "world_defs/first_names_male.txt");
	load_string_table(FIRST_NAMES_FEMALE, "world_defs/first_names_female.txt");
	load_string_table(LAST_NAMES, "world_defs/last_names.txt");
    load_string_table(NEW_ARRIVAL_QUIPS, "world_defs/newarrival.txt");
    load_string_table(MENU_SUBTITLES, "world_defs/menu_text.txt");

	// Setup LUA
	lua_handle = std::make_unique<lua_lifecycle>();

	// Load game data via LUA
	load_string_table(-1, "world_defs/index.txt");
	for (const std::string &filename : string_table(-1)->strings) {
		load_lua_script("world_defs/" + filename);
	}
	std::cout << "Loading tables\n";

	// Extract game tables
	load_game_tables();
}

void spawn_item_on_ground(const int x, const int y, const int z, const std::string &tag, const std::size_t &material, uint8_t quality, uint8_t wear, std::size_t creator_id, std::string creator_name) {
    auto finder = get_item_def(tag);
    if (finder == nullptr) throw std::runtime_error(std::string("Unknown item tag: ") + tag);

    auto mat = get_material(material);
    if (!mat) throw std::runtime_error(std::string("Unknown material tag: ") + std::to_string(material));

    auto entity = bengine::create_entity()
        ->assign(position_t{ x,y,z })
        ->assign(renderable_t{ finder->glyph, finder->glyph_ascii, mat->fg, mat->bg, finder->voxel_model })
        ->assign(item_t{tag, finder->name, material, finder->stack_size, finder->clothing_layer})
		->assign(item_quality_t{ quality })
		->assign(item_wear_t{ wear })
		->assign(item_creator_t{ creator_id, creator_name });

    //std::cout << "Spawned item on ground: " << entity->id << ", " << entity->component<item_t>()->item_tag << "\n";
    entity_octree.add_node(octree_location_t{x,y,z,entity->id});
	render::models_changed = true;
}

void decorate_item_categories(bengine::entity_t &item, std::bitset<NUMBER_OF_ITEM_CATEGORIES> &categories) {
	if (categories.test(TOOL_CHOPPING)) item.assign(item_chopping_t{});
	if (categories.test(TOOL_DIGGING)) item.assign(item_digging_t{});
	if (categories.test(WEAPON_MELEE)) item.assign(item_melee_t{});
	if (categories.test(WEAPON_RANGED)) item.assign(item_ranged_t{});
	if (categories.test(WEAPON_AMMO)) item.assign(item_ammo_t{});
	if (categories.test(ITEM_FOOD)) item.assign(item_food_t{});
	if (categories.test(ITEM_SPICE)) item.assign(item_spice_t{});
	if (categories.test(ITEM_DRINK)) item.assign(item_drink_t{});
	if (categories.test(ITEM_HIDE)) item.assign(item_hide_t{});
	if (categories.test(ITEM_BONE)) item.assign(item_bone_t{});
	if (categories.test(ITEM_SKULL)) item.assign(item_skull_t{});
	if (categories.test(ITEM_LEATHER)) item.assign(item_leather_t{});
}

bengine::entity_t * spawn_item_on_ground_ret(const int x, const int y, const int z, const std::string &tag, const std::size_t &material, uint8_t quality, uint8_t wear, std::size_t creator_id, std::string creator_name) {
    auto finder = get_item_def(tag);
    if (finder == nullptr) throw std::runtime_error(std::string("Unknown item tag: ") + tag);

    auto mat = get_material(material);
    if (!mat) throw std::runtime_error(std::string("Unknown material tag: ") + std::to_string(material));

	auto entity = bengine::create_entity()
		->assign(position_t{ x,y,z })
		->assign(renderable_t{ finder->glyph, finder->glyph_ascii, mat->fg, mat->bg, finder->voxel_model })
		->assign(item_t{ tag, finder->name, material, finder->stack_size, finder->clothing_layer })
		->assign(item_quality_t{ quality })
		->assign(item_wear_t{ wear })
		->assign(item_creator_t{ creator_id, creator_name });
	decorate_item_categories(*entity, finder->categories);
    entity_octree.add_node(octree_location_t{x,y,z,entity->id});
	render::models_changed = true;
    return entity;
}

void spawn_item_in_container(const std::size_t container_id, const std::string &tag, const std::size_t &material, uint8_t quality, uint8_t wear, std::size_t creator_id, std::string creator_name) {
    auto finder = get_item_def(tag);
    if (finder == nullptr) throw std::runtime_error(std::string("Unknown item tag: ") + tag);

    auto mat = get_material(material);

    std::cout << "Spawning [" << tag << "], glyph " << +finder->glyph << "\n";

	auto entity = bengine::create_entity()
		->assign(item_stored_t{ container_id })
		->assign(renderable_t{ finder->glyph, finder->glyph_ascii, mat->fg, mat->bg, finder->voxel_model })
		->assign(item_t{ tag, finder->name, material, finder->stack_size, finder->clothing_layer })
		->assign(item_quality_t{ quality })
		->assign(item_wear_t{ wear })
		->assign(item_creator_t{ creator_id, creator_name });
	decorate_item_categories(*entity, finder->categories);
}

void spawn_item_carried(const std::size_t holder_id, const std::string &tag, const std::size_t &material, const item_location_t &loc, uint8_t quality, uint8_t wear, std::size_t creator_id, std::string creator_name) {

	auto mat = get_material(material);
	auto clothing_finder = get_clothing_by_tag(tag);
	if (clothing_finder) {
		// Clothing needs to be handled differently
		auto entity = bengine::create_entity()
			->assign(item_carried_t{ loc, holder_id })
			->assign(renderable_t{ clothing_finder->clothing_glyph, clothing_finder->clothing_glyph, mat->fg, mat->bg, clothing_finder->voxel_model })
			->assign(item_t{ tag })
			->assign(item_quality_t{ quality })
			->assign(item_wear_t{ wear })
			->assign(item_creator_t{ creator_id, creator_name });
		entity->component<item_t>()->material = material;
	} else {
		auto finder = get_item_def(tag);
		if (finder == nullptr) throw std::runtime_error(std::string("Unknown item tag: ") + tag);

		auto entity = bengine::create_entity()
			->assign(item_carried_t{ loc, holder_id })
			->assign(renderable_t{ finder->glyph, finder->glyph_ascii, mat->fg, mat->bg, finder->voxel_model })
			->assign(item_t{ tag, finder->name, material, finder->stack_size, finder->clothing_layer })
			->assign(item_quality_t{ quality })
			->assign(item_wear_t{ wear })
			->assign(item_creator_t{ creator_id, creator_name });
		decorate_item_categories(*entity, finder->categories);
	}
}
