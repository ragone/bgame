#include "stdafx.h"
#include "ascii_mode.hpp"
#include "../bengine/main_window.hpp"
#include "../planet/region/region.hpp"
#include "../global_assets/game_camera.hpp"
#include "../raws/materials.hpp"
#include "../raws/defs/material_def_t.hpp"
#include "../global_assets/texture_storage.hpp"
#include "../global_assets/shader_storage.hpp"
#include "../bengine/textures.hpp"
#include "../render_engine/fbo/buffertest.hpp"
#include "../raws/plants.hpp"
#include "../raws/defs/plant_t.hpp"
#include "../components/renderable.hpp"
#include "../components/building.hpp"
#include "../components/renderable_composite.hpp"
#include "../systems/mouse.hpp"
#include "../global_assets/game_mode.hpp"
#include "../global_assets/game_designations.hpp"
#include "../global_assets/game_building.hpp"
#include "../raws/buildings.hpp"
#include "../raws/defs/building_def_t.hpp"
#include "../systems/ai/inventory_system.hpp"
#include "../render_engine/chunks/chunks.hpp"
#include "../systems/gui/particle_system.hpp"
#include "../render_engine/pointlights.hpp"
#include "../render_engine/render_engine.hpp"
#include "../render_engine/vox/renderables.hpp"
#include "../global_assets/game_mining.hpp"
#include "../systems/gui/design_mining.hpp"
#include "../raws/plants.hpp"
#include "../raws/defs/plant_t.hpp"
#include <array>

namespace render {

	namespace ascii {

		struct vertex_t {
			float x;
			float y;
			float world_x;
			float world_y;
			float world_z;
			float tx;
			float ty;
			float br;
			float bg;
			float bb;
			float r;
			float g;
			float b;
		};
		static constexpr int N_VERTICES = 13;

		static unsigned int ascii_vao = 0;
		static unsigned int ascii_vbo = 0;
		unsigned int ascii_fbo = 0;
		unsigned int ascii_position_tex = 0;
		static unsigned int albedo_tex = 0;
		static bool first_run = true;

		static std::array<glyph_t, REGION_WIDTH * REGION_HEIGHT> terminal;
		static std::array<vertex_t, REGION_WIDTH * REGION_HEIGHT * 6> buffer;
		static glm::mat4 camera_projection_matrix;
		static glm::mat4 camera_modelview_matrix;

		static std::map<int, std::vector<glyph_t>> renderables;
		static uint8_t cycle = 0;
		static double cycle_timer = 0.0;
		static constexpr double CYCLE_TIME = 500.0;

		static void build_buffers() {
			int width, height;
			glfwGetWindowSize(bengine::main_window, &width, &height);

			// Make the FBO
			glGenFramebuffers(1, &ascii_fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, ascii_fbo);

			// position color buffer
			glGenTextures(1, &ascii_position_tex);
			glBindTexture(GL_TEXTURE_2D, ascii_position_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ascii_position_tex, 0);

			// color buffer
			glGenTextures(1, &albedo_tex);
			glBindTexture(GL_TEXTURE_2D, albedo_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, albedo_tex, 0);

			unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
			glDrawBuffers(2, attachments);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				std::cout << "Framebuffer not complete!" << std::endl;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// Make the VBO and VAO combination
			glGenVertexArrays(1, &ascii_vao);
			glGenBuffers(1, &ascii_vbo);

			/*
			ASCII vertex:
			0,1			Position
			2,3,4		World Position
			5,6			Texture Co-Ordinates
			7,8,9		Background Color
			10,11,12	Foreground Color
			*/
			glBindVertexArray(ascii_vao);
			glBindBuffer(GL_ARRAY_BUFFER, ascii_vbo);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, N_VERTICES * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0); // 0 = Vertex Position

			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, N_VERTICES * sizeof(float), (char *) nullptr + 2 * sizeof(float));
			glEnableVertexAttribArray(1); // 1 = World Position

			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, N_VERTICES * sizeof(float), (char *) nullptr + 5 * sizeof(float));
			glEnableVertexAttribArray(2); // 2 = Texture

			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, N_VERTICES * sizeof(float), (char *) nullptr + 7 * sizeof(float));
			glEnableVertexAttribArray(3); // 3 = Background

			glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, N_VERTICES * sizeof(float), (char *) nullptr + 10 * sizeof(float));
			glEnableVertexAttribArray(4); // 4 = Foreground

			glBindVertexArray(0);
			glCheckError();
		}

		static void ascii_camera() {
			int screen_w, screen_h;
			glfwGetWindowSize(bengine::main_window, &screen_w, &screen_h);
			camera_projection_matrix = glm::perspective(glm::radians(90.0f), (float)screen_w / (float)screen_h, 1.0f, 300.0f);
			const glm::vec3 up{ 0.0f, 1.0f, 0.0f };
			const glm::vec3 target{ (float)camera_position->region_x, (float)camera_position->region_z, (float)camera_position->region_y };
			glm::vec3 camera_position_v;
			camera_position_v = { (float)camera_position->region_x, ((float)camera_position->region_z) + (float)camera->zoom_level, ((float)camera_position->region_y) + 0.1f };
			camera_modelview_matrix = glm::lookAt(camera_position_v, target, up);
		}

		static constexpr int termidx(const int x, const int y) {
			return (y * REGION_HEIGHT) + x;
		}

		static inline glyph_t get_material_glyph(const int &idx, uint8_t glyph_override = 0, bool kill_background = false) {
			const std::size_t material_index = region::material(idx);
			const auto mat = get_material(material_index);
			glyph_t result;
			if (mat) {
				result = glyph_t{ glyph_override > 0 ? glyph_override : static_cast<uint8_t>(mat->glyph), mat->fg.r, mat->fg.g, mat->fg.b, mat->bg.r, mat->bg.g, mat->bg.b };
			}
			else {
				result = glyph_t{ glyph_override, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };
			}
			if (kill_background) {
				result.br = 0.0f;
				result.bb = 0.0f;
				result.bg = 0.0f;
			}
			return result;
		}

		static inline glyph_t get_floor_tile(const int &idx) {
			if (region::flag(idx, CONSTRUCTION)) {
				auto glyph = get_material_glyph(idx, '+');
				glyph.bb = 0.0;
				glyph.bg = 0.0;
				glyph.br = 0.0;
				return glyph;
			}
			else {
				size_t vegtype = region::veg_type(idx);
				if (vegtype > 0) {
					return region::veg_ascii_cache(idx);
				}
				else {
					auto glyph = get_material_glyph(idx, '.');
					glyph.bb = 0.0;
					glyph.bg = 0.0;
					glyph.br = 0.0;
					return glyph;
				}
			}
		}

		static inline glyph_t get_wall_tile(const int &idx) {
			uint8_t wall_mask = 0;
			if (region::tile_type(idx - 1) == tile_type::WALL) wall_mask += 1;
			if (region::tile_type(idx + 1) == tile_type::WALL) wall_mask += 2;
			if (region::tile_type(idx - REGION_WIDTH) == tile_type::WALL) wall_mask += 4;
			if (region::tile_type(idx + REGION_WIDTH) == tile_type::WALL) wall_mask += 8;

			uint8_t glyph = 0;
			switch (wall_mask) {
			case 0:
				glyph = 79;
				break; // Isolated
			case 1:
				glyph = 181;
				break; // West only
			case 2:
				glyph = 198;
				break; // East only
			case 3:
				glyph = 205;
				break; // East and West
			case 4:
				glyph = 208;
				break; // North only
			case 5:
				glyph = 188;
				break; // North and west
			case 6:
				glyph = 200;
				break; // North and east
			case 7:
				glyph = 202;
				break; // North and east/west
			case 8:
				glyph = 210;
				break; // South only
			case 9:
				glyph = 187;
				break; // South and west
			case 10:
				glyph = 201;
				break; // South and east
			case 11:
				glyph = 203;
				break; // South east/west
			case 12:
				glyph = 186;
				break; // North and South
			case 13:
				glyph = 185;
				break; // North/South/West
			case 14:
				glyph = 204;
				break; // North/South/East
			case 15:
				glyph = 206;
				break; // All
			default: {
				std::cout << "WARNING: Wall calculator hit a case of " << +wall_mask << "\n";
				glyph = 79;
			}
			}
			return get_material_glyph(idx, glyph);
		}

		static inline void populate_renderables() {
			renderables.clear();

			// Add buildings
			bengine::each<building_t, position_t>([](bengine::entity_t &e, building_t &b, position_t &pos) {
				if (b.glyphs_ascii.empty()) {
					std::cout << "WARNING: Building [" << b.tag << "] is lacking ASCII render data.\n";
					return;
				}
				int i = 0;
				int offX = b.width == 3 ? -1 : 0;
				int offY = b.height == 3 ? -1 : 0;
				for (int y = 0; y < b.height; ++y) {
					for (int x = 0; x < b.width; ++x) {
						const float R = b.complete ? b.glyphs_ascii[i].foreground.r : 0.3f;
						const float G = b.complete ? b.glyphs_ascii[i].foreground.g : 0.3f;
						const float B = b.complete ? b.glyphs_ascii[i].foreground.b : 0.3f;
						const float BR = b.complete ? b.glyphs_ascii[i].background.r : 0.0f;
						const float BG = b.complete ? b.glyphs_ascii[i].background.g : 0.0f;
						const float BB = b.complete ? b.glyphs_ascii[i].background.b : 0.0f;
						const int idx = mapidx(pos.x + x + offX, pos.y + y + offY, pos.z);
						renderables[idx].push_back(glyph_t{ static_cast<uint8_t>(b.glyphs_ascii[i].glyph), R, G, B, BR, BG, BB });
						++i;
					}
				}
			});

			// Add renderables
			bengine::each<renderable_t, position_t>([](bengine::entity_t &e, renderable_t &r, position_t &pos) {
				const int idx = mapidx(pos.x, pos.y, pos.z);
				renderables[idx].push_back(glyph_t{ static_cast<uint8_t>(r.glyph_ascii), r.foreground.r, r.foreground.g, r.foreground.b, r.background.r, r.background.g, r.background.b });
			});
			bengine::each<renderable_composite_t, position_t>([](bengine::entity_t &e, renderable_composite_t &r, position_t &pos) {
				const int idx = mapidx(pos.x, pos.y, pos.z);
				renderables[idx].push_back(glyph_t{ static_cast<uint8_t>(r.ascii_char), 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f });
			});

			// Add particles
			for (const auto &p : systems::particles::positions) {
				const int x = static_cast<int>(p.x);
				const int y = static_cast<int>(p.z);
				const int z = static_cast<int>(p.y);
				const int idx = mapidx(x, y, z);
				renderables[idx].push_back(glyph_t{ '*', p.r, p.g, p.b, 0.0f, 0.0f, 0.0f });
			}
		}

		static inline glyph_t get_dive_tile(const int &idx) {
			glyph_t result = glyph_t{ ' ', 0, 0, 0, 0, 0, 0 };
			int dive_depth = 1;
			constexpr int MAX_DIVE = 7;
			int check_idx = idx - (REGION_WIDTH * REGION_HEIGHT);
			bool done = false;
			while (check_idx > 0 && dive_depth < MAX_DIVE && !done) {
				if (!renderables[check_idx].empty()) {
					const std::vector<glyph_t> * element = &renderables[check_idx];
					const auto n_renderables = element->size();
					std::size_t element_idx = cycle % n_renderables;
					result = element->at(element_idx);
				}
				else if (region::revealed(check_idx)) {
					const uint8_t ttype = region::tile_type(check_idx);
					switch (ttype) {
					case tile_type::SEMI_MOLTEN_ROCK: result = glyph_t{ 177, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f }; break;
					case tile_type::SOLID: result = get_material_glyph(check_idx); break;
					case tile_type::WALL: result = get_wall_tile(check_idx); break;
					case tile_type::RAMP: result = get_material_glyph(check_idx, 30, true); break;
					case tile_type::STAIRS_UP: result = get_material_glyph(check_idx, '<'); break;
					case tile_type::STAIRS_DOWN: result = get_material_glyph(check_idx, '>'); break;
					case tile_type::STAIRS_UPDOWN: result = get_material_glyph(check_idx, 'X'); break;
					case tile_type::FLOOR: result = get_floor_tile(check_idx); break;
					case tile_type::TREE_TRUNK: result = glyph_t{ 186, 1.0f, 1.0f, 0.5f, 0, 0, 0 }; break;
					case tile_type::TREE_LEAF: result = glyph_t{ 5, 0.0f, 1.0f, 0, 0, 0, 0 }; break;
					case tile_type::WINDOW: result = get_material_glyph(check_idx, 176); break;
					case tile_type::CLOSED_DOOR: result = get_material_glyph(check_idx, '+'); break;
					default: result = glyph_t{ ' ', 0, 0, 0, 0, 0, 0 };
					}

					// Add water - TODO: Add some variance
					switch (region::water_level(check_idx)) {
					case 1: result = glyph_t{ '1', 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f }; break;
					case 2: result = glyph_t{ '2', 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f }; break;
					case 3: result = glyph_t{ '3', 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f }; break;
					case 4: result = glyph_t{ '4', 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f }; break;
					case 5: result = glyph_t{ '5', 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f }; break;
					case 6: result = glyph_t{ '6', 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f }; break;
					case 7: result = glyph_t{ '7', 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f }; break;
					case 8: result = glyph_t{ '8', 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f }; break;
					case 9: result = glyph_t{ '9', 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f }; break;
					case 10: result = glyph_t{ '0', 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f }; break;
					}
				}

				if (result.glyph != ' ') {
					done = true;
					const float darken = 1.0f - (static_cast<float>(dive_depth) * 0.3f);
					result.r *= darken;
					result.g *= darken;
					result.b *= darken;
					result.br *= darken;
					result.bb *= darken;
					result.bg *= darken;
				}
				else {
					++dive_depth;
					check_idx -= (REGION_WIDTH * REGION_HEIGHT);
				}
			}

			return result;
		}

		static float ascii_frame_counter = 0.0f;

		static inline void populate_ascii() {
			const int z = camera_position->region_z;
			ascii_frame_counter += 0.01f;
			const float water_variance = std::sin(ascii_frame_counter);


			// Add terrain
			for (int y = 0; y < REGION_HEIGHT; ++y) {
				for (int x = 0; x < REGION_WIDTH; ++x) {
					const int tidx = termidx(x, y);
					const int idx = mapidx(x, y, z);

					// Put in terrain
					if (!renderables[idx].empty()) {
						const auto n_renderables = renderables[idx].size();
						std::size_t element_idx = cycle % n_renderables;
						terminal[tidx] = renderables[idx][element_idx];
					} 
					else if (region::revealed(idx)) {
						const uint8_t ttype = region::tile_type(idx);
						switch (ttype) {
						case tile_type::SEMI_MOLTEN_ROCK: terminal[tidx] = glyph_t{ 177, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f }; break;
						case tile_type::SOLID: terminal[tidx] = get_material_glyph(idx); break;
						case tile_type::WALL: terminal[tidx] = get_wall_tile(idx); break;
						case tile_type::RAMP: terminal[tidx] = get_material_glyph(idx, 30, true); break;
						case tile_type::STAIRS_UP: terminal[tidx] = get_material_glyph(idx, '<'); break;
						case tile_type::STAIRS_DOWN: terminal[tidx] = get_material_glyph(idx, '>'); break;
						case tile_type::STAIRS_UPDOWN: terminal[tidx] = get_material_glyph(idx, 'X'); break;
						case tile_type::FLOOR: terminal[tidx] = get_floor_tile(idx); break;
						case tile_type::TREE_TRUNK: terminal[tidx] = glyph_t{ 186, 1.0f, 1.0f, 0.5f, 0, 0, 0 }; break;
						case tile_type::TREE_LEAF: terminal[tidx] = glyph_t{ 5, 0.0f, 1.0f, 0, 0, 0, 0 }; break;
						case tile_type::WINDOW: terminal[tidx] = get_material_glyph(idx, 176); break;
						case tile_type::CLOSED_DOOR: terminal[tidx] = get_material_glyph(idx, '+'); break;
						case tile_type::OPEN_SPACE: terminal[tidx] = get_dive_tile(idx); break;
						default: terminal[tidx] = glyph_t{ ' ', 0, 0, 0, 0, 0, 0 };
						}

						// Add water
						switch (region::water_level(idx)) {
						case 1: terminal[tidx] = glyph_t{ '1', 0.0f, 0.0f, 1.0f - water_variance, 0.0f, 0.0f, 0.0f }; break;
						case 2: terminal[tidx] = glyph_t{ '2', 0.0f, 0.0f, 1.0f - water_variance, 0.0f, 0.0f, 0.0f }; break;
						case 3: terminal[tidx] = glyph_t{ '3', 0.0f, 0.0f, 1.0f - water_variance, 0.0f, 0.0f, 0.0f }; break;
						case 4: terminal[tidx] = glyph_t{ '4', 0.0f, 0.0f, 1.0f - water_variance, 0.0f, 0.0f, 0.0f }; break;
						case 5: terminal[tidx] = glyph_t{ '5', 0.0f, 0.0f, 1.0f - water_variance, 0.0f, 0.0f, 0.0f }; break;
						case 6: terminal[tidx] = glyph_t{ '6', 0.0f, 0.0f, 1.0f - water_variance, 0.0f, 0.0f, 0.0f }; break;
						case 7: terminal[tidx] = glyph_t{ '7', 0.0f, 0.0f, 1.0f - water_variance, 0.0f, 0.0f, 0.0f }; break;
						case 8: terminal[tidx] = glyph_t{ '8', 0.0f, 0.0f, 1.0f - water_variance, 0.0f, 0.0f, 0.0f }; break;
						case 9: terminal[tidx] = glyph_t{ '9', 0.0f, 0.0f, 1.0f - water_variance, 0.0f, 0.0f, 0.0f }; break;
						case 10: terminal[tidx] = glyph_t{ '0', 0.0f, 0.0f, 1.0f - water_variance, 0.0f, 0.0f, 0.0f }; break;
						}

						// Blood stains
						if (region::blood_stain(idx)) {
							terminal[tidx].br = 0.7f;
							terminal[tidx].bg = 0.0f;
							terminal[tidx].bb = 0.0f;
						}
					}
					else {
						terminal[tidx] = glyph_t{ ' ', 0, 0, 0, 0, 0, 0 };
					}
				}
			}			

			if (game_master_mode == DESIGN && game_design_mode == BUILDING && buildings::has_build_mode_building) {
				// We have a building selected; determine if it can be built and show it
				const auto tag = buildings::build_mode_building.tag;
				auto building_def = get_building_def(tag);
				if (building_def) {
					// We have the model and the definition; see if its possible to build
					bool can_build = true;

					std::vector<int> target_tiles;
					for (int y = systems::mouse_wy; y < systems::mouse_wy + building_def->height; ++y) {
						for (int x = systems::mouse_wx; x < systems::mouse_wx + building_def->width; ++x) {
							const auto idx = mapidx(x, y, camera_position->region_z);
							if (!region::flag(idx, CAN_STAND_HERE)) can_build = false;
							if (region::flag(idx, CONSTRUCTION)) can_build = false;
							target_tiles.emplace_back(idx);
						}
					}

					auto x = (float)systems::mouse_wx;
					const auto z = (float)camera_position->region_z;
					auto y = (float)systems::mouse_wy;

					if (building_def->glyphs_ascii.empty()) {
						std::cout << "WARNING: Building [" << building_def->tag << "] has no ASCII data.\n";
					}
					else {

						int i = 0;
						int offX = building_def->width == 3 ? -1 : 0;
						int offY = building_def->height == 3 ? -1 : 0;
						for (int Y = 0; Y < building_def->height; ++Y) {
							for (int X = 0; X < building_def->width; ++X) {
								const float R = can_build ? building_def->glyphs_ascii[i].foreground.r : 1.0f;
								const float G = can_build ? building_def->glyphs_ascii[i].foreground.g : 0.3f;
								const float B = can_build ? building_def->glyphs_ascii[i].foreground.b : 0.3f;
								const float BR = can_build ? building_def->glyphs_ascii[i].background.r : 0.0f;
								const float BG = can_build ? building_def->glyphs_ascii[i].background.g : 0.0f;
								const float BB = can_build ? building_def->glyphs_ascii[i].background.b : 0.0f;
								terminal[termidx(x + X + offX, Y + y + offY)] = glyph_t{ static_cast<uint8_t>(building_def->glyphs_ascii[i].glyph), R, G, B, BR, BG, BB };
								++i;
							}
						}

						if (can_build) {
							if (systems::left_click) {
								// Perform the building
								systems::inventory_system::building_request(systems::mouse_wx, systems::mouse_wy, systems::mouse_wz, buildings::build_mode_building);
								buildings::has_build_mode_building = false;
								for (const auto &idx : target_tiles) {
									region::set_flag(idx, CONSTRUCTION);
								}
							}
						}
					}
				}
			}			
		}

		static inline void render_cursors() {
			if (systems::mouse_wx < 1 || systems::mouse_wx > REGION_WIDTH || systems::mouse_wy < 1 || systems::mouse_wy > REGION_HEIGHT) return;

			// Highlight the mouse position in yellow background
			if (game_master_mode != DESIGN) {
				terminal[termidx(systems::mouse_wx, systems::mouse_wy)].br = 1.0f;
				terminal[termidx(systems::mouse_wx, systems::mouse_wy)].bg = 1.0f;
				terminal[termidx(systems::mouse_wx, systems::mouse_wy)].bb = 0.0f;
			}

			if (game_master_mode == DESIGN) {
				if (game_design_mode == CHOPPING) {
					for (size_t i = 0; i < REGION_TILES_COUNT; ++i) {
						auto tree_id = region::tree_id(i);
						if (tree_id > 0) {
							auto[x, y, z] = idxmap(i);
							if (designations->chopping.find(tree_id) != designations->chopping.end() && z == camera_position->region_z) {
								terminal[termidx(x, y)].br = 1.0f;
								terminal[termidx(x, y)].bg = 0.0f;
								terminal[termidx(x, y)].bb = 0.0f;
							}
						}
					}
				}
				else if (game_design_mode == GUARDPOINTS) {
					for (const auto& gp : designations->guard_points) {
						if (gp.second.z == camera_position->region_z) {
							terminal[termidx(gp.second.x, gp.second.y)].br = 1.0f;
							terminal[termidx(gp.second.x, gp.second.y)].bg = 0.0f;
							terminal[termidx(gp.second.x, gp.second.y)].bb = 0.0f;
						}
					}
				}
				else if (game_design_mode == HARVEST) {
					for (const auto& gp : designations->harvest) {
						if (gp.second.z == camera_position->region_z) {
							terminal[termidx(gp.second.x, gp.second.y)].br = 1.0f;
							terminal[termidx(gp.second.x, gp.second.y)].bg = 0.0f;
							terminal[termidx(gp.second.x, gp.second.y)].bb = 0.0f;
						}
					}
				}
				else if (game_design_mode == STOCKPILES) {
					for (size_t i = 0; i < REGION_TILES_COUNT; ++i) {
						auto stockpile_id = region::stockpile_id(i);
						if (stockpile_id > 0 && stockpile_id == current_stockpile) {
							auto[x, y, z] = idxmap(i);
							if (z == camera_position->region_z) {
								terminal[termidx(x, y)].br = 1.0f;
								terminal[termidx(x, y)].bg = 0.0f;
								terminal[termidx(x, y)].bb = 0.0f;
							}
						}
					}
				}
				else if (game_design_mode == ARCHITECTURE) {
					for (const auto &arch : designations->architecture) {
						auto[x, y, z] = idxmap(arch.first);
						uint8_t glyph = 1;
						switch (arch.second) {
						case 0: glyph = 219; break; // Wall
						case 1: glyph = '+'; break; // Floor
						case 2: glyph = '<'; break; // Up
						case 3: glyph = '>'; break; // Down
						case 4: glyph = 'X'; break; // Up-Down
						case 5: glyph = 30; break; // Ramp
						case 6: glyph = '#'; break; // Bridge
						}
						terminal[termidx(x, y)] = glyph_t{ glyph, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f};
					}
				}
				else if (game_design_mode == DIGGING) {
					// Add the "hot" list
					for (const auto &idx : systems::design_mining::mining_cursor_list) {
						auto[x, y, z] = idxmap(idx.first);
						if (z == camera_position->region_z) {
							uint8_t glyph = 1;
							switch (idx.second) {
							case MINE_DIG: glyph = 176; break;
							case MINE_CHANNEL: glyph = 31; break;
							case MINE_RAMP: glyph = 30; break;
							case MINE_STAIRS_UP: glyph = '<'; break;
							case MINE_STAIRS_DOWN: glyph = '>'; break;
							case MINE_STAIRS_UPDOWN: glyph = 'X'; break;
							}
							terminal[termidx(x, y)] = glyph_t{ glyph, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
						}
					}

					// Actual designations
					for (const auto &idx : mining_designations->mining_targets) {
						auto[x, y, z] = idxmap(idx.first);
						if (z == camera_position->region_z) {
							uint8_t glyph = 1;
							switch (idx.second) {
							case MINE_DIG: glyph = 176; break;
							case MINE_CHANNEL: glyph = 31; break;
							case MINE_RAMP: glyph = 30; break;
							case MINE_STAIRS_UP: glyph = '<'; break;
							case MINE_STAIRS_DOWN: glyph = '>'; break;
							case MINE_STAIRS_UPDOWN: glyph = 'X'; break;
							}
							terminal[termidx(x, y)] = glyph_t{ glyph, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
						}
					}
				}
			}
		}

		static inline void render_ascii_ambient()
		{
			constexpr float glyph_width_texture_space = 1.0f / 16.0f;

			const float wz = static_cast<float>(camera_position->region_z);
			constexpr float width = 1.0f;
			constexpr float height = 1.0f;
			std::size_t buffer_idx = 0;
			for (int y = 0; y < REGION_HEIGHT; ++y) {
				for (int x = 0; x < REGION_WIDTH; ++x) {
					const int tidx = termidx(x, y);
					const float wx = static_cast<float>(x);
					const float wy = static_cast<float>(y);

					const float x0 = -0.5f + wx;
					const float x1 = x0 + width;
					const float z0 = -0.5f + wy;
					const float z1 = z0 + height;

					const float TX0 = static_cast<float>(terminal[tidx].glyph % 16) * glyph_width_texture_space;
					const float TY0 = (terminal[tidx].glyph / 16) * glyph_width_texture_space;
					const float TW = TX0 + glyph_width_texture_space;
					const float TH = TY0 + glyph_width_texture_space;
					const float R = terminal[tidx].r * 0.25f;
					const float G = terminal[tidx].g * 0.25f;
					const float B = terminal[tidx].b * 0.25f;
					const float BR = terminal[tidx].br * 0.25f;
					const float BG = terminal[tidx].bg * 0.25f;
					const float BB = terminal[tidx].bb * 0.25f;

					buffer[buffer_idx] =   vertex_t{ x1, z1, wx, wy, wz, TW, TH, R, G, B, BR, BG, BB};
					buffer[buffer_idx +1] = vertex_t{ x1, z0, wx, wy, wz, TW, TY0, R, G, B, BR, BG, BB };
					buffer[buffer_idx +2] = vertex_t{ x0, z0, wx, wy, wz, TX0, TY0, R, G, B, BR, BG, BB };
					buffer[buffer_idx +3] = vertex_t{ x0, z0, wx, wy, wz, TX0, TY0, R, G, B, BR, BG, BB };
					buffer[buffer_idx +4] = vertex_t{ x0, z1, wx, wy, wz, TX0, TH, R, G, B, BR, BG, BB };
					buffer[buffer_idx +5] = vertex_t{ x1, z1, wx, wy, wz, TW, TH, R, G, B, BR, BG, BB };

					buffer_idx += 6;
				}
			}


			// Map the data
			//glInvalidateBufferData(ascii_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, ascii_vbo);
			glBufferData(GL_ARRAY_BUFFER, buffer.size() * N_VERTICES * sizeof(float), &buffer[0], GL_DYNAMIC_DRAW);
			glCheckError();
		}

		static inline void present() {
			glDisable(GL_DEPTH_TEST);
			glBindFramebuffer(GL_FRAMEBUFFER, ascii_fbo);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			glCheckError();
			glUseProgram(assets::ascii_shader);
			glCheckError();
			glBindVertexArray(ascii_vao);
			glCheckError();
			glUniformMatrix4fv(glGetUniformLocation(assets::ascii_shader, "projection_matrix"), 1, GL_FALSE, glm::value_ptr(camera_projection_matrix));
			glUniformMatrix4fv(glGetUniformLocation(assets::ascii_shader, "view_matrix"), 1, GL_FALSE, glm::value_ptr(camera_modelview_matrix));
			glUniform1i(glGetUniformLocation(assets::ascii_shader, "ascii_tex"), 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, assets::ascii_texture->texture_id);
			glDrawArrays(GL_TRIANGLES, 0, buffer.size());
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}

	void ascii_render(const double &duration_ms) 
	{
		ascii::cycle_timer += duration_ms;
		if (ascii::cycle_timer > ascii::CYCLE_TIME) {
			ascii::cycle_timer = 0.0;
			++ascii::cycle;
		}

		// If necessary, build the ASCII grid buffers
		if (ascii::ascii_vao == 0) ascii::build_buffers();

		// Chunks
		if (!chunks::chunks_initialized) {
			chunks::initialize_chunks();
		}
		chunks::update_dirty_chunks();
		chunks::update_buffers();
		render::update_buffers();

		// Compile the ASCII render data
		ascii::ascii_camera();
		ascii::populate_renderables();
		ascii::first_run = false;
		render::models_changed = false;
		render::camera_moved = false;
		ascii::populate_ascii();

		// Cursor handling
		ascii::render_cursors();

		// Draw it
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		render::update_pointlights();
		ascii::render_ascii_ambient();
		ascii::present();
		render::render_ascii_pointlights(ascii::ascii_fbo, ascii::ascii_vao, assets::ascii_texture->texture_id, ascii::buffer.size(), ascii::camera_projection_matrix, ascii::camera_modelview_matrix);

		// Present the FBO to the screen
		render_test_quad(ascii::albedo_tex);
	}
}