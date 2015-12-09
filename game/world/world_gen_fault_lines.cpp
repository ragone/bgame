#include "world_gen_fault_lines.hpp"
#include <utility>
#include <vector>

using std::pair;
using std::make_pair;
using std::vector;
using std::unique_ptr;

void add_fault_lines ( heightmap_t* heightmap, engine::random_number_generator * rng )
{
    heightmap_t map = *heightmap; // Dereference for convenience
    unique_ptr<heightmap_t> vmap_p = get_height_map();
    heightmap_t vmap = *vmap_p.get();
    
    vector<pair<uint16_t,uint16_t>> points;
    for (int i=0; i<NUMBER_OF_TILES_IN_THE_WORLD/16; ++i) {
	points.push_back ( make_pair( rng->roll_dice( 1, WORLD_WIDTH*REGION_WIDTH ), rng->roll_dice( 1, WORLD_HEIGHT*REGION_HEIGHT ) ) );
    }
    
    // Iterate through and mark cell membership by nearest neighbor (voronoi)
    for (int y=0; y<WORLD_HEIGHT*REGION_HEIGHT; ++y) {
	for (int x = 0; x<WORLD_WIDTH*REGION_WIDTH; ++x) {
	    uint16_t distance = 64000;
	    int closest_point = -1;
	    
	    for (int j=0; j<points.size(); ++j) {
		const int dx = std::abs(x - points[j].first);
		const int dy = std::abs(y - points[j].second);
		const int dist = std::sqrt( (dx*dx)+(dy*dy) );
		if (dist < distance) {
		    distance = dist;
		    closest_point = j;
		}
	    }
	    vmap[ height_map_idx( x, y ) ] = closest_point;
	}
    }
    
    // Then marching squares to find the edge cells; these are raised as fault lines.
}