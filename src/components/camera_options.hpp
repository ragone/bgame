#pragma once

#include <cereal/cereal.hpp>
#include <cereal/types/polymorphic.hpp>
#include "../bengine/ecs_impl.hpp"

enum game_camera_mode_t { TOP_DOWN, FRONT, DIAGONAL };

struct camera_options_t {

    camera_options_t() {}
	camera_options_t(game_camera_mode_t MODE, bool ASCII, int ZOOM) : camera_mode(MODE), ascii_mode(ASCII), zoom_level(ZOOM) {}

    game_camera_mode_t camera_mode = TOP_DOWN;
    bool ascii_mode = false;
    int zoom_level = 12;

    template<class Archive>
    void serialize(Archive & archive)
    {
        archive( camera_mode, ascii_mode, zoom_level );
    }
};

CEREAL_REGISTER_TYPE(bengine::impl::component_store_t<bengine::impl::component_t<camera_options_t>>)
