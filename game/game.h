#pragma once

#include "../engine/engine.h"
#include "components/components.h"
#include "messages/messages.h"

using engine::message_list_t;
using engine::component_list_t;

using my_messages = message_list_t<command_message, power_consumed_message, chat_emote_message, mouse_motion_message>;

using my_components = component_list_t<calendar_component, debug_name_component, obstruction_component, 
  position_component, power_battery_component, power_generator_component, renderable_component, settler_ai_component, 
  viewshed_component, game_stats_component, game_species_component, game_health_component, provisions_component, 
  item_storage_component, item_carried_component>;
  
using my_engine = engine::bracket_engine<my_components, my_messages>;
  
extern my_engine * game_engine;
  
void run_game();