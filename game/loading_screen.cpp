#include "loading_screen.h"
#include "main_menu.h"
#include <memory>

using std::make_unique;

void loading_screen::init()
{
     loaded_assets = false;
     displayed = false; // Next time we are here we exit
     
     unique_ptr<gui_frame> frame = make_unique<gui_frame>(FILL_REGION);
     frame->add_child(make_unique<gui_static_text>("Black Future", 1, 1, red, black, true));
     frame->add_child(make_unique<gui_static_text>("Pre-Alpha Not Even A Version Number Yet!", 1, 2, white, black, true));
     frame->add_child(make_unique<gui_static_text>("Loading assets... please wait", 1, 4, grey, black, true));
     splash_interface.add_child(std::move(frame));
     count  = 0;
}

void loading_screen::done()
{
     // Nothing to do here.
}

pair< return_mode, unique_ptr< base_mode > > loading_screen::tick ( const double time_elapsed )
{
     // Display the loading screen
     splash_interface.render();

     // Exit conditions
     ++count;
     if (count > 10) loaded_assets = true;

     // We've already displayed the splash screen and gone to the menu; therefore,
     // popping by this way means we want to quit.
     if ( displayed ) {
          return make_pair ( POP, NO_PUSHED_MODE );
     }

     // If we haven't loaded the assets, keep going
     if ( !loaded_assets ) {
          return make_pair ( CONTINUE, NO_PUSHED_MODE );
     } else {
          // Assets are loaded, so we can continue to the main menu
          displayed = true;
          return make_pair ( PUSH, make_unique<main_menu>() );
     }
}
