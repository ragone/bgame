#include "stdafx.h"
#include "play_game.hpp"
#include "../bengine/imgui.h"
#include "../bengine/imgui_impl_glfw_gl3.h"
#include "../global_assets/game_planet.hpp"
#include "../bengine/telemetry.hpp"
#include "../bengine/filesystem.hpp"
#include "../planet/region/region.hpp"
#include "../render_engine/render_engine.hpp"
#include "../components/camera_options.hpp"
#include "../components/logger.hpp"
#include "../components/world_position.hpp"
#include "../components/calendar.hpp"
#include "../components/designations.hpp"
#include "../components/mining/mining_designations.hpp"
#include "../components/logger.hpp"
#include "../global_assets/game_camera.hpp"
#include "../systems/run_systems.hpp"
#include "../global_assets/game_calendar.hpp"
#include "../global_assets/game_designations.hpp"
#include "../global_assets/game_logger.hpp"
#include "../global_assets/game_mode.hpp"
#include "../global_assets/game_pause.hpp"
#include "../global_assets/game_mining.hpp"
#include "../ascii_engine/ascii_mode.hpp"

namespace play_game {

    static std::atomic<bool> loaded(false);
    static std::unique_ptr<std::thread> loader_thread = nullptr;

    static void do_load_game() {
        using namespace bengine;

        call_home("menu", "playgame");
        // Load the game
        std::cout << "Loading the planet\n";
        planet = load_planet();

        // Load the ECS
        std::cout << "Loading game state\n";
        {
            const std::string save_filename = get_save_path() + std::string("/savegame.dat");
            std::unique_ptr<std::ifstream> lbfile = std::make_unique<std::ifstream>(save_filename, std::ios::in | std::ios::binary);
            ecs_load(lbfile);
        }

        // Load the current region - check the camera for the world position
        std::cout << "Storing important entity handles\n";

        int region_x, region_y;
        each<world_position_t, calendar_t, designations_t, logger_t, camera_options_t, mining_designations_t>(
			[&region_x, &region_y] (entity_t &entity, world_position_t &pos, calendar_t &cal, designations_t &design,
                                    logger_t &log, camera_options_t &camera_prefs, mining_designations_t &mining) 
		{
            camera_entity = entity.id;
            region_x = pos.world_x;
            region_y = pos.world_y;
            camera_position = &pos;
            calendar = &cal;
            designations = &design;
            logger = &log;
            camera = &camera_prefs;
			mining_designations = &mining;
        });

         // Loading the region
        std::cout << "Loading the region\n";
        region::load_current_region(region_x, region_y);
        region::tile_recalc_all();

        // Setup systems
        std::cout << "Setting up systems\n";
        //add_systems_to_ecs();
        systems::init();

        std::cout << "Go!\n";

		game_master_mode = PLAY;
		pause_mode = ONE_STEP;

        loaded = true;
    }

    void tick(const double &duration_ms) {
        if (!loader_thread) {
            loader_thread = std::make_unique<std::thread>(do_load_game);
        }

        ImGui_ImplGlfwGL3_NewFrame();

        if (!loaded) {
            ImGui::Begin("Please wait - loading");
            ImGui::Text("Play game");
            ImGui::End();
        } else {
            // Game render goes here
			if (camera->ascii_mode) {
				render::ascii_render(duration_ms);
			}
			else {
				render::render_gl(duration_ms);
			}

            //ImGui::Begin("Please wait - not written yet");
            //ImGui::Text("Frame time: %f ms, %f FPS", duration_ms, 1000.0/duration_ms);
            //ImGui::End();

            systems::run(duration_ms);
        }

        ImGui::Render();
    }
}