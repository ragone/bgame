#include "renderable_system.hpp"

#include <iostream>
#include <tuple>
#include "../game.h"
#include "../../engine/virtual_terminal.hpp"
#include "../world/planet.hpp"
#include "../world/universe.hpp"
#include "../render/render_list.hpp"

void renderable_system::tick(const double &duration_ms)
{
	bool changes = false;
	vector<entity_moved_message> * change_list = game_engine->messaging->get_messages_by_type<entity_moved_message>();
	for (entity_moved_message &msg : *change_list)
	{
		if (!msg.deleted)
			changes = true;
		msg.deleted = true;
	}

	if (changes)
	{

		std::fill(render_list_3d.begin(), render_list_3d.end(), boost::optional<engine::vterm::screen_character>());

		const position_component3d * camera_pos = get_camera_position();
		const int top_y = camera_pos->pos.y - ((768 / 8) / 2);
		const int bottom_y = camera_pos->pos.y + ((768 / 8) / 2);
		const int left_x = camera_pos->pos.x - ((1024 / 8) / 2);
		const int right_x = camera_pos->pos.x + ((1024 / 8) / 2);

		const vector<position_component3d> * positions = ECS->find_components_by_type<position_component3d>();
		for (const position_component3d &pos : *positions)
		{
			if (pos.pos.region == camera_pos->pos.region and pos.pos.x > left_x
					and pos.pos.x <= right_x and pos.pos.y > top_y
					and pos.pos.y <= bottom_y)
			{
				renderable_component * render_info = ECS->find_entity_component<
						renderable_component>(pos.entity_id);
				if (render_info != nullptr)
				{
					const int idx = get_tile_index(pos.pos.x, pos.pos.y,
							pos.pos.z);
					render_list_3d[idx] = engine::vterm::screen_character
					{ render_info->glyph, render_info->foreground,
							render_info->background };
					//std::cout << "Renderable at " << pos.pos.x << "/" << pos.pos.y << "/" << +pos.pos.z << "\n";
				}
			}
		}
	}
}