#include "stdafx.h"
#include "ai_idle_time.hpp"
#include "../../../components/ai_tags/ai_tag_my_turn.hpp"
#include "../../../components/grazer_ai.hpp"
#include "../../../components/sentient_ai.hpp"
#include "../../../planet/region/region.hpp"
#include "../../../global_assets/rng.hpp"
#include "../../../raws/raws.hpp"
#include "../../../raws/materials.hpp"
#include "../../../components/riding_t.hpp"
#include "../../../global_assets/game_planet.hpp"
#include "../distance_map_system.hpp"
#include "../../physics/movement_system.hpp"
#include "../../../components/settler_ai.hpp"
#include "../../../components/ai_tags/ai_mode_idle.hpp"
#include "../../../render_engine/vox/renderables.hpp"
#include "../../physics/vegetation_system.hpp"
#include "../../../components/name.hpp"

namespace systems {
	namespace ai_idle_time {

		using namespace bengine;
		using namespace region;
		using namespace dijkstra;
		using namespace distance_map;
		using namespace movement;

		void idle_grazer(entity_t &e, ai_tag_my_turn_t &t, grazer_ai &grazer) {
			auto pos = e.component<position_t>();
			delete_component<ai_tag_my_turn_t>(e.id);

			// Grazers simply eat vegetation or move
			const auto idx = mapidx(pos->x, pos->y, pos->z);
			if (veg_type(idx) > 0 && rng.roll_dice(1,6)>3) {
				if (rng.roll_dice(1, 6) == 1) {
					vegetation::inflict_damage(idx, 1);
				}
			}
			else {
				request_random_move(e.id);
				distance_map::refresh_hunting_map();
			}

			const int poop_chance = rng.roll_dice(1, 100);
			if (poop_chance == 100) {
				std::string cname = "";
				auto name = e.component<name_t>();
				if (name) cname = name->first_name + std::string(" ") + name->last_name;
				spawn_item_on_ground(pos->x, pos->y, pos->z, "dung", get_material_by_tag("organic"), 3, 100, e.id, cname);
			}

			delete_component<ai_tag_my_turn_t>(e.id);
		}

		void idle_sentient(entity_t &e, ai_tag_my_turn_t &t, sentient_ai &sentient) {
			auto mounted = e.component<riding_t>();
			auto pos = e.component<position_t>();

			if (mounted) {
				auto mount = entity(mounted->riding);
				if (!mount) {
					delete_component<riding_t>(e.id);
				}
			}

			int feelings = planet.civs.civs[sentient.civ_id].cordex_feelings;

			if (sentient.hostile || feelings < 0) {
				sentient.goal = SENTIENT_GOAL_KILL;
				sentient.hostile = true;
			}
			else {
				sentient.goal = SENTIENT_GOAL_IDLE;
				sentient.hostile = false;
			}

			if (sentient.goal == SENTIENT_GOAL_KILL) {
				sentient.hostile = true;
				//std::cout << "Sentient kill mode\n";
				// Close for the kill!
				const int idx = mapidx(*pos);
				if (settler_map.get(idx) < MAX_DIJSTRA_DISTANCE - 1) {
					position_t destination = settler_map.find_destination(*pos);
					move_to(e.id, destination);
					render::models_changed = true;
				}
				else {
					sentient.goal = SENTIENT_GOAL_IDLE;
				}
			}
			else {
				// Wander aimlessly
				request_random_move(e.id);
			}
			delete_component<ai_tag_my_turn_t>(e.id);
		}

		void idle_iterator(entity_t &e, ai_tag_my_turn_t &t, ai_mode_idle_t &idle, settler_ai_t &settler) {
			// Nothing for settlers to do, currently.
		}

		void run(const double &duration_ms) {
			each<ai_tag_my_turn_t, ai_mode_idle_t, settler_ai_t>(idle_iterator);
			each<ai_tag_my_turn_t, sentient_ai>(idle_sentient);
			each<ai_tag_my_turn_t, grazer_ai>(idle_grazer);
		}
	}
}