#pragma once

#include <string>
#include <vector>
#include <utility>
#include "../../engine/sdl2_backend.hpp"
#include "colors.h"

using std::string;
using std::vector;
using std::pair;

namespace render {

class line_buffer {
public:
  line_buffer () : font("disco14") {}
  line_buffer ( const string &font_name ) : font(font_name) {}
  
  void add_line ( sdl2_backend * SDL, string text, SDL_Color color ) {
    if (text.empty()) text = "BUG - EMPTY LINE";
    pair<int,int> size = SDL->text_size( font, text );
    if (size.first > width) width = size.first;
    lines.push_back( std::make_pair( text, color ) );
  }
  
  std::size_t size() { return lines.size(); }
  int get_width() { return width; }
  vector< pair<string, SDL_Color> > lines;
  
private:
  const string font;
  int width = 0;
};
  
}