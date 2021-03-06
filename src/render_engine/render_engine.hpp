#pragma once

#include <memory>
#include "fbo/gbuffer.hpp"

namespace render {
    extern bool camera_moved;
    extern bool models_changed;
	extern bool depth_test_render;
    void render_gl(const double &duration_ms);
	void update_buffers();
}