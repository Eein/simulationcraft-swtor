// ==========================================================================
// SimulationCraft for Star Wars: The Old Republic
// http://code.google.com/p/simulationcraft-swtor/
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Log
// ==========================================================================

// log_t::output ============================================================

void log_t::output( sim_t* sim, const char* format, ... )
{
  va_list vap;
  char buffer[2048];

  va_start( vap, format );
  vsnprintf( buffer, sizeof( buffer ),  format, vap );
  va_end( vap );

  util_t::fprintf( sim -> output_file, "%-8.2f %s\n",
                   to_seconds( sim -> current_time ), buffer );
  //fflush( sim -> output_file );
}
