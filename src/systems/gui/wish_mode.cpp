#include "stdafx.h"
#include "wish_mode.hpp"
#include "../../bengine/IconsFontAwesome.h"
#include "../../bengine/imgui.h"
#include "../../bengine/imgui_impl_glfw_gl3.h"
#include "../../global_assets/game_mode.hpp"
#include "../../global_assets/debug_flags.hpp"
#include "../../planet/region/region.hpp"
#include "../../global_assets/rng.hpp"
#include "../../bengine/ecs.hpp"
#include "../../components/position.hpp"
#include "../../components/explosion_t.hpp"

namespace systems {
	namespace wishmode {
		static char wish_command[255] = "";

		struct wish_t {
			std::string command;
			std::function<void()> func;
		};

		static std::vector<wish_t> wishes{
			wish_t{ "show flags", []() { debug::show_flags = !debug::show_flags; } },
			wish_t{ "show distance", []() { debug::show_dijkstra = !debug::show_dijkstra; }},
			wish_t{ "sploosh", []() {
			for (int y = 1; y<REGION_HEIGHT - 1; ++y) {
				for (int x = 1; x<REGION_WIDTH - 1; ++x) {
					region::set_water_level(mapidx(x,y,REGION_DEPTH - 2), 3);
				}
			}
			}},
			wish_t{ "asteroids", []() {
				using namespace bengine;
				const int n_missiles = rng.roll_dice(3,10);
				for (int i = 0; i<n_missiles; ++i) {
					const int x = rng.roll_dice(1, REGION_WIDTH - 1);
					const int y = rng.roll_dice(1, REGION_HEIGHT - 1);
					const int z = region::ground_z(x, y);
					create_entity()
						->assign(position_t{ x, y, z })
						->assign(explosion_t{ 10, 10, 5, 20 });
				}
			}},
			wish_t{ "nuke", []() {
				using namespace bengine;
				const int x = REGION_WIDTH / 2 - 20;
				const int y = REGION_HEIGHT / 2 - 20;
				const int z = std::max(REGION_DEPTH - 2, region::ground_z(x, y));
				create_entity()
					->assign(position_t{ x, y, z })
					->assign(explosion_t{ 10, 30, 10, 200 });
			}}
		};

		static void make_wish(const std::string &wish) {
			for (const auto &w : wishes) {
				if (w.command == wish) w.func();
			}
		}

		void run(const double &duration_ms) {
			ImGui::Begin("Make A Wish - Debug Command", nullptr, ImVec2{ 400, 100 }, ImGuiWindowFlags_AlwaysAutoResize);
			ImGui::InputText("Wish Command", wish_command, 255);
			if (ImGui::Button("Wish!")) {
				std::string wishstring(wish_command);
				make_wish(wishstring);
			}
			ImGui::SameLine();
			if (ImGui::Button("Close")) {
				game_master_mode = PLAY;
			}
			ImGui::End();
		}
	}
}
