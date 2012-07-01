#ifndef WARR_KNIGHT_HPP
#define WARR_KNIGHT_HPP

#include "../simulationcraft.hpp"

namespace warr_knight {

class class_t : public player_t
{
  public:
    class_t( sim_t* sim, player_type pt, const std::string& name, race_type rt ) :
      player_t( sim, pt, name, rt )
    {
      primary_attribute   = ATTR_STRENGTH;
      secondary_attribute = ATTR_WILLPOWER;
    }
};

class targetdata_t : public ::targetdata_t
{
  public:
    targetdata_t( class_t& source, player_t& target ) :
      ::targetdata_t (source, target)
    {}
};

}


#endif // WARR_KNIGHT_HPP
