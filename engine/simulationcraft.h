// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef __SIMULATIONCRAFT_H
#define __SIMULATIONCRAFT_H

// Platform Initialization ==================================================

#if defined( _MSC_VER ) || defined( __MINGW__ ) || defined( _WINDOWS ) || defined( WIN32 )
#  define WIN32_LEAN_AND_MEAN
#  define VC_EXTRALEAN
#  define _CRT_SECURE_NO_WARNINGS
#  define DIRECTORY_DELIMITER "\\"
#  ifndef UNICODE
#    define UNICODE
#  endif
#else
#  define DIRECTORY_DELIMITER "/"
#  define SC_SIGACTION
#endif

#if defined(_LP64) || defined(__LP64__) || defined(_WIN64) || defined(_AMD64_)
#  define SC_X64
#endif

#if defined( _MSC_VER )
#  include "../vs/stdint.h"
#else
#  include <stdint.h>
#endif

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <sstream>
#include <stack>
#include <string>
#include <typeinfo>
#include <vector>

#if _MSC_VER || __cplusplus >= 201103L
#include <unordered_map>
#if _MSC_VER < 1600
namespace std {using namespace tr1; }
#endif
#else
#include <tr1/unordered_map>
namespace std {using namespace tr1; }
#endif

// GCC (and probably the C++ standard in general) doesn't like offsetof on non-POD types
#ifdef _MSC_VER
#define nonpod_offsetof(t, m) offsetof(t, m)
#else
#define nonpod_offsetof(t, m) ((size_t) ( (volatile char *)&((volatile t *)(size_t)0x10000)->m - (volatile char *)(size_t)0x10000 ))
#endif

#include "data_enums.hh"

#if defined( _MSC_VER )
# define finline                     __forceinline
# define SC_FINLINE_EXT
#elif defined( __GNUC__ )
# define finline                     inline
# define SC_FINLINE_EXT              __attribute__((always_inline))
#endif

#include "xs_float/xs_Float.h"

#if __BSD_VISIBLE
#  include <netinet/in.h>
#  if !defined(CLOCKS_PER_SEC)
#    define CLOCKS_PER_SEC 1000000
#  endif
#endif

#if defined( NO_THREADS )
#elif defined( __MINGW32__ ) || defined( _MSC_VER )
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else // POSIX
#include <pthread.h>
#include <unistd.h>
#endif

#if defined(__GNUC__)
#  define likely(x)       __builtin_expect((x),1)
#  define unlikely(x)     __builtin_expect((x),0)
#else
#  define likely(x) (x)
#  define unlikely(x) (x)
#  define __attribute__(x)
#endif
#define PRINTF_ATTRIBUTE(a,b) __attribute__((format(printf,a,b)))

#include "data_definitions.hh"

#define SC_MAJOR_VERSION "113"
#define SC_MINOR_VERSION "2"
#define SC_USE_PTR ( 0 )
#define SC_BETA ( 0 )
#define SC_EPSILON ( 0.000001 )
#ifndef M_PI
#define M_PI ( 3.14159265358979323846 )
#endif

#define MAX_PLAYERS_PER_CHART 20

// TODO: Integer time is only partially working.
//#define SC_USE_INTEGER_TIME

// Forward Declarations =====================================================

struct action_t;
struct action_callback_t;
struct action_expr_t;
struct action_priority_list_t;
struct alias_t;
struct benefit_t;
struct buff_t;
struct buff_uptime_t;
struct callback_t;
struct companion_t;
struct cooldown_t;
class  dbc_t;
struct dot_t;
struct effect_t;
struct enemy_t;
struct enchant_t;
struct event_t;
struct gain_t;
struct item_t;
struct sage_sorcerer_t;
struct js_node_t;
struct option_t;
struct pet_t;
struct player_t;
struct plot_t;
struct proc_t;
struct raid_event_t;
struct rating_t;
struct reforge_plot_t;
struct reforge_plot_data_t;
struct report_t;
struct rng_t;
struct talent_t;
struct spell_id_t;
struct scaling_t;
struct sim_t;
struct spell_data_t;
struct spelleffect_data_t;
struct sample_data_t;
struct shadow_assassin_t;
struct heal_t;
struct stats_t;
struct talent_t;
struct talent_translation_t;
struct timespan_t;
struct unique_gear_t;
struct uptime_t;
struct weapon_t;
struct xml_node_t;

struct targetdata_t;
struct jedi_sage_targetdata_t;
struct sith_sorcerer_targetdata_t;
struct shadow_assassin_targetdata_t;

void register_jedi_sage_targetdata( sim_t* sim );
void register_sith_sorcerer_targetdata( sim_t* sim );
void register_shadow_assassin_targetdata( sim_t* sim );

#define DATA_DOT 0
#define DATA_AURA 1
#define DATA_COUNT 2

struct actor_pair_t
{
  player_t* target;
  player_t* source;

  actor_pair_t( player_t* target, player_t* source )
    : target( target ), source( source )
  {}

  actor_pair_t( player_t* p = 0 )
    : target( p ), source( p )
  {}

  actor_pair_t( targetdata_t* td );
};

// Enumerations =============================================================

enum race_type
{
  RACE_NONE=0,
  // Target Races
  RACE_BEAST, RACE_DRAGONKIN, RACE_GIANT, RACE_HUMANOID, RACE_DEMON, RACE_ELEMENTAL,
  // Player Races
  RACE_CHISS, RACE_CYBORG, RACE_HUMAN, RACE_MIRALUKA, RACE_MIRIALAN, RACE_RATTATAKI, RACE_SITH_PUREBLOOD, RACE_TWILEK, RACE_ZABRAK,
  RACE_MAX
};

enum player_type
{
  PLAYER_SPECIAL_SCALE=-1,
  PLAYER_NONE=0,

  SITH_MARAUDER, SITH_JUGGERNAUT,
  SITH_ASSASSIN, SITH_SORCERER,
  BH_MERCENARY, BH_POWERTECH,
  IA_SNIPER, IA_OPERATIVE,

  JEDI_GUARDIAN, JEDI_SENTINEL,
  JEDI_SAGE, JEDI_SHADOW,
  S_GUNSLINGER, S_SCOUNDREL,
  T_COMMANDO, T_VANGUARD,

  PLAYER_PET, PLAYER_COMPANION, PLAYER_GUARDIAN,
  ENEMY, ENEMY_ADD,
  PLAYER_MAX
};

enum pet_type_t
{
  PET_NONE=0,

  PET_ENEMY,

  PET_MAX
};

enum dmg_type { DMG_DIRECT=0, DMG_OVER_TIME=1, HEAL_DIRECT, HEAL_OVER_TIME, ABSORB };

enum stats_type { STATS_DMG, STATS_HEAL, STATS_ABSORB };

enum dot_behavior_type { DOT_CLIP=0, DOT_REFRESH };

enum attribute_type { ATTRIBUTE_NONE=0, ATTR_STRENGTH, ATTR_AIM, ATTR_CUNNING, ATTR_WILLPOWER, ATTR_ENDURANCE, ATTR_PRESENCE, ATTRIBUTE_MAX };

enum base_stat_type { BASE_STAT_STRENGTH=0, BASE_STAT_AIM, BASE_STAT_CUNNING, BASE_STAT_WILLPOWER, BASE_STAT_ENDURANCE, BASE_STAT_PRESENCE,
                      BASE_STAT_HEALTH, BASE_STAT_MANA,
                      BASE_STAT_MELEE_CRIT_PER_AGI, BASE_STAT_SPELL_CRIT_PER_INT,
                      BASE_STAT_DODGE_PER_AGI,
                      BASE_STAT_MELEE_CRIT, BASE_STAT_SPELL_CRIT, BASE_STAT_SPI_REGEN, BASE_STAT_MAX
                    };

enum resource_type
{
  RESOURCE_NONE=0,
  RESOURCE_HEALTH, RESOURCE_MANA,  RESOURCE_RAGE, RESOURCE_ENERGY, RESOURCE_AMMO,
  RESOURCE_FORCE,
  RESOURCE_MAX
};

enum result_type
{
  RESULT_UNKNOWN=-1,
  RESULT_NONE=0,
  RESULT_MISS,  RESULT_DODGE, RESULT_PARRY,
  RESULT_BLOCK, RESULT_CRIT_BLOCK, RESULT_CRIT, RESULT_HIT,
  RESULT_MAX
};

#define RESULT_HIT_MASK  ( (1<<RESULT_BLOCK) | (1<<RESULT_CRIT_BLOCK) | (1<<RESULT_CRIT) | (1<<RESULT_HIT) )
#define RESULT_CRIT_MASK ( (1<<RESULT_CRIT) )
#define RESULT_MISS_MASK ( (1<<RESULT_MISS) )
#define RESULT_NONE_MASK ( (1<<RESULT_NONE) )
#define RESULT_ALL_MASK  -1

enum proc_type
{
  PROC_NONE=0,
  PROC_DAMAGE,
  PROC_HEAL,
  PROC_TICK_DAMAGE,
  PROC_DIRECT_DAMAGE,
  PROC_DIRECT_HEAL,
  PROC_TICK_HEAL,
  PROC_ATTACK,
  PROC_SPELL,
  PROC_TICK,
  PROC_SPELL_AND_TICK,
  PROC_HEAL_SPELL,
  PROC_HARMFUL_SPELL,
  PROC_DAMAGE_HEAL,
  PROC_MAX
};

enum action_type { ACTION_USE=0, ACTION_SPELL, ACTION_ATTACK, ACTION_HEAL, ACTION_ABSORB, ACTION_SEQUENCE, ACTION_OTHER, ACTION_MAX };

enum school_type
{
  SCHOOL_NONE=0,

  SCHOOL_KINETIC,  SCHOOL_ENERGY,
  SCHOOL_INTERNAL, SCHOOL_ELEMENTAL,

  SCHOOL_MAX
};

#define SCHOOL_ALL_MASK ( ( 1 << SCHOOL_KINETIC ) | ( 1 << SCHOOL_ENERGY ) | \
                          ( 1 << SCHOOL_INTERNAL ) | ( 1 << SCHOOL_ELEMENTAL ) )

enum talent_tree_type
{
  TREE_NONE=0,
  TREE_SEER,         TREE_TELEKINETICS,  TREE_BALANCE,
  TREE_CORRUPTION,   TREE_LIGHTNING,     TREE_MADNESS,
  TREE_KINETIC_COMBAT, TREE_INFILTRATION,
  TREE_DARKNESS, TREE_DECEPTION,
  TALENT_TREE_MAX
};

enum talent_tab_type
{
  TALENT_TAB_NONE = -1,

  JEDI_SAGE_SEER = 0,           JEDI_SAGE_TELEKINETICS,   JEDI_SAGE_BALANCE,
  SITH_SORCERER_CORRUPTION = 0, SITH_SORCERER_LIGHTNING,  SITH_SORCERER_MADNESS,

  JEDI_SHADOW_KINETIC_COMBAT = 0,   JEDI_SHADOW_INFILTRATION, JEDI_SHADOW_BALANCE,
  SITH_ASSASSIN_DARKNESS = 0,       SITH_ASSASSIN_DECEPTION,  SITH_ASSASSIN_MADNESS,
};

enum weapon_type
{
  WEAPON_NONE=0,
  WEAPON_DAGGER,                                                                                   WEAPON_SMALL,
  WEAPON_BEAST,    WEAPON_SWORD,    WEAPON_MACE,     WEAPON_AXE,    WEAPON_FIST,                   WEAPON_1H,
  WEAPON_BEAST_2H, WEAPON_SWORD_2H, WEAPON_MACE_2H,  WEAPON_AXE_2H, WEAPON_STAFF,  WEAPON_POLEARM, WEAPON_2H,
  WEAPON_BOW,      WEAPON_CROSSBOW, WEAPON_GUN,      WEAPON_WAND,   WEAPON_THROWN,                 WEAPON_RANGED,
  WEAPON_MAX
};

enum glyph_type
{
  GLYPH_MAJOR=0,
  GLYPH_MINOR,
  GLYPH_PRIME,
  GLYPH_MAX
};

enum slot_type   // these enum values should match armory settings
{
  SLOT_NONE      = -1,
  SLOT_HEAD      = 0,
  SLOT_EAR       = 1,
  SLOT_SHOULDERS = 2,
  SLOT_SHIRT     = 3,
  SLOT_CHEST     = 4,
  SLOT_WAIST     = 5,
  SLOT_LEGS      = 6,
  SLOT_FEET      = 7,
  SLOT_WRISTS    = 8,
  SLOT_HANDS     = 9,
  SLOT_IMPLANT_1 = 10,
  SLOT_IMPLANT_2 = 11,
  SLOT_RELIC_1   = 12,
  SLOT_RELIC_2   = 13,
  SLOT_BACK      = 14,
  SLOT_MAIN_HAND = 15,
  SLOT_OFF_HAND  = 16,
  SLOT_RANGED    = 17,
  SLOT_TABARD    = 18,
  SLOT_MAX       = 19
};

// Tiers 11..14 + PVP
#define N_TIER 5

// Caster 2/4, Melee 2/4, Tank 2/4, Heal 2/4
#define N_TIER_BONUS 8

typedef uint32_t set_bonus_description_t[N_TIER][N_TIER_BONUS];

enum set_type
{
  SET_NONE = 0,
  SET_RAKATA_COMBAT_TECH_GEAR, SET_RAKATA_COMBAT_TECH_GEAR_2PC, SET_RAKATA_COMBAT_TECH_GEAR_4PC,
  SET_INDOMITABLE, SET_INDOMITABLE_2PC, SET_INDOMITABLE_4PC,
  SET_MAX
};
// static_assert( SET_MAX == N_TIER * 3 * N_TIER_BONUS / 2 );

enum gem_type
{
  GEM_NONE=0,
  GEM_META, GEM_PRISMATIC,
  GEM_RED, GEM_YELLOW, GEM_BLUE,
  GEM_ORANGE, GEM_GREEN, GEM_PURPLE,
  GEM_COGWHEEL,
  GEM_MAX
};

enum meta_gem_type
{
  META_GEM_NONE=0,
  META_GEM_MAX
};

// Keep this in sync with enum attribute_type
enum stat_type
{
  STAT_NONE=0,
  STAT_STRENGTH, STAT_AIM, STAT_CUNNING, STAT_WILLPOWER, STAT_ENDURANCE, STAT_PRESENCE,
  STAT_HEALTH, STAT_MANA, STAT_RAGE, STAT_ENERGY, STAT_AMMO,
  STAT_MAX_HEALTH, STAT_MAX_MANA, STAT_MAX_RAGE, STAT_MAX_ENERGY, STAT_MAX_AMMO,
  STAT_EXPERTISE_RATING, STAT_EXPERTISE_RATING2,
  STAT_HIT_RATING, STAT_HIT_RATING2, STAT_CRIT_RATING, STAT_ALACRITY_RATING,
  STAT_WEAPON_DPS, STAT_WEAPON_SPEED,
  STAT_WEAPON_OFFHAND_DPS, STAT_WEAPON_OFFHAND_SPEED,
  STAT_ARMOR, STAT_BONUS_ARMOR, STAT_RESILIENCE_RATING, STAT_DODGE_RATING, STAT_PARRY_RATING,
  STAT_BLOCK_RATING,
  STAT_POWER, STAT_FORCE_POWER, STAT_TECH_POWER,
  STAT_SURGE_RATING,
  STAT_MAX
};

enum stim_type
{
  STIM_NONE=0,
  STIM_EXOTECH_RESOLVE,
  STIM_RAKATA_RESOLVE,
  STIM_MAX
};

enum position_type { POSITION_NONE=0, POSITION_FRONT, POSITION_BACK, POSITION_RANGED_FRONT, POSITION_RANGED_BACK, POSITION_MAX };

enum profession_type
{
  PROFESSION_NONE=0,
  PROFESSION_BIOCHEM,
  PROFESSION_MAX
};

enum role_type { ROLE_NONE=0, ROLE_ATTACK, ROLE_SPELL, ROLE_HYBRID, ROLE_DPS, ROLE_TANK, ROLE_HEAL, ROLE_MAX };

enum rng_type
{
  // Specifies the general class of RNG desired
  RNG_DEFAULT=0,     // Do not care/know where it will be used
  RNG_GLOBAL,        // Returns reference to global RNG on sim_t
  RNG_DETERMINISTIC, // Returns reference to global deterministic RNG on sim_t
  RNG_CYCLIC,        // Normalized even/periodical results are acceptable
  RNG_DISTRIBUTED,   // Normalized variable/distributed values should be returned

  // Specifies a particular RNG desired
  RNG_STANDARD,          // Creates RNG using srand() and rand()
  RNG_MERSENNE_TWISTER,  // Creates RNG using SIMD oriented Fast Mersenne Twister
  RNG_PHASE_SHIFT,       // Simplistic cycle-based RNG, unsuitable for overlapping procs
  RNG_DISTANCE_SIMPLE,   // Simple normalized proc-separation RNG, suitable for fixed proc chance
  RNG_DISTANCE_BANDS,    // Complex normalized proc-separation RNG, suitable for varying proc chance
  RNG_PRE_FILL,          // Deterministic number of procs with random distribution
  RNG_MAX
};

enum save_type
{
  // Specifies the type of profile data to be saved
  SAVE_ALL=0,
  SAVE_GEAR,
  SAVE_TALENTS,
  SAVE_ACTIONS,
  SAVE_MAX
};

enum format_type
{
  FORMAT_NONE=0,
  FORMAT_NAME,
  FORMAT_CHAR_NAME,
  FORMAT_CONVERT_HEX,
  FORMAT_CONVERT_UTF8,
  FORMAT_MAX
};

#define FORMAT_CHAR_NAME_MASK  ( (1<<FORMAT_NAME) | (1<<FORMAT_CHAR_NAME) )
#define FORMAT_GUILD_NAME_MASK ( (1<<FORMAT_NAME) )
#define FORMAT_ALL_NAME_MASK   ( (1<<FORMAT_NAME) | (1<<FORMAT_CHAR_NAME) )
#define FORMAT_UTF8_MASK       ( (1<<FORMAT_CONVERT_HEX) | (1<<FORMAT_CONVERT_UTF8) )
#define FORMAT_ASCII_MASK      ( (1<<FORMAT_CONVERT_UTF8) )
#define FORMAT_CONVERT_MASK    ( (1<<FORMAT_CONVERT_HEX) | (1<<FORMAT_CONVERT_UTF8) )
#define FORMAT_DEFAULT         ( FORMAT_ASCII_MASK )
#define FORMAT_ALL_MASK        -1

// Data Access ==============================================================
#ifndef MAX_LEVEL
#define MAX_LEVEL (50)
#endif

#ifndef MAX_RANK
#define MAX_RANK (3)
#endif

#ifndef NUM_SPELL_FLAGS
#define NUM_SPELL_FLAGS (10)
#endif

#ifndef MAX_EFFECTS
#define MAX_EFFECTS (3)
#endif

#ifndef MAX_TALENT_TABS
#define MAX_TALENT_TABS (3)
#endif

#ifndef MAX_TALENTS
#define MAX_TALENTS (100)
#endif

enum power_type
{
  POWER_MANA = 0,
  POWER_RAGE = 1,
  POWER_FOCUS = 2,
  POWER_ENERGY = 3,
  POWER_HAPPINESS = 4,
  // not yet used
  POWER_RUNE = 5,
  POWER_RUNIC_POWER = 6,
  POWER_SOUL_SHARDS = 7,
  POWER_ECLIPSE = 8,
  POWER_HOLY_POWER = 9,
  POWER_HEALTH = 0xFFFFFFFE, // (or -2 if signed)
  POWER_NONE = 0xFFFFFFFF, // None.
  //these are not yet used
  //POWER_RUNE_BLOOD = 10,
  //POWER_RUNE_FROST = 11,
  //POWER_RUNE_UNHOLY = 12
};

enum rating_type
{
  RATING_DODGE = 0,
  RATING_PARRY,
  RATING_BLOCK,
  RATING_MELEE_HIT,
  RATING_RANGED_HIT,
  RATING_SPELL_HIT,
  RATING_MELEE_CRIT,
  RATING_RANGED_CRIT,
  RATING_SPELL_CRIT,
  RATING_MELEE_ALACRITY,
  RATING_RANGED_ALACRITY,
  RATING_SPELL_ALACRITY,
  RATING_EXPERTISE,
  RATING_MASTERY,
  RATING_MAX
};

// Type utilities and generic programming tools =============================
template <typename T, std::size_t N>
inline std::size_t sizeof_array( const T ( & )[N] )
{ return N; }

class noncopyable
{
public:
  noncopyable() {} // = default
  // noncopyable( noncopyable&& ) = default;
  // noncopyable& operator = ( noncopyable&& ) = default;
private:
  noncopyable( const noncopyable& ); // = delete
  noncopyable& operator = ( const noncopyable& ); // = delete
};

class nonmoveable : public noncopyable
{
private:
  // nonmoveable( nonmoveable&& ) = delete;
  // nonmoveable& operator = ( nonmoveable&& ) = delete;
};

struct delete_disposer_t
{
  template <typename T>
  void operator () ( T* t ) const { delete t; }
};

template <typename T>
struct iterator_type
{ typedef typename T::iterator type; };

template <typename T>
struct iterator_type<const T>
{ typedef typename T::const_iterator type; };

// Generic algorithms =======================================================

// Wrappers for std::fill, std::fill_n, and std::find that perform any type
// conversions for t at the callsite instead of per assignment in the loop body.
template <typename I>
inline void fill( I first, I last, typename std::iterator_traits<I>::value_type const& t )
{ std::fill( first, last, t ); }

template <typename I>
inline void fill_n( I first, typename std::iterator_traits<I>::difference_type n,
                    typename std::iterator_traits<I>::value_type const& t )
{ std::fill_n( first, n, t ); }

template <typename I>
inline I find( I first, I last, typename std::iterator_traits<I>::value_type const& t )
{ return std::find( first, last, t ); }

template <typename I, typename D>
void dispose( I first, I last, D disposer )
{
  while ( first != last )
    disposer( *first++ );
}

template <typename I>
inline void dispose( I first, I last )
{ dispose( first, last, delete_disposer_t() ); }

template <unsigned HW, typename Fwd, typename Out>
void sliding_window_average( Fwd first, Fwd last, Out out )
{
  typedef typename std::iterator_traits<Fwd>::value_type value_t;
  typedef typename std::iterator_traits<Fwd>::difference_type diff_t;
  const diff_t n = std::distance( first, last );
  const diff_t HALFWINDOW = static_cast<diff_t>( HW );

  if ( n >= 2 * HALFWINDOW )
  {
    value_t window_sum = value_t();

    // Fill right half of sliding window
    Fwd right = first;
    for ( diff_t count = 0; count < HALFWINDOW; ++count )
      window_sum += *right++;

    // Fill left half of sliding window
    for ( diff_t count = HALFWINDOW; count < 2 * HALFWINDOW; ++count )
    {
      window_sum += *right++;
      *out++ = window_sum / ( count + 1 );
    }

    // Slide until window hits end of data
    while ( right != last )
    {
      window_sum += *right++;
      *out++ = window_sum / ( 2 * HALFWINDOW + 1 );
      window_sum -= *first++;
    }

    // Empty right half of sliding window
    for ( diff_t count = 2 * HALFWINDOW; count > HALFWINDOW; --count )
    {
      *out++ = window_sum / count;
      window_sum -= *first++;
    }
  }
  else
  {
    // input is pathologically small compared to window size, just average everything.
    fill_n( out, n, std::accumulate( first, last, value_t() ) / n );
  }
}


// Machinery for range-based generic algorithms =============================

namespace range { // ========================================================
template <typename T>
struct traits
{
  typedef typename iterator_type<T>::type iterator;
  static iterator begin( T& t ) { return t.begin(); }
  static iterator end( T& t ) { return t.end(); }
};

template <typename T, size_t N>
struct traits<T[N]>
{
  typedef T* iterator;
  static iterator begin( T ( &t )[N] ) { return &t[0]; }
  static iterator end( T ( &t )[N] ) { return begin( t ) + N; }
};

template <typename T>
struct traits< std::pair<T,T> >
{
  typedef T iterator;
  static iterator begin( const std::pair<T,T>& t ) { return t.first; }
  static iterator end( const std::pair<T,T>& t ) { return t.second; }
};

template <typename T>
struct traits< const std::pair<T,T> >
{
  typedef T iterator;
  static iterator begin( const std::pair<T,T>& t ) { return t.first; }
  static iterator end( const std::pair<T,T>& t ) { return t.second; }
};

template <typename T>
struct value_type
{
  typedef typename std::iterator_traits<typename traits<T>::iterator>::value_type type;
};

template <typename T>
inline typename traits<T>::iterator begin( T& t )
{ return traits<T>::begin( t ); }

template <typename T>
inline typename traits<const T>::iterator cbegin( const T& t )
{ return range::begin( t ); }

template <typename T>
inline typename traits<T>::iterator end( T& t )
{ return traits<T>::end( t ); }

template <typename T>
inline typename traits<const T>::iterator cend( const T& t )
{ return range::end( t ); }

// Range-based generic algorithms ===========================================

template <typename Range, typename Out>
inline Out copy( const Range& r, Out o )
{ return std::copy( range::begin( r ), range::end( r ), o ); }

template <typename Range, typename D>
inline Range& dispose( Range& r, D disposer )
{ dispose( range::begin( r ), range::end( r ), disposer ); return r; }

template <typename Range>
inline Range& dispose( Range& r )
{ return dispose( r, delete_disposer_t() ); }

template <typename Range>
inline Range& fill( Range& r, typename range::value_type<Range>::type const& t )
{ std::fill( range::begin( r ), range::end( r ), t ); return r; }

template <typename Range>
inline typename range::traits<Range>::iterator
find( Range& r, typename range::value_type<Range>::type const& t )
{ return std::find( range::begin( r ), range::end( r ), t ); }

template <typename Range, typename F>
inline F for_each( Range& r, F f )
{ return std::for_each( range::begin( r ), range::end( r ), f ); }

template <typename Range1, typename Range2, typename Out>
inline Out set_difference( const Range1& left, const Range2& right, Out o )
{
  return std::set_difference( range::begin( left ), range::end( left ),
                              range::begin( right ), range::end( right ), o );
}

template <typename Range1, typename Range2, typename Out, typename Compare>
inline Out set_difference( const Range1& left, const Range2& right, Out o, Compare c )
{
  return std::set_difference( range::begin( left ), range::end( left ),
                              range::begin( right ), range::end( right ), o, c );
}

template <typename Range1, typename Range2, typename Out>
inline Out set_intersection( const Range1& left, const Range2& right, Out o )
{
  return std::set_intersection( range::begin( left ), range::end( left ),
                                range::begin( right ), range::end( right ), o );
}

template <typename Range1, typename Range2, typename Out, typename Compare>
inline Out set_intersection( const Range1& left, const Range2& right, Out o, Compare c )
{
  return std::set_intersection( range::begin( left ), range::end( left ),
                                range::begin( right ), range::end( right ), o, c );
}

template <typename Range1, typename Range2, typename Out>
inline Out set_union( const Range1& left, const Range2& right, Out o )
{
  return std::set_union( range::begin( left ), range::end( left ),
                         range::begin( right ), range::end( right ), o );
}

template <typename Range1, typename Range2, typename Out, typename Compare>
inline Out set_union( const Range1& left, const Range2& right, Out o, Compare c )
{
  return std::set_union( range::begin( left ), range::end( left ),
                         range::begin( right ), range::end( right ), o, c );
}

template <unsigned HW, typename Range, typename Out>
inline Range& sliding_window_average( Range& r, Out out )
{ ::sliding_window_average<HW>( range::begin( r ), range::end( r ), out ); return r; }

template <typename Range>
inline Range& sort( Range& r )
{ std::sort( range::begin( r ), range::end( r ) ); return r; }

template <typename Range, typename Comp>
inline Range& sort( Range& r, Comp c )
{ std::sort( range::begin( r ), range::end( r ), c ); return r; }

template <typename Range>
inline typename range::traits<Range>::iterator unique( Range& r )
{ return std::unique( range::begin( r ), range::end( r ) ); }

template <typename Range, typename Comp>
inline typename range::traits<Range>::iterator unique( Range& r, Comp c )
{ return std::unique( range::begin( r ), range::end( r ), c ); }

} // namespace range ========================================================

struct timespan_t
{

#ifdef SC_USE_INTEGER_TIME

#ifdef SC_X64
  typedef int64_t time_t;
#else
  // CAREFUL: Using int32_t implies that no overflowing happens during calculation.
  typedef int32_t time_t;
#endif

private:
  time_t time;

  static const time_t MILLIS_PER_SECOND;
  static const double SECONDS_PER_MILLI;
  static const time_t MILLIS_PER_MINUTE;
  static const double MINUTES_PER_MILLI;


  explicit timespan_t( const time_t millis ) : time( millis ) { }

public:

  double total_minutes() const { return time * MINUTES_PER_MILLI; }
  double total_seconds() const { return time * SECONDS_PER_MILLI; }
  time_t total_millis() const { return time; }

  static timespan_t from_millis( const uint64_t millis ) { return timespan_t( ( time_t )millis ); }
  static timespan_t from_millis( const int64_t millis ) { return timespan_t( ( time_t )millis ); }
  static timespan_t from_millis( const uint32_t millis ) { return timespan_t( ( time_t )millis ); }
  static timespan_t from_millis( const int32_t millis ) { return timespan_t( ( time_t )millis ); }
  static timespan_t from_millis( const double millis ) { return timespan_t( ( time_t )millis ); }
  static timespan_t from_seconds( const double seconds ) { return timespan_t( ( time_t )( seconds * MILLIS_PER_SECOND ) );  }
  static timespan_t from_minutes( const double minutes ) { return timespan_t( ( time_t )( minutes * MILLIS_PER_MINUTE ) ); }

#else // !SC_USE_INTEGER_TIME

  typedef double time_t;

private:
  time_t time;

  static const double MILLIS_PER_SECOND;
  static const double MINUTES_PER_SECOND;
  static const time_t SECONDS_PER_MILLI;
  static const time_t SECONDS_PER_MINUTE;


  explicit timespan_t( const time_t millis ) : time( millis ) { }

public:
  double total_minutes() const { return time * MINUTES_PER_SECOND; }
  double total_seconds() const { return time; }
  time_t total_millis() const { return time * MILLIS_PER_SECOND; }

  static timespan_t from_millis( const uint64_t millis ) { return timespan_t( ( time_t )millis * SECONDS_PER_MILLI ); }
  static timespan_t from_millis( const int64_t millis ) { return timespan_t( ( time_t )millis * SECONDS_PER_MILLI ); }
  static timespan_t from_millis( const uint32_t millis ) { return timespan_t( ( time_t )millis * SECONDS_PER_MILLI ); }
  static timespan_t from_millis( const int32_t millis ) { return timespan_t( ( time_t )millis * SECONDS_PER_MILLI ); }
  static timespan_t from_millis( const double millis ) { return timespan_t( ( time_t )millis * SECONDS_PER_MILLI ); }
  static timespan_t from_seconds( const double seconds ) { return timespan_t( seconds );  }
  static timespan_t from_minutes( const double minutes ) { return timespan_t( ( time_t )( minutes * SECONDS_PER_MINUTE ) ); }

#endif

public:
  timespan_t() : time( 0 ) { }

  bool operator==( const timespan_t right ) const { return time == right.time; }
  bool operator!=( const timespan_t right ) const { return time != right.time; }

  bool operator>( const timespan_t right ) const { return time > right.time; }
  bool operator>=( const timespan_t right ) const { return time >= right.time; }
  bool operator<( const timespan_t right ) const { return time < right.time; }
  bool operator<=( const timespan_t right ) const { return time <= right.time; }

  timespan_t & operator+=( const timespan_t right )
  {
    time += right.time;
    return *this;
  }
  timespan_t operator-=( const timespan_t right )
  {
    time -= right.time;
    return *this;
  }
  timespan_t operator*=( const double right )
  {
    time = ( time_t )( time * right );
    return *this;
  }
  timespan_t operator*=( const int32_t right )
  {
    time *= right;
    return *this;
  }
  timespan_t operator*=( const int64_t right )
  {
    time = ( time_t )( time * right );
    return *this;
  }
  timespan_t operator*=( const uint32_t right )
  {
    time *= right;
    return *this;
  }
  timespan_t operator*=( const uint64_t right )
  {
    time = ( time_t )( time * right );
    return *this;
  }

  timespan_t operator/=( const double right )
  {
    time = ( time_t )( time / right );
    return *this;
  }
  timespan_t operator/=( const int32_t right )
  {
    time /= right;
    return *this;
  }
  timespan_t operator/=( const int64_t right )
  {
    time = ( time_t )( time / right );
    return *this;
  }
  timespan_t operator/=( const uint32_t right )
  {
    time /= right;
    return *this;
  }
  timespan_t operator/=( const uint64_t right )
  {
    time = ( time_t )( time / right );
    return *this;
  }

  friend timespan_t operator+( const timespan_t right );
  friend timespan_t operator-( const timespan_t right );

  friend timespan_t operator+( const timespan_t left, const timespan_t right );
  friend timespan_t operator-( const timespan_t left, const timespan_t right );
  friend timespan_t operator*( const timespan_t left, const double right );
  friend timespan_t operator*( const timespan_t left, const int32_t right );
  friend timespan_t operator*( const timespan_t left, const int64_t right );
  friend timespan_t operator*( const timespan_t left, const uint32_t right );
  friend timespan_t operator*( const timespan_t left, const uint64_t right );
  friend timespan_t operator/( const timespan_t left, const double right );
  friend timespan_t operator/( const timespan_t left, const int32_t right );
  friend timespan_t operator/( const timespan_t left, const int64_t right );
  friend timespan_t operator/( const timespan_t left, const uint32_t right );
  friend timespan_t operator/( const timespan_t left, const uint64_t right );
  friend double operator/( const timespan_t left, const timespan_t right );

  friend timespan_t operator*( const double left, const timespan_t right );
  friend timespan_t operator*( const int32_t left, const timespan_t right );
  friend timespan_t operator*( const int64_t left, const timespan_t right );
  friend timespan_t operator*( const uint32_t left, const timespan_t right );
  friend timespan_t operator*( const uint64_t left, const timespan_t right );

  static const timespan_t zero;
  static const timespan_t min;
  static const timespan_t max;
};

#ifdef SC_USE_INTEGER_TIME
#define TIMESPAN_TO_NATIVE_VALUE(t) ((t).total_millis())
#define TIMESPAN_FROM_NATIVE_VALUE(v) (timespan_t::from_millis(v))
#else // #ifndef SC_USE_INTEGER_TIME
#define TIMESPAN_TO_NATIVE_VALUE(t) ((t).total_seconds())
#define TIMESPAN_FROM_NATIVE_VALUE(v) (timespan_t::from_seconds(v))
#endif

inline timespan_t operator+( const timespan_t right )
{
  return right;
}

inline timespan_t operator-( const timespan_t right )
{
  return timespan_t( -right.time );
}

inline timespan_t operator+( const timespan_t left, const timespan_t right )
{
  return timespan_t( left.time + right.time );
}
inline timespan_t operator-( const timespan_t left, const timespan_t right )
{
  return timespan_t( left.time - right.time );
}
inline timespan_t operator*( const timespan_t left, const double right )
{
  return timespan_t( ( timespan_t::time_t )( left.time * right ) );
}
inline timespan_t operator*( const timespan_t left, const int32_t right )
{
  return timespan_t( ( timespan_t::time_t )( left.time * right ) );
}
inline timespan_t operator*( const timespan_t left, const int64_t right )
{
  return timespan_t( ( timespan_t::time_t )( left.time * right ) );
}
inline timespan_t operator*( const timespan_t left, const uint32_t right )
{
  return timespan_t( ( timespan_t::time_t )( left.time * right ) );
}
inline timespan_t operator*( const timespan_t left, const uint64_t right )
{
  return timespan_t( ( timespan_t::time_t )( left.time * right ) );
}
inline timespan_t operator/( const timespan_t left, const double right )
{
  return timespan_t( ( timespan_t::time_t )( left.time / right ) );
}
inline timespan_t operator/( const timespan_t left, const int32_t right )
{
  return timespan_t( ( timespan_t::time_t )( left.time / right ) );
}
inline timespan_t operator/( const timespan_t left, const int64_t right )
{
  return timespan_t( ( timespan_t::time_t )( left.time / ( timespan_t::time_t )right ) );
}
inline timespan_t operator/( const timespan_t left, const uint32_t right )
{
  return timespan_t( ( timespan_t::time_t )( left.time / ( timespan_t::time_t )right ) );
}
inline timespan_t operator/( const timespan_t left, const uint64_t right )
{
  return timespan_t( ( timespan_t::time_t )( left.time / ( timespan_t::time_t )right ) );
}
inline double operator/( const timespan_t left, const timespan_t right )
{
  return ( double )left.time / right.time;
}

inline timespan_t operator*( const double left, const timespan_t right )
{
  return timespan_t( ( timespan_t::time_t )( left * right.time ) );
}
inline timespan_t operator*( const int32_t left, const timespan_t right )
{
  return timespan_t( ( timespan_t::time_t )( left * right.time ) );
}
inline timespan_t operator*( const int64_t left, const timespan_t right )
{
  return timespan_t( ( timespan_t::time_t )( left * right.time ) );
}
inline timespan_t operator*( const uint32_t left, const timespan_t right )
{
  return timespan_t( ( timespan_t::time_t )( left * right.time ) );
}
inline timespan_t operator*( const uint64_t left, const timespan_t right )
{
  return timespan_t( ( timespan_t::time_t )( left * right.time ) );
}

// Cache Control ============================================================

namespace cache {

typedef int era_t;
static const era_t INVALID_ERA = -1;
static const era_t IN_THE_BEGINNING = 0;  // A time before any other possible era;
                                          // used to mark persistent caches at load.

enum behavior_t
{
  ANY,      // * Use any version present in the cache, retrieve if not present.
  CURRENT,  // * Use only current info from the cache; validate old versions as needed.
  ONLY,     // * Use any version present in the cache, fail if not present.
};

class cache_control_t
{
private:
  era_t current_era;
  behavior_t player_cache_behavior;
  behavior_t item_cache_behavior;

public:
  cache_control_t() :
    current_era( IN_THE_BEGINNING ),
    player_cache_behavior( CURRENT ),
    item_cache_behavior( ANY )
  {}

  era_t era() const { return current_era; }
  void advance_era() { ++current_era; }

  behavior_t cache_players() const { return player_cache_behavior; }
  void cache_players( behavior_t b ) { player_cache_behavior = b; }

  behavior_t cache_items() const { return item_cache_behavior; }
  void cache_items( behavior_t b ) { item_cache_behavior = b; }

  static cache_control_t singleton;
};

// Caching system's global notion of the current time.
inline era_t era()
{ return cache_control_t::singleton.era(); }

// Time marches on.
inline void advance_era()
{ cache_control_t::singleton.advance_era(); }

// Get/Set default cache behaviors.
inline behavior_t players()
{ return cache_control_t::singleton.cache_players(); }
inline void players( behavior_t b )
{ cache_control_t::singleton.cache_players( b ); }

inline behavior_t items()
{ return cache_control_t::singleton.cache_items(); }
inline void items( behavior_t b )
{ cache_control_t::singleton.cache_items( b ); }

}

struct stat_data_t
{
  double strength;
  double agility;
  double stamina;
  double intellect;
  double spirit;
  double base_health;
  double base_resource;
};

#define DEFAULT_MISC_VALUE std::numeric_limits<int>::min()

// These names come from the MANGOS project.
enum spell_attribute_t
{
  SPELL_ATTR_UNK0 = 0x0000, // 0
  SPELL_ATTR_RANGED, // 1 All ranged abilites have this flag
  SPELL_ATTR_ON_NEXT_SWING_1, // 2 on next swing
  SPELL_ATTR_UNK3, // 3 not set in 3.0.3
  SPELL_ATTR_UNK4, // 4 isAbility
  SPELL_ATTR_TRADESPELL, // 5 trade spells, will be added by client to a sublist of profession spell
  SPELL_ATTR_PASSIVE, // 6 Passive spell
  SPELL_ATTR_UNK7, // 7 can't be linked in chat?
  SPELL_ATTR_UNK8, // 8 hide created item in tooltip (for effect=24)
  SPELL_ATTR_UNK9, // 9
  SPELL_ATTR_ON_NEXT_SWING_2, // 10 on next swing 2
  SPELL_ATTR_UNK11, // 11
  SPELL_ATTR_DAYTIME_ONLY, // 12 only useable at daytime, not set in 2.4.2
  SPELL_ATTR_NIGHT_ONLY, // 13 only useable at night, not set in 2.4.2
  SPELL_ATTR_INDOORS_ONLY, // 14 only useable indoors, not set in 2.4.2
  SPELL_ATTR_OUTDOORS_ONLY, // 15 Only useable outdoors.
  SPELL_ATTR_NOT_SHAPESHIFT, // 16 Not while shapeshifted
  SPELL_ATTR_ONLY_STEALTHED, // 17 Must be in stealth
  SPELL_ATTR_UNK18, // 18
  SPELL_ATTR_LEVEL_DAMAGE_CALCULATION, // 19 spelldamage depends on caster level
  SPELL_ATTR_STOP_ATTACK_TARGE, // 20 Stop attack after use this spell (and not begin attack if use)
  SPELL_ATTR_IMPOSSIBLE_DODGE_PARRY_BLOCK, // 21 Cannot be dodged/parried/blocked
  SPELL_ATTR_UNK22, // 22
  SPELL_ATTR_UNK23, // 23 castable while dead?
  SPELL_ATTR_CASTABLE_WHILE_MOUNTED, // 24 castable while mounted
  SPELL_ATTR_DISABLED_WHILE_ACTIVE, // 25 Activate and start cooldown after aura fade or remove summoned creature or go
  SPELL_ATTR_UNK26, // 26
  SPELL_ATTR_CASTABLE_WHILE_SITTING, // 27 castable while sitting
  SPELL_ATTR_CANT_USED_IN_COMBAT, // 28 Cannot be used in combat
  SPELL_ATTR_UNAFFECTED_BY_INVULNERABILITY, // 29 unaffected by invulnerability (hmm possible not...)
  SPELL_ATTR_UNK30, // 30 breakable by damage?
  SPELL_ATTR_CANT_CANCEL, // 31 positive aura can't be canceled

  SPELL_ATTR_EX_UNK0 = 0x0100, // 0
  SPELL_ATTR_EX_DRAIN_ALL_POWER, // 1 use all power (Only paladin Lay of Hands and Bunyanize)
  SPELL_ATTR_EX_CHANNELED_1, // 2 channeled 1
  SPELL_ATTR_EX_UNK3, // 3
  SPELL_ATTR_EX_UNK4, // 4
  SPELL_ATTR_EX_NOT_BREAK_STEALTH, // 5 Not break stealth
  SPELL_ATTR_EX_CHANNELED_2, // 6 channeled 2
  SPELL_ATTR_EX_NEGATIVE, // 7
  SPELL_ATTR_EX_NOT_IN_COMBAT_TARGET, // 8 Spell req target not to be in combat state
  SPELL_ATTR_EX_UNK9, // 9
  SPELL_ATTR_EX_NO_INITIAL_AGGRO, // 10 no generates threat on cast 100%
  SPELL_ATTR_EX_UNK11, // 11
  SPELL_ATTR_EX_UNK12, // 12
  SPELL_ATTR_EX_UNK13, // 13
  SPELL_ATTR_EX_UNK14, // 14
  SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY, // 15 remove auras on immunity
  SPELL_ATTR_EX_UNAFFECTED_BY_SCHOOL_IMMUNE, // 16 unaffected by school immunity
  SPELL_ATTR_EX_UNK17, // 17 for auras AURA_TRACK_CREATURES, AURA_TRACK_RESOURCES and AURA_TRACK_STEALTHED select non-stacking tracking spells
  SPELL_ATTR_EX_UNK18, // 18
  SPELL_ATTR_EX_UNK19, // 19
  SPELL_ATTR_EX_REQ_COMBO_POINTS1, // 20 Req combo points on target
  SPELL_ATTR_EX_UNK21, // 21
  SPELL_ATTR_EX_REQ_COMBO_POINTS2, // 22 Req combo points on target
  SPELL_ATTR_EX_UNK23, // 23
  SPELL_ATTR_EX_UNK24, // 24 Req fishing pole??
  SPELL_ATTR_EX_UNK25, // 25
  SPELL_ATTR_EX_UNK26, // 26
  SPELL_ATTR_EX_UNK27, // 27
  SPELL_ATTR_EX_UNK28, // 28
  SPELL_ATTR_EX_UNK29, // 29
  SPELL_ATTR_EX_UNK30, // 30 overpower
  SPELL_ATTR_EX_UNK31, // 31

  SPELL_ATTR_EX2_UNK0 = 0x0200, // 0
  SPELL_ATTR_EX2_UNK1, // 1
  SPELL_ATTR_EX2_CANT_REFLECTED, // 2 ? used for detect can or not spell reflected // do not need LOS (e.g. 18220 since 3.3.3)
  SPELL_ATTR_EX2_UNK3, // 3 auto targeting? (e.g. fishing skill enhancement items since 3.3.3)
  SPELL_ATTR_EX2_UNK4, // 4
  SPELL_ATTR_EX2_AUTOREPEAT_FLAG, // 5
  SPELL_ATTR_EX2_UNK6, // 6 only usable on tabbed by yourself
  SPELL_ATTR_EX2_UNK7, // 7
  SPELL_ATTR_EX2_UNK8, // 8 not set in 3.0.3
  SPELL_ATTR_EX2_UNK9, // 9
  SPELL_ATTR_EX2_UNK10, // 10
  SPELL_ATTR_EX2_HEALTH_FUNNEL, // 11
  SPELL_ATTR_EX2_UNK12, // 12
  SPELL_ATTR_EX2_UNK13, // 13
  SPELL_ATTR_EX2_UNK14, // 14
  SPELL_ATTR_EX2_UNK15, // 15 not set in 3.0.3
  SPELL_ATTR_EX2_UNK16, // 16
  SPELL_ATTR_EX2_UNK17, // 17 suspend weapon timer instead of resetting it, (?Hunters Shot and Stings only have this flag?)
  SPELL_ATTR_EX2_UNK18, // 18 Only Revive pet - possible req dead pet
  SPELL_ATTR_EX2_NOT_NEED_SHAPESHIFT, // 19 does not necessarly need shapeshift
  SPELL_ATTR_EX2_UNK20, // 20
  SPELL_ATTR_EX2_DAMAGE_REDUCED_SHIELD, // 21 for ice blocks, pala immunity buffs, priest absorb shields, but used also for other spells -> not sure!
  SPELL_ATTR_EX2_UNK22, // 22
  SPELL_ATTR_EX2_UNK23, // 23 Only mage Arcane Concentration have this flag
  SPELL_ATTR_EX2_UNK24, // 24
  SPELL_ATTR_EX2_UNK25, // 25
  SPELL_ATTR_EX2_UNK26, // 26 unaffected by school immunity
  SPELL_ATTR_EX2_UNK27, // 27
  SPELL_ATTR_EX2_UNK28, // 28 no breaks stealth if it fails??
  SPELL_ATTR_EX2_CANT_CRIT, // 29 Spell can't crit
  SPELL_ATTR_EX2_UNK30, // 30
  SPELL_ATTR_EX2_FOOD_BUFF, // 31 Food or Drink Buff (like Well Fed)

  SPELL_ATTR_EX3_UNK0 = 0x0300, // 0
  SPELL_ATTR_EX3_UNK1, // 1
  SPELL_ATTR_EX3_UNK2, // 2
  SPELL_ATTR_EX3_UNK3, // 3
  SPELL_ATTR_EX3_UNK4, // 4 Druid Rebirth only this spell have this flag
  SPELL_ATTR_EX3_UNK5, // 5
  SPELL_ATTR_EX3_UNK6, // 6
  SPELL_ATTR_EX3_UNK7, // 7 create a separate (de)buff stack for each caster
  SPELL_ATTR_EX3_UNK8, // 8
  SPELL_ATTR_EX3_UNK9, // 9
  SPELL_ATTR_EX3_MAIN_HAND, // 10 Main hand weapon required
  SPELL_ATTR_EX3_BATTLEGROUND, // 11 Can casted only on battleground
  SPELL_ATTR_EX3_CAST_ON_DEAD, // 12 target is a dead player (not every spell has this flag)
  SPELL_ATTR_EX3_UNK13, // 13
  SPELL_ATTR_EX3_UNK14, // 14 "Honorless Target" only this spells have this flag
  SPELL_ATTR_EX3_UNK15, // 15 Auto Shoot, Shoot, Throw, - this is autoshot flag
  SPELL_ATTR_EX3_UNK16, // 16 no triggers effects that trigger on casting a spell??
  SPELL_ATTR_EX3_UNK17, // 17 no triggers effects that trigger on casting a spell??
  SPELL_ATTR_EX3_UNK18, // 18
  SPELL_ATTR_EX3_UNK19, // 19
  SPELL_ATTR_EX3_DEATH_PERSISTENT, // 20 Death persistent spells
  SPELL_ATTR_EX3_UNK21, // 21
  SPELL_ATTR_EX3_REQ_WAND, // 22 Req wand
  SPELL_ATTR_EX3_UNK23, // 23
  SPELL_ATTR_EX3_REQ_OFFHAND, // 24 Req offhand weapon
  SPELL_ATTR_EX3_UNK25, // 25 no cause spell pushback ?
  SPELL_ATTR_EX3_UNK26, // 26
  SPELL_ATTR_EX3_UNK27, // 27
  SPELL_ATTR_EX3_UNK28, // 28
  SPELL_ATTR_EX3_UNK29, // 29
  SPELL_ATTR_EX3_UNK30, // 30
  SPELL_ATTR_EX3_UNK31, // 31

  SPELL_ATTR_EX4_UNK0 = 0x0400, // 0
  SPELL_ATTR_EX4_UNK1, // 1 proc on finishing move?
  SPELL_ATTR_EX4_UNK2, // 2
  SPELL_ATTR_EX4_UNK3, // 3
  SPELL_ATTR_EX4_UNK4, // 4 This will no longer cause guards to attack on use??
  SPELL_ATTR_EX4_UNK5, // 5
  SPELL_ATTR_EX4_NOT_STEALABLE, // 6 although such auras might be dispellable, they cannot be stolen
  SPELL_ATTR_EX4_UNK7, // 7
  SPELL_ATTR_EX4_STACK_DOT_MODIFIER, // 8 no effect on non DoTs?
  SPELL_ATTR_EX4_UNK9, // 9
  SPELL_ATTR_EX4_SPELL_VS_EXTEND_COST, // 10 Rogue Shiv have this flag
  SPELL_ATTR_EX4_UNK11, // 11
  SPELL_ATTR_EX4_UNK12, // 12
  SPELL_ATTR_EX4_UNK13, // 13
  SPELL_ATTR_EX4_UNK14, // 14
  SPELL_ATTR_EX4_UNK15, // 15
  SPELL_ATTR_EX4_NOT_USABLE_IN_ARENA, // 16 not usable in arena
  SPELL_ATTR_EX4_USABLE_IN_ARENA, // 17 usable in arena
  SPELL_ATTR_EX4_UNK18, // 18
  SPELL_ATTR_EX4_UNK19, // 19
  SPELL_ATTR_EX4_UNK20, // 20 do not give "more powerful spell" error message
  SPELL_ATTR_EX4_UNK21, // 21
  SPELL_ATTR_EX4_UNK22, // 22
  SPELL_ATTR_EX4_UNK23, // 23
  SPELL_ATTR_EX4_UNK24, // 24
  SPELL_ATTR_EX4_UNK25, // 25 pet scaling auras
  SPELL_ATTR_EX4_CAST_ONLY_IN_OUTLAND, // 26 Can only be used in Outland.
  SPELL_ATTR_EX4_UNK27, // 27
  SPELL_ATTR_EX4_UNK28, // 28
  SPELL_ATTR_EX4_UNK29, // 29
  SPELL_ATTR_EX4_UNK30, // 30
  SPELL_ATTR_EX4_UNK31, // 31

  SPELL_ATTR_EX5_UNK0 = 0x0500, // 0
  SPELL_ATTR_EX5_NO_REAGENT_WHILE_PREP, // 1 not need reagents if UNIT_FLAG_PREPARATION
  SPELL_ATTR_EX5_UNK2, // 2 removed at enter arena (e.g. 31850 since 3.3.3)
  SPELL_ATTR_EX5_USABLE_WHILE_STUNNED, // 3 usable while stunned
  SPELL_ATTR_EX5_UNK4, // 4
  SPELL_ATTR_EX5_SINGLE_TARGET_SPELL, // 5 Only one target can be apply at a time
  SPELL_ATTR_EX5_UNK6, // 6
  SPELL_ATTR_EX5_UNK7, // 7
  SPELL_ATTR_EX5_UNK8, // 8
  SPELL_ATTR_EX5_START_PERIODIC_AT_APPLY, // 9 begin periodic tick at aura apply
  SPELL_ATTR_EX5_UNK10, // 10
  SPELL_ATTR_EX5_UNK11, // 11
  SPELL_ATTR_EX5_UNK12, // 12
  SPELL_ATTR_EX5_UNK13, // 13 haste affects duration (e.g. 8050 since 3.3.3)
  SPELL_ATTR_EX5_UNK14, // 14
  SPELL_ATTR_EX5_UNK15, // 15
  SPELL_ATTR_EX5_UNK16, // 16
  SPELL_ATTR_EX5_USABLE_WHILE_FEARED, // 17 usable while feared
  SPELL_ATTR_EX5_USABLE_WHILE_CONFUSED, // 18 usable while confused
  SPELL_ATTR_EX5_UNK19, // 19
  SPELL_ATTR_EX5_UNK20, // 20
  SPELL_ATTR_EX5_UNK21, // 21
  SPELL_ATTR_EX5_UNK22, // 22
  SPELL_ATTR_EX5_UNK23, // 23
  SPELL_ATTR_EX5_UNK24, // 24
  SPELL_ATTR_EX5_UNK25, // 25
  SPELL_ATTR_EX5_UNK26, // 26
  SPELL_ATTR_EX5_UNK27, // 27
  SPELL_ATTR_EX5_UNK28, // 28
  SPELL_ATTR_EX5_UNK29, // 29
  SPELL_ATTR_EX5_UNK30, // 30
  SPELL_ATTR_EX5_UNK31, // 31 Forces all nearby enemies to focus attacks caster

  SPELL_ATTR_EX6_UNK0 = 0x0600, // 0 Only Move spell have this flag
  SPELL_ATTR_EX6_ONLY_IN_ARENA, // 1 only usable in arena, not used in 3.2.0a and early
  SPELL_ATTR_EX6_UNK2, // 2
  SPELL_ATTR_EX6_UNK3, // 3
  SPELL_ATTR_EX6_UNK4, // 4
  SPELL_ATTR_EX6_UNK5, // 5
  SPELL_ATTR_EX6_UNK6, // 6
  SPELL_ATTR_EX6_UNK7, // 7
  SPELL_ATTR_EX6_UNK8, // 8
  SPELL_ATTR_EX6_UNK9, // 9
  SPELL_ATTR_EX6_UNK10, // 10
  SPELL_ATTR_EX6_NOT_IN_RAID_INSTANCE, // 11 not usable in raid instance
  SPELL_ATTR_EX6_UNK12, // 12 for auras AURA_TRACK_CREATURES, AURA_TRACK_RESOURCES and AURA_TRACK_STEALTHED select non-stacking tracking spells
  SPELL_ATTR_EX6_UNK13, // 13
  SPELL_ATTR_EX6_UNK14, // 14
  SPELL_ATTR_EX6_UNK15, // 15 not set in 3.0.3
  SPELL_ATTR_EX6_UNK16, // 16
  SPELL_ATTR_EX6_UNK17, // 17
  SPELL_ATTR_EX6_UNK18, // 18
  SPELL_ATTR_EX6_UNK19, // 19
  SPELL_ATTR_EX6_UNK20, // 20
  SPELL_ATTR_EX6_UNK21, // 21
  SPELL_ATTR_EX6_UNK22, // 22
  SPELL_ATTR_EX6_UNK23, // 23 not set in 3.0.3
  SPELL_ATTR_EX6_UNK24, // 24 not set in 3.0.3
  SPELL_ATTR_EX6_UNK25, // 25 not set in 3.0.3
  SPELL_ATTR_EX6_UNK26, // 26 not set in 3.0.3
  SPELL_ATTR_EX6_UNK27, // 27 not set in 3.0.3
  SPELL_ATTR_EX6_UNK28, // 28 not set in 3.0.3
  SPELL_ATTR_EX6_NO_DMG_PERCENT_MODS, // 29 do not apply damage percent mods (usually in cases where it has already been applied)
  SPELL_ATTR_EX6_UNK30, // 30 not set in 3.0.3
  SPELL_ATTR_EX6_UNK31, // 31 not set in 3.0.3

  SPELL_ATTR_EX7_UNK0 = 0x0700, // 0
  SPELL_ATTR_EX7_UNK1, // 1
  SPELL_ATTR_EX7_UNK2, // 2
  SPELL_ATTR_EX7_UNK3, // 3
  SPELL_ATTR_EX7_UNK4, // 4
  SPELL_ATTR_EX7_UNK5, // 5
  SPELL_ATTR_EX7_UNK6, // 6
  SPELL_ATTR_EX7_UNK7, // 7
  SPELL_ATTR_EX7_UNK8, // 8
  SPELL_ATTR_EX7_UNK9, // 9
  SPELL_ATTR_EX7_UNK10, // 10
  SPELL_ATTR_EX7_UNK11, // 11
  SPELL_ATTR_EX7_UNK12, // 12
  SPELL_ATTR_EX7_UNK13, // 13
  SPELL_ATTR_EX7_UNK14, // 14
  SPELL_ATTR_EX7_UNK15, // 15
  SPELL_ATTR_EX7_UNK16, // 16
  SPELL_ATTR_EX7_UNK17, // 17
  SPELL_ATTR_EX7_UNK18, // 18
  SPELL_ATTR_EX7_UNK19, // 19
  SPELL_ATTR_EX7_UNK20, // 20
  SPELL_ATTR_EX7_UNK21, // 21
  SPELL_ATTR_EX7_UNK22, // 22
  SPELL_ATTR_EX7_UNK23, // 23
  SPELL_ATTR_EX7_UNK24, // 24
  SPELL_ATTR_EX7_UNK25, // 25
  SPELL_ATTR_EX7_UNK26, // 26
  SPELL_ATTR_EX7_UNK27, // 27
  SPELL_ATTR_EX7_UNK28, // 28
  SPELL_ATTR_EX7_UNK29, // 29
  SPELL_ATTR_EX7_UNK30, // 30
  SPELL_ATTR_EX7_UNK31, // 31

  SPELL_ATTR_EX8_UNK0 = 0x0800, // 0
  SPELL_ATTR_EX8_UNK1, // 1
  SPELL_ATTR_EX8_UNK2, // 2
  SPELL_ATTR_EX8_UNK3, // 3
  SPELL_ATTR_EX8_UNK4, // 4
  SPELL_ATTR_EX8_UNK5, // 5
  SPELL_ATTR_EX8_UNK6, // 6
  SPELL_ATTR_EX8_UNK7, // 7
  SPELL_ATTR_EX8_UNK8, // 8
  SPELL_ATTR_EX8_UNK9, // 9
  SPELL_ATTR_EX8_UNK10, // 10
  SPELL_ATTR_EX8_UNK11, // 11
  SPELL_ATTR_EX8_UNK12, // 12
  SPELL_ATTR_EX8_UNK13, // 13
  SPELL_ATTR_EX8_UNK14, // 14
  SPELL_ATTR_EX8_UNK15, // 15
  SPELL_ATTR_EX8_UNK16, // 16
  SPELL_ATTR_EX8_UNK17, // 17
  SPELL_ATTR_EX8_UNK18, // 18
  SPELL_ATTR_EX8_UNK19, // 19
  SPELL_ATTR_EX8_UNK20, // 20
  SPELL_ATTR_EX8_UNK21, // 21
  SPELL_ATTR_EX8_UNK22, // 22
  SPELL_ATTR_EX8_UNK23, // 23
  SPELL_ATTR_EX8_UNK24, // 24
  SPELL_ATTR_EX8_UNK25, // 25
  SPELL_ATTR_EX8_UNK26, // 26
  SPELL_ATTR_EX8_UNK27, // 27
  SPELL_ATTR_EX8_UNK28, // 28
  SPELL_ATTR_EX8_UNK29, // 29
  SPELL_ATTR_EX8_UNK30, // 30
  SPELL_ATTR_EX8_UNK31, // 31

  SPELL_ATTR_EX9_UNK0 = 0x0900, // 0
  SPELL_ATTR_EX9_UNK1, // 1
  SPELL_ATTR_EX9_UNK2, // 2
  SPELL_ATTR_EX9_UNK3, // 3
  SPELL_ATTR_EX9_UNK4, // 4
  SPELL_ATTR_EX9_UNK5, // 5
  SPELL_ATTR_EX9_UNK6, // 6
  SPELL_ATTR_EX9_UNK7, // 7
  SPELL_ATTR_EX9_UNK8, // 8
  SPELL_ATTR_EX9_UNK9, // 9
  SPELL_ATTR_EX9_UNK10, // 10
  SPELL_ATTR_EX9_UNK11, // 11
  SPELL_ATTR_EX9_UNK12, // 12
  SPELL_ATTR_EX9_UNK13, // 13
  SPELL_ATTR_EX9_UNK14, // 14
  SPELL_ATTR_EX9_UNK15, // 15
  SPELL_ATTR_EX9_UNK16, // 16
  SPELL_ATTR_EX9_UNK17, // 17
  SPELL_ATTR_EX9_UNK18, // 18
  SPELL_ATTR_EX9_UNK19, // 19
  SPELL_ATTR_EX9_UNK20, // 20
  SPELL_ATTR_EX9_UNK21, // 21
  SPELL_ATTR_EX9_UNK22, // 22
  SPELL_ATTR_EX9_UNK23, // 23
  SPELL_ATTR_EX9_UNK24, // 24
  SPELL_ATTR_EX9_UNK25, // 25
  SPELL_ATTR_EX9_UNK26, // 26
  SPELL_ATTR_EX9_UNK27, // 27
  SPELL_ATTR_EX9_UNK28, // 28
  SPELL_ATTR_EX9_UNK29, // 29
  SPELL_ATTR_EX9_UNK30, // 30
  SPELL_ATTR_EX9_UNK31, // 31
};

// DBC related classes ======================================================

struct spell_data_t
{
private:
  static const unsigned FLAG_USED = 1;
  static const unsigned FLAG_DISABLED = 2;

  friend class dbc_t;
  static void link( bool ptr );

public:
  const char* _name;               // Spell name from Spell.dbc stringblock (enGB)
  unsigned    _id;                 // Spell ID in dbc
  unsigned    _flags;              // Unused for now, 0x00 for all
  double      _prj_speed;          // Projectile Speed
  unsigned    _school;             // Spell school mask
  int         _power_type;         // Resource type
  unsigned    _class_mask;         // Class mask for spell
  unsigned    _race_mask;          // Racial mask for the spell
  int         _scaling_type;       // Array index for gtSpellScaling.dbc. -1 means the last sub-array, 0 disabled
  double      _extra_coeff;        // An "extra" coefficient (used for some spells to indicate AP based coefficient)
  // SpellLevels.dbc
  unsigned    _spell_level;        // Spell learned on level. NOTE: Only accurate for "class abilities"
  unsigned    _max_level;          // Maximum level for scaling
  // SpellRange.dbc
  double      _min_range;          // Minimum range in yards
  double      _max_range;          // Maximum range in yards
  // SpellCooldown.dbc
  unsigned    _cooldown;           // Cooldown in milliseconds
  unsigned    _gcd;                // GCD in milliseconds
  // SpellCategories.dbc
  unsigned    _category;           // Spell category (for shared cooldowns, effects?)
  // SpellDuration.dbc
  double      _duration;           // Spell duration in milliseconds
  // SpellPower.dbc
  unsigned    _cost;               // Resource cost
  // SpellRuneCost.dbc
  unsigned    _rune_cost;          // Bitmask of rune cost 0x1, 0x2 = Blood | 0x4, 0x8 = Unholy | 0x10, 0x20 = Frost
  unsigned    _runic_power_gain;   // Amount of runic power gained ( / 10 )
  // SpellAuraOptions.dbc
  unsigned    _max_stack;          // Maximum stack size for spell
  unsigned    _proc_chance;        // Spell proc chance in percent
  unsigned    _proc_charges;       // Per proc charge amount
  // SpellEquippedItems.dbc
  unsigned    _equipped_class;
  unsigned    _equipped_invtype_mask;
  unsigned    _equipped_subclass_mask;
  // SpellScaling.dbc
  int         _cast_min;           // Minimum casting time in milliseconds
  int         _cast_max;           // Maximum casting time in milliseconds
  int         _cast_div;           // A divisor used in the formula for casting time scaling (20 always?)
  double      _c_scaling;          // A scaling multiplier for level based scaling
  unsigned    _c_scaling_level;    // A scaling divisor for level based scaling
  // SpellEffect.dbc
  unsigned    _effect[MAX_EFFECTS];// Effect identifiers
  // Spell.dbc flags
  unsigned    _attributes[NUM_SPELL_FLAGS];// Spell.dbc "flags", record field 1..10, note that 12694 added a field here after flags_7
  const char* _desc;               // Spell.dbc description stringblock
  const char* _tooltip;            // Spell.dbc tooltip stringblock
  // SpellDescriptionVariables.dbc
  const char* _desc_vars;          // Spell description variable stringblock, if present
  // SpellIcon.dbc
  const char* _icon;

  // Pointers for runtime linking
  spelleffect_data_t* _effect1;
  spelleffect_data_t* _effect2;
  spelleffect_data_t* _effect3;

  const spelleffect_data_t& effect1() const { return *_effect1; }
  const spelleffect_data_t& effect2() const { return *_effect2; }
  const spelleffect_data_t& effect3() const { return *_effect3; }

  bool                 is_used() const { return _flags & FLAG_USED; }
  void                 set_used( bool value );

  bool                 is_enabled() const { return ! ( _flags & FLAG_DISABLED ); }
  void                 set_enabled( bool value );

  unsigned             id() const { return _id; }
  uint32_t             school_mask() const { return _school; }
  resource_type        power_type() const;

  bool                 is_class( player_type c ) const;
  uint32_t             class_mask() const { return _class_mask; }

  bool                 is_race( race_type r ) const;
  uint32_t             race_mask() const { return _race_mask; }

  bool                 is_level( uint32_t level ) const { return level >= _spell_level; }
  uint32_t             level() const { return _spell_level; }
  uint32_t             max_level() const { return _max_level; }

  player_type          scaling_class() const;

  double               missile_speed() const { return _prj_speed; }
  double               min_range() const { return _min_range; }
  double               max_range() const { return _max_range; }
  bool                 in_range( double range ) const { return range >= _min_range && range <= _max_range; }

  timespan_t           cooldown() const { return timespan_t::from_millis( _cooldown ); }
  timespan_t           duration() const { return timespan_t::from_millis( _duration ); }
  timespan_t           gcd() const { return timespan_t::from_millis( _gcd ); }
  timespan_t           cast_time( uint32_t level ) const;

  uint32_t             category() const { return _category; }

  double               cost() const;
  uint32_t             rune_cost() const { return _rune_cost; }
  double               runic_power_gain() const { return _runic_power_gain * ( 1 / 10.0 ); }

  uint32_t             max_stacks() const { return _max_stack; }
  uint32_t             initial_stacks() const { return _proc_charges; }

  double               proc_chance() const { return _proc_chance * ( 1 / 100.0 ); }

  uint32_t             effect_id( uint32_t effect_num ) const
  {
    assert( effect_num >= 1 && effect_num <= MAX_EFFECTS );
    return _effect[ effect_num - 1 ];
  }

  bool                 flags( spell_attribute_t f ) const;

  const char*          name_cstr() const { return _name; }
  const char*          desc() const { return _desc; }
  const char*          tooltip() const { return _tooltip; }

  double               scaling_multiplier() const { return _c_scaling; }
  unsigned             scaling_threshold() const { return _c_scaling_level; }
  double               extra_coeff() const { return _extra_coeff; }

  static spell_data_t* nil();
  static spell_data_t* find( const char* name, bool ptr = false );
  static spell_data_t* find( unsigned id, bool ptr = false );
  static spell_data_t* find( unsigned id, const char* confirmation, bool ptr = false );
  static spell_data_t* list( bool ptr = false );
};

class spell_data_nil_t : public spell_data_t
{
public:
  spell_data_nil_t();
  static spell_data_nil_t singleton;
};

inline spell_data_t* spell_data_t::nil()
{ return &spell_data_nil_t::singleton; }

// SpellEffect.dbc
struct spelleffect_data_t
{
private:
  static const unsigned int FLAG_USED = 1;
  static const unsigned int FLAG_ENABLED = 2;

public:
  unsigned         _id;              // Effect id
  unsigned         _flags;           // Unused for now, 0x00 for all
  unsigned         _spell_id;        // Spell this effect belongs to
  unsigned         _index;           // Effect index for the spell
  effect_type_t    _type;            // Effect type
  effect_subtype_t _subtype;         // Effect sub-type
  // SpellScaling.dbc
  double           _m_avg;           // Effect average spell scaling multiplier
  double           _m_delta;         // Effect delta spell scaling multiplier
  double           _m_unk;           // Unused effect scaling multiplier
  //
  double           _coeff;           // Effect coefficient
  double           _amplitude;       // Effect amplitude (e.g., tick time)
  // SpellRadius.dbc
  double           _radius;          // Minimum spell radius
  double           _radius_max;      // Maximum spell radius
  //
  int              _base_value;      // Effect value
  int              _misc_value;      // Effect miscellaneous value
  int              _misc_value_2;    // Effect miscellaneous value 2
  int              _trigger_spell_id;// Effect triggers this spell id
  double           _m_chain;         // Effect chain multiplier
  double           _pp_combo_points; // Effect points per combo points
  double           _real_ppl;        // Effect real points per level
  int              _die_sides;       // Effect damage range

  // Pointers for runtime linking
  spell_data_t*    _spell;
  spell_data_t*    _trigger_spell;

  bool                       is_used() const { return ( _flags & FLAG_USED ) != 0; }
  void                       set_used( bool value );

  bool                       is_enabled() const { return ( _flags & FLAG_ENABLED ) != 0; }
  void                       set_enabled( bool value );

  unsigned                   id() const { return _id; }
  unsigned                   index() const { return _index; }
  unsigned                   spell_id() const { return _spell_id; }
  unsigned                   spell_effect_num() const { return _index; }

  effect_type_t              type() const { return _type; }
  effect_subtype_t           subtype() const { return _subtype; }

  int                        base_value() const { return _base_value; }
  double percent() const { return _base_value * ( 1 / 100.0 ); }
  timespan_t time_value() const { return timespan_t::from_millis( _base_value ); }
  double resource( int type ) const
  {
    switch ( type )
    {
    case RESOURCE_RAGE:
      return _base_value * ( 1 / 10.0 );
    case RESOURCE_MANA:
      return _base_value * ( 1 / 100.0 );
    default:
      return _base_value;
    }
  }

  int                        misc_value1() const { return _misc_value; }
  int                        misc_value2() const { return _misc_value_2; }

  unsigned                   trigger_spell_id() const { return _trigger_spell_id >= 0 ? _trigger_spell_id : 0; }
  double                     chain_multiplier() const { return _m_chain; }

  double                     m_average() const { return _m_avg; }
  double                     m_delta() const { return _m_delta; }
  double                     m_unk() const { return _m_unk; }

  double                     coeff() const { return _coeff; }

  timespan_t                 period() const { return timespan_t::from_millis( _amplitude ); }

  double                     radius() const { return _radius; }
  double                     radius_max() const { return _radius_max; }

  double                     pp_combo_points() const { return _pp_combo_points; }

  double                     real_ppl() const { return _real_ppl; }

  int                        die_sides() const { return _die_sides; }

  spell_data_t& spell()   const { return *_spell; }
  spell_data_t& trigger() const { return *_trigger_spell; }

  static spelleffect_data_t* nil();
  static spelleffect_data_t* find( unsigned, bool ptr = false );
  static spelleffect_data_t* list( bool ptr = false );
  static void                link( bool ptr = false );
};

class spelleffect_data_nil_t : public spelleffect_data_t
{
public:
  spelleffect_data_nil_t();
  static spelleffect_data_nil_t singleton;
};

inline spelleffect_data_t* spelleffect_data_t::nil()
{ return &spelleffect_data_nil_t::singleton; }

struct talent_data_t
{
private:
  static const unsigned FLAG_USED = 0x01;
  static const unsigned FLAG_DISABLED = 0x02;

public:
  const char * _name;        // Talent name
  unsigned     _id;          // Talent id
  unsigned     _flags;       // Unused for now, 0x00 for all
  unsigned     _tab_page;    // Talent tab page
  unsigned     _m_class;     // Class mask
  unsigned     _m_pet;       // Pet mask
  unsigned     _dependance;  // Talent depends on this talent id
  unsigned     _depend_rank; // Requires this rank of depended talent
  unsigned     _col;         // Talent column
  unsigned     _row;         // Talent row
  unsigned     _rank_id[MAX_RANK]; // Talent spell rank identifiers for ranks 1..3

  // Pointers for runtime linking
  spell_data_t* spell1;
  spell_data_t* spell2;
  spell_data_t* spell3;

  bool                  is_used() const { return ( _flags & FLAG_USED ) != 0; }
  bool                  is_enabled() const { return ( _flags & FLAG_DISABLED ) == 0; }
  void                  set_used( bool value );
  void                  set_enabled( bool value );

  inline unsigned       id() const { return _id; }
  const char*           name_cstr() const { return _name; }
  unsigned              tab_page() const { return _tab_page; }
  bool                  is_class( player_type c ) const;
  bool                  is_pet( pet_type_t p ) const;
  unsigned              depends_id() const { return _dependance; }
  unsigned              depends_rank() const { return _depend_rank + 1; }

  unsigned              col() const { return _col; }
  unsigned              row() const { return _row; }
  unsigned              rank_spell_id( unsigned rank ) const;
  unsigned              mask_class() const { return _m_class; }
  unsigned              mask_pet() const { return _m_pet; }

  unsigned              max_rank() const;

  spell_data_t& spell( unsigned r )
  {
    if ( r == 0 ) return *spell1;
    if ( r == 1 ) return *spell2;
    if ( r == 2 ) return *spell3;
    return *spell_data_t::nil();
  }

  static talent_data_t* nil();
  static talent_data_t* find( unsigned, bool ptr = false );
  static talent_data_t* find( unsigned, const char* confirmation, bool ptr = false );
  static talent_data_t* find( const char* name, bool ptr = false );
  static talent_data_t* list( bool ptr = false );
  static void           link( bool ptr = false );
};

class talent_data_nil_t : public talent_data_t
{
public:
  talent_data_nil_t();
  static talent_data_nil_t singleton;
};

inline talent_data_t* talent_data_t::nil()
{ return &talent_data_nil_t::singleton; }

class dbc_t
{
public:
  bool ptr;

  dbc_t( bool ptr=false ) : ptr( ptr ) { }

  // Static Initialization
  static void init();
  static void de_init() {}
  static std::vector<unsigned> glyphs( int cid, bool ptr = false );

  static const char* build_level( bool ptr = false );
  static const char* wow_version( bool ptr = false );

  static const item_data_t* items( bool ptr = false );
  static std::size_t        n_items( bool ptr = false );

  // Game data table access
  double melee_crit_base( player_type t ) const;
  double melee_crit_base( pet_type_t t ) const;
  double spell_crit_base( player_type t ) const;
  double spell_crit_base( pet_type_t t ) const;
  double dodge_base( player_type t ) const;
  double dodge_base( pet_type_t t ) const;
  double regen_base( player_type t, unsigned level ) const;
  double regen_base( pet_type_t t, unsigned level ) const;
  stat_data_t& attribute_base( player_type t, unsigned level ) const;
  stat_data_t& attribute_base( pet_type_t t, unsigned level ) const;
  stat_data_t& race_base( race_type r ) const;
  stat_data_t& race_base( pet_type_t t ) const;

  double spell_scaling( player_type t, unsigned level ) const;
  double melee_crit_scaling( player_type t, unsigned level ) const;
  double melee_crit_scaling( pet_type_t t, unsigned level ) const;
  double spell_crit_scaling( player_type t, unsigned level ) const;
  double spell_crit_scaling( pet_type_t t, unsigned level ) const;
  double dodge_scaling( player_type t, unsigned level ) const;
  double dodge_scaling( pet_type_t t, unsigned level ) const;

  double regen_spirit( player_type t, unsigned level ) const;
  double regen_spirit( pet_type_t t, unsigned level ) const;
  double oct_regen_mp( player_type t, unsigned level ) const;
  double oct_regen_mp( pet_type_t t, unsigned level ) const;

  double combat_rating( unsigned combat_rating_id, unsigned level ) const;
  double oct_combat_rating( unsigned combat_rating_id, player_type t ) const;

  const spell_data_t*            spell( unsigned spell_id ) const { return spell_data_t::find( spell_id, ptr ); }
  const spelleffect_data_t*      effect( unsigned effect_id ) const { return spelleffect_data_t::find( effect_id, ptr ); }
  const talent_data_t*           talent( unsigned talent_id ) const { return talent_data_t::find( talent_id, ptr ); }
  const item_data_t*             item( unsigned item_id ) const;

  const random_suffix_data_t&    random_suffix( unsigned suffix_id ) const;
  const item_enchantment_data_t& item_enchantment( unsigned enchant_id ) const;
  const gem_property_data_t&     gem_property( unsigned gem_id ) const;

  const random_prop_data_t&      random_property( unsigned ilevel ) const;
  const item_scale_data_t&       item_damage_1h( unsigned ilevel ) const;
  const item_scale_data_t&       item_damage_2h( unsigned ilevel ) const;
  const item_scale_data_t&       item_damage_caster_1h( unsigned ilevel ) const;
  const item_scale_data_t&       item_damage_caster_2h( unsigned ilevel ) const;
  const item_scale_data_t&       item_damage_ranged( unsigned ilevel ) const;
  const item_scale_data_t&       item_damage_thrown( unsigned ilevel ) const;
  const item_scale_data_t&       item_damage_wand( unsigned ilevel ) const;

  const item_scale_data_t&       item_armor_quality( unsigned ilevel ) const;
  const item_scale_data_t&       item_armor_shield( unsigned ilevel ) const;
  const item_armor_type_data_t&  item_armor_total( unsigned ilevel ) const;
  const item_armor_type_data_t&  item_armor_inv_type( unsigned inv_type ) const;

  // Derived data access
  unsigned class_ability( unsigned class_id, unsigned tree_id, unsigned n ) const;
  unsigned class_ability_tree_size() const;
  unsigned class_ability_size() const;

  unsigned race_ability( unsigned race_id, unsigned class_id, unsigned n ) const;
  unsigned race_ability_size() const;

  unsigned specialization_ability( unsigned class_id, unsigned tree_id, unsigned n ) const;
  unsigned specialization_ability_tree_size() const;
  unsigned specialization_ability_size() const;

  unsigned mastery_ability( unsigned class_id, unsigned n ) const;
  unsigned mastery_ability_size() const;

  unsigned glyph_spell( unsigned class_id, unsigned glyph_type, unsigned n ) const;
  unsigned glyph_spell_size() const;

  unsigned set_bonus_spell( unsigned class_id, unsigned tier, unsigned n ) const;
  unsigned set_bonus_spell_size() const;

  // Helper methods
  double   weapon_dps( unsigned item_id ) const;

  double   effect_average( unsigned effect_id, unsigned level ) const;
  double   effect_delta( unsigned effect_id, unsigned level ) const;

  double   effect_min( unsigned effect_id, unsigned level ) const;
  double   effect_max( unsigned effect_id, unsigned level ) const;
  double   effect_bonus( unsigned effect_id, unsigned level ) const;

  unsigned class_ability_id( player_type c, const char* spell_name, int tree = -1 ) const;
  unsigned race_ability_id( player_type c, race_type r, const char* spell_name ) const;
  unsigned specialization_ability_id( player_type c, const char* spell_name, int tree = -1 ) const;
  unsigned mastery_ability_id( player_type c, const char* spell_name ) const;
  unsigned glyph_spell_id( player_type c, const char* spell_name ) const;
  unsigned set_bonus_spell_id( player_type c, const char* spell_name, int tier = -1 ) const;

  int      class_ability_tree( player_type c, uint32_t spell_id ) const;
  int      specialization_ability_tree( player_type c, uint32_t spell_id ) const;

  bool     is_class_ability( uint32_t spell_id ) const;
  bool     is_race_ability( uint32_t spell_id ) const;
  bool     is_specialization_ability( uint32_t spell_id ) const;
  bool     is_mastery_ability( uint32_t spell_id ) const;
  bool     is_glyph_spell( uint32_t spell_id ) const;
  bool     is_set_bonus_spell( uint32_t spell_id ) const;

  // Static helper methods
  static double fmt_value( double v, effect_type_t type, effect_subtype_t sub_type );
};

// Options ==================================================================

enum option_type_t
{
  OPT_NONE=0,
  OPT_STRING,     // std::string*
  OPT_APPEND,     // std::string* (append)
  OPT_CHARP,      // char*
  OPT_BOOL,       // int (only valid values are 1 and 0)
  OPT_INT,        // int
  OPT_FLT,        // double
  OPT_TIMESPAN,   // time interval
  OPT_LIST,       // std::vector<std::string>*
  OPT_FUNC,       // function pointer
  OPT_TALENT_RANK, // talent rank
  OPT_SPELL_ENABLED, // spell enabled
  OPT_DEPRECATED,
  OPT_UNKNOWN
};

typedef bool ( *option_function_t )( sim_t* sim, const std::string& name, const std::string& value );

struct option_t
{
  const char* name;
  int type;
  void* address;

  void print( FILE* );
  void save ( FILE* );
  bool parse( sim_t*, const std::string& name, const std::string& value );

  static void add( std::vector<option_t>&, const char* name, int type, void* address );
  static void copy( std::vector<option_t>& opt_vector, const option_t* opt_array );
  static bool parse( sim_t*, std::vector<option_t>&, const std::string& name, const std::string& value );
  static bool parse( sim_t*, const char* context, std::vector<option_t>&, const std::string& options_str );
  static bool parse( sim_t*, const char* context, const option_t*,        const std::string& options_str );
  static bool parse_file( sim_t*, FILE* file );
  static bool parse_line( sim_t*, char* line );
  static bool parse_token( sim_t*, std::string& token );
  static option_t* merge( std::vector<option_t>& out, const option_t* in1, const option_t* in2 );
};

// Talent Translation =======================================================

#define MAX_TALENT_POINTS 41
#define MAX_TALENT_ROW ((MAX_TALENT_POINTS+4)/5)
#define MAX_TALENT_TREES 3
#define MAX_TALENT_COL 4
#define MAX_TALENT_SLOTS (MAX_TALENT_TREES*MAX_TALENT_ROW*MAX_TALENT_COL)
#define MAX_TALENT_RANK_SLOTS ( 90 )

#define MAX_CLASS ( 12 )

struct talent_translation_t
{
  int  index;
  int  max;
  int* address;
  int  row;
  int  req;
  int  tree;
  std::string name;
};

// Utilities ================================================================

#ifdef _MSC_VER
// C99-compliant snprintf - MSVC _snprintf is NOT the same.

#undef vsnprintf
int vsnprintf_simc( char* buf, size_t size, const char* fmt, va_list ap );
#define vsnprintf vsnprintf_simc

#undef snprintf
inline int snprintf( char* buf, size_t size, const char* fmt, ... )
{
  va_list ap;
  va_start( ap, fmt );
  int rval = vsnprintf( buf, size, fmt, ap );
  va_end( ap );
  return rval;
}
#endif

struct util_t
{
private:
  static void str_to_utf8_( std::string& str );
  static void str_to_latin1_( std::string& str );
  static void urlencode_( std::string& str );
  static void urldecode_( std::string& str );
  static void format_text_( std::string& name, bool input_is_utf8 );
  static void html_special_char_decode_( std::string& str );
  static void tolower_( std::string& );
  static void string_split_( std::vector<std::string>& results, const std::string& str, const char* delim, bool allow_quotes );
  static void replace_all_( std::string&, const char* from, char to );
  static void replace_all_( std::string&, char from, const char* to );
  static int vfprintf_helper( FILE *stream, const char *format, va_list fmtargs );

public:
  static double talent_rank( int num, int max, double increment );
  static double talent_rank( int num, int max, double value1, double value2, ... );

  static int talent_rank( int num, int max, int increment );
  static int talent_rank( int num, int max, int value1, int value2, ... );

  static double ability_rank( int player_level, double ability_value, int ability_level, ... );
  static int    ability_rank( int player_level, int    ability_value, int ability_level, ... );

  static char* dup( const char* );

  static const char* attribute_type_string     ( int type );
  static const char* dmg_type_string           ( int type );
  static const char* stim_type_string         ( int type );
  static const char* gem_type_string           ( int type );
  static const char* meta_gem_type_string      ( int type );
  static const char* player_type_string        ( int type );
  static const char* pet_type_string           ( int type );
  static const char* position_type_string      ( int type );
  static const char* profession_type_string    ( int type );
  static const char* race_type_string          ( int type );
  static const char* role_type_string          ( int type );
  static const char* resource_type_string      ( int type );
  static const char* result_type_string        ( int type );
  static int         school_type_component     ( int s_type, int c_type );
  static const char* school_type_string        ( int type );
  static const char* armor_type_string         ( player_type ptype, int slot_type );
  static const char* set_bonus_string          ( set_type type );
  static const char* slot_type_string          ( int type );
  static const char* stat_type_string          ( int type );
  static const char* stat_type_abbrev          ( int type );
  static const char* stat_type_wowhead         ( int type );
  static int         talent_tree               ( int tree, player_type ptype );
  static const char* talent_tree_string        ( int tree, bool armory_format = true );
  static const char* weapon_type_string        ( int type );
  static const char* weapon_class_string       ( int class_ );
  static const char* weapon_subclass_string    ( int subclass );
  static const char* set_item_type_string      ( int item_set );
  static const char* item_quality_string       ( int item_quality );

  static int parse_attribute_type              ( const std::string& name );
  static int parse_dmg_type                    ( const std::string& name );
  static int parse_stim_type                  ( const std::string& name );
  static int parse_gem_type                    ( const std::string& name );
  static int parse_meta_gem_type               ( const std::string& name );
  static player_type parse_player_type         ( const std::string& name );
  static pet_type_t parse_pet_type             ( const std::string& name );
  static int parse_profession_type             ( const std::string& name );
  static position_type parse_position_type     ( const std::string& name );
  static race_type parse_race_type             ( const std::string& name );
  static role_type parse_role_type             ( const std::string& name );
  static int parse_resource_type               ( const std::string& name );
  static int parse_result_type                 ( const std::string& name );
  static school_type parse_school_type         ( const std::string& name );
  static set_type parse_set_bonus              ( const std::string& name );
  static int parse_slot_type                   ( const std::string& name );
  static stat_type parse_stat_type             ( const std::string& name );
  static stat_type parse_reforge_type          ( const std::string& name );
  static int parse_talent_tree                 ( const std::string& name );
  static int parse_weapon_type                 ( const std::string& name );
  static int parse_item_quality                ( const std::string& quality );

  static bool parse_origin( std::string& region, std::string& server, std::string& name, const std::string& origin );

  static int class_id_mask( int type );
  static int class_id( int type );
  static unsigned race_mask( int type );
  static unsigned race_id( int type );
  static unsigned pet_mask( int type );
  static unsigned pet_id( int type );
  static player_type pet_class_type( int type );

  static const char* class_id_string( int type );
  static int translate_class_id( int cid );
  static int translate_class_str( const std::string& s );
  static race_type translate_race_id( int rid );
  static stat_type translate_item_mod( int stat_mod );
  static slot_type translate_invtype( int inv_type );
  static weapon_type translate_weapon_subclass( item_subclass_weapon weapon_subclass );
  static profession_type translate_profession_id( int skill_id );

  static bool socket_gem_match( int socket, int gem );

  static int string_split( std::vector<std::string>& results, const std::string& str, const char* delim, bool allow_quotes = false )
  { string_split_( results, str, delim, allow_quotes ); return static_cast<int>( results.size() ); }
  static int string_split( const std::string& str, const char* delim, const char* format, ... );
  static void string_strip_quotes( std::string& str );
  static std::string& replace_all( std::string& s, const char* from, char to )
  { replace_all_( s, from, to ); return s; }
  static std::string& replace_all( std::string& s, char from, const char* to )
  { replace_all_( s, from, to ); return s; }

  template <typename T>
  static std::string to_string( const T& t )
  { std::ostringstream s; s << t; return s.str(); }

  static std::string to_string( double f );
  static std::string to_string( double f, int precision );

  static int64_t milliseconds();
  static int64_t parse_date( const std::string& month_day_year );

  static int printf( const char *format, ... ) PRINTF_ATTRIBUTE( 1,2 );
  static int fprintf( FILE *stream, const char *format, ... ) PRINTF_ATTRIBUTE( 2,3 );
  static int vfprintf( FILE *stream, const char *format, va_list fmtargs ) PRINTF_ATTRIBUTE( 2,0 )
  { return vfprintf_helper( stream, format, fmtargs ); }
  static int vprintf( const char *format, va_list fmtargs ) PRINTF_ATTRIBUTE( 1,0 )
  { return vfprintf( stdout, format, fmtargs ); }

  static std::string& str_to_utf8( std::string& str ) { str_to_utf8_( str ); return str; }
  static std::string& str_to_latin1( std::string& str ) { str_to_latin1_( str ); return str; }
  static std::string& urlencode( std::string& str ) { urlencode_( str ); return str; }
  static std::string& urldecode( std::string& str ) { urldecode_( str ); return str; }

  static std::string& format_text( std::string& name, bool input_is_utf8 )
  { format_text_( name, input_is_utf8 ); return name; }

  static std::string& html_special_char_decode( std::string& str )
  { html_special_char_decode_( str ); return str; }

  static bool str_compare_ci( const std::string& l, const std::string& r );
  static bool str_in_str_ci ( const std::string& l, const std::string& r );
  static bool str_prefix_ci ( const std::string& str, const std::string& prefix );

  static double floor( double X, unsigned int decplaces = 0 );
  static double ceil( double X, unsigned int decplaces = 0 );
  static double round( double X, unsigned int decplaces = 0 );

  static std::string& tolower( std::string& str ) { tolower_( str ); return str; }

  static int snprintf( char* buf, size_t size, const char* fmt, ... ) PRINTF_ATTRIBUTE( 3,4 );

  static int32_t DoubleToInt( double d ) SC_FINLINE_EXT;
  static int32_t FloorToInt ( double d ) SC_FINLINE_EXT;
  static int32_t CeilToInt  ( double d ) SC_FINLINE_EXT;
  static int32_t RoundToInt ( double d ) SC_FINLINE_EXT;
  static int32_t CRoundToInt( double d ) SC_FINLINE_EXT;

  template <typename T, std::size_t N>
  static std::vector<T> array_to_vector( const T (&array)[N] )
  { return std::vector<T>( array, array + N ); }
};

finline int32_t util_t::DoubleToInt( double d )
{
  union Cast
  {
    double d;
    int32_t l;
  };
  volatile Cast c;
  c.d = d + 6755399441055744.0;
  return c.l;
}

finline int32_t util_t::FloorToInt( double d )
{
  return xs_FloorToInt( d );
}

finline int32_t util_t::CeilToInt( double d )
{
  return xs_CeilToInt( d );
}

finline int32_t util_t::RoundToInt( double d )
{
  return xs_RoundToInt( d );
}

finline int32_t util_t::CRoundToInt( double d )
{
  return xs_CRoundToInt( d );
}

// Spell information struct, holding static functions to output spell data in a human readable form

/*
struct spell_info_t
{
  static std::string to_str( sim_t* sim, const spell_data_t* spell );
  static std::string to_str( sim_t* sim, uint32_t spell_id );
  static std::string talent_to_str( sim_t* sim, const talent_data_t* talent );
  static std::ostringstream& effect_to_str( sim_t* sim, const spell_data_t* spell, const spelleffect_data_t* effect, std::ostringstream& s );
};
*/

// Spell ID class

enum s_type_t
{
  T_SPELL = 0,
  T_TALENT,
  T_MASTERY,
  T_GLYPH,
  T_CLASS,
  T_RACE,
  T_SPEC,
  T_ITEM
};

struct spell_id_t
{
  s_type_t                   s_type;
  uint32_t                   s_id;
  const spell_data_t*        s_data;
  bool                       s_enabled;
  player_t*                  s_player;
  bool                       s_overridden;      // Spell enable/disable status was manually overridden
  std::string                s_token;
  const talent_t*            s_required_talent;
  const spelleffect_data_t*  s_effects[ MAX_EFFECTS ];
  const spelleffect_data_t*  s_single;
  int                        s_tree;

  std::list<spell_id_t*>* s_list;
  std::list<spell_id_t*>::iterator s_list_iter;
  void queue();

  // Construction & deconstruction
  // spell_id_t( const spell_id_t& copy ) = default;
  spell_id_t( player_t* player = 0, const char* t_name = 0 );
  spell_id_t( player_t* player, const char* t_name, const uint32_t id, talent_t* talent = 0 );
  spell_id_t( player_t* player, const char* t_name, const char* s_name, talent_t* talent = 0 );
  virtual ~spell_id_t();

  // Generic spell data initialization
  bool initialize( const char* s_name = 0 );
  virtual bool enable( bool override_value );

  // Spell data object validity check
  virtual bool ok() const;

  // Spell state output
  std::string to_str() const;

  // Access methods
  virtual uint32_t spell_id() const { return ( s_player && s_data ) ? s_id : 0; }
  virtual const char* real_name() const;
  virtual const std::string token() const;
  virtual double missile_speed() const;
  virtual uint32_t school_mask() const;
  virtual school_type get_school_type() const;
  virtual resource_type power_type() const;
  virtual double min_range() const;
  virtual double max_range() const;
  virtual double extra_coeff() const;
  virtual bool in_range() const;
  virtual timespan_t cooldown() const;
  virtual timespan_t gcd() const;
  virtual uint32_t category() const;
  virtual timespan_t duration() const;
  virtual double cost() const;
  virtual uint32_t rune_cost() const;
  virtual double runic_power_gain() const;
  virtual uint32_t max_stacks() const;
  virtual uint32_t initial_stacks() const;
  virtual double proc_chance() const;
  virtual timespan_t cast_time() const;
  virtual uint32_t effect_id( uint32_t effect_num ) const;
  virtual bool flags( spell_attribute_t f ) const;
  virtual const char* desc() const;
  virtual const char* tooltip() const;
  virtual int32_t effect_type( uint32_t effect_num ) const;
  virtual int32_t effect_subtype( uint32_t effect_num ) const;
  virtual int32_t effect_base_value( uint32_t effect_num ) const;
  virtual int32_t effect_misc_value1( uint32_t effect_num ) const;
  virtual int32_t effect_misc_value2( uint32_t effect_num ) const;
  virtual uint32_t effect_trigger_spell( uint32_t effect_num ) const;
  virtual double effect_chain_multiplier( uint32_t effect_num ) const;
  virtual double effect_average( uint32_t effect_num ) const;
  virtual double effect_delta( uint32_t effect_num ) const;
  virtual double effect_bonus( uint32_t effect_num ) const;
  virtual double effect_min( uint32_t effect_num ) const;
  virtual double effect_max( uint32_t effect_num ) const;
  virtual double effect_coeff( uint32_t effect_num ) const;
  virtual timespan_t effect_period( uint32_t effect_num ) const;
  virtual double effect_radius( uint32_t effect_num ) const;
  virtual double effect_radius_max( uint32_t effect_num ) const;
  virtual double effect_pp_combo_points( uint32_t effect_num ) const;
  virtual double effect_real_ppl( uint32_t effect_num ) const;
  virtual int    effect_die_sides( uint32_t effect_num ) const;

  // Generalized access API
  virtual double base_value( effect_type_t type = E_MAX, effect_subtype_t sub_type = A_MAX, int misc_value = DEFAULT_MISC_VALUE, int misc_value2 = DEFAULT_MISC_VALUE ) const;

  // Return an additive modifier from the spell (E_APPLY_AURA -> A_ADD_PCT_MODIFIER|A_ADD_FLAT_MODIFIER ),
  // based on the property of the modifier (as defined by misc_value)
  virtual double mod_additive( property_type_t = P_MAX ) const;
  timespan_t mod_additive_time( property_type_t = P_MAX ) const;
  const spelleffect_data_t* get_effect( property_type_t p_type ) const;

  // Spell data specific static methods
  static uint32_t get_school_mask( const school_type s );
  static school_type get_school_type( const uint32_t mask );
  static bool is_school( const school_type s, const school_type s2 );

  const spell_data_t& spell() const { return ( ok() ? *s_data : *spell_data_t::nil() ); }
  const spelleffect_data_t& effect1() const { return ( ok() ? s_data -> effect1(): *spelleffect_data_t::nil() ); }
  const spelleffect_data_t& effect2() const { return ( ok() ? s_data -> effect2(): *spelleffect_data_t::nil() ); }
  const spelleffect_data_t& effect3() const { return ( ok() ? s_data -> effect3(): *spelleffect_data_t::nil() ); }
private:
};

// Talent class
struct talent_t
{
  const char * _name;        // Talent name
  unsigned     _tab_page;    // Talent tab page
  //unsigned     _col;         // Talent column
  //unsigned     _row;         // Talent row
  unsigned     _max_rank;

  unsigned t_rank;

  talent_t( player_t* p, const char*, unsigned, unsigned );
  virtual ~talent_t();

  virtual bool set_rank( uint32_t value );
  virtual bool ok() const;

  virtual uint32_t rank() const;

  virtual uint32_t max_rank() const;
  const char*          name_cstr() const { return _name; }
};

/*
struct talent_t : spell_id_t
{
  const talent_data_t* t_data;
  unsigned             t_rank;
  bool                 t_enabled;
  bool                 t_overridden;
  const spell_id_t*    t_rank_spells[ MAX_RANK ];
  const spell_id_t*    t_default_rank;

  talent_t( player_t* p, talent_data_t* td );
  virtual ~talent_t();
  virtual bool set_rank( uint32_t value, bool overridden = false );
  virtual bool ok() const;
  std::string to_str() const;
  virtual uint32_t rank() const;

  virtual uint32_t spell_id() const;
  virtual uint32_t max_rank() const;
  virtual uint32_t rank_spell_id( const uint32_t r ) const;
  virtual const spell_id_t* rank_spell( uint32_t r = 0 ) const;

  // Future trimmed down access
  talent_data_t* td;
  spell_data_t* sd;
  spell_data_t* trigger;
  // unsigned rank;

  talent_data_t& data() { return *td; }
  spell_data_t& spell( unsigned r=0 )
  {
    if ( r >= 1 && r - 1 < MAX_RANK )
      return td -> spell( r - 1 );
    return *sd;
  }
  const spelleffect_data_t& effect1() const { return sd -> effect1(); }
  const spelleffect_data_t& effect2() const { return sd -> effect2(); }
  const spelleffect_data_t& effect3() const { return sd -> effect3(); }
}; */

// Glyph

struct glyph_t : public spell_id_t
{
  // Future trimmed down access
  spell_data_t* sd;
  spell_data_t* sd_enabled;

  glyph_t( player_t* p, spell_data_t* sd );

  virtual bool enable( bool override_value = true );
  bool enabled() const { return ( sd == sd_enabled ); }

  const spell_data_t& spell() const { return *sd_enabled; }
  const spelleffect_data_t& effect1() const { return sd_enabled -> effect1(); }
  const spelleffect_data_t& effect2() const { return sd_enabled -> effect2(); }
  const spelleffect_data_t& effect3() const { return sd_enabled -> effect3(); }
};

// Mastery

struct mastery_t : public spell_id_t
{
  int m_tree;

  mastery_t( player_t* player = 0, const char* t_name = 0 ) : spell_id_t( player, t_name ) { }
  mastery_t( player_t* player, const char* t_name, const uint32_t id, int tree );
  mastery_t( player_t* player, const char* t_name, const char* s_name, int tree );

  std::string to_str() const;

  virtual bool ok() const;
  virtual double base_value( effect_type_t type = E_MAX,
                             effect_subtype_t sub_type = A_MAX,
                             int misc_value = DEFAULT_MISC_VALUE,
                             int misc_value2 = DEFAULT_MISC_VALUE ) const;
};

// Raid Event

struct raid_event_t
{
  sim_t* sim;
  std::string name_str;
  int64_t num_starts;
  timespan_t first, last;
  timespan_t cooldown;
  timespan_t cooldown_stddev;
  timespan_t cooldown_min;
  timespan_t cooldown_max;
  timespan_t duration;
  timespan_t duration_stddev;
  timespan_t duration_min;
  timespan_t duration_max;
  double     distance_min;
  double     distance_max;
  timespan_t saved_duration;
  rng_t* rng;
  std::vector<player_t*> affected_players;

  raid_event_t( sim_t*, const char* name );
  virtual ~raid_event_t() {}

  virtual timespan_t cooldown_time() const;
  virtual timespan_t duration_time() const;
  virtual void schedule();
  virtual void reset();
  virtual void start();
  virtual void finish();
  virtual void parse_options( option_t*, const std::string& options_str );
  virtual const char* name() const { return name_str.c_str(); }
  static raid_event_t* create( sim_t* sim, const std::string& name, const std::string& options_str );
  static void init( sim_t* );
  static void reset( sim_t* );
  static void combat_begin( sim_t* );
  static void combat_end( sim_t* ) {}
};

// Gear Stats ===============================================================

namespace internal {
struct gear_stats_t
{
  double attribute[ ATTRIBUTE_MAX ];
  double resource[ RESOURCE_MAX ];
  double expertise_rating;
  double expertise_rating2;
  double hit_rating;
  double hit_rating2;
  double crit_rating;
  double alacrity_rating;
  double weapon_dps;
  double weapon_speed;
  double weapon_offhand_dps;
  double weapon_offhand_speed;
  double armor;
  double bonus_armor;
  double dodge_rating;
  double parry_rating;
  double block_rating;
  double power;
  double force_power;
  double tech_power;
  double surge_rating;
};
}

struct gear_stats_t : public internal::gear_stats_t
{
  typedef internal::gear_stats_t base_t;
  gear_stats_t() : base_t( base_t() ) {}

  void   add_stat( int stat, double value );
  void   set_stat( int stat, double value );
  double get_stat( int stat ) const;
  void   print( FILE* );
  static double stat_mod( int stat );
};


// Statistical Sample Data

struct sample_data_t
{
  std::vector<double> data;
  // Analyzed Results
  double sum;
  double mean;
  double min;
  double max;
  double variance;
  double std_dev;
  double mean_std_dev;
  std::vector<int> distribution;
  const bool simple;
  const bool min_max;

private:
  int count;

  bool analyzed_basics;
  bool analyzed_variance;
  bool created_dist;
  bool is_sorted;
public:

  sample_data_t( bool s=true, bool mm=false );

  void reserve( std::size_t capacity )
  { if ( ! simple ) data.reserve( capacity ); }

  void add( double x=0 );

  bool basics_analyzed() const { return analyzed_basics; }
  bool variance_analyzed() const { return analyzed_variance; }
  bool distribution_created() const { return created_dist; }
  bool sorted() const { return is_sorted; }
  int size() const { if ( simple ) return count; return ( int ) data.size(); }

  void analyze(
    bool calc_basics=true,
    bool calc_variance=true,
    bool s=true,
    unsigned int create_dist=0 );


  void analyze_basics();

  void analyze_variance();

  void sort();

  void create_distribution( unsigned int num_buckets=50 );

  double percentile( double );

  void merge( const sample_data_t& );

  void clear() { count = 0; sum = 0; data.clear(); distribution.clear(); }

  static double pearson_correlation( const sample_data_t&, const sample_data_t& );
};

// Buffs ====================================================================

struct buff_t : public spell_id_t
{
  double current_value, react;
  timespan_t buff_duration, buff_cooldown;
  double default_chance;
  timespan_t last_start;
  timespan_t last_trigger;
  timespan_t start_intervals_sum;
  timespan_t trigger_intervals_sum;
  timespan_t iteration_uptime_sum;
  int64_t up_count, down_count, start_intervals, trigger_intervals, start_count, refresh_count;
  int64_t trigger_attempts, trigger_successes;
  double benefit_pct, trigger_pct, avg_start_interval, avg_trigger_interval, avg_start, avg_refresh;
  std::string name_str;
  std::vector<timespan_t> stack_occurrence, stack_react_time;
  std::vector<buff_uptime_t> stack_uptime;
  sim_t* sim;
  player_t* player;
  player_t* source;
  player_t* initial_source;
  event_t* expiration;
  event_t* delay;
  rng_t* rng;
  cooldown_t* cooldown;
  buff_t* next;
  int current_stack, max_stack;
  int aura_id;
  int rng_type;
  bool activated;
  bool reverse, constant, quiet, overridden;
  sample_data_t uptime_pct;

  buff_t() : sim( 0 ) {}
  virtual ~buff_t();

  // Raid Aura
  buff_t( sim_t*, const std::string& name,
          int max_stack=1, timespan_t buff_duration=timespan_t::zero, timespan_t buff_cooldown=timespan_t::zero,
          double chance=1.0, bool quiet=false, bool reverse=false, int rng_type=RNG_CYCLIC, int aura_id=0 );

  // Player Buff
  buff_t( actor_pair_t pair, const std::string& name,
          int max_stack=1, timespan_t buff_duration=timespan_t::zero, timespan_t buff_cooldown=timespan_t::zero,
          double chance=1.0, bool quiet=false, bool reverse=false, int rng_type=RNG_CYCLIC, int aura_id=0, bool activated=true );

  // Player Buff with extracted data
private:
  void init_from_spell_( player_t*, spell_data_t* );
public:
  buff_t( actor_pair_t pair, spell_data_t*, ... );

  // Player Buff as spell_id_t by name
  buff_t( actor_pair_t pair, const std::string& name, const char* sname,
          double chance=-1, timespan_t cd=timespan_t::min,
          bool quiet=false, bool reverse=false, int rng_type=RNG_CYCLIC, bool activated=true );

  // Player Buff as spell_id_t by id
  buff_t( actor_pair_t pair, const uint32_t id, const std::string& name,
          double chance=-1, timespan_t cd=timespan_t::min,
          bool quiet=false, bool reverse=false, int rng_type=RNG_CYCLIC, bool activated=true );

  // Use check() inside of ready() methods to prevent skewing of "benefit" calculations.
  // Use up() where the presence of the buff affects the action mechanics.

  int    check() { return current_stack; }
  inline bool   up()    { if ( current_stack > 0 ) { up_count++; } else { down_count++; } return current_stack > 0; }
  inline int    stack() { if ( current_stack > 0 ) { up_count++; } else { down_count++; } return current_stack; }
  inline double value() { if ( current_stack > 0 ) { up_count++; } else { down_count++; } return current_value; }
  timespan_t remains();
  bool   remains_gt( timespan_t time );
  bool   remains_lt( timespan_t time );
  bool   trigger  ( action_t*, int stacks=1, double value=-1.0 );
  virtual bool   trigger  ( int stacks=1, double value=-1.0, double chance=-1.0 );
  virtual void   execute ( int stacks=1, double value=-1.0 );
  virtual void   increment( int stacks=1, double value=-1.0 );
  void   decrement( int stacks=1, double value=-1.0 );
  void   extend_duration( player_t* p, timespan_t seconds );
  void   start_expiration( timespan_t );

  virtual void start    ( int stacks=1, double value=-1.0 );
  virtual void refresh  ( int stacks=0, double value=-1.0 );
  virtual void bump     ( int stacks=1, double value=-1.0 );
  virtual void override ( int stacks=1, double value=-1.0 );
  virtual bool may_react( int stacks=1 );
  virtual int stack_react();
  virtual void expire();
  virtual void predict();
  virtual void reset();
  virtual void aura_gain();
  virtual void aura_loss();
  virtual void merge( const buff_t* other_buff );
  virtual void analyze();
  void init_buff_shared();
  void init();
  void init_buff_t_();
  virtual void parse_options( va_list vap );
  virtual void combat_begin();
  virtual void combat_end();

  const char* name() { return name_str.c_str(); }

  action_expr_t* create_expression( action_t*, const std::string& type );
  std::string    to_str() const;

  static buff_t* find(   buff_t*, const std::string& name );
  static buff_t* find(    sim_t*, const std::string& name );
  static buff_t* find( player_t*, const std::string& name );

  const spelleffect_data_t& effect1() const { return s_data -> effect1(); }
  const spelleffect_data_t& effect2() const { return s_data -> effect2(); }
  const spelleffect_data_t& effect3() const { return s_data -> effect3(); }
};

struct stat_buff_t : public buff_t
{
  double amount;
  int stat;

  stat_buff_t( player_t*, const std::string& name,
               int stat, double amount,
               int max_stack=1, timespan_t buff_duration=timespan_t::zero, timespan_t buff_cooldown=timespan_t::zero,
               double chance=1.0, bool quiet=false, bool reverse=false, int rng_type=RNG_CYCLIC, int aura_id=0, bool activated=true );
  stat_buff_t( player_t*, const uint32_t id, const std::string& name,
               int stat, double amount,
               double chance=1.0, timespan_t buff_cooldown=timespan_t::min, bool quiet=false, bool reverse=false, int rng_type=RNG_CYCLIC, bool activated=true );

  virtual void bump     ( int stacks=1, double value=-1.0 );
  virtual void decrement( int stacks=1, double value=-1.0 );
  virtual void expire();
};

struct cost_reduction_buff_t : public buff_t
{
  double amount;
  int school;
  bool refreshes;

  cost_reduction_buff_t( player_t*, const std::string& name,
                         int school, double amount,
                         int max_stack=1, timespan_t buff_duration=timespan_t::zero, timespan_t buff_cooldown=timespan_t::zero,
                         double chance=1.0, bool refreshes=false, bool quiet=false, bool reverse=false, int rng_type=RNG_CYCLIC, int aura_id=0, bool activated=true );
  cost_reduction_buff_t( player_t*, const uint32_t id, const std::string& name,
                         int school, double amount,
                         double chance=1.0, timespan_t buff_cooldown=timespan_t::min, bool refreshes=false, bool quiet=false, bool reverse=false, int rng_type=RNG_CYCLIC, bool activated=true );

  virtual void bump     ( int stacks=1, double value=-1.0 );
  virtual void decrement( int stacks=1, double value=-1.0 );
  virtual void expire();
  virtual void refresh  ( int stacks=0, double value=-1.0 );
};

struct debuff_t : public buff_t
{
  // Player De-Buff
  debuff_t( player_t*, const std::string& name,
            int max_stack=1, timespan_t buff_duration=timespan_t::zero, timespan_t buff_cooldown=timespan_t::zero,
            double chance=1.0, bool quiet=false, bool reverse=false, int rng_type=RNG_CYCLIC, int aura_id=0 );

  // Player De-Buff as spell_id_t by id
  debuff_t( player_t*, const uint32_t id, const std::string& name,
            double chance=-1, timespan_t duration=timespan_t::min,
            bool quiet=false, bool reverse=false, int rng_type=RNG_CYCLIC );

};

typedef struct buff_t aura_t;

// Expressions ==============================================================

enum token_type_t
{
  TOK_UNKNOWN=0,
  TOK_PLUS,
  TOK_MINUS,
  TOK_MULT,
  TOK_DIV,
  TOK_ADD,
  TOK_SUB,
  TOK_AND,
  TOK_OR,
  TOK_NOT,
  TOK_EQ,
  TOK_NOTEQ,
  TOK_LT,
  TOK_LTEQ,
  TOK_GT,
  TOK_GTEQ,
  TOK_LPAR,
  TOK_RPAR,
  TOK_IN,
  TOK_NOTIN,
  TOK_NUM,
  TOK_STR,
  TOK_ABS,
  TOK_SPELL_LIST,
  TOK_FLOOR,
  TOK_CEIL
};

struct expr_token_t
{
  int type;
  std::string label;
};

enum expr_data_type_t
{
  DATA_SPELL = 0,
  DATA_TALENT,
  DATA_EFFECT,
  DATA_TALENT_SPELL,
  DATA_CLASS_SPELL,
  DATA_RACIAL_SPELL,
  DATA_MASTERY_SPELL,
  DATA_SPECIALIZATION_SPELL,
  DATA_GLYPH_SPELL,
  DATA_SET_BONUS_SPELL
};

struct expression_t
{
  static int precedence( int token_type );
  static int is_unary( int token_type );
  static int is_binary( int token_type );
  static int next_token( action_t* action, const std::string& expr_str, int& current_index, std::string& token_str, token_type_t prev_token );
  static void parse_tokens( action_t* action, std::vector<expr_token_t>& tokens, const std::string& expr_str );
  static void print_tokens( std::vector<expr_token_t>& tokens, sim_t* sim );
  static void convert_to_unary( action_t* action, std::vector<expr_token_t>& tokens );
  static bool convert_to_rpn( action_t* action, std::vector<expr_token_t>& tokens );
};

struct action_expr_t
{
  action_t* action;
  std::string name_str;

  int result_type;
  double result_num;
  std::string result_str;

  action_expr_t( action_t* a, const std::string& n, int t=TOK_UNKNOWN ) : action( a ), name_str( n ), result_type( t ), result_num( 0 ) {}
  action_expr_t( action_t* a, const std::string& n, double       constant_value ) : action( a ), name_str( n ) { result_type = TOK_NUM; result_num = constant_value; }
  action_expr_t( action_t* a, const std::string& n, std::string& constant_value ) : action( a ), name_str( n ) { result_type = TOK_STR; result_str = constant_value; }
  virtual ~action_expr_t() {}
  virtual int evaluate() { return result_type; }
  virtual const char* name() { return name_str.c_str(); }
  virtual bool success() { return ( evaluate() == TOK_NUM ) && ( result_num != 0 ); }

  static action_expr_t* parse( action_t*, const std::string& expr_str );
};


struct spell_data_expr_t
{
  std::string name_str;
  sim_t* sim;
  expr_data_type_t data_type;
  bool effect_query;

  int result_type;
  double result_num;
  std::vector<uint32_t> result_spell_list;
  std::string result_str;

  spell_data_expr_t( sim_t* sim, const std::string& n, expr_data_type_t dt = DATA_SPELL, bool eq = false, int t=TOK_UNKNOWN ) : name_str( n ), sim( sim ), data_type( dt ), effect_query( eq ), result_type( t ), result_num( 0 ), result_spell_list() {}
  spell_data_expr_t( sim_t* sim, const std::string& n, double       constant_value ) : name_str( n ), sim( sim ), data_type( DATA_SPELL ) { result_type = TOK_NUM; result_num = constant_value; }
  spell_data_expr_t( sim_t* sim, const std::string& n, std::string& constant_value ) : name_str( n ), sim( sim ), data_type( DATA_SPELL ) { result_type = TOK_STR; result_str = constant_value; }
  spell_data_expr_t( sim_t* sim, const std::string& n, std::vector<uint32_t>& constant_value ) : name_str( n ), sim( sim ), data_type( DATA_SPELL ) { result_type = TOK_SPELL_LIST; result_spell_list = constant_value; }
  virtual ~spell_data_expr_t() {}
  virtual int evaluate() { return result_type; }
  virtual const char* name() { return name_str.c_str(); }

  virtual std::vector<uint32_t> operator|( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator&( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator-( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }

  virtual std::vector<uint32_t> operator<( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator>( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator<=( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator>=( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator==( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> operator!=( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }

  virtual std::vector<uint32_t> in( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }
  virtual std::vector<uint32_t> not_in( const spell_data_expr_t& /* other */ ) { return std::vector<uint32_t>(); }

  static spell_data_expr_t* parse( sim_t* sim, const std::string& expr_str );
  static spell_data_expr_t* create_spell_expression( sim_t* sim, const std::string& name_str );
};

namespace thread_impl { // ===================================================

#if defined( NO_THREADS )

class mutex : public nonmoveable
{
public:
  void lock() {}
  void unlock() {}
};

class thread : public noncopyable
{
public:
  virtual void run() = 0;

  void launch() { run(); } // Run sequentially in foreground.
  void wait() {}

  static void sleep( int seconds );
};

#elif defined( __MINGW32__ ) || defined( _MSC_VER )

class mutex : public nonmoveable
{
  CRITICAL_SECTION cs;
public:
  mutex() { InitializeCriticalSection( &cs ); }
  ~mutex() { DeleteCriticalSection( &cs ); }
  void lock() { EnterCriticalSection( &cs ); }
  void unlock() { LeaveCriticalSection( &cs ); }
};

class thread : public noncopyable
{
private:
  HANDLE handle;
public:
  virtual void run() = 0;

  void launch();
  void wait();

  static void sleep( int seconds ) { Sleep( seconds * 1000 ); }
};

#else // POSIX

class mutex : public nonmoveable
{
  pthread_mutex_t m;
public:
  mutex() { pthread_mutex_init( &m, NULL ); }
  ~mutex() { pthread_mutex_destroy( &m ); }
  void lock() { pthread_mutex_lock( &m ); }
  void unlock() { pthread_mutex_unlock( &m ); }
};

class thread : public noncopyable
{
  pthread_t t;
public:
  virtual void run() = 0;

  void launch();
  void wait() { pthread_join( t, NULL ); }

  static void sleep( int seconds ) { ::sleep( seconds ); }
};

#endif

} // namespace thread_impl ===================================================

typedef thread_impl::mutex mutex_t;

class thread_t : public thread_impl::thread
{
protected:
  thread_t() {}
  virtual ~thread_t() {}
public:
  static void init() {}
  static void de_init() {}
};

class auto_lock_t
{
private:
  mutex_t& mutex;
public:
  auto_lock_t( mutex_t& mutex_ ) : mutex( mutex_ ) { mutex.lock(); }
  ~auto_lock_t() { mutex.unlock(); }
};


// Simple freelist allocator for events =====================================

class event_freelist_t
{
private:
  struct free_event_t { free_event_t* next; };
  free_event_t* list;

public:
  event_freelist_t() : list( 0 ) {}
  ~event_freelist_t();

  void* allocate( std::size_t );
  void deallocate( void* );
};


// Simulation Engine ========================================================

#define REGISTER_DOT(n, q) sim->register_targetdata_item(DATA_DOT, #q, t, nonpod_offsetof(type, dots_##n))
#define REGISTER_BUFF(n) sim->register_targetdata_item(DATA_AURA, #n, t, nonpod_offsetof(type, buffs_##n))
#define REGISTER_DEBUFF(n) sim->register_targetdata_item(DATA_AURA, #n, t, nonpod_offsetof(type, debuffs_##n))

struct sim_t : private thread_t
{
  int         argc;
  char**      argv;
  sim_t*      parent;
  event_freelist_t free_list;
  player_t*   target;
  player_t*   target_list;
  player_t*   player_list;
  player_t*   active_player;
  int         num_players;
  int         num_enemies;
  int         num_targetdata_ids;
  int         max_player_level;
  int         canceled;
  timespan_t  queue_lag, queue_lag_stddev;
  timespan_t  gcd_lag, gcd_lag_stddev;
  timespan_t  channel_lag, channel_lag_stddev;
  timespan_t  queue_gcd_reduction;
  int         strict_gcd_queue;
  double      confidence;
  double      confidence_estimator;
  // Latency
  timespan_t  world_lag, world_lag_stddev;
  double      travel_variance, default_skill;
  timespan_t  reaction_time, regen_periodicity;
  timespan_t  current_time, max_time, expected_time;
  double      vary_combat_length;
  timespan_t  last_event;
  int         fixed_time;
  int64_t     events_remaining, max_events_remaining;
  int64_t     events_processed, total_events_processed;
  int         seed, id, iterations, current_iteration, current_slot;
  int         armor_update_interval, weapon_speed_scale_factors;
  int         optimal_raid, log, debug;
  int         save_profiles, default_actions;
  int         normalized_stat;
  std::string current_name, default_region_str, default_server_str, save_prefix_str,save_suffix_str;
  int         save_talent_str;
  bool        input_is_utf8;
  std::vector<player_t*> actor_list;
  std::string main_target_str;

  // Target options
  double      target_death_pct;
  int         target_level;
  std::string target_race;
  int         target_adds;

  // Data access
  dbc_t       dbc;

  // Default stat enchants
  gear_stats_t enchant;

  std::map<std::string,std::string> var_map;
  std::vector<option_t> options;
  std::vector<std::string> party_encoding;
  std::vector<std::string> item_db_sources;

  // Random Number Generation
  rng_t* rng;
  rng_t* deterministic_rng;
  rng_t* rng_list;
  int smooth_rng, deterministic_roll, average_range, average_gauss;

  // Timing Wheel Event Management
  event_t** timing_wheel;
  int    wheel_seconds, wheel_size, wheel_mask, timing_slice;
  double wheel_granularity;

  // Raid Events
  std::vector<raid_event_t*> raid_events;
  std::string raid_events_str;
  std::string fight_style;

  // Buffs and Debuffs Overrides
  struct overrides_t
  {
    int bleeding;
    int coordination;// 5% crit. implement as buff, also add equal republic buffs
    int force_valor; // 5% stat bonus. implement sith version
    int shatter_shot; // -20% target armor. implement as buff, also add equal buffs
    int unnatural_might;// 5% damage bonus. implement as buff, also add equal republic buffs
  };
  overrides_t overrides;

  // Auras
  struct auras_t
  {

  };
  auras_t auras;

  // Auras and De-Buffs
  buff_t* buff_list;
  timespan_t aura_delay;

  // Global aura related delay
  timespan_t default_aura_delay;
  timespan_t default_aura_delay_stddev;

  cooldown_t* cooldown_list;

  // Replenishment
  int replenishment_targets;
  std::vector<player_t*> replenishment_candidates;

  // Reporting
  report_t*  report;
  scaling_t* scaling;
  plot_t*    plot;
  reforge_plot_t* reforge_plot;
  timespan_t elapsed_cpu;
  double     iteration_dmg, iteration_heal;
  sample_data_t raid_dps, total_dmg, raid_hps, total_heal, simulation_length;
  int        report_progress;
  std::string reference_player_str;
  std::vector<player_t*> players_by_dps;
  std::vector<player_t*> players_by_hps;
  std::vector<player_t*> players_by_name;
  std::vector<player_t*> targets_by_name;
  std::vector<std::string> id_dictionary;
  std::vector<std::string> dps_charts, hps_charts, gear_charts, dpet_charts;
  std::string downtime_chart;
  std::vector<timespan_t> iteration_timeline;
  std::vector<int> divisor_timeline;
  std::string timeline_chart;
  std::string output_file_str, html_file_str;
  std::string xml_file_str, xml_stylesheet_file_str;
  std::string path_str;
  std::stack<std::string> active_files;
  std::vector<std::string> error_list;
  FILE* output_file;
  int armory_throttle;
  int current_throttle;
  int debug_exp;
  int report_precision;
  int report_pets_separately;
  int report_targets;
  int report_details;
  int report_rng;
  int hosted_html;
  int print_styles;
  int report_overheal;
  int save_raid_summary;
  int statistics_level;
  int separate_stats_by_actions;

  std::unordered_map<std::string, std::pair<player_type, size_t> > targetdata_items[DATA_COUNT];
  std::vector<std::pair<size_t, std::string> > targetdata_dots[PLAYER_MAX];

  // Multi-Threading
  int threads;
  std::vector<sim_t*> children;
  int thread_index;
  virtual void run() { iterate(); }

  // Spell database access
  spell_data_expr_t* spell_query;

  sim_t( sim_t* parent=0, int thrdID=0 );
  virtual ~sim_t();

  int       main( int argc, char** argv );
  void      cancel();
  double    progress( std::string& phase );
  void      combat( int iteration );
  void      combat_begin();
  void      combat_end();
  void      add_event( event_t*, timespan_t delta_time );
  void      reschedule_event( event_t* );
  void      flush_events();
  void      cancel_events( player_t* );
  event_t*  next_event();
  void      reset();
  bool      init();
  void      analyze_player( player_t* p );
  void      analyze();
  void      merge( sim_t& other_sim );
  void      merge();
  bool      iterate();
  void      partition();
  bool      execute();
  void      print_options();
  void      create_options();
  bool      parse_option( const std::string& name, const std::string& value );
  bool      parse_options( int argc, char** argv );
  bool      time_to_think( timespan_t proc_time );
  timespan_t total_reaction_time ();
  int       roll( double chance );
  double    range( double min, double max );
  double    gauss( double mean, double stddev );
  timespan_t gauss( timespan_t mean, timespan_t stddev );
  double    real();
  rng_t*    get_rng( const std::string& name, int type=RNG_DEFAULT );
  double    iteration_adjust();
  player_t* find_player( const std::string& name );
  player_t* find_player( int index );
  cooldown_t* get_cooldown( const std::string& name );
  void      use_optimal_buffs_and_debuffs( int value );
  void      aura_gain( const char* name, int aura_id=0 );
  void      aura_loss( const char* name, int aura_id=0 );
  action_expr_t* create_expression( action_t*, const std::string& name );
  int       errorf( const char* format, ... ) PRINTF_ATTRIBUTE( 2,3 );
  void register_targetdata_item( int kind, const char* name, player_type type, size_t offset );
  void* get_targetdata_item( player_t* source, player_t* target, int kind, const std::string& name );

  dot_t* get_targetdata_dot( player_t* source, player_t* target, const std::string& name )
  {
    return ( dot_t* )get_targetdata_item( source, target, DATA_DOT, name );
  }

  buff_t* get_targetdata_aura( player_t* source, player_t* target, const std::string& name )
  {
    return ( buff_t* )get_targetdata_item( source, target, DATA_AURA, name );
  }
};

// Scaling ==================================================================

struct scaling_t
{
  sim_t* sim;
  sim_t* baseline_sim;
  sim_t* ref_sim;
  sim_t* delta_sim;
  sim_t* ref_sim2;
  sim_t* delta_sim2;
  int    scale_stat;
  double scale_value;
  double scale_delta_multiplier;
  int    calculate_scale_factors;
  int    center_scale_delta;
  int    positive_scale_delta;
  int    scale_lag;
  double scale_factor_noise;
  int    normalize_scale_factors;
  int    smooth_scale_factors;
  int    debug_scale_factors;
  std::string scale_only_str;
  int    current_scaling_stat, num_scaling_stats, remaining_scaling_stats;
  double    scale_alacrity_iterations, scale_expertise_iterations, scale_crit_iterations, scale_hit_iterations;
  std::string scale_over;
  std::string scale_over_player;

  // Gear delta for determining scale factors
  gear_stats_t stats;

  scaling_t( sim_t* s );

  void init_deltas();
  void analyze();
  void analyze_stats();
  void analyze_lag();
  void analyze_gear_weights();
  void normalize();
  void derive();
  double progress( std::string& phase );
  void create_options();
  bool has_scale_factors();
  double scale_over_function( sim_t* s, player_t* p );
  double scale_over_function_error( sim_t* s, player_t* p );
};

// Plot =====================================================================

struct plot_t
{
  sim_t* sim;
  std::string dps_plot_stat_str;
  double dps_plot_step;
  int    dps_plot_points;
  int    dps_plot_iterations;
  int    dps_plot_debug;
  int    current_plot_stat, num_plot_stats, remaining_plot_stats;
  int    remaining_plot_points;
  bool	 dps_plot_positive;

  plot_t( sim_t* s );

  void analyze();
  void analyze_stats();
  double progress( std::string& phase );
  void create_options();
};

// Reforge Plot =============================================================

struct reforge_plot_t
{
  sim_t* sim;
  sim_t* current_reforge_sim;
  std::string reforge_plot_stat_str;
  std::string reforge_plot_output_file_str;
  FILE* reforge_plot_output_file;
  std::vector<int> reforge_plot_stat_indices;
  int    reforge_plot_step;
  int    reforge_plot_amount;
  int    reforge_plot_iterations;
  int    reforge_plot_debug;
  int    current_stat_combo;
  int    num_stat_combos;

  reforge_plot_t( sim_t* s );

  void generate_stat_mods( std::vector<std::vector<int> > &stat_mods,
                           const std::vector<int> &stat_indices,
                           int cur_mod_stat,
                           std::vector<int> cur_stat_mods );
  void analyze();
  void analyze_stats();
  double progress( std::string& phase );
  void create_options();
};

struct reforge_plot_data_t
{
  double value;
  double error;
};

// Event ====================================================================

struct event_t : public noncopyable
{
private:
  static void cancel_( event_t* e );
  static void early_( event_t* e );

  static void* operator new( std::size_t ) throw(); // DO NOT USE!

public:
  event_t*  next;
  sim_t*    sim;
  player_t* player;
  uint32_t  id;
  timespan_t time;
  timespan_t reschedule_time;
  int       canceled;
  const char* name;
  event_t( sim_t* s, player_t* p=0, const char* n="" ) :
    next( 0 ), sim( s ), player( p ), time( timespan_t::zero ), reschedule_time( timespan_t::zero ), canceled( 0 ), name( n )
  {
    if ( ! name ) name = "unknown";
  }
  timespan_t occurs()  const { return ( reschedule_time != timespan_t::zero ) ? reschedule_time : time; }
  timespan_t remains() const { return occurs() - sim -> current_time; }
  virtual void reschedule( timespan_t new_time );
  virtual void execute() = 0;
  virtual ~event_t() {}

  // T must be implicitly convertible to event_t* --
  // basically, a pointer to a type derived from event_t.
  template <typename T> static void cancel( T& e )
  { if ( e ) { cancel_( e ); e = 0; } }
  template <typename T> static void early( T& e )
  { if ( e ) { early_( e ); e = 0; } }

  // Simple free-list memory manager.
  static void* operator new( std::size_t size, sim_t* sim )
  { return sim -> free_list.allocate( size ); }

  static void operator delete( void* p )
  {
    event_t* e = static_cast<event_t*>( p );
    e -> sim -> free_list.deallocate( e );
  }

  static void operator delete( void* p, sim_t* sim )
  { sim -> free_list.deallocate( p ); }
};

// Gear Rating Conversions ==================================================

namespace internal {
struct rating_t
{
  double  spell_alacrity,  spell_hit,  spell_crit;
  double attack_alacrity, attack_hit, attack_crit;
  double ranged_alacrity, ranged_hit,ranged_crit;
  double expertise;
  double dodge, parry, block;
};
}

struct rating_t : public internal::rating_t
{
  typedef internal::rating_t base_t;
  rating_t() : base_t( base_t() ) {}

  void init( sim_t*, dbc_t& pData, int level, int type );

  static double interpolate( int level, double val_60, double val_70, double val_80, double val_85 = -1 );
  static double get_attribute_base( sim_t*, dbc_t& pData, int level, player_type class_type, race_type race, base_stat_type stat_type );
  static double standardhealth_damage( int level );
  static double standardhealth_healing( int level );
  static int get_base_health( int level );

  static double swtor_diminishing_return( double cap, double divisor, int level, double rating )
  {
    return cap * ( 1.0 - std::pow( ( 1.0 - ( 0.01 / cap ) ),
                                   std::max( rating, 0.0 ) / std::max( 20, level ) / divisor ) );
  }

  static double crit_from_stat( double amount, int level )
  { return swtor_diminishing_return( 0.3, 2.5, level, amount ); }

  static double crit_from_rating( double amount, int level )
  { return swtor_diminishing_return( 0.3, 0.45, level, amount ); }

  static double alacrity_from_rating( double amount, int level )
  { return swtor_diminishing_return( 0.3, 0.55, level, amount ); }

  static double accuracy_from_rating( double amount, int level )
  { return swtor_diminishing_return( 0.3, 0.55, level, amount ); }

  static double surge_from_rating( double amount, int level )
  { return swtor_diminishing_return( 0.3, 0.11, level, amount ); }

  static double base_hit_chance( int attacker_level, int defender_level )
  {
    ( void )attacker_level; ( void )defender_level;
    return 1.0;
  }
};

// Weapon ===================================================================

struct weapon_t
{
  int    type;
  school_type school;
  double damage, dps;
  double min_dmg, max_dmg;
  timespan_t swing_time;
  int    slot;
  int    buff_type;
  double buff_value;
  double bonus_dmg;

  int    group() const;
  timespan_t normalized_weapon_speed() const;
  double proc_chance_on_swing( double PPM, timespan_t adjusted_swing_time=timespan_t::zero ) const;

  weapon_t( int t=WEAPON_NONE, double d=0, timespan_t st=timespan_t::from_seconds( 2.0 ), school_type s=SCHOOL_KINETIC ) :
    type( t ), school( s ), damage( d ), min_dmg( d ), max_dmg( d ), swing_time( st ), slot( SLOT_NONE ), buff_type( 0 ), buff_value( 0 ), bonus_dmg( 0 ) { }
};

// Item =====================================================================

struct item_t
{
  sim_t* sim;
  player_t* player;
  int slot, quality, ilevel;
  bool unique, unique_enchant, unique_addon, is_heroic, is_lfr, is_ptr, is_matching_type, is_reforged;
  stat_type reforged_from;
  stat_type reforged_to;

  // Option Data
  std::string option_name_str;
  std::string option_id_str;
  std::string option_stats_str;
  std::string option_gems_str;
  std::string option_enchant_str;
  std::string option_addon_str;
  std::string option_equip_str;
  std::string option_use_str;
  std::string option_weapon_str;
  std::string option_heroic_str;
  std::string option_lfr_str;
  std::string option_armor_type_str;
  std::string option_reforge_str;
  std::string option_random_suffix_str;
  std::string option_ilevel_str;
  std::string option_quality_str;
  std::string option_data_source_str;
  std::string options_str;

  // Armory Data
  std::string armory_name_str;
  std::string armory_id_str;
  std::string armory_stats_str;
  std::string armory_gems_str;
  std::string armory_enchant_str;
  std::string armory_addon_str;
  std::string armory_weapon_str;
  std::string armory_heroic_str;
  std::string armory_lfr_str;
  std::string armory_armor_type_str;
  std::string armory_reforge_str;
  std::string armory_ilevel_str;
  std::string armory_quality_str;
  std::string armory_random_suffix_str;

  // Encoded Data
  std::string id_str;
  std::string encoded_name_str;
  std::string encoded_stats_str;
  std::string encoded_gems_str;
  std::string encoded_enchant_str;
  std::string encoded_addon_str;
  std::string encoded_equip_str;
  std::string encoded_use_str;
  std::string encoded_weapon_str;
  std::string encoded_heroic_str;
  std::string encoded_lfr_str;
  std::string encoded_armor_type_str;
  std::string encoded_reforge_str;
  std::string encoded_ilevel_str;
  std::string encoded_quality_str;
  std::string encoded_random_suffix_str;

  // Extracted data
  gear_stats_t base_stats,stats;
  struct special_effect_t
  {
    std::string name_str, trigger_str;
    int trigger_type;
    int64_t trigger_mask;
    int stat;
    school_type school;
    int max_stacks;
    double stat_amount, discharge_amount, discharge_scaling;
    double proc_chance;
    timespan_t duration, cooldown, tick;
    bool cost_reduction;
    bool no_crit;
    bool no_player_benefits;
    bool no_debuffs;
    bool no_refresh;
    bool chance_to_discharge;
    bool reverse;
    special_effect_t() :
      trigger_type( 0 ), trigger_mask( 0 ), stat( 0 ), school( SCHOOL_NONE ),
      max_stacks( 0 ), stat_amount( 0 ), discharge_amount( 0 ), discharge_scaling( 0 ),
      proc_chance( 0 ), duration( timespan_t::zero ), cooldown( timespan_t::zero ),
      tick( timespan_t::zero ), cost_reduction( false ), no_crit( false ), no_player_benefits( false ), no_debuffs( false ),
      no_refresh( false ), chance_to_discharge( false ), reverse( false ) {}
    bool active() { return stat || school; }
  } use, equip, enchant, addon;

  item_t() : sim( 0 ), player( 0 ), slot( SLOT_NONE ), quality( 0 ), ilevel( 0 ), unique( false ), unique_enchant( false ),
    unique_addon( false ), is_heroic( false ), is_lfr( false ), is_ptr( false ), is_matching_type( false ), is_reforged( false ) {}
  item_t( player_t*, const std::string& options_str );
  bool active() const;
  bool heroic() const;
  bool lfr() const;
  bool ptr() const;
  bool reforged() const;
  bool matching_type();
  const char* name() const;
  const char* slot_name() const;
  const char* armor_type();
  weapon_t* weapon() const;
  bool init();
  bool parse_options();
  void encode_options();
  bool decode_stats();
  bool decode_enchant();
  bool decode_addon();
  bool decode_special( special_effect_t&, const std::string& encoding );
  bool decode_weapon();
  bool decode_heroic();
  bool decode_lfr();
  bool decode_armor_type();
  bool decode_reforge();
  bool decode_random_suffix();
  bool decode_ilevel();
  bool decode_quality();

  static bool download_slot( item_t& item,
                             const std::string& item_id,
                             const std::string& enchant_id,
                             const std::string& addon_id,
                             const std::string& reforge_id,
                             const std::string& rsuffix_id,
                             const std::string gem_ids[ 3 ] );
  static bool download_item( item_t&, const std::string& item_id );

#if 0
  bool decode_gems();
  static bool download_glyph( player_t* player, std::string& glyph_name, const std::string& glyph_id );
  static int  parse_gem( item_t&            item,
                         const std::string& gem_id );
#endif
};

// Item database ============================================================
struct item_database_t
{
  static bool     download_slot(      item_t& item,
                                      const std::string& item_id,
                                      const std::string& enchant_id,
                                      const std::string& addon_id,
                                      const std::string& reforge_id,
                                      const std::string& rsuffix_id,
                                      const std::string gem_ids[ 3 ] );
  static bool     download_item(      item_t& item, const std::string& item_id );
  static bool     initialize_item_sources( const item_t& item, std::vector<std::string>& source_list );

  static int      random_suffix_type( const item_t& item );
  static int      random_suffix_type( const item_data_t* );
  static uint32_t armor_value(        const item_t& item, unsigned item_id );
  static uint32_t armor_value(        const item_data_t*, const dbc_t& );
  static uint32_t weapon_dmg_min(     const item_t& item, unsigned item_id );
  static uint32_t weapon_dmg_min(     const item_data_t*, const dbc_t& );
  static uint32_t weapon_dmg_max(     const item_t& item, unsigned item_id );
  static uint32_t weapon_dmg_max(     const item_data_t*, const dbc_t& );

  static bool     load_item_from_data( item_t& item, const item_data_t* item_data );

  static bool     parse_enchant(      item_t& item, const std::string& enchant_id );

#if 0
  static bool     download_glyph(     player_t* player, std::string& glyph_name, const std::string& glyph_id );
  static int      parse_gem(          item_t& item, const std::string& gem_id );
  static bool     parse_gems(         item_t&            item,
                                      const item_data_t* item_data,
                                      const std::string  gem_ids[ 3 ] );
#endif
};

// Set Bonus ================================================================

struct set_bonus_t
{
  set_bonus_t* next;
  std::string name;
  std::vector<std::string> filters;
  int count;

  set_bonus_t( const std::string& name, const std::string& filters=std::string() );

  void init( const player_t& );

  bool two_pc() const { return ( count >= 2 ); }
  bool four_pc() const { return ( count >= 4 ); }

  //action_expr_t* create_expression( action_t*, const std::string& type );

private:
  bool decode( const item_t& item ) const;
};

// Player ===================================================================

struct player_t : public noncopyable
{
  sim_t*      sim;
  std::string name_str, talents_str, id_str, target_str;
  std::string region_str, server_str, origin_str;
  player_t*   next;
  int         index;
  player_type type;
  role_type   role;
  player_t*   target;
  int         level, use_pre_potion, party, member;
  double      skill, initial_skill, distance, default_distance;
  timespan_t  gcd_ready;
  timespan_t  base_gcd;
  int         potion_used, sleeping, initial_sleeping, initialized;
  rating_t    rating;
  pet_t*      pet_list;
  int         bugs;
  int         specialization;
  int         invert_scaling;
  bool        vengeance_enabled;
  double      vengeance_damage, vengeance_value, vengeance_max;
  bool        vengeance_was_attacked;
  int         active_pets;
  timespan_t  reaction_mean,reaction_stddev,reaction_nu;
  int         infinite_resource[ RESOURCE_MAX ];
  std::vector<buff_t*> absorb_buffs;
  int         scale_player;
  double      avg_ilvl;
  pet_t*      active_companion;

  // Latency
  timespan_t  world_lag, world_lag_stddev;
  timespan_t  brain_lag, brain_lag_stddev;
  bool        world_lag_override, world_lag_stddev_override;

  int    events;

  // Data access
  dbc_t       dbc;

  // Option Parsing
  std::vector<option_t> options;

  // Talent Parsing
  int tree_type[ MAX_TALENT_TREES ];
  int talent_tab_points[ MAX_TALENT_TREES ];
  std::vector<talent_t*> talent_trees[ MAX_TALENT_TREES ];
  //std::vector<glyph_t*> glyphs;

  std::list<spell_id_t*> spell_list;

  // Profs
  std::string professions_str;
  int profession[ PROFESSION_MAX ];

  // Race
  std::string race_str;
  race_type race;

  // Ratings
  double initial_alacrity_rating, alacrity_rating;
  double initial_crit_rating, crit_rating;
  double initial_accuracy_rating, accuracy_rating;
  double initial_surge_rating, surge_rating;

  double surge_bonus, buffed_surge;

  // Attributes
  attribute_type primary_attribute, secondary_attribute;
  double attribute                   [ ATTRIBUTE_MAX ];
  double attribute_base              [ ATTRIBUTE_MAX ];
  double attribute_initial           [ ATTRIBUTE_MAX ];
  double attribute_multiplier        [ ATTRIBUTE_MAX ];
  double attribute_multiplier_initial[ ATTRIBUTE_MAX ];

  // Spell Mechanics
  double base_power,       initial_power,       power;
  double base_force_power, initial_force_power, force_power;
  double base_tech_power,  initial_tech_power,  tech_power;

  double base_energy_regen_per_second;
  double base_ammo_regen_per_second;
  double base_force_regen_per_second;
  double resource_reduction[ SCHOOL_MAX ], initial_resource_reduction[ SCHOOL_MAX ];

  int    position;
  std::string position_str;

  // Defense Mechanics
  event_t* target_auto_attack;
  double base_armor,       initial_armor,       armor,       buffed_armor;
  double base_bonus_armor, initial_bonus_armor, bonus_armor;
  double base_miss,        initial_miss,        miss,        buffed_miss;
  double base_dodge,       initial_dodge,       dodge,       buffed_dodge;
  double base_parry,       initial_parry,       parry,       buffed_parry;
  double base_block,       initial_block,       block,       buffed_block;
  double base_block_reduction, initial_block_reduction, block_reduction;
  double armor_multiplier,  initial_armor_multiplier;
  double armor_coeff;

  // Weapons
  weapon_t main_hand_weapon;
  weapon_t off_hand_weapon;
  weapon_t ranged_weapon;

  // Resources
  double  resource_base   [ RESOURCE_MAX ];
  double  resource_initial[ RESOURCE_MAX ];
  double  resource_max    [ RESOURCE_MAX ];
  double  resource_current[ RESOURCE_MAX ];
  double  health_per_endurance;
  uptime_t* primary_resource_cap;

  // Replenishment
  std::vector<player_t*> replenishment_targets;

  // Callbacks
  action_callback_t* dark_intent_cb;

  // Consumables
  int stim;

  // Events
  action_t* executing;
  action_t* channeling;
  event_t*  readying;
  event_t*  off_gcd;
  bool      in_combat;
  bool      action_queued;

  // Delay time used by "cast_delay" expression to determine when an action
  // can be used at minimum after a spell cast has finished, including GCD
  timespan_t cast_delay_reaction;
  timespan_t cast_delay_occurred;

  // Callbacks
  std::vector<action_callback_t*> all_callbacks;
  std::vector<action_callback_t*> attack_callbacks[ RESULT_MAX ];
  std::vector<action_callback_t*>  spell_callbacks[ RESULT_MAX ];
  std::vector<action_callback_t*>  heal_callbacks[ RESULT_MAX ];
  std::vector<action_callback_t*>   tick_callbacks[ RESULT_MAX ];
  std::vector<action_callback_t*> direct_damage_callbacks[ SCHOOL_MAX ];
  std::vector<action_callback_t*>   tick_damage_callbacks[ SCHOOL_MAX ];
  std::vector<action_callback_t*>   direct_heal_callbacks[ SCHOOL_MAX ];
  std::vector<action_callback_t*>     tick_heal_callbacks[ SCHOOL_MAX ];
  std::vector<action_callback_t*> resource_gain_callbacks[ RESOURCE_MAX ];
  std::vector<action_callback_t*> resource_loss_callbacks[ RESOURCE_MAX ];

  // Action Priority List
  action_t*   action_list;
  std::vector<action_t*> off_gcd_actions;
  std::string action_list_str;
  std::string choose_action_list;
  std::string action_list_skip;
  std::string modify_action;
  int         action_list_default;
  cooldown_t* cooldown_list;
  dot_t*      dot_list;
  std::map<std::string,int> action_map;
  std::vector<action_priority_list_t*> action_priority_list;

  // Reporting
  int       quiet;
  action_t* last_foreground_action;
  timespan_t iteration_fight_length,arise_time;
  sample_data_t fight_length, waiting_time, executed_foreground_actions;
  timespan_t iteration_waiting_time;
  int       iteration_executed_foreground_actions;
  double    resource_lost  [ RESOURCE_MAX ];
  double    resource_gained[ RESOURCE_MAX ];
  double    rps_gain, rps_loss;
  sample_data_t deaths;
  double    deaths_error;

  // Buffed snapshot_stats (for reporting)
  struct buffed_stats_t
  {
    double attribute[ ATTRIBUTE_MAX ];
    double resource[ RESOURCE_MAX ];
    double alacrity;

    double power, force_power, tech_power;

    double melee_hit,  range_hit,  force_hit,  tech_hit;
    double melee_crit, range_crit, force_crit, tech_crit;
  } buffed;

  buff_t*   buff_list;
  proc_t*   proc_list;
  gain_t*   gain_list;
  stats_t*  stats_list;
  benefit_t* benefit_list;
  uptime_t* uptime_list;
  set_bonus_t* set_bonus_list;
  std::vector<double> dps_plot_data[ STAT_MAX ];
  std::vector<std::vector<reforge_plot_data_t> > reforge_plot_data;
  std::vector<std::vector<double> > timeline_resource;

  // Damage
  double iteration_dmg, iteration_dmg_taken;
  double dps_error, dpr, dtps_error;
  sample_data_t dmg;
  sample_data_t compound_dmg;
  sample_data_t dps;
  sample_data_t dpse;
  sample_data_t dtps;
  sample_data_t dmg_taken;
  std::vector<double> timeline_dmg;
  std::vector<double> timeline_dps;

  // Heal
  double iteration_heal,iteration_heal_taken;
  double hps_error,hpr;
  sample_data_t heal;
  sample_data_t compound_heal;
  sample_data_t hps;
  sample_data_t hpse;
  sample_data_t htps;
  sample_data_t heal_taken;


  std::string action_sequence;
  std::string action_dpet_chart, action_dmg_chart, time_spent_chart, gains_chart;
  std::vector<std::string> timeline_resource_chart;
  std::string timeline_dps_chart, timeline_resource_health_chart;
  std::string distribution_dps_chart, scaling_dps_chart, scale_factors_chart;
  std::string reforge_dps_chart, dps_error_chart, distribution_deaths_chart;
  std::string gear_weights_lootrank_link, gear_weights_wowhead_link, gear_weights_wowreforge_link;
  std::string gear_weights_pawn_std_string, gear_weights_pawn_alt_string;
  std::string save_str;
  std::string save_gear_str;
  std::string save_talents_str;
  std::string save_actions_str;
  std::string comment_str;
  std::string thumbnail_url;

  // Gear
  std::string items_str;
  std::vector<item_t> items;
  gear_stats_t stats, initial_stats, gear, enchant, temporary;

  // Scale Factors
  gear_stats_t scaling;
  gear_stats_t scaling_normalized;
  gear_stats_t scaling_error;
  gear_stats_t scaling_delta_dps;
  gear_stats_t scaling_compare_error;
  double       scaling_lag, scaling_lag_error;
  int          scales_with[ STAT_MAX ];
  double       over_cap[ STAT_MAX ];
  std::vector<int> scaling_stats; // sorting vector

  // Movement & Position
  double base_movement_speed;
  double x_position, y_position;

  struct buffs_t
  {
    buff_t* coordination;
    buff_t* force_valor;
    buff_t* power_potion;
    buff_t* raid_movement;
    buff_t* self_movement;
    buff_t* stunned;
    buff_t* unnatural_might;
  };
  buffs_t buffs;

  struct debuffs_t
  {
    debuff_t* bleeding;
    debuff_t* flying;
    debuff_t* invulnerable;
    debuff_t* shatter_shot;
    debuff_t* vulnerable;

    bool snared();
  };
  debuffs_t debuffs;

  struct gains_t
  {
    gain_t* energy_regen;
    gain_t* ammo_regen;
    gain_t* force_regen;
    void reset() { *this = gains_t(); }
  };
  gains_t gains;

  rng_t* rng_list;

  struct rngs_t
  {
    rng_t* lag_channel;
    rng_t* lag_gcd;
    rng_t* lag_queue;
    rng_t* lag_ability;
    rng_t* lag_reaction;
    rng_t* lag_world;
    rng_t* lag_brain;
    void reset() { *this = rngs_t(); }
  };
  rngs_t rngs;

  struct set_bonuses_t
  {
    set_bonus_t* rakata_force_masters;
    set_bonus_t* battlemaster_force_masters;
    set_bonus_t* rakata_stalkers;
    void reset() { *this = set_bonuses_t(); }
  };
  set_bonuses_t set_bonus;

  int targetdata_id;
  std::vector<targetdata_t*> targetdata;

  player_t( sim_t* sim, player_type type, const std::string& name, race_type race_type = RACE_NONE );

  virtual ~player_t();

  virtual const char* name() const { return name_str.c_str(); }

  virtual targetdata_t* new_targetdata( player_t* source, player_t* target );
  virtual void init();
  virtual void init_base() = 0;
  virtual void init_items();
  virtual void init_core();
  virtual void init_position();
  virtual void init_race();
  virtual void init_racials();
  virtual void init_spell();
  virtual void init_attack();
  virtual void init_defense();
  virtual void init_weapon( weapon_t* );
  virtual void init_unique_gear();
  virtual void init_enchant();
  virtual void init_resources( bool force = false );
  virtual void init_professions();
  virtual void init_professions_bonus();
  virtual std::string init_use_item_actions( const std::string& append = std::string() );
  virtual std::string init_use_profession_actions( const std::string& append = std::string() );
  virtual std::string init_use_racial_actions( const std::string& append = std::string() );
  virtual void init_actions();
  virtual void init_rating();
  virtual void init_scaling();
  virtual void init_talents();
  virtual void init_spells();
  virtual void init_buffs();
  virtual void init_gains();
  virtual void init_procs();
  virtual void init_uptimes();
  virtual void init_benefits();
  virtual void init_rng();
  virtual void init_stats();
  virtual void init_values();
  virtual void init_target();

  virtual void reset();
  virtual void combat_begin();
  virtual void combat_end();
  virtual void merge( player_t& other );

  virtual double energy_regen_per_second() const;
  virtual double ammo_regen_per_second() const;
  virtual double force_regen_per_second() const;

  virtual double composite_power() const;
  virtual double composite_force_power() const;
  virtual double composite_tech_power() const;

  virtual double composite_armor() const;
  virtual double composite_armor_multiplier() const;

  virtual double shield_chance() const { return 0; }
  virtual double shield_absorb() const { return 0; }

  virtual double composite_tank_miss( const school_type school ) const;
  virtual double composite_tank_dodge()            const;
  virtual double composite_tank_parry()            const;
  virtual double composite_tank_block()            const;
  virtual double composite_tank_block_reduction()  const;
  virtual double composite_tank_crit_block()            const;
  virtual double composite_tank_crit( const school_type school ) const;

  virtual double composite_attribute_multiplier( int attr ) const;

  virtual double composite_player_multiplier( const school_type school, action_t* a = NULL ) const;
  virtual double composite_player_dd_multiplier( const school_type /* school */, action_t* /* a */ = NULL ) const { return 1; }
  virtual double composite_player_td_multiplier( const school_type school, action_t* a = NULL ) const;

  virtual double composite_player_heal_multiplier( const school_type school ) const;
  virtual double composite_player_dh_multiplier( const school_type /* school */ ) const { return 1; }
  virtual double composite_player_th_multiplier( const school_type school ) const;

  virtual double composite_player_absorb_multiplier( const school_type school ) const;

  virtual double composite_movement_speed() const;

private:
  double damage_bonus( double stats, double multiplier, double extra_power=0 ) const;
  double healing_bonus( double stats, double multiplier, double extra_power=0 ) const;
  double default_hit_chance() const;
  double default_crit_chance() const;

protected:
  virtual double melee_bonus_stats() const;
  virtual double melee_bonus_multiplier() const;
  virtual double melee_crit_from_stats() const;

  virtual double range_bonus_stats() const;
  virtual double range_bonus_multiplier() const;
  virtual double range_crit_from_stats() const;

  virtual double force_bonus_stats() const;
  virtual double force_bonus_multiplier() const;
  virtual double force_crit_from_stats() const;

  virtual double tech_bonus_stats() const;
  virtual double tech_bonus_multiplier() const;
  virtual double tech_crit_from_stats() const;

  virtual double force_healing_bonus_stats() const;
  virtual double force_healing_bonus_multiplier() const;
  virtual double force_healing_crit_from_stats() const;

  virtual double tech_healing_bonus_stats() const;
  virtual double tech_healing_bonus_multiplier() const;
  virtual double tech_healing_crit_from_stats() const;

public:
  double melee_damage_bonus() const;
  virtual double melee_hit_chance() const;
  virtual double melee_crit_chance() const;
  virtual double melee_avoidance() const;

  double range_damage_bonus() const;
  virtual double range_hit_chance() const;
  virtual double range_crit_chance() const;
  virtual double range_avoidance() const;

  double force_damage_bonus() const;
  double force_healing_bonus() const;
  virtual double force_hit_chance() const;
  virtual double force_crit_chance() const;
  virtual double force_avoidance() const;

  double tech_damage_bonus() const;
  double tech_healing_bonus() const;
  virtual double tech_hit_chance() const;
  virtual double tech_crit_chance() const;
  virtual double tech_avoidance() const;

  double get_attribute( attribute_type a ) const;
  double strength() const { return get_attribute( ATTR_STRENGTH ); }
  double aim() const { return get_attribute( ATTR_AIM ); }
  double cunning() const { return get_attribute( ATTR_CUNNING ); }
  double willpower() const { return get_attribute( ATTR_WILLPOWER ); }
  double endurance() const { return get_attribute( ATTR_ENDURANCE ); }
  double presence() const { return get_attribute( ATTR_PRESENCE ); }

  virtual double alacrity() const;

  virtual void      interrupt();
  virtual void      halt();
  virtual void      moving();
  virtual void      stun();
  virtual void      clear_debuffs();
  virtual void      schedule_ready( timespan_t delta_time=timespan_t::zero, bool waiting=false );
  virtual void      arise();
  virtual void      demise();
  virtual timespan_t available() const { return timespan_t::from_seconds( 0.1 ); }
  virtual action_t* execute_action();

  virtual std::string print_action_map( int iterations, int precision );

  virtual void   regen( timespan_t periodicity=timespan_t::from_seconds( 0.25 ) );
  virtual double resource_gain( int resource, double amount, gain_t* g=0, action_t* a=0 );
  virtual double resource_loss( int resource, double amount, action_t* a=0 );
  virtual void   recalculate_resource_max( int resource );
  virtual bool   resource_available( int resource, double cost ) const;
  virtual int    primary_resource() const { return RESOURCE_NONE; }
  virtual int    primary_role() const;
  virtual int    primary_tree() const;
  int            primary_tab() const;
  const char*    primary_tree_name() const;
  virtual int    normalize_by() const;

  virtual double health_percentage() const;
  virtual timespan_t time_to_die() const;
  virtual timespan_t total_reaction_time() const;

  virtual void stat_gain( int stat, double amount, gain_t* g=0, action_t* a=0, bool temporary=false );
  virtual void stat_loss( int stat, double amount, action_t* a=0, bool temporary=false );

  virtual void cost_reduction_gain( int school, double amount, gain_t* g=0, action_t* a=0 );
  virtual void cost_reduction_loss( int school, double amount, action_t* a=0 );

  virtual double assess_damage( double amount, const school_type school, int type, int result, action_t* a=0 );
  virtual double target_mitigation( double amount, const school_type school, int type, int result, action_t* a=0 );

  struct heal_info_t { double actual, amount; };
  virtual heal_info_t assess_heal( double amount, const school_type school, int type, int result, action_t* a=0 );

  virtual void  summon_pet( const char* name, timespan_t duration=timespan_t::zero );
  virtual void dismiss_pet( const char* name );

  virtual bool ooc_buffs() { return true; }

  virtual bool is_moving() { return buffs.raid_movement -> check() || buffs.self_movement -> check(); }

  virtual void register_callbacks();
  virtual void register_resource_gain_callback( int resource,        action_callback_t* );
  virtual void register_resource_loss_callback( int resource,        action_callback_t* );
  virtual void register_attack_callback       ( int64_t result_mask, action_callback_t* );
  virtual void register_spell_callback        ( int64_t result_mask, action_callback_t* );
  virtual void register_tick_callback         ( int64_t result_mask, action_callback_t* );
  virtual void register_heal_callback         ( int64_t result_mask, action_callback_t* );
  virtual void register_harmful_spell_callback( int64_t result_mask, action_callback_t* );
  virtual void register_tick_damage_callback  ( int64_t result_mask, action_callback_t* );
  virtual void register_direct_damage_callback( int64_t result_mask, action_callback_t* );
  virtual void register_tick_heal_callback    ( int64_t result_mask, action_callback_t* );
  virtual void register_direct_heal_callback  ( int64_t result_mask, action_callback_t* );

  bool parse_talent_trees( const int talents[MAX_TALENT_SLOTS] );
  bool parse_talents_armory ( const std::string& talent_string );
  bool parse_talents_wowhead( const std::string& talent_string );

  virtual void create_talents();
  //virtual void create_glyphs();

  virtual talent_t* find_talent( const std::string& name, int tree = TALENT_TAB_NONE );
  //virtual glyph_t*  find_glyph ( const std::string& name );

  virtual action_expr_t* create_expression( action_t*, const std::string& name );

  virtual void create_options();
  virtual bool create_profile( std::string& profile_str, int save_type=SAVE_ALL, bool save_html=false );

  virtual void copy_from( player_t* source );

  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual void      create_pets() { }
  virtual pet_t*    create_pet( const std::string& /* name*/,  const std::string& /* type */ = std::string() ) { return 0; }
  virtual pet_t*    find_pet  ( const std::string& name );

  virtual void trigger_replenishment();

  virtual void recalculate_surge();

  virtual void armory_extensions( const std::string& /* region */, const std::string& /* server */, const std::string& /* character */,
                                  cache::behavior_t /* behavior */=cache::players() )
  {}

  // Class-Specific Methods

  static player_t* create( sim_t* sim, const std::string& type, const std::string& name, race_type r = RACE_NONE );

  static player_t* create_jedi_sage( sim_t* sim, const std::string& name, race_type r = RACE_NONE );
  static player_t* create_sith_sorcerer( sim_t* sim, const std::string& name, race_type r = RACE_NONE );
  static player_t* create_jedi_shadow( sim_t* sim, const std::string& name, race_type r = RACE_NONE );
  static player_t* create_sith_assassin( sim_t* sim, const std::string& name, race_type r = RACE_NONE );
  static player_t* create_enemy       ( sim_t* sim, const std::string& name, race_type r = RACE_NONE );

  // Raid-wide aura/buff/debuff maintenance
  static bool init        ( sim_t* sim );
  static void combat_begin( sim_t* sim );
  static void combat_end  ( sim_t* sim );

  // Raid-wide Sage / Sorcerer buff maintenance
  static void jedi_sage_init        ( sim_t* sim );
  static void jedi_sage_combat_begin( sim_t* sim );
  static void jedi_sage_combat_end  ( sim_t* /* sim */ ) {}

  // Raid-wide Shadow / Assassin buff maintenance
  static void shadow_assassin_init        ( sim_t* sim );
  static void shadow_assassin_combat_begin( sim_t* sim );
  static void shadow_assassin_combat_end  ( sim_t* /* sim */ ) {}

  // Raid-wide Enemy buff maintenance
  static void enemy_init        ( sim_t* sim );
  static void enemy_combat_begin( sim_t* sim );
  static void enemy_combat_end  ( sim_t* /* sim */ ) {}

  bool is_pet() const { return type == PLAYER_PET || type == PLAYER_GUARDIAN || type == ENEMY_ADD; }
  bool is_enemy() const { return type == ENEMY || type == ENEMY_ADD; }
  bool is_add() const { return type == ENEMY_ADD; }
  bool is_sage_sorcerer() const { return ( type == JEDI_SAGE || type == SITH_SORCERER ); }
  bool is_shadow_assassin() const { return ( type == JEDI_SHADOW || type == SITH_ASSASSIN ); }

  pet_t* cast_pet() { assert( is_pet() ); return ( pet_t* )this; }
  enemy_t* cast_enemy() { assert( type == ENEMY ); return ( enemy_t*)this; }
  sage_sorcerer_t* cast_sage_sorcerer() { assert( is_sage_sorcerer() ); return ( sage_sorcerer_t*)this; }
  shadow_assassin_t* cast_shadow_assassin() { assert( is_shadow_assassin() ); return ( shadow_assassin_t* )this; }

  bool      in_gcd() const { return gcd_ready > sim -> current_time; }
  item_t*   find_item( const std::string& );
  action_t* find_action( const std::string& );
  set_bonus_t* find_set_bonus( const std::string& name );
  bool      dual_wield() const { return main_hand_weapon.type != WEAPON_NONE && off_hand_weapon.type != WEAPON_NONE; }
  void      aura_gain( const char* name, double value=0 );
  void      aura_loss( const char* name, double value=0 );

  cooldown_t* find_cooldown( const std::string& name ) const;
  dot_t*      find_dot     ( const std::string& name ) const;
  action_priority_list_t* find_action_priority_list( const std::string& name ) const;

  cooldown_t* get_cooldown( const std::string& name );
  dot_t*      get_dot     ( const std::string& name );
  gain_t*     get_gain    ( const std::string& name );
  proc_t*     get_proc    ( const std::string& name );
  stats_t*    get_stats   ( const std::string& name, action_t* action=0 );
  benefit_t*  get_benefit ( const std::string& name );
  uptime_t*   get_uptime  ( const std::string& name );
  rng_t*      get_rng     ( const std::string& name, int type=RNG_DEFAULT );
  set_bonus_t* get_set_bonus( const std::string& name, std::string filter );
  double      get_player_distance( const player_t* p ) const;
  double      get_position_distance( double m=0, double v=0 ) const;
  action_priority_list_t* get_action_priority_list( const std::string& name );

  // Opportunity to perform any stat fixups before analysis
  virtual void pre_analyze_hook() {}
};

struct targetdata_t : public noncopyable
{
  player_t* source;
  player_t* target;

  dot_t* dot_list;

  targetdata_t( player_t* source, player_t* target );

  virtual ~targetdata_t();
  virtual void reset();
  virtual void clear_debuffs();

  static targetdata_t* get( player_t* source, player_t* target );

  jedi_sage_targetdata_t* cast_jedi_sage() { assert( source->type == JEDI_SAGE  ); return ( jedi_sage_targetdata_t* ) this; }
  sith_sorcerer_targetdata_t* cast_sith_sorcerer() { assert( source->type == SITH_SORCERER ); return ( sith_sorcerer_targetdata_t* ) this; }
  shadow_assassin_targetdata_t* cast_shadow_assassin() { assert( source->type == JEDI_SAGE || source -> type == SITH_ASSASSIN ); return ( shadow_assassin_targetdata_t* ) this; }

protected:
  dot_t* add_dot( dot_t* d );
  aura_t* add_aura( aura_t* b );
};

// Pet ======================================================================

struct pet_t : public player_t
{
  std::string full_name_str;
  player_t* owner;
  pet_t* next_pet;
  double endurance_per_owner;
  bool summoned;
  pet_type_t pet_type;
  event_t* expiration;

private:
  void init_pet_t_();
public:
  pet_t( sim_t* sim, player_t* owner, const std::string& name, bool guardian=false );
  pet_t( sim_t* sim, player_t* owner, const std::string& name, pet_type_t pt, player_type type=PLAYER_PET );

  // Pets gain their owners' hit rating, but it rounds down to a
  // percentage.  Also, heroic presence does not contribute to pet
  // expertise, so we use raw attack_hit.
#if 0
  virtual double composite_attack_expertise() const { return floor( floor( 100.0 * owner -> attack_hit ) * ( 26.0 / 8.0 ) ) / 100.0; }
  virtual double composite_attack_hit()       const { return floor( 100.0 * owner -> composite_attack_hit() ) / 100.0; }
  virtual double composite_spell_hit()        const { return floor( 100.0 * owner -> composite_spell_hit() ) / 100.0;  }
#endif
  virtual double endurance() const;

  virtual void init_base();
  virtual void init_talents();
  virtual void init_target();
  virtual void reset();
  virtual void summon( timespan_t duration=timespan_t::zero );
  virtual void dismiss();
  virtual bool ooc_buffs() { return false; }
  virtual double assess_damage( double amount, const school_type school, int type, int result, action_t* a=0 );
  virtual void combat_begin();

  virtual const char* name() const { return full_name_str.c_str(); }
  virtual const char* id();
};

// Companion

struct companion_t : pet_t
{
  companion_t( sim_t* sim, player_t* owner, const std::string& name, pet_type_t pt );

  virtual void summon( timespan_t duration=timespan_t::zero );
  virtual void dismiss();
};

// Stats ====================================================================

struct stats_t
{
  std::string name_str;
  sim_t* sim;
  player_t* player;
  stats_t* next;
  stats_t* parent;
  school_type school;
  stats_type type;
  std::vector<action_t*> action_list;
  bool analyzed;
  bool quiet;
  bool background;

  int resource;
  double resource_consumed, resource_portion;
  double frequency, num_executes, num_ticks;
  double num_direct_results, num_tick_results;
  timespan_t total_execute_time, total_tick_time, total_time;
  double portion_amount, overkill_pct;
  double aps, ape, apet, apr, rpe, etpe, ttpt;
  timespan_t total_intervals;
  double num_intervals;
  timespan_t last_execute;
  double iteration_actual_amount, iteration_total_amount;
  sample_data_t actual_amount, total_amount, portion_aps;
  std::string aps_distribution_chart;

  std::vector<stats_t*> children;
  double compound_actual,compound_amount;
  double opportunity_cost;

  struct stats_results_t
  {
    sample_data_t actual_amount, total_amount,fight_actual_amount, fight_total_amount,count,avg_actual_amount;
    int iteration_count;
    double iteration_actual_amount, iteration_total_amount,pct, overkill_pct;

    stats_results_t( sim_t* s );

    void merge( const stats_results_t& other );
    void combat_end();
  };
  std::vector<stats_results_t> direct_results;
  std::vector<stats_results_t>   tick_results;

  std::vector<double> timeline_amount;
  std::vector<double> timeline_aps;
  std::string timeline_aps_chart;

  stats_t( const std::string& name, player_t* );

  void add_child( stats_t* child );
  void consume_resource( double r ) { resource_consumed += r; }
  void add_result( double act_amount, double tot_amount, int dmg_type, int result );
  void add_tick   ( timespan_t time );
  void add_execute( timespan_t time );
  void combat_begin();
  void combat_end();
  void reset();
  void analyze();
  void merge( const stats_t* other );
};

// Action ===================================================================

struct action_t
{
  class attack_policy_t;
  typedef const attack_policy_t* policy_t;

  static policy_t default_policy;
  static policy_t melee_policy;
  static policy_t range_policy;
  static policy_t force_policy;
  static policy_t tech_policy;
  static policy_t force_heal_policy;
  static policy_t tech_heal_policy;

  sim_t* const sim;
  const int type;
  std::string name_str;
  player_t* const player;
  player_t* target;
  uint32_t id;
  policy_t attack_policy;
  school_type school;
  int resource, tree, result, aoe;
  bool dual, callbacks, channeled, background, sequence, use_off_gcd;
  bool direct_tick, repeating, harmful, proc, item_proc, proc_ignores_slot, discharge_proc, auto_cast, initialized;
  bool may_crit, tick_may_crit, tick_zero, hasted_ticks;
  bool no_buffs, no_debuffs;
  int dot_behavior;
  timespan_t ability_lag, ability_lag_stddev;
  timespan_t min_gcd, trigger_gcd;
  double range;
  double weapon_power_mod;
  timespan_t base_execute_time;
  timespan_t base_tick_time;
  double base_cost;

  struct damage_factors_t {
    double standardhealthpercentmin, standardhealthpercentmax;
    double base_min, base_max;
    double base_multiplier;
    double player_multiplier;
    double target_multiplier;
    double power_mod;

    damage_factors_t() :
      standardhealthpercentmin( 0 ), standardhealthpercentmax( 0 ),
      base_min( 0 ), base_max( 0 ),
      base_multiplier( 1 ), player_multiplier( 1 ), target_multiplier( 1 ),
      power_mod( 0 )
    {}
  } dd, td;

  double   base_multiplier,   base_hit,   base_crit;
  double player_multiplier, player_hit, player_crit;
  double target_multiplier, target_hit, target_crit;
  double target_avoidance, target_shield, target_absorb;
  double crit_multiplier, crit_bonus_multiplier, crit_bonus;
  double base_dd_adder, player_dd_adder, target_dd_adder;
  double player_alacrity;
  double resource_consumed;
  double direct_dmg, tick_dmg;
  int num_ticks;
  weapon_t* weapon;
  double weapon_multiplier;
  double base_add_multiplier;
  double base_aoe_multiplier; // Static reduction of damage for AoE
  bool normalize_weapon_speed;
  rng_t* rng[ RESULT_MAX ];
  rng_t* rng_travel;
  cooldown_t* cooldown;
  mutable dot_t* action_dot;
  stats_t* stats;
  event_t* execute_event;
  event_t* travel_event;
  timespan_t time_to_execute, time_to_travel;
  double travel_speed;
  int bloodlust_active;
  double max_alacrity;
  double alacrity_gain_percentage;
  timespan_t min_current_time, max_current_time;
  double min_health_percentage, max_health_percentage;
  int moving, vulnerable, invulnerable, wait_on_ready, interrupt, not_flying, flying;
  bool round_base_dmg;

  std::string if_expr_str;
  action_expr_t* if_expr;
  std::string interrupt_if_expr_str;
  action_expr_t* interrupt_if_expr;
  std::string sync_str;
  action_t* sync_action;
  action_t* next;
  char marker;
  std::string signature_str;
  std::string target_str;
  std::string label_str;
  timespan_t last_reaction_time;
  int targetdata_dot_offset;

protected:
  std::vector<int> rank_level_list;
  int rank_level;

private:
  mutable player_t* cached_targetdata_target;
  mutable targetdata_t* cached_targetdata;

  void init_action_t_();

public:
  action_t( int type, const char* name, player_t* p=0, policy_t policy=default_policy,
            int r=RESOURCE_NONE, const school_type s=SCHOOL_NONE, int t=TREE_NONE );
  virtual ~action_t();
  void init_dot( const std::string& dot_name );

  //void parse_data();
  //void parse_effect_data( int spell_id, int effect_nr );
  virtual void   parse_options( option_t*, const std::string& options_str );
  virtual double cost() const;
  virtual double alacrity() const;
  virtual timespan_t gcd() const;
  virtual timespan_t execute_time() const;
  virtual timespan_t tick_time() const;
  virtual int    hasted_num_ticks( timespan_t d=timespan_t::min ) const;
  virtual timespan_t travel_time();
  virtual void   player_buff();
  virtual void   target_debuff( player_t* t, int dmg_type );
  virtual void   calculate_result();
  virtual bool   result_is_hit ( int r=RESULT_UNKNOWN ) const;
  virtual bool   result_is_miss( int r=RESULT_UNKNOWN ) const;
  virtual double calculate_direct_damage( int = 0 );
  virtual double calculate_tick_damage();
  virtual double calculate_weapon_damage();
  virtual double armor() const;
  virtual void   consume_resource();
  virtual void   execute();
  virtual void   tick( dot_t* d );
  virtual void   last_tick( dot_t* d );
  virtual void   impact( player_t*, int result, double dmg );
  virtual void   assess_damage( player_t* t, double amount, int dmg_type, int impact_result );
  virtual void   additional_damage( player_t* t, double amount, int dmg_type, int impact_result );
  virtual void   schedule_execute();
  virtual void   schedule_travel( player_t* t );
  virtual void   reschedule_execute( timespan_t time );
  virtual void   update_ready();
  virtual bool   usable_moving();
  virtual bool   ready();
  virtual void   init();
  virtual void   reset();
  virtual void   cancel();
  virtual void   interrupt_action();
  virtual void   check_talent( int talent_rank );
  virtual void   check_spec( int necessary_spec );
  virtual void   check_race( int race );
  virtual const char* name() const { return name_str.c_str(); }

  double total_multiplier() const { return base_multiplier * player_multiplier * target_multiplier; }
  double total_hit() const        { return base_hit        + player_hit;                            }
  double total_crit() const       { return base_crit       + player_crit;                           }
  double total_crit_bonus() const;

  virtual double total_power() const;

  // Some actions require different multipliers for the "direct" and "tick" portions.

  virtual double total_dd_multiplier() const { return total_multiplier() * dd.base_multiplier * dd.player_multiplier * dd.target_multiplier; }
  virtual double total_td_multiplier() const { return total_multiplier() * td.base_multiplier * td.player_multiplier * td.target_multiplier; }

  virtual action_expr_t* create_expression( const std::string& name );

  virtual double ppm_proc_chance( double PPM ) const;

  dot_t* dot() const
  {
    if ( targetdata_dot_offset >= 0 )
      return *( dot_t** )( ( char* )targetdata() + targetdata_dot_offset );
    else
    {
      if ( ! action_dot )
        action_dot = player -> get_dot( name_str );
      return action_dot;
    }
  }

  void add_child( action_t* child ) { stats -> add_child( child -> stats ); }

  // Move to ability_t in future
  const spell_data_t* spell;
  const spelleffect_data_t& effect1() const { return spell -> effect1(); }
  const spelleffect_data_t& effect2() const { return spell -> effect2(); }
  const spelleffect_data_t& effect3() const { return spell -> effect3(); }

  targetdata_t* targetdata() const
  {
    if ( cached_targetdata_target != target )
    {
      cached_targetdata_target = target;
      cached_targetdata = targetdata_t::get( player, target );
    }
    return cached_targetdata;
  }

  virtual std::vector< player_t* > available_targets() const;
  virtual std::vector< player_t* > target_list() const;
};

struct attack_t : public action_t
{
  attack_t( const char* n=0, player_t* p=0, int r=RESOURCE_NONE, school_type s=SCHOOL_KINETIC, int t=TREE_NONE ) :
    action_t( ACTION_ATTACK, n, p, melee_policy, r, s, t )
  {}
};

// Heal =====================================================================

struct heal_t : public action_t
{
  bool group_only;

  heal_t( const char* n=0, player_t* p=0, policy_t policy=default_policy,
          int r=RESOURCE_NONE, school_type s=SCHOOL_NONE, int t=TREE_NONE );

  virtual void player_buff();
  virtual void execute();
  virtual void assess_damage( player_t* t, double amount,
                              int dmg_type, int impact_result );

  player_t* find_greatest_difference_player();
  player_t* find_lowest_player();

  virtual std::vector<player_t*> available_targets() const;
};

// Absorb ===================================================================

struct absorb_t : public action_t
{
  absorb_t( const char* n=0, player_t* p=0, policy_t policy=default_policy,
            int r=RESOURCE_NONE, const school_type s=SCHOOL_NONE, int t=TREE_NONE );

  virtual void player_buff();
  virtual void execute();
  virtual void assess_damage( player_t* t, double amount,
                              int dmg_type, int impact_result );
  virtual void calculate_result();
  virtual void impact( player_t*, int impact_result, double travel_dmg );
};

// Sequence =================================================================

struct sequence_t : public action_t
{
  std::vector<action_t*> sub_actions;
  int current_action;
  bool restarted;

  sequence_t( player_t*, const std::string& sub_action_str );

  virtual void schedule_execute();
  virtual void reset();
  virtual bool ready();
  virtual void restart() { current_action=0; restarted=true;}
};

// Cooldown =================================================================

struct cooldown_t
{
  sim_t* sim;
  player_t* player;
  std::string name_str;
  timespan_t duration;
  timespan_t ready;
  cooldown_t* next;

  cooldown_t( const std::string& n, player_t* p ) : sim( p->sim ), player( p ), name_str( n ), duration( timespan_t::zero ), ready( timespan_t::min ), next( 0 ) {}
  cooldown_t( const std::string& n, sim_t* s ) : sim( s ), player( 0 ), name_str( n ), duration( timespan_t::zero ), ready( timespan_t::min ), next( 0 ) {}

  void reset() { ready=timespan_t::min; }
  void start( timespan_t override=timespan_t::min, timespan_t delay=timespan_t::zero )
  {
    if ( override >= timespan_t::zero ) duration = override;
    if ( duration > timespan_t::zero ) ready = sim -> current_time + duration + delay;
  }
  timespan_t remains()
  {
    timespan_t diff = ready - sim -> current_time;
    if ( diff < timespan_t::zero ) diff = timespan_t::zero;
    return diff;
  }
  const char* name() { return name_str.c_str(); }
};

// DoT ======================================================================

struct dot_t
{
  sim_t* sim;
  player_t* player;
  action_t* action;
  event_t* tick_event;
  dot_t* next;
  int num_ticks, current_tick, added_ticks, ticking;
  timespan_t added_seconds;
  timespan_t ready;
  timespan_t miss_time;
  timespan_t time_to_tick;
  std::string name_str;

  dot_t() : player( 0 ) {}
  dot_t( const std::string& n, player_t* p );

  void   cancel();
  void   extend_duration( int extra_ticks, bool cap=false );
  void   extend_duration_seconds( timespan_t extra_seconds );
  void   recalculate_ready();
  void   refresh_duration();
  void   reset();
  timespan_t remains();
  void   schedule_tick();
  int    ticks();

  const char* name() { return name_str.c_str(); }
};

// Action Callback ==========================================================

struct action_callback_t
{
  sim_t* sim;
  player_t* listener;
  bool active;
  bool allow_self_procs;
  bool allow_item_procs;
  bool allow_procs;

  action_callback_t( sim_t* s, player_t* l, bool ap=false, bool aip=false, bool asp=false ) :
    sim( s ), listener( l ), active( true ), allow_self_procs( asp ), allow_item_procs( aip ), allow_procs( ap )
  {
    if ( l )
    {
      l -> all_callbacks.push_back( this );
    }
  }
  virtual ~action_callback_t() {}
  virtual void trigger( action_t*, void* call_data=0 ) = 0;
  virtual void reset() {}
  virtual void activate() { active=true; }
  virtual void deactivate() { active=false; }

  static void trigger( std::vector<action_callback_t*>& v, action_t* a, void* call_data=0 )
  {
    if ( a && ! a -> player -> in_combat ) return;

    std::size_t size = v.size();
    for ( std::size_t i=0; i < size; i++ )
    {
      action_callback_t* cb = v[ i ];
      if ( cb -> active )
      {
        if ( ! cb -> allow_item_procs && a && a -> item_proc ) return;
        if ( ! cb -> allow_procs && a && a -> proc ) return;
        cb -> trigger( a, call_data );
      }
    }
  }

  static void reset( std::vector<action_callback_t*>& v )
  {
    std::size_t size = v.size();
    for ( std::size_t i=0; i < size; i++ )
    {
      v[ i ] -> reset();
    }
  }
};

// Action Priority List =====================================================

struct action_priority_list_t
{
  std::string name_str;
  std::string action_list_str;
  player_t* player;
  action_priority_list_t( std::string name, player_t* p ) : name_str( name ), player( p )
  {}
};

// Player Ready Event =======================================================

struct player_ready_event_t : public event_t
{
  player_ready_event_t( sim_t* sim, player_t* p, timespan_t delta_time );
  virtual void execute();
};

struct player_gcd_event_t : public event_t
{
  player_gcd_event_t( sim_t* sim, player_t* p, timespan_t delta_time );
  virtual void execute();
};

// Action Execute Event =====================================================

struct action_execute_event_t : public event_t
{
  action_t* action;
  action_execute_event_t( sim_t* sim, action_t* a, timespan_t time_to_execute );
  virtual void execute();
};

// DoT Tick Event ===========================================================

struct dot_tick_event_t : public event_t
{
  dot_t* dot;
  dot_tick_event_t( sim_t* sim, dot_t* d, timespan_t time_to_tick );
  virtual void execute();
};

// Action Travel Event ======================================================

struct action_travel_event_t : public event_t
{
  action_t* action;
  player_t* target;
  int result;
  double damage;
  action_travel_event_t( sim_t* sim, player_t* t, action_t* a, timespan_t time_to_travel );
  virtual void execute();
};

// Regen Event ==============================================================

struct regen_event_t : public event_t
{
  regen_event_t( sim_t* sim );
  virtual void execute();
};

// Unique Gear ==============================================================

struct unique_gear_t
{
  static void init( player_t* );

  static action_callback_t* register_stat_proc( int type, int64_t mask, const std::string& name, player_t*,
                                                int stat, int max_stacks, double amount,
                                                double proc_chance, timespan_t duration, timespan_t cooldown,
                                                timespan_t tick=timespan_t::zero, bool reverse=false, int rng_type=RNG_DEFAULT );

  static action_callback_t* register_cost_reduction_proc( int type, int64_t mask, const std::string& name, player_t*,
                                                          int school, int max_stacks, double amount,
                                                          double proc_chance, timespan_t duration, timespan_t cooldown,
                                                          bool refreshes=false, bool reverse=false, int rng_type=RNG_DEFAULT );

  static action_callback_t* register_discharge_proc( int type, int64_t mask, const std::string& name, player_t*,
                                                     int max_stacks, const school_type school, double amount, double scaling,
                                                     double proc_chance, timespan_t cooldown, bool no_crits, bool no_buffs, bool no_debuffs,
                                                     int rng_type=RNG_DEFAULT );

  static action_callback_t* register_chance_discharge_proc( int type, int64_t mask, const std::string& name, player_t*,
                                                            int max_stacks, const school_type school, double amount, double scaling,
                                                            double proc_chance, timespan_t cooldown, bool no_crits, bool no_buffs, bool no_debuffs,
                                                            int rng_type=RNG_DEFAULT );

  static action_callback_t* register_stat_discharge_proc( int type, int64_t mask, const std::string& name, player_t*,
                                                          int stat, int max_stacks, double stat_amount,
                                                          const school_type school, double discharge_amount, double discharge_scaling,
                                                          double proc_chance, timespan_t duration, timespan_t cooldown, bool no_crits, bool no_buffs,
                                                          bool no_debuffs );

  static action_callback_t* register_stat_proc( item_t&, item_t::special_effect_t& );
  static action_callback_t* register_cost_reduction_proc( item_t&, item_t::special_effect_t& );
  static action_callback_t* register_discharge_proc( item_t&, item_t::special_effect_t& );
  static action_callback_t* register_chance_discharge_proc( item_t&, item_t::special_effect_t& );
  static action_callback_t* register_stat_discharge_proc( item_t&, item_t::special_effect_t& );

  static bool get_equip_encoding( std::string& encoding,
                                  const std::string& item_name,
                                  const bool         item_heroic,
                                  const bool         item_lfr,
                                  const bool         ptr,
                                  const std::string& item_id=std::string() );

  static bool get_use_encoding  ( std::string& encoding,
                                  const std::string& item_name,
                                  const bool         heroic,
                                  const bool         lfr,
                                  const bool         ptr,
                                  const std::string& item_id=std::string() );
};

// Enchants =================================================================

struct enchant_t
{
  static void init( player_t* );
  static bool get_encoding        ( std::string& name, std::string& encoding, const std::string& enchant_id, const bool ptr );
  static bool get_addon_encoding  ( std::string& name, std::string& encoding, const std::string& addon_id, const bool ptr );
  static bool get_reforge_encoding( std::string& name, std::string& encoding, const std::string& reforge_id );
  static int  get_reforge_id      ( stat_type stat_from, stat_type stat_to );
  static bool download        ( item_t&, const std::string& enchant_id );
  static bool download_addon  ( item_t&, const std::string& addon_id   );
  static bool download_reforge( item_t&, const std::string& reforge_id );
  static bool download_rsuffix( item_t&, const std::string& rsuffix_id );
};

// Consumable ===============================================================

struct consumable_t
{
  static action_t* create_action( player_t*, const std::string& name, const std::string& options );
};

// Benefit ==================================================================

struct benefit_t : public noncopyable
{
  int up, down;

  double ratio;

  benefit_t* next;
  std::string name_str;

  explicit benefit_t( const std::string& n ) :
    up( 0 ), down( 0 ),
    ratio( 0.0 ), name_str( n ) {}

  void update( int is_up ) { if ( is_up ) up++; else down++; }

  const char* name() const { return name_str.c_str(); }

  void analyze()
  {
    if ( up != 0 )
      ratio = 1.0 * up / ( down + up );
  }

  void merge( const benefit_t* other )
  { up += other -> up; down += other -> down; }
};

// Uptime ==================================================================

struct uptime_common_t
{
  timespan_t last_start;
  timespan_t uptime_sum;
  sim_t* sim;

  double uptime;

  uptime_common_t( sim_t* s ) :
    last_start( timespan_t::min ), uptime_sum( timespan_t::zero ), sim( s ),
    uptime( std::numeric_limits<double>::quiet_NaN() )
  {}

  void update( bool is_up )
  {
    if ( is_up )
    {
      if ( last_start < timespan_t::zero )
        last_start = sim -> current_time;
    }
    else if ( last_start >= timespan_t::zero )
    {
      uptime_sum += sim -> current_time - last_start;
      last_start = timespan_t::min;
    }
  }

  void reset() { last_start = timespan_t::min; }

  void analyze()
  { uptime = uptime_sum.total_seconds() / sim -> iterations / sim -> simulation_length.mean; }

  void merge( const uptime_common_t& other )
  { uptime_sum += other.uptime_sum; }
};

struct uptime_t : public uptime_common_t
{
  std::string name_str;
  uptime_t* next;

  uptime_t( sim_t* s, const std::string& n ) :
    uptime_common_t( s ), name_str( n )
  {}

  const char* name() const { return name_str.c_str(); }
};

struct buff_uptime_t : public uptime_common_t
{ buff_uptime_t( sim_t* s ) : uptime_common_t( s ) {} };

// Gain =====================================================================

struct gain_t
{
  double actual, overflow, count;

  std::string name_str;
  resource_type type;
  gain_t* next;

  gain_t( const std::string& n ) :
    actual( 0 ), overflow( 0 ), count( 0 ), name_str( n ), type( RESOURCE_NONE )
  {}

  void add( double a, double o=0 ) { actual += a; overflow += o; count++; }
  void merge( const gain_t* other ) { actual += other -> actual; overflow += other -> overflow; count += other -> count; }
  void analyze( const sim_t* sim ) { actual /= sim -> iterations; overflow /= sim -> iterations; count /= sim -> iterations; }
  const char* name() const { return name_str.c_str(); }
};

// Proc =====================================================================

struct proc_t
{
  double count;
  timespan_t last_proc;
  timespan_t interval_sum;
  double interval_count;

  double frequency;
  sim_t* sim;

  player_t* player;
  std::string name_str;
  proc_t* next;

  proc_t( sim_t* s, const std::string& n ) :
    count( 0 ), last_proc( timespan_t::zero ), interval_sum( timespan_t::zero ), interval_count( 0 ),
    frequency( 0 ), sim( s ), player( 0 ), name_str( n ), next( 0 )
  {}

  void occur()
  {
    count++;
    if ( last_proc > timespan_t::zero && last_proc < sim -> current_time )
    {
      interval_sum += sim -> current_time - last_proc;
      interval_count++;
    }
    last_proc = sim -> current_time;
  }

  void merge( const proc_t* other )
  {
    count          += other -> count;
    interval_sum   += other -> interval_sum;
    interval_count += other -> interval_count;
  }

  void analyze( const sim_t* sim )
  {
    count /= sim -> iterations;
    if ( interval_count > 0 ) frequency = interval_sum.total_seconds() / interval_count;
  }

  const char* name() const { return name_str.c_str(); }
};


// Report ===================================================================

struct report_t
{
  static void encode_html( std::string& buffer );
  static std::string encode_html( const char* str );
  //static void print_spell_query( sim_t* );
  static void print_profiles( sim_t* );
  static void print_text( FILE*, sim_t*, bool detail=true );
  static void print_html( sim_t* );
  static void print_html_player( FILE*, player_t*, int );
  static void print_xml( sim_t* );
  static void print_suite( sim_t* );
};

// Chart ====================================================================

struct chart_t
{
  static int raid_aps ( std::vector<std::string>& images, sim_t*, std::vector<player_t*>, bool dps );
  static int raid_dpet( std::vector<std::string>& images, sim_t* );
  static int raid_gear( std::vector<std::string>& images, sim_t* );

  static const char* raid_downtime    ( std::string& s, sim_t* );
  static const char* action_dpet      ( std::string& s, player_t* );
  static const char* action_dmg       ( std::string& s, player_t* );
  static const char* time_spent       ( std::string& s, player_t* );
  static const char* gains            ( std::string& s, player_t*, resource_type );
  static const char* timeline         ( std::string& s, player_t*, const std::vector<double>&, const std::string&, double avg=0, const char* color="FDD017" );
  static const char* scale_factors    ( std::string& s, player_t* );
  static const char* scaling_dps      ( std::string& s, player_t* );
  static const char* reforge_dps      ( std::string& s, player_t* );
  static const char* distribution ( std::string& s, sim_t*, const std::vector<int>&, const std::string&, double, double, double );

  static const char* gear_weights_lootrank  ( std::string& s, player_t* );
  static const char* gear_weights_wowhead   ( std::string& s, player_t* );
  static const char* gear_weights_wowreforge( std::string& s, player_t* );
  static const char* gear_weights_pawn      ( std::string& s, player_t*, bool hit_expertise=true );

  static const char* dps_error( std::string& s, player_t* );

  static const char* resource_color( int type );
};

// Log ======================================================================

struct log_t
{
  // Generic Output
  static void output( sim_t*, const char* format, ... ) PRINTF_ATTRIBUTE( 2,3 );

  // Combat Log (unsupported)
};

// Pseudo Random Number Generation ==========================================

struct rng_t
{
  std::string name_str;
  bool   gauss_pair_use;
  double gauss_pair_value;
  double expected_roll,  actual_roll,  num_roll;
  double expected_range, actual_range, num_range;
  double expected_gauss, actual_gauss, num_gauss;
  bool   average_range, average_gauss;
  rng_t* next;

  rng_t( const std::string& n, bool avg_range=false, bool avg_gauss=false );
  virtual ~rng_t() {}

  virtual int    type() const { return RNG_STANDARD; }
  virtual double real();
  virtual bool    roll( double chance );
  virtual double range( double min, double max );
  timespan_t range( timespan_t min, timespan_t max );
  virtual double gauss( double mean, double stddev );
  timespan_t gauss( timespan_t mean, timespan_t stddev );
  double exgauss( double mean, double stddev, double nu );
  timespan_t exgauss( timespan_t mean, timespan_t stddev, timespan_t nu );
  virtual void   seed( uint32_t start );
  void   report( FILE* );
  static double stdnormal_cdf( double u );
  static double stdnormal_inv( double p );

  static rng_t* create( sim_t*, const std::string& name, int type=RNG_STANDARD );
};

// String utils =============================================================

std::string tolower( const std::string& src );
std::string proper_option_name( const std::string& full_name );
#if 0 // UNUSED
std::string trim( const std::string& src );
void replace_char( std::string& str, char old_c, char new_c  );
void replace_str( std::string& str, const std::string& old_str, const std::string& new_str  );
bool str_to_float( const std::string& src, double& dest );
#endif // UNUSED

// Armory ===================================================================

struct armory_t
{
#if 0
  static bool download_guild( sim_t* sim,
                              const std::string& region,
                              const std::string& server,
                              const std::string& name,
                              const std::vector<int>& ranks,
                              int player_type= PLAYER_NONE,
                              int max_rank=0,
                              cache::behavior_t b=cache::players() );
  static player_t* download_player( sim_t* sim,
                                    const std::string& region,
                                    const std::string& server,
                                    const std::string& name,
                                    const std::string& talents,
                                    cache::behavior_t  b=cache::players() );
  static bool download_slot( item_t&, const std::string& item_id, cache::behavior_t b=cache::items() );
  static bool download_item( item_t&, const std::string& item_id, cache::behavior_t b=cache::items() );
  static int  parse_meta_gem( const std::string& description );
#endif // 0
  static void fuzzy_stats( std::string& encoding, const std::string& description );
  static std::string& format( std::string& name, int format_type = FORMAT_DEFAULT );
};

#if 0
// Battle Net ===============================================================

struct battle_net_t
{
  static player_t* download_player( sim_t* sim,
                                    const std::string& region,
                                    const std::string& server,
                                    const std::string& name,
                                    const std::string& talents,
                                    cache::behavior_t b=cache::players() );
  static bool download_guild( sim_t* sim,
                              const std::string& region,
                              const std::string& server,
                              const std::string& name,
                              const std::vector<int>& ranks,
                              int player_type=PLAYER_NONE,
                              int max_rank=0,
                              cache::behavior_t b=cache::players() );
};

// Wowhead  =================================================================

struct wowhead_t
{
  static player_t* download_player( sim_t* sim,
                                    const std::string& region,
                                    const std::string& server,
                                    const std::string& name,
                                    bool active=true,
                                    cache::behavior_t b=cache::players() );
  static player_t* download_player( sim_t* sim,
                                    const std::string& id,
                                    bool active=true,
                                    cache::behavior_t b=cache::players() );
  static bool download_slot( item_t&,
                             const std::string& item_id,
                             const std::string& enchant_id,
                             const std::string& addon_id,
                             const std::string& reforge_id,
                             const std::string& rsuffix_id,
                             const std::string gem_ids[ 3 ],
                             bool ptr=false,
                             cache::behavior_t b=cache::items() );
  static bool download_item( item_t&, const std::string& item_id,
                             bool ptr=false, cache::behavior_t b=cache::items() );
  static bool download_glyph( player_t* player, std::string& glyph_name, const std::string& glyph_id,
                              bool ptr=false, cache::behavior_t b=cache::items() );
  static int  parse_gem( item_t& item, const std::string& gem_id,
                         bool ptr=false, cache::behavior_t b=cache::items() );
};

// MMO Champion =============================================================

struct mmo_champion_t
{
  static bool download_slot( item_t&,
                             const std::string& item_id,
                             const std::string& enchant_id,
                             const std::string& addon_id,
                             const std::string& reforge_id,
                             const std::string& rsuffix_id,
                             const std::string gem_ids[ 3 ],
                             cache::behavior_t b=cache::items() );
  static bool download_item( item_t&, const std::string& item_id,
                             cache::behavior_t b=cache::items() );
  static bool download_glyph( player_t* player, std::string& glyph_name,
                              const std::string& glyph_id, cache::behavior_t b=cache::items() );
  static int  parse_gem( item_t& item, const std::string& gem_id,
                         cache::behavior_t b=cache::items() );
};

// Blizzard Community Platform API ==========================================

namespace bcp_api
{
bool download_guild( sim_t* sim,
                     const std::string& region,
                     const std::string& server,
                     const std::string& name,
                     const std::vector<int>& ranks,
                     int player_type = PLAYER_NONE,
                     int max_rank=0,
                     cache::behavior_t b=cache::players() );
player_t* download_player( sim_t*,
                           const std::string& region,
                           const std::string& server,
                           const std::string& name,
                           const std::string& talents=std::string( "active" ),
                           cache::behavior_t b=cache::players() );
bool download_item( item_t&, const std::string& item_id, cache::behavior_t b=cache::items() );
bool download_glyph( player_t* player, std::string& glyph_name, const std::string& glyph_id,
                     cache::behavior_t b=cache::items() );
bool download_slot( item_t& item,
                    const std::string& item_id,
                    const std::string& enchant_id,
                    const std::string& addon_id,
                    const std::string& reforge_id,
                    const std::string& rsuffix_id,
                    const std::string gem_ids[ 3 ],
                    cache::behavior_t b=cache::items() );
int parse_gem( item_t& item, const std::string& gem_id, cache::behavior_t b=cache::items() );
}

// Wowreforge ===============================================================

namespace wowreforge
{
player_t* download_player( sim_t* sim, const std::string& id, cache::behavior_t b=cache::players() );
};
#endif // 0

// Knotor ===================================================================

namespace knotor {
bool parse_talents( player_t* player, const std::string& encoding );
}

// HTTP Download  ===========================================================

struct http_t
{
private:
  static void format_( std::string& encoded_url, const std::string& url );
public:
  struct proxy_t
  {
    std::string type;
    std::string host;
    int port;
  };
  static proxy_t proxy;

  static void cache_load();
  static void cache_save();
  static bool clear_cache( sim_t*, const std::string& name, const std::string& value );

  static bool get( std::string& result, const std::string& url, cache::behavior_t b,
                   const std::string& confirmation=std::string(), int throttle_seconds=0 );

  static std::string& format( std::string& encoded_url, const std::string& url )
  { format_( encoded_url, url ); return encoded_url; }
  static std::string& format( std::string& url )
  { format_( url, url ); return url; }
};

// XML ======================================================================

struct xml_t
{
  static const char* get_name( xml_node_t* node );
  static xml_node_t* get_child( xml_node_t* root, const std::string& name );
  static xml_node_t* get_node ( xml_node_t* root, const std::string& path );
  static xml_node_t* get_node ( xml_node_t* root, const std::string& path, const std::string& parm_name, const std::string& parm_value );
  static int  get_children( std::vector<xml_node_t*>&, xml_node_t* root, const std::string& name = std::string() );
  static int  get_nodes   ( std::vector<xml_node_t*>&, xml_node_t* root, const std::string& path );
  static int  get_nodes   ( std::vector<xml_node_t*>&, xml_node_t* root, const std::string& path, const std::string& parm_name, const std::string& parm_value );
  static bool get_value( std::string& value, xml_node_t* root, const std::string& path = std::string() );
  static bool get_value( int&         value, xml_node_t* root, const std::string& path = std::string() );
  static bool get_value( double&      value, xml_node_t* root, const std::string& path = std::string() );
  static xml_node_t* get( sim_t* sim, const std::string& url, cache::behavior_t b,
                          const std::string& confirmation=std::string(), int throttle_seconds=0 );
  static xml_node_t* create( sim_t* sim, const std::string& input );
  static xml_node_t* create( sim_t* sim, FILE* input );
  static void print( xml_node_t* root, FILE* f=0, int spacing=0 );
};


// Java Script ==============================================================

struct js_t
{
  static js_node_t* get_child( js_node_t* root, const std::string& name );
  static js_node_t* get_node ( js_node_t* root, const std::string& path );
  static int  get_children( std::vector<js_node_t*>&, js_node_t* root );
  static int  get_value( std::vector<std::string>& value, js_node_t* root, const std::string& path = std::string() );
  static bool get_value( std::string& value, js_node_t* root, const std::string& path = std::string() );
  static bool get_value( int&         value, js_node_t* root, const std::string& path = std::string() );
  static bool get_value( double&      value, js_node_t* root, const std::string& path = std::string() );
  static js_node_t* create( sim_t* sim, const std::string& input );
  static js_node_t* create( sim_t* sim, FILE* input );
  static void print( js_node_t* root, FILE* f=0, int spacing=0 );
  static const char* get_name( js_node_t* root );
};


// Handy Actions ============================================================

struct wait_action_base_t : public action_t
{
  wait_action_base_t( player_t* player, const char* name ) :
    action_t( ACTION_OTHER, name, player )
  { trigger_gcd = timespan_t::zero; }

  virtual void execute()
  { player -> iteration_waiting_time += time_to_execute; }
};

// Wait For Cooldown Action =================================================

struct wait_for_cooldown_t : public wait_action_base_t
{
  cooldown_t* wait_cd;
  action_t* a;
  wait_for_cooldown_t( player_t* player, const char* cd_name );
  virtual bool usable_moving() { return a -> usable_moving(); }
  virtual timespan_t execute_time() const;
};

inline buff_t* buff_t::find( sim_t* s, const std::string& name ) { return find( s -> buff_list, name ); }
inline buff_t* buff_t::find( player_t* p, const std::string& name ) { return find( p -> buff_list, name ); }

inline actor_pair_t::actor_pair_t( targetdata_t* td )
  : target( td->target ), source( td->source )
{}

#ifdef WHAT_IF

#define AOE_CAP 10

struct ticker_t // replacement for dot_t, handles any ticking buff/debuff
{
  dot_t stuff
  double crit;
  double haste;
};

struct sim_t
{
  ...
  actor_t* actor_list;
  std::vector<player_t*> player_list;
  std::vector<mob_t*> mob_list;
  ...
  int get_aura_slot( const std::string& n, actor_t* source );
  ...
};

struct actor_t
{
  items, stats, action list, resource management...
  actor_t( const std::string& n, int type ) {}
  virtual actor_t*  choose_target();
  virtual void      execute_action() { choose_target(); execute_first_ready_action(); }
  virtual debuff_t* get_debuff( int aura_slot );
  virtual ticker_t* get_ticker( int aura_slot );
  virtual bool      is_ally( actor_t* other );
};

struct player_t : public actor_t
{
  scaling, current_target ( actor ), pet_list ...
  player_t( const std::string& n, int type ) : actor_t( n, type ) { sim.player_list.push_back( this ); }
};

struct pet_t : public actor_t
{
  owner, summon, dismiss...
  pet_t( const std::string& n, int type ) : actor_t( n, type ) {}
};

struct enemy_t : public actor_t
{
  health_by_time, health_per_player, arrise_at_time, arise_at_percent, ...
  enemy_t( const std::string& n ) : actor_t( n, ACTOR_ENEMY ) { sim.enemy_list.push_back( this ); }
};

struct action_t
{
  actor_t, ...
  action_t( const std::string& n );
  virtual int execute();
  virtual void schedule_execute();
  virtual double execute_time();
  virtual double haste();
  virtual double gcd();
  virtual bool ready();
  virtual void cancel();
  virtual void reset();
};

struct result_t
{
  actor_t* target;
  int type;
  bool hit;  // convenience
  bool crit; // convenience, two-roll (blocked crits, etc)
  double amount;
  ticker_t ticker;
};

struct ability_t : public action_t
{
  spell_data, resource, weapon, two_roll, self_cast, aoe, base_xyz (no player_xyz or target_xyz),
  direct_sp_coeff, direct_ap_coeff, tick_sp_coeff, tick_ap_coeff,
  harmful, healing, callbacks, std::vector<result_t> results,
  NO binary, NO repeating, NO direct_dmg, NO tick_dmg
  ability_t( spell_data_t* s ) : action_t( s -> name() ) {}
  virtual void  execute()
  {
    actor_t* targets[ AOE_CAP ];
    int num_targets = area_of_effect( targets );
    results.resize( num_targets );
    for ( int i=0; i < num_targets; i++ )
    {
      calculate_result( results[ i ], targets[ i ] );
      // "result" callbacks
    }
    consume_resource();
    for ( int i=0; i < num_targets; i++ )
    {
      schedule_travel( results[ i ] );
    }
    update_ready();
    // "cast" callbacks
  }
  virtual void impact( result_t& result )
  {
    if ( result.hit )
    {
      if ( result.amount > 0 )
      {
        if ( harmful )
        {
          assess_damage( result );
        }
        else if ( healing )
        {
          assess_healing( result );
        }
      }
      if ( num_ticks > 0 )
      {
        ticker_t* ticker = get_ticker( result.target );  // caches aura_slot
        ticker -> trigger( this, result.ticker );
        // ticker_t::trigger() handles dot work in existing action_t::impact()
      }
    }
    else
    {
      // miss msg
      stat -> add_result( result );
    }
  }
  virtual void   tick         ( ticker_t* );
  virtual void   last_tick    ( ticker_t* );
  virtual void   schedule_tick( ticker_t* );
  virtual int    calculate_num_ticks( double haste, double duration=0 );
  virtual double cost();
  virtual double haste();
  virtual bool   ready();
  virtual void   cancel();
  virtual void   reset();
  virtual void   consume_resource();
  virtual result_type calculate_attack_roll( actor_t* target );
  virtual result_type calculate_spell_roll( actor_t* target );
  virtual result_type calculate_roll( actor_t* target )
  {
    if ( weapon )
      return calculate_attack_roll( target );
    else
      return calculate_spell_roll( target );
  }
  virtual void calculate_result( result_t& result, actor_t* target )
  {
    result.type = calculate_roll( target );
    result.hit  = ( roll == ? );
    result.crit = ( roll == ? ) || ( two_roll );
    result.amount = calculate_direct_amount( target );
    if ( result.hit && num_ticks )
    {
      calculate_ticker( &( result.ticker ), target );
    }
    return result;
  }
  virtual void calculate_ticker( ticker_t* ticker, target )
  {
    if ( target ) ticker -> target = target;
    ticker -> amount  = calculate_tick_amount( ticker -> target );
    ticker -> crit    = calculate_crit_chance( ticker -> target );
    ticker -> haste   = calculate_haste      ( ticker -> target );
  }
  virtual void refresh_ticker( ticker_t* ticker )
  {
    calculate_ticker( ticker );
    ticker -> current_tick = 0;
    ticker -> added_ticks = 0;
    ticker -> added_time = 0;
    ticker -> num_ticks = calculate_num_ticks( ticker -> haste );
    ticker -> recalculate_finish();
  }
  virtual void extend_ticker_by_time( ticker_t* ticker, double extra_time )
  {
    int    full_tick_count   = ticker -> ticks() - 1;
    double full_tick_remains = ticker -> finish - ticker -> tick_event -> time;

    ticker -> haste = calculate_haste( ticker -> target );
    ticker -> num_ticks += calculate_num_ticks( ticker -> haste, full_tick_remains ) - full_tick_count;
    ticker -> recalculate_finish();
  }
  virtual void extend_ticker_by_ticks( ticker_t* ticker, double extra_ticks )
  {
    calculate_ticker( ticker );
    ticker -> added_ticks += extra_ticks;
    ticker -> num_ticks   += extra_ticks;
    ticker -> recalculate_finish();
  }
  virtual double calculate_weapon_amount( actor_t* target );
  virtual double calculate_direct_amount( actor_t* target );
  virtual double calculate_tick_amount  ( actor_t* target );
  virtual double calculate_power( actor_t* target )
  {
    return AP_multiplier * AP() + SP_multiplier * SP();
  }
  virtual double calculate_haste( actor_t* target )
  {
    if ( weapon )
      return calculate_attack_haste( target );
    else
      return calculate_spell_haste( target );
  }
  virtual double calculate_miss_chance( actor_t* target )
  {
    if ( weapon )
      return calculate_attack_miss_chance( target );
    else
      return calculate_spell_miss_chance( target );
  }
  virtual double calculate_dodge_chance( actor_t* target )
  {
    if ( weapon )
      return calculate_attack_dodge_chance( target );
    else
      return 0;
  }
  virtual double calculate_parry_chance( actor_t* target )
  {
    if ( weapon )
      return calculate_attack_parry_chance( target );
    else
      return 0;
  }
  virtual double calculate_block_chance( actor_t* target )
  {
    if ( weapon )
      return calculate_attack_block_chance( target );
    else
      return 0;
  }
  virtual double calculate_crit_chance( actor_t* target )
  {
    if ( weapon )
      return calculate_attack_crit_chance( target );
    else
      return calculate_spell_crit_chance( target );
  }
  virtual double calculate_crit_chance( ticker_t* ticker );
  virtual double calculate_source_multiplier();
  virtual double calculate_direct_multiplier() { return calculate_source_multiplier(); } // include crit bonus here
  virtual double calculate_tick_multiplier  () { return calculate_source_multiplier(); }
  virtual double calculate_target_multiplier( actor_t* target );
  virtual int       area_of_effect( actor_t* targets[] ) { targets[ 0 ] = self_cast ? actor : actor -> current_target; return 1; }
  virtual result_t& result(); // returns 0th "result", asserts if aoe
  virtual double    travel_time( actor_t* target );
  virtual void      schedule_travel( result_t& result );
  virtual void assess_damage( result_t& result )
  {
    result.amount *= calculate_target_multiplier( result.target ); // allows for action-specific target multipliers
    result.target -> assess_damage( result ); // adjust result as needed for flexibility
    stat -> add_result( result );
  }
  virtual void assess_healing( result_t& result )
  {
    result.amount *= calculate_target_multiplier( result.target ); // allows for action-specific target multipliers
    result.target -> assess_healing( result ); // adjust result as needed for flexibility
    stat -> add_result( result );
  }
};

#endif

#endif // __SIMULATIONCRAFT_H

