#include "solar_system.hpp"

using std::make_pair;

void solar_system_t::load( std::fstream &lbfile )
{
    lbfile.read ( reinterpret_cast<char *> ( &universe_idx ), sizeof ( universe_idx ) );
    lbfile.read ( reinterpret_cast<char *> ( &universe_x ), sizeof ( universe_x ) );
    lbfile.read ( reinterpret_cast<char *> ( &universe_y ), sizeof ( universe_y ) );
    lbfile.read ( reinterpret_cast<char *> ( &system_class ), sizeof ( system_class ) );
    lbfile.read ( reinterpret_cast<char *> ( &binary_system ), sizeof ( binary_system ) );
    lbfile.read ( reinterpret_cast<char *> ( &inhabited_by_man ), sizeof ( inhabited_by_man ) );
    lbfile.read ( reinterpret_cast<char *> ( &owner ), sizeof ( owner ) );
    
    std::size_t n_bodies = 0;
    lbfile.read ( reinterpret_cast<char *> ( n_bodies ), sizeof ( n_bodies ) );
    
    for (std::size_t i = 0; i < n_bodies; ++i ) {
	solar_system_body_t body;
	body.load ( lbfile );
	bodies.push_back( body );
    }
}

void solar_system_t::save( std::fstream &lbfile ) const
{
    lbfile.write ( reinterpret_cast<const char *> ( &universe_idx ), sizeof( universe_idx ) );
    lbfile.write ( reinterpret_cast<const char *> ( &universe_x ), sizeof( universe_x ) );
    lbfile.write ( reinterpret_cast<const char *> ( &universe_y ), sizeof( universe_y ) );
    lbfile.write ( reinterpret_cast<const char *> ( &system_class ), sizeof( system_class ) );
    lbfile.write ( reinterpret_cast<const char *> ( &binary_system ), sizeof( binary_system ) );
    lbfile.write ( reinterpret_cast<const char *> ( &inhabited_by_man ), sizeof( inhabited_by_man ) );
    lbfile.write ( reinterpret_cast<const char *> ( &owner ), sizeof( owner ) );
    
    std::size_t n_bodies = bodies.size();
    lbfile.write ( reinterpret_cast<const char *> ( &n_bodies ), sizeof( n_bodies ) );
    
    for (const solar_system_body_t &body : bodies ) {
	body.save ( lbfile );
    }
}

std::pair<SDL_Color, unsigned char> solar_system_renderable( const solar_system_t &system)
{
    if ( !system.inhabited_by_man ) {
	switch ( system.system_class ) {
	  case O : return make_pair ( SDL_Color { 64, 64, 255, 0}, '`');
	  case B : return make_pair ( SDL_Color { 128, 128, 255, 0}, 249);
	  case A : return make_pair ( SDL_Color { 192, 192, 255, 0}, 15);
	  case F : return make_pair ( SDL_Color { 255, 255, 255, 0}, 15);
	  case G : return make_pair ( SDL_Color { 255, 255, 0, 0}, 15);
	  case K : return make_pair ( SDL_Color { 128, 128, 0, 0}, 15);
	  case M : return make_pair ( SDL_Color { 128, 128, 0, 0}, 249);
	  default: return std::make_pair( SDL_Color{255,255,255,0}, 15 );
	}
    } else if ( system.owner == NO_OWNER ) {
      switch ( system.system_class ) {
	  case O : return make_pair ( SDL_Color { 64, 64, 255, 0}, 1);
	  case B : return make_pair ( SDL_Color { 128, 128, 255, 0}, 1);
	  case A : return make_pair ( SDL_Color { 192, 192, 255, 0}, 1);
	  case F : return make_pair ( SDL_Color { 255, 255, 255, 0}, 1);
	  case G : return make_pair ( SDL_Color { 255, 255, 0, 0}, 1);
	  case K : return make_pair ( SDL_Color { 128, 128, 0, 0}, 1);
	  case M : return make_pair ( SDL_Color { 128, 128, 0, 0}, 1);
	  default: return std::make_pair( SDL_Color{255,255,255,0}, 1 );
	}
    } else {
	  SDL_Color fg;
	  switch ( system.owner ) {
	    case NO_OWNER : fg = SDL_Color{255,255,255,0}; break;
	    case ARSAK : fg = SDL_Color{255,0,0,0}; break;
	    case CARVAZ : fg = SDL_Color{255,0,255,0}; break;
	    case LAMINTREZ : fg = SDL_Color{255,255,0,0}; break;
	    case HERITEZ : fg = SDL_Color{0,255,255,0}; break;
	    case ERENTAR : fg = SDL_Color{0,0,255,0}; break;
	    case SYL : fg = SDL_Color{0,255,255,0}; break;
	  }
	  return std::make_pair( fg, 1 );
    }
}