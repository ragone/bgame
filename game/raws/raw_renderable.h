#pragma once

#include <iostream>
#include "base_raw.h"
#include "../components/renderable_component.h"
#include "raw_glyph.h"
#include "raw_color_pair.h"
#include "../../engine/globals.h"

using std::vector;

namespace engine {
namespace raws {

struct raw_renderable : public base_raw {
     raw_renderable() {
          type = RENDERABLE;
     };

     virtual void build_components ( entity &parent, const int &x, const int &y ) const {
          unsigned char glyph_to_use = '!';
          color_t fg {255,255,255};
          color_t bg {0,0,0};

          for ( const unique_ptr<base_raw> &child : children ) {
               if ( child->type == GLYPH ) {
                    raw_glpyh * tmp = static_cast<raw_glpyh *> ( child.get() );
                    glyph_to_use = tmp->character;
                    //std::cout << "Glyph to use: " << +glyph << "\n";
               } else if ( child->type == COLOR_PAIR ) {
                    raw_color_pair * tmp = static_cast<raw_color_pair *> ( child.get() );
                    fg = tmp->foreground;
                    bg = tmp->background;
               }
          }
          engine::globals::ecs->add_component ( parent, engine::ecs::renderable_component( glyph_to_use, fg, bg ) );
     }
};

}
}