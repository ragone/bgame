#pragma once

#include "planet/planet.hpp"
#include "planet/region.hpp"
#include "components/components.hpp"
#include <rltk.hpp>
#include <boost/optional.hpp>

struct available_building_t {
	available_building_t(const std::string &n, const std::string &t) : name(n), tag(t) {}

	std::string name;
	std::string tag;
	std::vector<std::string> components;
    int width;
    int height;
    std::vector<rltk::vchar> glyphs;
};

enum pause_mode_t { RUNNING, PAUSED, ONE_STEP };
enum game_master_mode_t { PLAY, DESIGN };
enum game_design_mode_t { DIGGING, BUILDING, CHOPPING };
enum game_mining_mode_t { DIG, CHANNEL, RAMP, UP, DOWN, UPDOWN, DELETE };

extern planet_t planet;
extern region_t current_region;
extern std::size_t camera_entity;
extern world_position_t * camera_position;
extern calendar_t * calendar;
extern designations_t * designations;
extern int clip_left;
extern int clip_right;
extern int clip_top;
extern int clip_bottom;
extern rltk::random_number_generator rng;
extern pause_mode_t pause_mode;
extern game_master_mode_t game_master_mode;
extern game_design_mode_t game_design_mode;
extern game_mining_mode_t game_mining_mode;
extern boost::optional<available_building_t> build_mode_building;
