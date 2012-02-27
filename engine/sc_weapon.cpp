// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Weapon
// ==========================================================================

// weapon_t::group ==========================================================

int weapon_t::group() const
{
  switch ( type )
  {

  case WEAPON_LIGHTSABER:
  case WEAPON_VIBROKNIFE:
  case WEAPON_TECHBLADE:
  case WEAPON_VIBROSWORD:
    return WEAPON_1H;

  case WEAPON_DOUBLE_BLADED_LIGHTSABER:
  case WEAPON_ELECTROSTAVE:
  case WEAPON_TECHSTAVE:
    return WEAPON_2H;

  case WEAPON_ASSAULT_CANON:
  case WEAPON_BLASTER_PISTOL:
  case WEAPON_BLASTER_RIFLE:
  case WEAPON_SCATTERGUN:
  case WEAPON_SNIPER_RIFLE:
    return WEAPON_RANGED;
  }
  return WEAPON_NONE;
}
