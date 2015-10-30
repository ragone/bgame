#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <iostream>
#include "raws.h"

#include "../../engine/virtual_terminal.h"
#include "raw_renderable.h"
#include "raw_name.h"
#include "raw_glyph.h"
#include "raw_color_pair.h"
#include "raw_obstruction.h"
#include "raw_power_generator.h"
#include "raw_power_battery.h"
#include "raw_description.h"

#include "../../engine/globals.h"

using std::ifstream;
using std::string;
using std::vector;
using std::unordered_map;
using std::unique_ptr;
using std::make_unique;

namespace engine {
namespace raws {

namespace detail {
  

unordered_map<string, engine::vterm::color_t> colors;
unordered_map<string, unsigned char> glyphs;
unordered_map<string, unique_ptr<base_raw>> structures;
  
}

const string get_index_filename()
{
     return "../raw/index.txt";
}

vector<string> get_files_to_read()
{
     vector<string> result;

     ifstream index ( get_index_filename() );
     if ( !index.is_open() ) throw 101;

     string line;
     while ( getline ( index, line ) ) {
          result.push_back ( "../raw/" + line );
     }

     return result;
}

unique_ptr<base_raw> current;
unique_ptr<base_raw> current_render;
string current_name;
enum nested_enum {NONE,STRUCTURE};
nested_enum nested_type;
bool nested;

/* Lookup Functions */

unsigned char find_glyph_by_name(const string &name) {
    auto finder = detail::glyphs.find(name);
    if (finder == detail::glyphs.end()) {
      std::cout << "Error; could not find glyph; [" << name << "]\n";
      throw 104;
    }
    return finder->second;
}

engine::vterm::color_t find_color_by_name(const string &name) {
    auto finder = detail::colors.find(name);
    if (finder == detail::colors.end()) {
      std::cout << "Error; could not find color; [" << name << "]\n";
      throw 105;
    }
    return finder->second;
}

/* Raw Factory Functions */

void parse_raw_name(const vector<string> &chunks) {
    //std::cout << "Parsing name: " << chunks[1] << "\n";
    current_name = chunks[1];
    unique_ptr<raw_name> name = make_unique<raw_name>(chunks[1]);
    current->children.push_back(std::move(name));
}

void parse_raw_description(const vector<string> &chunks) {
    const string description = chunks[1];
    unique_ptr<raw_description> desc = make_unique<raw_description>(description);
    current->children.push_back(std::move(desc));
}

void parse_raw_obstruction(const vector<string> &chunks) {
    const string view_s = chunks[1];
    const string walk_s = chunks[2];
    
    bool view = false;
    bool walk = false;
    if (view_s == "Y") view = true;
    if (walk_s == "Y") walk = true;
    unique_ptr<raw_obstruction> obs = make_unique<raw_obstruction>(view,walk);
    current->children.push_back(std::move(obs));
}

void parse_raw_glyph(const vector<string> &chunks) {
    const string glyph_name = chunks[1];
    const unsigned char glyph = find_glyph_by_name(glyph_name);
    unique_ptr<raw_glpyh> g = make_unique<raw_glpyh>(glyph);
    current_render->children.push_back(std::move(g));
}

void parse_raw_color_pair(const vector<string> &chunks) {
    const string fg = chunks[1];
    const string bg = chunks[2];
    const color_t fgc = find_color_by_name(fg);
    const color_t bgc = find_color_by_name(bg);
    unique_ptr<raw_color_pair> cp = make_unique<raw_color_pair>(fgc,bgc);
    current_render->children.push_back(std::move(cp));
}

void parse_raw_power_generator(const vector<string> &chunks) {
    const string mode = chunks[1];
    const string amount_i = chunks[2];
    const int amount = std::stoi(amount_i);
    power_generator_condition power_mode;
    if (mode == "daylight") power_mode = DAYLIGHT;
    unique_ptr<raw_power_generator> gen = make_unique<raw_power_generator>(power_mode, amount);
    current->children.push_back(std::move(gen));
}

void parse_raw_power_battery(const vector<string> &chunks) {
    const string capacity_s = chunks[1];
    const int capacity = std::stoi(capacity_s);
    unique_ptr<raw_power_battery> batt = make_unique<raw_power_battery>(capacity);
    current->children.push_back(std::move(batt));
}

/* Specific parser functions */

void parse_structure(const vector<string> &chunks) {
    //std::cout << "Adding " << chunks[0] << " to structure.\n";
    if (chunks[0] == "/STRUCTURE") {
	nested = false;
	nested_type = NONE;
	detail::structures[current_name] = std::move(current);
	current.reset();
	return;
    }
    if (chunks[0] == "/RENDER") {
	current->children.push_back(std::move(current_render));
	current_render.reset();
	return;
    }
    if (chunks[0] == "RENDER") {
	current_render = make_unique<raw_renderable>();
	return;
    }
    if (chunks[0] == "NAME") {
	parse_raw_name(chunks);
	return;
    }
    if (chunks[0] == "DESCRIPTION") {
	parse_raw_description(chunks);
	return;
    }
    if (chunks[0] == "RENDER_GLYPH") {
	parse_raw_glyph(chunks);
	return;
    }
    if (chunks[0] == "RENDER_COLOR") {
	parse_raw_color_pair(chunks);
	return;
    }
    if (chunks[0] == "OBSTRUCTS") {
	parse_raw_obstruction(chunks);
	return;
    }
    if (chunks[0] == "GENERATOR") {
	parse_raw_power_generator(chunks);
	return;
    }
    if (chunks[0] == "BATTERY") {
	parse_raw_power_battery(chunks);
	return;
    }
    
    std::cout << "WARNING: GOT TO END OF STRUCTURE CHUNK READER WITH NO MATCH FOR [" << chunks[0] << "]\n";
}

/* Chunk Parsing */

void parse_nested_chunk ( const vector<string> &chunks )
{
    switch (nested_type) {
      case STRUCTURE : parse_structure(chunks); break;
      case NONE : throw 103;
    }
}

void parse_whole_chunk ( const vector<string> &chunks )
{
    //std::cout << "Reading whole chunk of type: " << chunks[0] << "\n";
  
     // Simple definition tags
     if ( chunks[0] == "GLYPH" ) {
          const string glyph_name = chunks[1];
          const string glyph_ascii_number = chunks[2];
          const int glyph_ascii_int = std::stoi ( glyph_ascii_number );
          const unsigned char glyph = static_cast<unsigned char>(glyph_ascii_int);
          detail::glyphs[glyph_name] = glyph;
	  //std::cout << "Learned glyph: " << glyph_name << "[" << +glyph << "]\n";
          return;
     }
     if ( chunks[0] == "COLOR" ) {
          const string color_name = chunks[1];
          const string red_string = chunks[2];
          const string green_string = chunks[3];
          const string blue_string = chunks[4];
          const int red_i = std::stoi ( red_string );
          const int green_i = std::stoi ( green_string );
          const int blue_i = std::stoi ( blue_string );
          detail::colors[color_name] = engine::vterm::color_t {red_i, green_i, blue_i};
          return;
     }
     
     // Tags that start a nested structure
     if (chunks[0] == "STRUCTURE") {
	nested = true;
	nested_type = STRUCTURE;
	current = make_unique<base_raw>();
	return;
     }
     
     std::cout << "WARNING: GOT TO END OF CHUNK READER WITH NO MATCH FOR [" << chunks[0] << "]\n";
}

void parse_chunks ( const vector<string> &chunks )
{
     //std::cout << "Nested? " << nested << "\n";
     if ( nested ) {
          parse_nested_chunk ( chunks );
     } else {
          parse_whole_chunk ( chunks );
     }
}

inline bool ignore_char(const char &c) {
  if (c == ' ') return false;
  if (c>40 and c<122 and c != '[' and c != ']') return false;
  return true;
}

void parse_line ( const string &line )
{
     if ( line.empty() ) return; // No parsing of empty lines
     if ( line[0] == '*' ) return; // No parsing of comments

     // Break the line into : separated chunks, ignoring [ and ]
     vector<string> chunks;
     string current_line;
     for ( unsigned int i=0; i<line.size(); ++i ) {
          if ( !ignore_char(line[i]) ) {
               if ( line[i] != ':' ) {
                    current_line += line[i];
               } else {
                    string chunk(current_line);
                    chunks.push_back ( chunk );
                    current_line.clear();
               }
          }
          if (line[i] == ']') chunks.push_back(current_line);
     }
     
     /*std::cout << "Parsed/Chunked Line: ";
     for (const string &s : chunks) {
	std::cout << "[" << s << "] ";
     }
     std::cout<< "\n";*/

     if (!chunks.empty()) {
	parse_chunks ( chunks );
     }
}

void parse_raw_file ( const string &filename )
{
     ifstream file ( filename );
     if ( !file.is_open() ) throw 102;
     string line;
     while ( getline ( file,line ) ) {
          parse_line ( line );
     }
}

/* Control Flow */

void read_raws()
{
     nested = false;
     nested_type = NONE;
     
     const vector<string> files_to_read = get_files_to_read();
     for ( string file_to_read : files_to_read ) {
          parse_raw_file ( file_to_read );
     }
}

/* Entity-Component Generation */

int create_structure_from_raws(const string &name, const int &x, const int &y) {
    auto finder = detail::structures.find(name);
    if (finder == detail::structures.end()) {
      std::cout << "ERROR: Cannot create structure of type [" << name << "]\n";
      throw 105;
    }
  
    engine::ecs::entity e;
    e.handle = engine::globals::ecs->get_next_entity_handle();
    engine::globals::ecs->add_entity(e);
    
    engine::globals::ecs->add_component(e, engine::ecs::position_component(x,y));
    
    finder->second->build_components(e, x, y);
    
    return e.handle;
}

}
}