// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// TODO
// * Implement defense_rating, melee_avoidance(), range_avoidance(),
//   force_avoidance(), tech_avoidance().
// * Move all of the buffed_XXX stats into player_t::buffed.

namespace { // ANONYMOUS NAMESPACE ==========================================

// compare_talents ==========================================================

struct compare_talents
{
  bool operator()( const talent_t* l, const talent_t* r ) const
  {
    return ( l -> _tab_page < r -> _tab_page );
  }
};

// has_foreground_actions ===================================================

static bool has_foreground_actions( player_t* p )
{
  for ( action_t* a = p -> action_list; a; a = a -> next )
  {
    if ( ! a -> background ) return true;
  }
  return false;
}

// parse_talent_url =========================================================

static bool parse_talent_url( sim_t* sim,
                              const std::string& name,
                              const std::string& url )
{
  assert( name == "talents" ); ( void )name;

  player_t* p = sim -> active_player;

  p -> talents_str = url;

  std::string::size_type cut_pt;

  if ( url.find( "worldofwarcraft" ) != url.npos )
  {
    if ( ( cut_pt = url.find_first_of( '=' ) ) != url.npos )
    {
      return p -> parse_talents_armory( url.substr( cut_pt + 1 ) );
    }
  }

  else if ( url.find( "wowarmory" ) != url.npos )
  {
    if ( ( cut_pt = url.find_last_of( '=' ) ) != url.npos )
    {
      return p -> parse_talents_armory( url.substr( cut_pt + 1 ) );
    }
  }

  else if ( url.find( "wowhead" ) != url.npos || url.find( "torhead" ) != url.npos )
  {
    if ( ( cut_pt = url.find_first_of( "#=" ) ) != url.npos )
    {
      std::string::size_type cut_pt2 = url.find_first_of( '-', cut_pt + 1 );
      // Add support for http://www.wowhead.com/talent#priest-033211000000000000000000000000000000000000322032210201222100231
      if ( cut_pt2 != url.npos )
        return p -> parse_talents_armory( url.substr( cut_pt2 + 1 ) );
      else
        return p -> parse_talents_wowhead( url.substr( cut_pt + 4 ) );
    }
  }

  else if ( url.find( "knotor" ) != url.npos )
  {
    // http://knotor.com/skills#.....
    if ( ( cut_pt = url.find( '#' ) ) != url.npos )
      return knotor::parse_talents( p, url.substr( cut_pt + 1 ) );
  }

  else
  {
    bool all_digits = true;
    for ( size_t i=0; i < url.size() && all_digits; i++ )
      if ( ! isdigit( url[ i ] ) )
        all_digits = false;

    if ( all_digits )
    {
      return p -> parse_talents_armory( url );
    }
  }

  sim -> errorf( "Unable to decode talent string %s for %s\n", url.c_str(), p -> name() );

  return false;
}

// parse_role_string ========================================================

static bool parse_role_string( sim_t* sim,
                               const std::string& name,
                               const std::string& value )
{
  assert( name == "role" ); ( void )name;



  sim -> active_player -> role = util_t::parse_role_type( value );

  return true;
}

// parse_role_string ========================================================

static bool parse_set_bonus( sim_t* sim,
                               const std::string& name,
                               const std::string& value )
{
  assert( name == "set_bonus" ); ( void )name;

  std::string::size_type cut_pt;
  if ( ( cut_pt = value.find( "_2pc" ) ) != value.npos )
  {
    set_bonus_t* sb = sim -> active_player -> find_set_bonus( value.substr( 0, cut_pt ) );
    if ( sb && sb -> count < 2 )
      sb -> count = 2;
  }
  if ( ( cut_pt = value.find( "_4pc" ) ) != value.npos )
  {
    set_bonus_t* sb = sim -> active_player -> find_set_bonus( value.substr( 0, cut_pt ) );
    if ( sb && sb -> count < 4 )
      sb -> count = 4;
  }

  return true;
}

// parse_world_lag ==========================================================

static bool parse_world_lag( sim_t* sim,
                             const std::string& name,
                             const std::string& value )
{
  assert( name == "world_lag" ); ( void )name;

  sim -> active_player -> world_lag = timespan_t::from_seconds( atof( value.c_str() ) );

  if ( sim -> active_player -> world_lag < timespan_t::zero )
  {
    sim -> active_player -> world_lag = timespan_t::zero;
  }

  sim -> active_player -> world_lag_override = true;

  return true;
}


// parse_world_lag ==========================================================

static bool parse_world_lag_stddev( sim_t* sim,
                                    const std::string& name,
                                    const std::string& value )
{
  assert( name == "world_lag_stddev" ); ( void )name;

  sim -> active_player -> world_lag_stddev = timespan_t::from_seconds( atof( value.c_str() ) );

  if ( sim -> active_player -> world_lag_stddev < timespan_t::zero )
  {
    sim -> active_player -> world_lag_stddev = timespan_t::zero;
  }

  sim -> active_player -> world_lag_stddev_override = true;

  return true;
}

// parse_brain_lag ==========================================================

static bool parse_brain_lag( sim_t* sim,
                             const std::string& name,
                             const std::string& value )
{
  assert( name == "brain_lag" ); ( void )name;

  sim -> active_player -> brain_lag = timespan_t::from_seconds( atof( value.c_str() ) );

  if ( sim -> active_player -> brain_lag < timespan_t::zero )
  {
    sim -> active_player -> brain_lag = timespan_t::zero;
  }

  return true;
}


// parse_brain_lag_stddev ===================================================

static bool parse_brain_lag_stddev( sim_t* sim,
                                    const std::string& name,
                                    const std::string& value )
{
  assert( name == "brain_lag_stddev" ); ( void )name;

  sim -> active_player -> brain_lag_stddev = timespan_t::from_seconds( atof( value.c_str() ) );

  if ( sim -> active_player -> brain_lag_stddev < timespan_t::zero )
  {
    sim -> active_player -> brain_lag_stddev = timespan_t::zero;
  }

  return true;
}

// The wowhead encoding that represents pairs of integers in [0..5] with a
// single character. The character at index i in this array encodes the pair
// ( floor( i / 6 ), i % 6 ).
const char wowhead_talent_encoding[] =
{
  '0', 'o', 'b', 'h', 'L', 'x',
  'z', 'k', 'd', 'u', 'p', 't',
  'M', 'R', 'r', 'G', 'T', 'g',
  'c', 's', 'f', 'I', 'j', 'e',
  'm', 'a', 'w', 'N', 'n', 'v',
  'V', 'q', 'i', 'A', 'y', 'E',
};

// An exception for wowhead_talent_decode_pair to throw on invalid input.
struct wowhead_bad_talent_encoding
{
  char c;
  wowhead_bad_talent_encoding( char c ) : c( c ) {}
};

char wowhead_talent_encode_pair( int first, int second=0 )
{
  assert( 0 <= first  && first <= 5 );
  assert( 0 <= second && second <= 5 );
  return wowhead_talent_encoding[ 6 * first + second ];
}

std::pair<int, int> wowhead_talent_decode_pair( char c )
{
  for( unsigned i = 0; i < sizeof( wowhead_talent_encoding ) / sizeof( wowhead_talent_encoding[ 0 ] ); ++i)
  {
    if ( wowhead_talent_encoding[ i ] == c )
    {
      return std::make_pair( i / 6, i % 6 );
    }
  }

  throw wowhead_bad_talent_encoding( c );
}

std::string wowhead_encode_tree( const std::vector<talent_t*>& tree )
{
  std::string result;

  unsigned i = 0;
  while( i < tree.size() )
  {
    int first = 0;
    if ( tree[ i ] )
      first = tree[ i ] -> rank();
    ++i;

    int second = 0;
    if ( i < tree.size() && tree[ i ] )
      second = tree[ i ] -> rank();
    ++i;

    result += wowhead_talent_encode_pair( first, second );
  }

  unsigned length = result.size();
  while ( length > 0 && result[ length - 1 ] == '0' )
    --length;
  result.resize( length );

  return result;
}

const char* torhead_ac_string( player_type pt )
{
  switch( pt )
  {
  case SITH_MARAUDER:
    return "100";
  case SITH_JUGGERNAUT:
    return "101";
  case SITH_ASSASSIN:
    return "200";
  case SITH_SORCERER:
    return "201";
  case BH_MERCENARY:
    return "300";
  case BH_POWERTECH:
    return "301";
  case IA_SNIPER:
    return "400";
  case IA_OPERATIVE:
    return "401";

  case JEDI_GUARDIAN:
    return "500";
  case JEDI_SENTINEL:
    return "501";
  case JEDI_SAGE:
    return "600";
  case JEDI_SHADOW:
    return "601";
  case S_GUNSLINGER:
    return "700";
  case S_SCOUNDREL:
    return "701";
  case T_COMMANDO:
    return "800";
  case T_VANGUARD:
    return "801";

  default:
    return "0";
  }
}

std::string torhead_encode_talents( const player_t& p )
{
  std::string encoding;

  if ( const char* ac_code = torhead_ac_string( p.type ) )
  {
    std::stringstream ss;

    ss << "http://www.torhead.com/skill-calc#" << ac_code;

    // This is necessary because sometimes the talent trees change shape between live/ptr.
    for ( unsigned i=0; i < sizeof_array( p.talent_trees ); ++i )
    {
      if ( i > 0 ) ss << 'Z';
      ss << wowhead_encode_tree( p.talent_trees[ i ] );
    }

    ss << ".1";
    encoding = ss.str();
  }

  return encoding;
}

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Player
// ==========================================================================

// player_t::player_t =======================================================

player_t::player_t( sim_t*             s,
                    player_type        t,
                    const std::string& n,
                    race_type          r ) :
  sim( s ), name_str( n ),
  region_str( s -> default_region_str ), server_str( s -> default_server_str ), origin_str( "unknown" ),
  next( 0 ), index( -1 ), type( t ), role( ROLE_HYBRID ), target( 0 ), level( 50 ), use_pre_potion( 0 ),
  party( 0 ), member( 0 ),
  skill( 0 ), initial_skill( s -> default_skill ), distance( 0 ), default_distance( 0 ), gcd_ready( timespan_t::zero ), base_gcd( timespan_t::from_seconds( 1.5 ) ),
  potion_used( 0 ), sleeping( 1 ), initial_sleeping( 0 ), initialized( 0 ),
  pet_list( 0 ), bugs( true ), specialization( TALENT_TAB_NONE ), invert_scaling( 0 ),
  vengeance_enabled( false ), vengeance_damage( 0.0 ), vengeance_value( 0.0 ), vengeance_max( 0.0 ), vengeance_was_attacked( false ),
  active_pets( 0 ),
  reaction_mean( timespan_t::from_seconds( 0.5 ) ), reaction_stddev( timespan_t::zero ), reaction_nu( timespan_t::from_seconds( 0.5 ) ),
  scale_player( 1 ), avg_ilvl( 0 ), active_companion( 0 ),
  // Latency
  world_lag( timespan_t::from_seconds( 0.1 ) ), world_lag_stddev( timespan_t::min ),
  brain_lag( timespan_t::min ), brain_lag_stddev( timespan_t::min ),
  world_lag_override( false ), world_lag_stddev_override( false ),
  events( 0 ),
  dbc( s -> dbc ),
  race( r ),
  // Ratings
  initial_alacrity_rating( 0 ), alacrity_rating( 0 ),
  initial_crit_rating( 0 ), crit_rating( 0 ),
  initial_accuracy_rating( 0 ), accuracy_rating( 0 ),
  initial_surge_rating( 0 ), surge_rating( 0 ),
  initial_defense_rating( 0 ), defense_rating( 0 ),
  surge_bonus( 0 ), buffed_surge( 0 ),
  primary_attribute( ATTRIBUTE_NONE ), secondary_attribute( ATTRIBUTE_NONE ),
  // Spell Mechanics
  base_power( 0 ),       initial_power( 0 ),       power( 0 ),
  base_force_power( 0 ), initial_force_power( 0 ), force_power( 0 ),
  base_tech_power( 0 ),  initial_tech_power( 0 ),  tech_power( 0 ),
  base_energy_regen_per_second( 0 ), base_ammo_regen_per_second( 0 ), base_force_regen_per_second( 0 ),
  position( POSITION_BACK ),

  // Defense Mechanics
  target_auto_attack( 0 ),
  base_armor( 0 ),       initial_armor( 0 ),       armor( 0 ),       buffed_armor( 0 ),
  base_bonus_armor( 0 ), initial_bonus_armor( 0 ), bonus_armor( 0 ),
  armor_multiplier( 1.0 ), initial_armor_multiplier( 1.0 ),
  armor_coeff( 0 ),

  base_melee_avoidance( .05 ), base_range_avoidance( .05 ),
  base_force_avoidance( 0 ),   base_tech_avoidance( 0 ),

  base_miss( 0 ),        initial_miss( 0 ),        miss( 0 ),        buffed_miss( 0 ),
  base_dodge( 0 ),       initial_dodge( 0 ),       dodge( 0 ),       buffed_dodge( 0 ),
  base_parry( 0 ),       initial_parry( 0 ),       parry( 0 ),       buffed_parry( 0 ),
  base_block( 0 ),       initial_block( 0 ),       block( 0 ),       buffed_block( 0 ),
  base_block_reduction( 0.3 ), initial_block_reduction( 0 ), block_reduction( 0 ),

  // Resources
  health_per_endurance( 10 ),
  // Consumables
  stim( STIM_NONE ),
  // Events
  executing( 0 ), channeling( 0 ), readying( 0 ), off_gcd( 0 ), in_combat( false ), action_queued( false ),
  cast_delay_reaction( timespan_t::zero ), cast_delay_occurred( timespan_t::zero ),
  // Actions
  action_list( 0 ), action_list_default( 0 ), cooldown_list( 0 ), dot_list( 0 ),
  // Reporting
  quiet( 0 ), last_foreground_action( 0 ),
  iteration_fight_length( timespan_t::zero ), arise_time( timespan_t::zero ),
  fight_length( s -> statistics_level < 2, true ), waiting_time( true ), executed_foreground_actions( s -> statistics_level < 3 ),
  iteration_waiting_time( timespan_t::zero ), iteration_executed_foreground_actions( 0 ),
  rps_gain( 0 ), rps_loss( 0 ),
  deaths(), deaths_error( 0 ),
  buffed( buffed_stats_t() ),
  buff_list( 0 ), proc_list( 0 ), gain_list( 0 ), stats_list( 0 ), benefit_list( 0 ), uptime_list( 0 ),set_bonus_list( 0 ),
  // Damage
  iteration_dmg( 0 ), iteration_dmg_taken( 0 ),
  dps_error( 0 ), dpr( 0 ), dtps_error( 0 ),
  dmg( s -> statistics_level < 2 ), compound_dmg( s -> statistics_level < 2 ),
  dps( s -> statistics_level < 1 ), dpse( s -> statistics_level < 2 ),
  dtps( s -> statistics_level < 2 ), dmg_taken( s -> statistics_level < 2 ),
  // Heal
  iteration_heal( 0 ),iteration_heal_taken( 0 ),
  hps_error( 0 ), hpr( 0 ),
  heal( s -> statistics_level < 2 ), compound_heal( s -> statistics_level < 2 ),
  hps( s -> statistics_level < 1 ), hpse( s -> statistics_level < 2 ),
  htps( s -> statistics_level < 2 ), heal_taken( s -> statistics_level < 2 ),
  // Scaling
  scaling_lag( 0 ), scaling_lag_error( 0 ),
  // Movement & Position
  base_movement_speed( 7.0 ), x_position( 0.0 ), y_position( 0.0 ),
  buffs( buffs_t() ), debuffs( debuffs_t() ), gains( gains_t() ), rng_list( 0 ), rngs( rngs_t() ),
  set_bonus( set_bonuses_t() ), targetdata_id( -1 )
{
  sim -> actor_list.push_back( this );

  if ( type != ENEMY && type != ENEMY_ADD )
  {
    if ( sim -> debug ) log_t::output( sim, "Creating Player %s", name() );
    player_t** last = &( sim -> player_list );
    while ( *last ) last = &( ( *last ) -> next );
    *last = this;
    next = 0;
    index = ++( sim -> num_players );
  }
  else
  {
    index = - ( ++( sim -> num_enemies ) );
  }

  race_str = util_t::race_type_string( race );

  if ( is_pet() ) skill = 1.0;

  range::fill( attribute, 0 );
  range::fill( attribute_base, 0 );
  range::fill( attribute_initial, 0 );

  range::fill( attribute_multiplier, 1 );
  range::fill( attribute_multiplier_initial, 1 );

  range::fill( infinite_resource, false );
  infinite_resource[ RESOURCE_HEALTH ] = true;

  range::fill( resource_reduction, 0 );
  range::fill( initial_resource_reduction, 0 );

  range::fill( resource_base, 0 );
  range::fill( resource_initial, 0 );
  range::fill( resource_max, 0 );
  range::fill( resource_current, 0 );
  range::fill( resource_lost, 0 );
  range::fill( resource_gained, 0 );

  range::fill( profession, 0 );

  range::fill( scales_with, 0 );
  range::fill( over_cap, 0 );

  items.resize( SLOT_MAX );
  for ( int i=0; i < SLOT_MAX; i++ )
  {
    items[ i ].slot = i;
    items[ i ].sim = sim;
    items[ i ].player = this;
  }

  main_hand_weapon.slot = SLOT_MAIN_HAND;
  off_hand_weapon.slot = SLOT_OFF_HAND;
  ranged_weapon.slot = SLOT_RANGED;

  if ( ! sim -> active_files.empty() ) origin_str = sim -> active_files.top();

  range::fill( talent_tab_points, 0 );
  range::fill( tree_type, TREE_NONE );

  if ( reaction_stddev == timespan_t::zero ) reaction_stddev = reaction_mean * 0.25;

  set_bonus.rakata_force_masters = get_set_bonus( "rakata_force_masters", "tionese_force_masters_/columi_force_masters_/rakata_force_masters_" );
  set_bonus.battlemaster_force_masters = get_set_bonus( "battlemaster_force_masters", "centurion_force_masters_/champion_force_masters_/battlemaster_force_masters_" );
  set_bonus.rakata_stalkers = get_set_bonus( "rakata_stalkers", "tionese_stalkers_/columi_stalkers_/rakata_stalkers_" );
}

// player_t::~player_t ======================================================

player_t::~player_t()
{
  range::dispose( targetdata );

  while ( action_t* a = action_list )
  {
    action_list = a -> next;
    delete a;
  }
  while ( proc_t* p = proc_list )
  {
    proc_list = p -> next;
    delete p;
  }
  while ( gain_t* g = gain_list )
  {
    gain_list = g -> next;
    delete g;
  }
  while ( stats_t* s = stats_list )
  {
    stats_list = s -> next;
    delete s;
  }
  while ( uptime_t* u = uptime_list )
  {
    uptime_list = u -> next;
    delete u;
  }
  while ( benefit_t* u = benefit_list )
  {
    benefit_list = u -> next;
    delete u;
  }
  while ( rng_t* r = rng_list )
  {
    rng_list = r -> next;
    delete r;
  }
  while ( dot_t* d = dot_list )
  {
    dot_list = d -> next;
    delete d;
  }
  while ( buff_t* d = buff_list )
  {
    buff_list = d -> next;
    delete d;
  }
  while ( cooldown_t* d = cooldown_list )
  {
    cooldown_list = d -> next;
    delete d;
  }

  if ( false )
  {
    // FIXME! This cannot be done until we use refcounts.
    // FIXME! I see the same callback pointer being registered multiple times.
    range::dispose( all_callbacks );
  }

  for ( size_t i=0; i < sizeof_array( talent_trees ); i++ )
    range::dispose( talent_trees[ i ] );

  //range::dispose( glyphs );
  range::dispose( spell_list );
}

// player_t::init ===========================================================

bool player_t::init( sim_t* sim )
{
  if ( sim -> debug )
    log_t::output( sim, "Creating Pets." );

  for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    p -> create_pets();
  }

  if ( sim -> debug )
    log_t::output( sim, "Initializing Auras, Buffs, and De-Buffs." );

  player_t::jedi_sage_init( sim );
  player_t::shadow_assassin_init( sim );
  player_t::enemy_init( sim );

  if ( sim -> debug )
    log_t::output( sim, "Initializing Players." );

  bool too_quiet = true; // Check for at least 1 active player
  bool zero_dds = true; // Check for at least 1 player != TANK/HEAL

  for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    if ( sim -> default_actions && ! p -> is_pet() ) p -> action_list_str.clear();
    p -> init();
    if ( ! p -> quiet ) too_quiet = false;
    if ( p -> primary_role() != ROLE_HEAL && p -> primary_role() != ROLE_TANK && ! p -> is_pet() ) zero_dds = false;
  }

  if ( too_quiet && ! sim -> debug )
  {
    sim -> errorf( "No active players in sim!" );
    return false;
  }

  // Set Fixed_Time when there are no DD's present
  if ( zero_dds && ! sim -> debug )
    sim -> fixed_time = true;

  // Parties
  if ( sim -> debug )
    log_t::output( sim, "Building Parties." );

  int party_index=0;
  for ( unsigned i=0; i < sim -> party_encoding.size(); i++ )
  {
    std::string& party_str = sim -> party_encoding[ i ];

    if ( party_str == "reset" )
    {
      party_index = 0;
      for ( player_t* p = sim -> player_list; p; p = p -> next ) p -> party = 0;
    }
    else if ( party_str == "all" )
    {
      int member_index = 0;
      for ( player_t* p = sim -> player_list; p; p = p -> next )
      {
        p -> party = 1;
        p -> member = member_index++;
      }
    }
    else
    {
      party_index++;

      std::vector<std::string> player_names;
      int num_players = util_t::string_split( player_names, party_str, ",;/" );
      int member_index=0;

      for ( int j=0; j < num_players; j++ )
      {
        player_t* p = sim -> find_player( player_names[ j ] );
        if ( ! p )
        {
          sim -> errorf( "Unable to find player %s for party creation.\n", player_names[ j ].c_str() );
          return false;
        }
        p -> party = party_index;
        p -> member = member_index++;
        for ( pet_t* pet = p -> pet_list; pet; pet = pet -> next_pet )
        {
          pet -> party = party_index;
          pet -> member = member_index++;
        }
      }
    }
  }

  // Callbacks
  if ( sim -> debug )
    log_t::output( sim, "Registering Callbacks." );

  for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    p -> register_callbacks();
  }

  return true;
}

// player_t::init ===========================================================

void player_t::init()
{
  if ( sim -> debug ) log_t::output( sim, "Initializing player %s", name() );

  initialized = 1;
  init_target();
  init_talents();
  init_spells();
  init_race();
  init_base();
  init_racials();
  init_position();
  init_professions();
  init_items();
  init_rating();
  init_core();
  init_spell();
  init_attack();
  init_defense();
  init_weapon( &main_hand_weapon );
  init_weapon( &off_hand_weapon );
  init_weapon( &ranged_weapon );
  init_professions_bonus();
  init_unique_gear();
  init_enchant();
  init_scaling();
  init_buffs();
  init_values();
  init_actions();
  init_gains();
  init_procs();
  init_uptimes();
  init_benefits();
  init_rng();
  init_stats();
}

// player_t::init_base ======================================================

void player_t::init_base()
{
  if ( sim -> debug ) log_t::output( sim, "Initializing base for player (%s)", name() );

  attribute_base[ ATTR_STRENGTH    ] =
    attribute_base[ ATTR_AIM       ] =
    attribute_base[ ATTR_CUNNING   ] =
    attribute_base[ ATTR_WILLPOWER ] = floor( 10 + 0.8 * level );
  attribute_base[ ATTR_ENDURANCE   ] =
    attribute_base[ ATTR_PRESENCE  ] = floor( 45 + 3.6 * level );

  assert( primary_attribute != secondary_attribute );
  attribute_base[ primary_attribute   ] = floor( 50 + 4 * level );
  attribute_base[ secondary_attribute ] = floor( 20 + 1.6 * level );

  resource_base[ RESOURCE_HEALTH ] = rating_t::get_base_health( level );

  /*
  attribute_base[ ATTR_STRENGTH  ] = rating_t::get_attribute_base( sim, dbc, level, type, race, BASE_STAT_STRENGTH );
  attribute_base[ ATTR_AGILITY   ] = rating_t::get_attribute_base( sim, dbc, level, type, race, BASE_STAT_AGILITY );
  attribute_base[ ATTR_STAMINA   ] = rating_t::get_attribute_base( sim, dbc, level, type, race, BASE_STAT_STAMINA );
  attribute_base[ ATTR_INTELLECT ] = rating_t::get_attribute_base( sim, dbc, level, type, race, BASE_STAT_INTELLECT );
  attribute_base[ ATTR_SPIRIT    ] = rating_t::get_attribute_base( sim, dbc, level, type, race, BASE_STAT_SPIRIT );
  resource_base[ RESOURCE_HEALTH ] = rating_t::get_attribute_base( sim, dbc, level, type, race, BASE_STAT_HEALTH );
  resource_base[ RESOURCE_MANA   ] = rating_t::get_attribute_base( sim, dbc, level, type, race, BASE_STAT_MANA );
  initial_spell_crit_per_intellect = rating_t::get_attribute_base( sim, dbc, level, type, race, BASE_STAT_SPELL_CRIT_PER_INT );
  initial_attack_crit_per_agility  = rating_t::get_attribute_base( sim, dbc, level, type, race, BASE_STAT_MELEE_CRIT_PER_AGI );
  base_mp5                         = rating_t::get_attribute_base( sim, dbc, level, type, race, BASE_STAT_MP5 );
*/

  if ( world_lag_stddev < timespan_t::zero  ) world_lag_stddev = world_lag * 0.1;
  if ( brain_lag_stddev < timespan_t::zero  ) brain_lag_stddev = brain_lag * 0.1;
}

// player_t::init_items =====================================================

void player_t::init_items()
{
  if ( is_pet() ) return;

  if ( sim -> debug ) log_t::output( sim, "Initializing items for player (%s)", name() );

  std::vector<std::string> splits;
  int num_splits = util_t::string_split( splits, items_str, "/" );
  int num_ilvl_items = 0;
  for ( int i=0; i < num_splits; i++ )
  {
    if ( find_item( splits[ i ] ) )
    {
      sim -> errorf( "Player %s has multiple %s equipped.\n", name(), splits[ i ].c_str() );
    }
    items.push_back( item_t( this, splits[ i ] ) );
  }

  gear_stats_t item_stats;

  bool slots[ SLOT_MAX ];
  for ( int i = 0; i < SLOT_MAX; i++ )
  {
    if ( util_t::armor_type_string( type, i ) )
    {
      slots[ i ] = false;
    }
    else
    {
      slots[ i ] = true;
    }
  }

  int num_items = ( int ) items.size();
  for ( int i=0; i < num_items; i++ )
  {
    // If the item has been specified in options we want to start from scratch, forgetting about lingering stuff from profile copy
    if ( items[ i ].options_str != "" )
    {
      items[ i ] = item_t( this, items[ i ].options_str );
      items[ i ].slot = i;
    }

    item_t& item = items[ i ];

    if ( ! item.init() )
    {
      sim -> errorf( "Unable to initialize item '%s' on player '%s'\n", item.name(), name() );
      sim -> cancel();
      return;
    }

    if ( item.slot != SLOT_SHIRT && item.slot != SLOT_TABARD && item.active() )
    {
      avg_ilvl += item.ilevel;
      num_ilvl_items++;
    }

    slots[ item.slot ] = item.matching_type();

    for ( int j=0; j < STAT_MAX; j++ )
    {
      item_stats.add_stat( j, item.stats.get_stat( j ) );
    }
  }

  if ( num_ilvl_items > 1 )
    avg_ilvl /= num_ilvl_items;

  for ( int i=0; i < STAT_MAX; i++ )
  {
    if ( gear.get_stat( i ) == 0 )
    {
      gear.set_stat( i, item_stats.get_stat( i ) );
    }
  }

  if ( sim -> debug )
  {
    log_t::output( sim, "%s gear:", name() );
    gear.print( sim -> output_file );
  }

  for ( set_bonus_t* sb = set_bonus_list; sb; sb = sb -> next )
  {
    sb -> init( *this );
  }
}

// player_t::init_core ======================================================

void player_t::init_core()
{
  if ( sim -> debug ) log_t::output( sim, "Initializing core for player (%s)", name() );

  initial_stats.  hit_rating = gear.  hit_rating + enchant.  hit_rating + ( is_pet() ? 0 : sim -> enchant.  hit_rating );
  initial_stats. crit_rating = gear. crit_rating + enchant. crit_rating + ( is_pet() ? 0 : sim -> enchant. crit_rating );
  initial_stats.alacrity_rating = gear.alacrity_rating + enchant.alacrity_rating + ( is_pet() ? 0 : sim -> enchant.alacrity_rating );
  initial_stats.surge_rating = gear.surge_rating + enchant.surge_rating + ( is_pet() ? 0 : sim -> enchant.surge_rating );

  initial_alacrity_rating    = initial_stats.alacrity_rating;
  initial_crit_rating     = initial_stats.crit_rating;
  initial_accuracy_rating = initial_stats.hit_rating;
  initial_surge_rating    = initial_stats.surge_rating;



  for ( int i=0; i < ATTRIBUTE_MAX; i++ )
  {
    initial_stats.attribute[ i ] = gear.attribute[ i ] + enchant.attribute[ i ] + ( is_pet() ? 0 : sim -> enchant.attribute[ i ] );

    attribute[ i ] = attribute_initial[ i ] = attribute_base[ i ] + initial_stats.attribute[ i ];
  }
}

// player_t::init_position ==================================================

void player_t::init_position()
{
  if ( sim -> debug ) log_t::output( sim, "Initializing position for player (%s)", name() );

  if ( position_str.empty() )
  {
    position_str = util_t::position_type_string( position );
  }
  else
  {
    position = util_t::parse_position_type( position_str );
  }

  // default to back when we have an invalid position
  if ( position == POSITION_NONE )
  {
    sim -> errorf( "Player %s has an invalid position of %s, defaulting to back.\n", name(), position_str.c_str() );
    position = POSITION_BACK;
    position_str = util_t::position_type_string( position );
  }
}

// player_t::init_race ======================================================

void player_t::init_race()
{
  if ( sim -> debug ) log_t::output( sim, "Initializing race for player (%s)", name() );

  if ( race_str.empty() )
  {
    race_str = util_t::race_type_string( race );
  }
  else
  {
    race = util_t::parse_race_type( race_str );
  }
}

// player_t::init_racials ===================================================

void player_t::init_racials()
{
  if ( sim -> debug ) log_t::output( sim, "Initializing racials for player (%s)", name() );

}

// player_t::init_spell =====================================================

void player_t::init_spell()
{
  if ( sim -> debug ) log_t::output( sim, "Initializing spells for player (%s)", name() );

  initial_stats.power       = gear.power       + enchant.power       + ( is_pet() ? 0 : sim -> enchant.power );
  initial_stats.force_power = gear.force_power + enchant.force_power + ( is_pet() ? 0 : sim -> enchant.force_power );
  initial_stats.tech_power  = gear.tech_power  + enchant.tech_power  + ( is_pet() ? 0 : sim -> enchant.tech_power );

  initial_power       = base_power       + initial_stats.power;
  initial_force_power = base_force_power + initial_stats.force_power;
  initial_tech_power =  base_tech_power  + initial_stats.tech_power;
}

// player_t::init_attack ====================================================

void player_t::init_attack()
{
  if ( sim -> debug ) log_t::output( sim, "Initializing attack for player (%s)", name() );

  initial_stats.expertise_rating = gear.expertise_rating + enchant.expertise_rating + ( is_pet() ? 0 : sim -> enchant.expertise_rating );

  double a,b;
  a = 200.0;
  b = 800.0;

  armor_coeff = a * level + b;
}

// player_t::init_defense ===================================================

void player_t::init_defense()
{
  if ( sim -> debug ) log_t::output( sim, "Initializing defense for player (%s)", name() );

  base_melee_avoidance = base_range_avoidance = .05;
  base_force_avoidance = base_tech_avoidance = 0;

  if ( type != ENEMY && type != ENEMY_ADD )
    base_dodge = dbc.dodge_base( type );

  initial_stats.armor          = gear.armor          + enchant.armor          + ( is_pet() ? 0 : sim -> enchant.armor );
  initial_stats.bonus_armor    = gear.bonus_armor    + enchant.bonus_armor    + ( is_pet() ? 0 : sim -> enchant.bonus_armor );
  initial_stats.dodge_rating   = gear.dodge_rating   + enchant.dodge_rating   + ( is_pet() ? 0 : sim -> enchant.dodge_rating );
  initial_stats.parry_rating   = gear.parry_rating   + enchant.parry_rating   + ( is_pet() ? 0 : sim -> enchant.parry_rating );
  initial_stats.block_rating   = gear.block_rating   + enchant.block_rating   + ( is_pet() ? 0 : sim -> enchant.block_rating );

  initial_armor             = base_armor       + initial_stats.armor;
  initial_bonus_armor       = base_bonus_armor + initial_stats.bonus_armor;
  initial_miss              = base_miss;
  initial_dodge             = base_dodge       + initial_stats.dodge_rating / rating.dodge;
  initial_parry             = base_parry       + initial_stats.parry_rating / rating.parry;
  initial_block             = base_block       + initial_stats.block_rating / rating.block;
  initial_block_reduction   = base_block_reduction;

  initial_melee_avoidance   = base_melee_avoidance + rating_t::defense_from_rating( defense_rating, level );
  initial_range_avoidance   = base_range_avoidance + rating_t::defense_from_rating( defense_rating, level );
  initial_force_avoidance   = base_force_avoidance;
  initial_tech_avoidance    = base_tech_avoidance;

  if ( primary_role() == ROLE_TANK ) position = POSITION_FRONT;
}

// player_t::init_weapon ====================================================

void player_t::init_weapon( weapon_t* w )
{
  if ( w -> type == WEAPON_NONE ) return;

  if ( w -> slot == SLOT_MAIN_HAND ) assert( w -> type >= WEAPON_NONE && w -> type < WEAPON_2H );
  if ( w -> slot == SLOT_OFF_HAND  ) assert( w -> type >= WEAPON_NONE && w -> type < WEAPON_2H );
  if ( w -> slot == SLOT_RANGED    ) assert( w -> type > WEAPON_2H && w -> type < WEAPON_RANGED );
}

// player_t::init_unique_gear ===============================================

void player_t::init_unique_gear()
{
  if ( sim -> debug ) log_t::output( sim, "Initializing unique gear for player (%s)", name() );
  unique_gear_t::init( this );
}

// player_t::init_enchant ===================================================

void player_t::init_enchant()
{
  if ( sim -> debug ) log_t::output( sim, "Initializing enchants for player (%s)", name() );
  enchant_t::init( this );
}

// player_t::init_resources =================================================

void player_t::init_resources( bool force )
{
  if ( sim -> debug ) log_t::output( sim, "Initializing resources for player (%s)", name() );
  // The first 20pts of intellect/stamina only provide 1pt of mana/health.

  for ( int i=0; i < RESOURCE_MAX; i++ )
  {
    if ( force || resource_initial[ i ] == 0 )
    {
      resource_initial[ i ] = resource_base[ i ] + gear.resource[ i ] + enchant.resource[ i ] + ( is_pet() ? 0 : sim -> enchant.resource[ i ] );

      if ( i == RESOURCE_HEALTH )
      {
        resource_initial[ i ] += floor( endurance() ) * health_per_endurance;
      }
    }
    resource_current[ i ] = resource_max[ i ] = resource_initial[ i ];
  }

  if ( timeline_resource.empty() )
  {
    timeline_resource.resize( RESOURCE_MAX );
    timeline_resource_chart.resize( RESOURCE_MAX );

    int size = ( int ) ( sim -> max_time.total_seconds() * ( 1.0 + sim -> vary_combat_length ) );
    if ( size <= 0 ) size = 600; // Default to 10 minutes
    size *= 2;
    size += 3; // Buffer against rounding.

    for ( int i = RESOURCE_NONE; i < RESOURCE_MAX; i++ )
      timeline_resource[i].assign( size, 0 );
  }
}

// player_t::init_professions ===============================================

void player_t::init_professions()
{
  if ( professions_str.empty() ) return;

  if ( sim -> debug ) log_t::output( sim, "Initializing professions for player (%s)", name() );

  std::vector<std::string> splits;
  int size = util_t::string_split( splits, professions_str, ",/" );

  for ( int i=0; i < size; i++ )
  {
    std::string prof_name;
    int prof_value=0;

    if ( 2 != util_t::string_split( splits[ i ], "=", "S i", &prof_name, &prof_value ) )
    {
      prof_name  = splits[ i ];
      prof_value = 525;
    }

    int prof_type = util_t::parse_profession_type( prof_name );
    if ( prof_type == PROFESSION_NONE )
    {
      sim -> errorf( "Invalid profession encoding: %s\n", professions_str.c_str() );
      return;
    }

    profession[ prof_type ] = prof_value;
  }
}

// player_t::init_professions_bonus =========================================

void player_t::init_professions_bonus()
{
  if ( sim -> debug ) log_t::output( sim, "Initializing professions bonuses for player (%s)", name() );

  // This has to be called after init_attack() and init_core()

}

// Execute Pet Action =======================================================

struct execute_pet_action_t : public action_t
{
  action_t* pet_action;
  pet_t* pet;
  std::string action_str;

  execute_pet_action_t( player_t* player, pet_t* p, const std::string& as, const std::string& options_str ) :
    action_t( ACTION_OTHER, "execute_pet_action", player ), pet_action( 0 ), pet( p ), action_str( as )
  {
    parse_options( NULL, options_str );
    trigger_gcd = timespan_t::zero;
  }

  virtual void reset()
  {
    for ( action_t* action = pet -> action_list; action; action = action -> next )
    {
      if ( action -> name_str == action_str )
      {
        action -> background = true;
        pet_action = action;
      }
    }
  }

  virtual void execute()
  {
    pet_action -> execute();
  }

  virtual bool ready()
  {
    if ( ! action_t::ready() ) return false;
    if ( pet_action -> player -> sleeping ) return false;
    return pet_action -> ready();
  }
};

// player_t::init_target ====================================================

void player_t::init_target()
{
  if ( ! target_str.empty() )
  {
    target = sim -> find_player( target_str );
  }
  if ( ! target )
  {
    target = sim -> target;
  }
}

// player_t::init_use_item_actions ==========================================

std::string player_t::init_use_item_actions( const std::string& append )
{
  std::string buffer;
  int num_items = ( int ) items.size();
  for ( int i=0; i < num_items; i++ )
  {
    if ( items[ i ].use.active() )
    {
      buffer += "/use_item,name=";
      buffer += items[ i ].name();
      if ( ! append.empty() )
      {
        buffer += append;
      }
    }
  }

  return buffer;
}

// player_t::init_use_profession_actions ====================================

std::string player_t::init_use_profession_actions( const std::string& /* append */ )
{
  std::string buffer;
  // Lifeblood

  return buffer;
}

// player_t::init_use_racial_actions ========================================

std::string player_t::init_use_racial_actions( const std::string& append )
{
  std::string buffer;
  bool race_action_found = false;


  if ( race_action_found && ! append.empty() )
  {
    buffer += append;
  }

  return buffer;
}

// player_t::init_actions ===================================================

void player_t::init_actions()
{
  if ( sim -> debug ) log_t::output( sim, "Initializing actions for player (%s)", name() );

  if ( ! choose_action_list.empty() )
  {
    action_priority_list_t* chosen_action_list = find_action_priority_list( choose_action_list );

    if ( chosen_action_list )
      action_list_str = chosen_action_list -> action_list_str;
    else if ( choose_action_list != "default" )
    {
      sim -> errorf( "Action List %s not found, using default action list.\n", choose_action_list.c_str() );
      action_priority_list_t* default_action_list = find_action_priority_list( "default" );
      if ( default_action_list )
        action_list_str = default_action_list -> action_list_str;
      else if ( action_list_str.empty() )
        sim -> errorf( "No Default Action List available.\n" );
    }
  }
  else if ( action_list_str.empty() )
  {
    action_priority_list_t* chosen_action_list = find_action_priority_list( "default" );
    if ( chosen_action_list )
      action_list_str = chosen_action_list -> action_list_str;
  }

  if ( ! action_list_str.empty() )
  {
    if ( action_list_default && sim -> debug ) log_t::output( sim, "Player %s using default actions", name() );

    if ( sim -> debug ) log_t::output( sim, "Player %s: action_list_str=%s", name(), action_list_str.c_str() );

    std::string modify_action_options = "";

    if ( ! modify_action.empty() )
    {
      std::string::size_type cut_pt = modify_action.find( "," );

      if ( cut_pt != modify_action.npos )
      {
        modify_action_options = modify_action.substr( cut_pt + 1 );
        modify_action         = modify_action.substr( 0, cut_pt );
      }
    }

    std::vector<std::string> splits;
    int num_splits = util_t::string_split( splits, action_list_str, "/" );

    for ( int i=0; i < num_splits; i++ )
    {
      std::string action_name    = splits[ i ];
      std::string action_options = "";

      std::string::size_type cut_pt = action_name.find( "," );

      if ( cut_pt != action_name.npos )
      {
        action_options = action_name.substr( cut_pt + 1 );
        action_name    = action_name.substr( 0, cut_pt );
      }

      action_t* a=0;

      cut_pt = action_name.find( ":" );
      if ( cut_pt != action_name.npos )
      {
        std::string pet_name   = action_name.substr( 0, cut_pt );
        std::string pet_action = action_name.substr( cut_pt + 1 );

        pet_t* pet = find_pet( pet_name );
        if ( pet )
        {
          a =  new execute_pet_action_t( this, pet, pet_action, action_options );
        }
        else
        {
          sim -> errorf( "Player %s refers to unknown pet %s in action: %s\n",
                         name(), pet_name.c_str(), splits[ i ].c_str() );
        }
      }
      else
      {
        if ( action_name == modify_action )
        {
          if ( sim -> debug )
            log_t::output( sim, "Player %s: modify_action=%s", name(), modify_action.c_str() );

          action_options = modify_action_options;
          splits[ i ] = modify_action + "," + modify_action_options;
        }
        a = create_action( action_name, action_options );
      }

      if ( a )
      {
        a -> marker = ( char ) ( ( i < 10 ) ? ( '0' + i      ) :
                                 ( i < 36 ) ? ( 'A' + i - 10 ) :
                                 ( i < 58 ) ? ( 'a' + i - 36 ) : '.' );

        a -> signature_str = splits[ i ];

        if (  sim -> separate_stats_by_actions > 0 && !is_pet() )
        {
          a -> stats = get_stats( a -> name_str + "__" + a -> marker, a );
        }
      }
      else
      {
        sim -> errorf( "Player %s unable to create action: %s\n", name(), splits[ i ].c_str() );
        sim -> cancel();
        return;
      }
    }
  }

  if ( ! action_list_skip.empty() )
  {
    if ( sim -> debug )
      log_t::output( sim, "Player %s: action_list_skip=%s", name(), action_list_skip.c_str() );

    std::vector<std::string> splits;
    int num_splits = util_t::string_split( splits, action_list_skip, "/" );
    for ( int i=0; i < num_splits; i++ )
    {
      action_t* action = find_action( splits[ i ] );
      if ( action ) action -> background = true;
    }
  }

  for ( action_t* action = action_list; action; action = action -> next )
  {
    action -> init();
    if ( action -> trigger_gcd == timespan_t::zero && ! action -> background && action -> use_off_gcd )
      off_gcd_actions.push_back( action );

  }

  int capacity = std::max( 1200, ( int ) ( sim -> max_time.total_seconds() / 2.0 ) );
  action_sequence.reserve( capacity );
  action_sequence = "";
}

// player_t::init_rating ====================================================

void player_t::init_rating()
{
  if ( sim -> debug )
    log_t::output( sim, "player_t::init_rating(): level=%d type=%s",
                   level, util_t::player_type_string( type ) );

  rating.init( sim, dbc, level, type );

}

// player_t::init_talents ===================================================

void player_t::init_talents()
{
  for ( int i=0; i < MAX_TALENT_TREES; i++ )
  {
    talent_tab_points[ i ] = 0;

    size_t size=talent_trees[ i ].size();
    for ( size_t j=0; j < size; j++ )
    {
      talent_tab_points[ i ] += talent_trees[ i ][ j ] -> rank();
    }
  }

  specialization = primary_tab();
}

// player_t::init_spells ====================================================

void player_t::init_spells()
{}

// player_t::init_buffs =====================================================

void player_t::init_buffs()
{
  if ( sim -> debug ) log_t::output( sim, "Initializing buffs for player (%s)", name() );

  buffs.power_potion    = new stat_buff_t( this, "power_potion", STAT_POWER, 565.0, 1, timespan_t::from_seconds( 15.0 ), timespan_t::from_seconds( 180.0 ) );
  buffs.coordination    = new buff_t( this, "coordination",     1 );
  buffs.unnatural_might = new buff_t( this, "unnatural_might",  1 );

  buffs.raid_movement   = new buff_t( this, "raid_movement",    1 );
  buffs.self_movement   = new buff_t( this, "self_movement",    1 );
  buffs.stunned         = new buff_t( this, "stunned",         -1 );
  debuffs.bleeding      = new debuff_t( this, "bleeding",     -1 );
  debuffs.invulnerable  = new debuff_t( this, "invulnerable", -1 );
  debuffs.vulnerable    = new debuff_t( this, "vulnerable",   -1 );

  debuffs.shatter_shot  = new debuff_t( this, "shatter_shot", 1, timespan_t::from_seconds( 45.0 ) ); // TODO: move to player_t extension of correct class
  debuffs.flying        = new debuff_t( this, "flying",   -1 );
}

// player_t::init_gains =====================================================

void player_t::init_gains()
{
  gains.force_regen  = get_gain( "force_regen" );
  gains.energy_regen = get_gain( "energy_regen" );
  gains.ammo_regen   = get_gain( "ammo_regen" );
}

// player_t::init_procs =====================================================

void player_t::init_procs()
{}

// player_t::init_uptimes ===================================================

void player_t::init_uptimes()
{
  primary_resource_cap = get_uptime( util_t::resource_type_string( primary_resource() ) + ( std::string ) "_cap" );
}

// player_t::init_benefits ===================================================

void player_t::init_benefits()
{ }

// player_t::init_rng =======================================================

void player_t::init_rng()
{
  rngs.lag_channel  = get_rng( "lag_channel"  );
  rngs.lag_ability  = get_rng( "lag_ability"  );
  rngs.lag_brain    = get_rng( "lag_brain"    );
  rngs.lag_gcd      = get_rng( "lag_gcd"      );
  rngs.lag_queue    = get_rng( "lag_queue"    );
  rngs.lag_reaction = get_rng( "lag_reaction" );
  rngs.lag_world    = get_rng( "lag_world"    );
}

// player_t::init_stats =====================================================

void player_t::init_stats()
{
  range::fill( resource_lost, 0 );
  range::fill( resource_gained, 0 );

  fight_length.reserve( sim -> iterations );
  waiting_time.reserve( sim -> iterations );
  executed_foreground_actions.reserve( sim -> iterations );

  dmg.reserve( sim -> iterations );
  compound_dmg.reserve( sim -> iterations );
  dps.reserve( sim -> iterations );
  dpse.reserve( sim -> iterations );
  dtps.reserve( sim -> iterations );

  heal.reserve( sim -> iterations );
  compound_heal.reserve( sim -> iterations );
  hps.reserve( sim -> iterations );
  hpse.reserve( sim -> iterations );
  htps.reserve( sim -> iterations );
}

// player_t::init_values ====================================================

void player_t::init_values()
{
  if ( sim -> debug ) log_t::output( sim, "Initializing values for player (%s)", name() );
}

// player_t::init_scaling ===================================================

void player_t::init_scaling()
{
  if ( sim -> debug ) log_t::output( sim, "Initializing scaling for player (%s)", name() );

  if ( ! is_pet() && ! is_enemy() )
  {
    invert_scaling = 0;

    int role = primary_role();

    int attack = ( ( role == ROLE_ATTACK ) || ( role == ROLE_HYBRID ) || role == ROLE_TANK ) ? 1 : 0;
    int spell  = ( ( role == ROLE_SPELL  ) || ( role == ROLE_HYBRID ) || ( role == ROLE_HEAL ) ) ? 1 : 0;
    int tank   = role == ROLE_TANK ? 1 : 0;

    scales_with[ STAT_STRENGTH  ] = attack;
    scales_with[ STAT_AIM       ] = 0;
    scales_with[ STAT_CUNNING   ] = 0;
    scales_with[ STAT_WILLPOWER ] = spell;
    scales_with[ STAT_ENDURANCE ] = 0;
    scales_with[ STAT_PRESENCE  ] = 0;

    scales_with[ STAT_HEALTH ] = 0;
    scales_with[ STAT_MANA   ] = 0;
    scales_with[ STAT_RAGE   ] = 0;
    scales_with[ STAT_ENERGY ] = 0;
    scales_with[ STAT_AMMO   ] = 0;

    scales_with[ STAT_EXPERTISE_RATING         ] = 0;
    scales_with[ STAT_EXPERTISE_RATING2        ] = 0;

    scales_with[ STAT_HIT_RATING                ] = 0;
    scales_with[ STAT_CRIT_RATING               ] = spell || attack;
    scales_with[ STAT_ALACRITY_RATING           ] = spell || attack;

    scales_with[ STAT_WEAPON_DPS   ] = attack;
    scales_with[ STAT_WEAPON_SPEED ] = sim -> weapon_speed_scale_factors ? attack : 0;

    scales_with[ STAT_WEAPON_OFFHAND_DPS   ] = 0;
    scales_with[ STAT_WEAPON_OFFHAND_SPEED ] = 0;

    scales_with[ STAT_ARMOR          ] = tank;
    scales_with[ STAT_BONUS_ARMOR    ] = 0;
    scales_with[ STAT_DODGE_RATING   ] = 0;
    scales_with[ STAT_PARRY_RATING   ] = 0;
    scales_with[ STAT_DEFENSE_RATING ] = tank;

    scales_with[ STAT_BLOCK_RATING ] = 0;

    scales_with[ STAT_POWER ] = spell || attack;
    scales_with[ STAT_FORCE_POWER ] = spell || attack;
    scales_with[ STAT_TECH_POWER ] = spell || attack;

    scales_with[ STAT_SURGE_RATING ] = spell || attack;

    if ( sim -> scaling -> scale_stat != STAT_NONE && scale_player )
    {
      double v = sim -> scaling -> scale_value;

      switch ( sim -> scaling -> scale_stat )
      {
      case STAT_STRENGTH:  attribute_initial[ ATTR_STRENGTH  ] += v; break;
      case STAT_AIM:       attribute_initial[ ATTR_AIM       ] += v; break;
      case STAT_CUNNING:   attribute_initial[ ATTR_CUNNING   ] += v; break;
      case STAT_WILLPOWER: attribute_initial[ ATTR_WILLPOWER ] += v; break;
      case STAT_ENDURANCE: attribute_initial[ ATTR_ENDURANCE ] += v; break;
      case STAT_PRESENCE:  attribute_initial[ ATTR_PRESENCE  ] += v; break;

      case STAT_POWER:           initial_power           += v; break;
      case STAT_FORCE_POWER:     initial_force_power     += v; break;
      case STAT_TECH_POWER:      initial_tech_power      += v; break;

      case STAT_SURGE_RATING:    initial_surge_rating    += v; break;

      case STAT_DEFENSE_RATING:  initial_defense_rating  += v; break;

      case STAT_HIT_RATING:      initial_accuracy_rating += v; break;
      case STAT_CRIT_RATING:     initial_crit_rating     += v; break;
      case STAT_ALACRITY_RATING: initial_alacrity_rating += v; break;

      case STAT_WEAPON_DPS:
        if ( main_hand_weapon.damage > 0 )
        {
          main_hand_weapon.damage  += main_hand_weapon.swing_time.total_seconds() * v;
          main_hand_weapon.min_dmg += main_hand_weapon.swing_time.total_seconds() * v;
          main_hand_weapon.max_dmg += main_hand_weapon.swing_time.total_seconds() * v;
        }
        if ( ranged_weapon.damage > 0 )
        {
          ranged_weapon.damage     += ranged_weapon.swing_time.total_seconds() * v;
          ranged_weapon.min_dmg    += ranged_weapon.swing_time.total_seconds() * v;
          ranged_weapon.max_dmg    += ranged_weapon.swing_time.total_seconds() * v;
        }
        break;

      case STAT_WEAPON_SPEED:
        if ( main_hand_weapon.swing_time > timespan_t::zero )
        {
          timespan_t new_speed = ( main_hand_weapon.swing_time + timespan_t::from_seconds( v ) );
          double mult = new_speed / main_hand_weapon.swing_time;

          main_hand_weapon.min_dmg *= mult;
          main_hand_weapon.max_dmg *= mult;
          main_hand_weapon.damage  *= mult;

          main_hand_weapon.swing_time = new_speed;
        }
        if ( ranged_weapon.swing_time > timespan_t::zero )
        {
          timespan_t new_speed = ( ranged_weapon.swing_time + timespan_t::from_seconds( v ) );

          double mult = new_speed / ranged_weapon.swing_time;

          ranged_weapon.min_dmg *= mult;
          ranged_weapon.max_dmg *= mult;
          ranged_weapon.damage  *= mult;

          ranged_weapon.swing_time = new_speed;
        }
        break;

      case STAT_WEAPON_OFFHAND_DPS:
        if ( off_hand_weapon.damage > 0 )
        {
          off_hand_weapon.damage   += off_hand_weapon.swing_time.total_seconds() * v;
          off_hand_weapon.min_dmg  += off_hand_weapon.swing_time.total_seconds() * v;
          off_hand_weapon.max_dmg  += off_hand_weapon.swing_time.total_seconds() * v;
        }
        break;

      case STAT_WEAPON_OFFHAND_SPEED:
        if ( off_hand_weapon.swing_time > timespan_t::zero )
        {
          timespan_t new_speed = ( off_hand_weapon.swing_time + timespan_t::from_seconds( v ) );
          double mult = new_speed / off_hand_weapon.swing_time;

          off_hand_weapon.min_dmg *= mult;
          off_hand_weapon.max_dmg *= mult;
          off_hand_weapon.damage  *= mult;

          off_hand_weapon.swing_time = new_speed;
        }
        break;

      case STAT_ARMOR:          initial_armor       += v; break;
      case STAT_BONUS_ARMOR:    initial_bonus_armor += v; break;
      case STAT_DODGE_RATING:   initial_dodge       += v / rating.dodge; break;
      case STAT_PARRY_RATING:   initial_parry       += v / rating.parry; break;

      case STAT_BLOCK_RATING:   initial_block       += v / rating.block; break;

      case STAT_MAX: break;

      default: assert( 0 );
      }
    }
  }
}

// player_t::find_item ======================================================

item_t* player_t::find_item( const std::string& str )
{
  int num_items = ( int ) items.size();

  for ( int i=0; i < num_items; i++ )
    if ( str == items[ i ].name() )
      return &( items[ i ] );

  return 0;
}

// player_t::get_attribute() ================================================

double player_t::get_attribute( attribute_type a ) const
{ return attribute[ a ] * composite_attribute_multiplier( a ); }

// player_t::energy_regen_per_second() ======================================

double player_t::energy_regen_per_second() const
{
  double r = base_energy_regen_per_second;

  return r;
}

// player_t::ammo_regen_per_second() ========================================

double player_t::ammo_regen_per_second() const
{
  double r = base_ammo_regen_per_second;

  return r;
}

// player_t::force_regen_per_second() =======================================

double player_t::force_regen_per_second() const
{
  double r = base_force_regen_per_second;

  return r;
}

// player_t::composite_power() ==============================================

double player_t::composite_power() const
{ return power; }

// player_t::composite_force_power() ========================================

double player_t::composite_force_power() const
{ return force_power; }

// player_t::composite_force_power() ========================================

double player_t::composite_tech_power() const
{ return tech_power; }

// player_t::composite_armor ================================================

double player_t::composite_armor() const
{
  double a = armor;

  a *= composite_armor_multiplier();

  a += bonus_armor;

  return a;
}

// player_t::composite_armor_multiplier =====================================

double player_t::composite_armor_multiplier() const
{
  double a = armor_multiplier;

  if ( debuffs.shatter_shot -> up() )
    a *= 0.8;

  return a;
}

// player_t::composite_tank_miss ============================================

double player_t::composite_tank_miss( const school_type /* school */ ) const
{
  double m = 0;

  if      ( m > 1.0 ) m = 1.0;
  else if ( m < 0.0 ) m = 0.0;

  return m;
}

// player_t::composite_tank_dodge ===========================================

double player_t::composite_tank_dodge() const
{
  double d = dodge;

  return d;
}

// player_t::composite_tank_parry ===========================================

double player_t::composite_tank_parry() const
{
  double p = parry;

  return p;
}

// player_t::composite_tank_block ===========================================

double player_t::composite_tank_block() const
{
  double b = block;

  return b;
}

// player_t::composite_tank_block_reduction =================================

double player_t::composite_tank_block_reduction() const
{
  double b = block_reduction;

  return b;
}

// player_t::composite_tank_crit_block ======================================

double player_t::composite_tank_crit_block() const
{
  return 0;
}

// player_t::composite_tank_crit ============================================

double player_t::composite_tank_crit( const school_type /* school */ ) const
{
  return 0;
}

// player_t::composite_attribute_multiplier =================================

double player_t::composite_attribute_multiplier( int attr ) const
{
  double m = attribute_multiplier[ attr ];

  if ( buffs.force_valor -> up() )
  {
    switch( attr )
    {
    case ATTR_STRENGTH:
    case ATTR_AIM:
    case ATTR_CUNNING:
    case ATTR_WILLPOWER:
      m += 0.05;
      break;
    default:
      break;
    }
  }

  return m;
}

// player_t::composite_player_multiplier ====================================

double player_t::composite_player_multiplier( const school_type /* school */, action_t* /* a */ ) const
{
  double m = 1.0;

  return m;
}

// player_t::composite_player_td_multiplier =================================

double player_t::composite_player_td_multiplier( const school_type /* school */, action_t* /* a */ ) const
{
  double m = 1.0;

  return m;
}

// player_t::composite_player_heal_multiplier ===============================

double player_t::composite_player_heal_multiplier( const school_type /* school */ ) const
{
  double m = 1.0;

  return m;
}

// player_t::composite_player_th_multiplier =================================

double player_t::composite_player_th_multiplier( const school_type /* school */ ) const
{
  double m = 1.0;

  return m;
}

// player_t::composite_player_absorb_multiplier =============================

double player_t::composite_player_absorb_multiplier( const school_type /* school */ ) const
{
  double m = 1.0;

  return m;
}

// player_t::composite_movement_speed =======================================

double player_t::composite_movement_speed() const
{
  double speed = base_movement_speed;

  return speed;
}

// player_t::damage_bonus ===================================================

double player_t::damage_bonus( double stats, double multiplier, double extra_power ) const
{ return multiplier * ( 0.2 * stats + 0.23 * ( composite_power() + extra_power ) ); }

double player_t::default_hit_chance() const
{ return rating_t::accuracy_from_rating( accuracy_rating, level ); }

double player_t::default_crit_chance() const
{
  double c = 0.05 + rating_t::crit_from_rating( crit_rating, level );

  if ( buffs.coordination -> up() )
    c += 0.05;

  return c;
}

// player_t::melee_damage_bonus() ===========================================

double player_t::melee_bonus_stats() const
{ return strength(); }

double player_t::melee_bonus_multiplier() const
{ return 1.0; }

double player_t::melee_crit_from_stats() const
{ return rating_t::crit_from_stat( strength(), level ); }

double player_t::melee_damage_bonus() const
{
  return damage_bonus( melee_bonus_stats(),
                       melee_bonus_multiplier() );
}

double player_t::melee_hit_chance() const
{ return default_hit_chance(); }

double player_t::melee_crit_chance() const
{ return default_crit_chance() + melee_crit_from_stats(); }

double player_t::melee_avoidance() const
{ return base_melee_avoidance + rating_t::defense_from_rating( defense_rating, level ); }

// player_t::range_damage_bonus() ===========================================

double player_t::range_bonus_stats() const
{ return aim(); }

double player_t::range_bonus_multiplier() const
{ return 1.0; }

double player_t::range_crit_from_stats() const
{ return rating_t::crit_from_stat( aim(), level ); }

double player_t::range_damage_bonus() const
{
  return damage_bonus( range_bonus_stats(),
                       range_bonus_multiplier() );
}

double player_t::range_hit_chance() const
{ return default_hit_chance(); }

double player_t::range_crit_chance() const
{ return default_crit_chance() + range_crit_from_stats(); }

double player_t::range_avoidance() const
{ return base_range_avoidance + rating_t::defense_from_rating( defense_rating, level ); }

// player_t::force_damage_bonus() ===========================================

double player_t::force_bonus_stats() const
{ return willpower(); }

double player_t::force_bonus_multiplier() const
{
  double m = 1.0;

  if ( buffs.unnatural_might -> up() )
    m += .05;

  return m;
}

double player_t::force_crit_from_stats() const
{ return rating_t::crit_from_stat( willpower(), level ); }

double player_t::force_damage_bonus() const
{
  return damage_bonus( force_bonus_stats(),
                       force_bonus_multiplier(),
                       composite_force_power() );
}

double player_t::force_hit_chance() const
{ return default_hit_chance(); }

double player_t::force_crit_chance() const
{ return default_crit_chance() + force_crit_from_stats(); }

double player_t::force_avoidance() const
{ return base_force_avoidance; }

// player_t::tech_damage_bonus() ============================================

double player_t::tech_bonus_stats() const
{ return cunning(); }

double player_t::tech_bonus_multiplier() const
{
  double m = 1.0;

  if ( buffs.unnatural_might -> up() )
    m += .05;

  return m;
}

double player_t::tech_crit_from_stats() const
{ return rating_t::crit_from_stat( cunning(), level ); }

double player_t::tech_damage_bonus() const
{
  return damage_bonus( tech_bonus_stats(),
                       tech_bonus_multiplier(),
                       composite_tech_power() );
}

double player_t::tech_hit_chance() const
{ return default_hit_chance(); }

double player_t::tech_crit_chance() const
{ return default_crit_chance() + tech_crit_from_stats(); }

double player_t::tech_avoidance() const
{ return base_tech_avoidance; }

// player_t::healing_bonus() ================================================

double player_t::healing_bonus( double stats, double multiplier, double extra_power ) const
{ return multiplier * ( 0.14 * stats + 0.17 * ( composite_power() + extra_power ) ); }

// player_t::force_healing_bonus() ==========================================

double player_t::force_healing_bonus_stats() const
{ return willpower(); }

double player_t::force_healing_crit_from_stats() const
{ return rating_t::crit_from_stat( willpower(), level ); }

double player_t::force_healing_crit_chance() const
{ return default_crit_chance() + force_healing_crit_from_stats(); }

double player_t::force_healing_bonus_multiplier() const
{
  double m = 1.0;

  if ( buffs.unnatural_might -> up() )
    m += .05;

  return m;
}

double player_t::force_healing_bonus() const
{
  return healing_bonus( force_healing_bonus_stats(),
                        force_healing_bonus_multiplier(),
                        composite_force_power() );
}

// player_t::tech_healing_bonus() ===========================================

double player_t::tech_healing_bonus_stats() const
{ return cunning(); }

double player_t::tech_healing_crit_from_stats() const
{ return rating_t::crit_from_stat( cunning(), level ); }

double player_t::tech_healing_crit_chance() const
{ return default_crit_chance() + tech_healing_crit_from_stats(); }

double player_t::tech_healing_bonus_multiplier() const
{
  double m = 1.0;

  if ( buffs.unnatural_might -> up() )
    m += .05;

  return m;
}

double player_t::tech_healing_bonus() const
{
  return healing_bonus( tech_healing_bonus_stats(),
                        tech_healing_bonus_multiplier(),
                        composite_tech_power() );
}

// player_t::combat_begin ===================================================

void player_t::combat_begin( sim_t* sim )
{
  player_t::jedi_sage_combat_begin( sim );
  player_t::shadow_assassin_combat_begin( sim );
  player_t::enemy_combat_begin( sim );

  // FIXME: Move to correct classes
  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( p -> ooc_buffs() )
    {
      if ( sim -> overrides.coordination > 0 )
        p -> buffs.coordination -> override();

      if ( sim -> overrides.unnatural_might > 0 )
        p -> buffs.unnatural_might -> override();
    }
  }
}

// player_t::combat_begin ===================================================

void player_t::combat_begin()
{
  if ( sim -> debug ) log_t::output( sim, "Combat begins for player %s", name() );

  if ( ! is_pet() && ! is_add() )
  {
    arise();
  }

  init_resources( true );

  if ( primary_resource() == RESOURCE_MANA )
  {
    get_gain( "initial_mana" ) -> add( resource_max[ RESOURCE_MANA ] );
    get_gain( "initial_mana" ) -> type = RESOURCE_MANA;
  }

  action_sequence = "";

  iteration_fight_length = timespan_t::zero;
  iteration_waiting_time = timespan_t::zero;
  iteration_executed_foreground_actions = 0;
  iteration_dmg = 0;
  iteration_heal = 0;
  iteration_dmg_taken = 0;
  iteration_heal_taken = 0;

  for ( buff_t* b = buff_list; b; b = b -> next )
    b -> combat_begin();

  for ( stats_t* s = stats_list; s; s = s -> next )
    s -> combat_begin();
}

// player_t::combat_end =====================================================

void player_t::combat_end( sim_t* sim )
{
  player_t::jedi_sage_combat_end( sim );
  player_t::shadow_assassin_combat_end( sim );
  player_t::enemy_combat_end( sim );
}

// player_t::combat_end =====================================================

void player_t::combat_end()
{
  for ( pet_t* pet = pet_list; pet; pet = pet -> next_pet )
  {
    pet -> combat_end();
  }

  if ( ! is_pet() )
  {
    demise();
  }
  else if ( is_pet() )
    cast_pet() -> dismiss();

  fight_length.add( iteration_fight_length.total_seconds() );

  executed_foreground_actions.add( iteration_executed_foreground_actions );
  waiting_time.add( iteration_waiting_time.total_seconds() );

  for ( stats_t* s = stats_list; s; s = s -> next )
    s -> combat_end();

  // DMG
  dmg.add( iteration_dmg );
  if ( ! is_enemy() && ! is_add() )
    sim -> iteration_dmg += iteration_dmg;
  for ( pet_t* pet = pet_list; pet; pet = pet -> next_pet )
  {
    iteration_dmg += pet -> iteration_dmg;
  }
  compound_dmg.add( iteration_dmg );

  dps.add( iteration_fight_length != timespan_t::zero ? iteration_dmg / iteration_fight_length.total_seconds() : 0 );
  dpse.add( sim -> current_time != timespan_t::zero ? iteration_dmg / sim -> current_time.total_seconds() : 0 );

  if ( sim -> debug ) log_t::output( sim, "Combat ends for player %s at time %.4f fight_length=%.4f", name(), sim -> current_time.total_seconds(), iteration_fight_length.total_seconds() );

  // Heal
  heal.add( iteration_heal );
  if ( ! is_enemy() && ! is_add() )
    sim -> iteration_heal += iteration_heal;
  for ( pet_t* pet = pet_list; pet; pet = pet -> next_pet )
  {
    iteration_heal += pet -> iteration_heal;
  }
  compound_heal.add( iteration_heal );

  hps.add( iteration_fight_length != timespan_t::zero ? iteration_heal / iteration_fight_length.total_seconds() : 0 );
  hpse.add( sim -> current_time != timespan_t::zero ? iteration_heal / sim -> current_time.total_seconds() : 0 );

  dmg_taken.add( iteration_dmg_taken );
  dtps.add( iteration_fight_length != timespan_t::zero ? iteration_dmg_taken / iteration_fight_length.total_seconds() : 0 );

  heal_taken.add( iteration_heal_taken );
  htps.add( iteration_fight_length != timespan_t::zero ? iteration_heal_taken / iteration_fight_length.total_seconds() : 0 );

  for ( buff_t* b = buff_list; b; b = b -> next )
  {
    b -> combat_end();
  }
}

// player_t::merge ==========================================================

void player_t::merge( player_t& other )
{
  fight_length.merge( other.fight_length );
  waiting_time.merge( other.waiting_time );
  executed_foreground_actions.merge( other.executed_foreground_actions );

  dmg.merge( other.dmg );
  compound_dmg.merge( other.compound_dmg );
  dps.merge( other.dps );
  dtps.merge( other.dtps );
  dpse.merge( other.dpse );
  dmg_taken.merge( other.dmg_taken );

  heal.merge( other.heal );
  compound_heal.merge( other.compound_heal );
  hps.merge( other.hps );
  htps.merge( other.htps );
  hpse.merge( other.hpse );
  heal_taken.merge( other.heal_taken );

  deaths.merge( other.deaths );

  for ( int i = RESOURCE_NONE; i < RESOURCE_MAX; i++ )
  {
    int num_buckets = ( int ) std::min(       timeline_resource[i].size(),
                                        other.timeline_resource[i].size() );

    for ( int j=0; j < num_buckets; j++ )
    {
      timeline_resource[i][ j ] += other.timeline_resource[i][ j ];
    }

    resource_lost  [ i ] += other.resource_lost  [ i ];
    resource_gained[ i ] += other.resource_gained[ i ];
  }

  for ( buff_t* b = buff_list; b; b = b -> next )
  {
    b -> merge( buff_t::find( &other, b -> name() ) );
  }

  for ( proc_t* proc = proc_list; proc; proc = proc -> next )
  {
    proc -> merge( other.get_proc( proc -> name_str ) );
  }

  for ( gain_t* gain = gain_list; gain; gain = gain -> next )
  {
    gain -> merge( other.get_gain( gain -> name_str ) );
  }

  for ( stats_t* stats = stats_list; stats; stats = stats -> next )
  {
    stats -> merge( other.get_stats( stats -> name_str ) );
  }

  for ( uptime_t* uptime = uptime_list; uptime; uptime = uptime -> next )
  {
    uptime -> merge( *other.get_uptime( uptime -> name_str ) );
  }

  for ( benefit_t* benefit = benefit_list; benefit; benefit = benefit -> next )
  {
    benefit -> merge( other.get_benefit( benefit -> name_str ) );
  }

  for ( std::map<std::string,int>::const_iterator it = other.action_map.begin(),
        end = other.action_map.end(); it != end; ++it )
    action_map[ it -> first ] += it -> second;
}

// player_t::reset ==========================================================

void player_t::reset()
{
  if ( sim -> debug ) log_t::output( sim, "Resetting player %s", name() );

  skill = initial_skill;

  gcd_ready = timespan_t::zero;

  sleeping = 1;
  events = 0;

  active_companion = 0;

  stats = initial_stats;

  vengeance_damage = vengeance_value = vengeance_max = 0.0;

  alacrity_rating = initial_alacrity_rating;
  crit_rating = initial_crit_rating;
  accuracy_rating = initial_accuracy_rating;
  surge_rating = initial_surge_rating;
  recalculate_surge();
  defense_rating = initial_defense_rating;

  range::copy( attribute_initial, attribute );
  range::copy( attribute_multiplier_initial, attribute_multiplier );
  range::copy( initial_resource_reduction, resource_reduction );

  armor           = initial_armor;
  bonus_armor     = initial_bonus_armor;
  dodge           = initial_dodge;
  parry           = initial_parry;
  block           = initial_block;
  block_reduction = initial_block_reduction;

  power           = initial_power;
  force_power     = initial_force_power;
  tech_power      = initial_tech_power;

  armor_multiplier   = initial_armor_multiplier;

  for ( buff_t* b = buff_list; b; b = b -> next )
  {
    b -> reset();
  }

  last_foreground_action = 0;

  executing = 0;
  channeling = 0;
  readying = 0;
  off_gcd = 0;
  in_combat = false;

  cast_delay_reaction = timespan_t::zero;
  cast_delay_occurred = timespan_t::zero;

  main_hand_weapon.buff_type  = 0;
  main_hand_weapon.buff_value = 0;
  main_hand_weapon.bonus_dmg  = 0;

  off_hand_weapon.buff_type  = 0;
  off_hand_weapon.buff_value = 0;
  off_hand_weapon.bonus_dmg  = 0;

  ranged_weapon.buff_type  = 0;
  ranged_weapon.buff_value = 0;
  ranged_weapon.bonus_dmg  = 0;

  stim            = STIM_NONE;
  for ( int i=0; i < RESOURCE_MAX; i++ )
  {
    action_callback_t::reset( resource_gain_callbacks[ i ] );
    action_callback_t::reset( resource_loss_callbacks[ i ] );
  }
  for ( int i=0; i < RESULT_MAX; i++ )
  {
    action_callback_t::reset( attack_callbacks[ i ] );
    action_callback_t::reset( spell_callbacks [ i ] );
    action_callback_t::reset( tick_callbacks  [ i ] );
  }
  for ( int i=0; i < SCHOOL_MAX; i++ )
  {
    action_callback_t::reset( tick_damage_callbacks  [ i ] );
    action_callback_t::reset( direct_damage_callbacks[ i ] );
  }

  replenishment_targets.clear();

  init_resources( true );

  for ( action_t* a = action_list; a; a = a -> next ) a -> reset();

  for ( cooldown_t* c = cooldown_list; c; c = c -> next ) c -> reset();

  for ( dot_t* d = dot_list; d; d = d -> next ) d -> reset();

  for ( std::vector<targetdata_t*>::iterator i = targetdata.begin(); i != targetdata.end(); ++i )
  {
    if ( *i )
      ( *i )->reset();
  }

  for ( stats_t* s = stats_list; s; s = s -> next ) s -> reset();

  potion_used = 0;

  temporary = gear_stats_t();
}

// player_t::schedule_ready =================================================

void player_t::schedule_ready( timespan_t delta_time,
                               bool   waiting )
{
  if ( readying )
  {
    sim -> errorf( "\nplayer_t::schedule_ready assertion error: readying == true ( player %s )\n", name() );
    assert( 0 );
  }
  action_t* was_executing = ( channeling ? channeling : executing );

  if ( sleeping )
    return;

  executing = 0;
  channeling = 0;
  action_queued = false;

  if ( ! has_foreground_actions( this ) ) return;

  timespan_t gcd_adjust = gcd_ready - ( sim -> current_time + delta_time );
  if ( gcd_adjust > timespan_t::zero ) delta_time += gcd_adjust;

  if ( unlikely( waiting ) )
  {
    iteration_waiting_time += delta_time;
  }
  else
  {
    timespan_t lag = timespan_t::zero;

    if ( last_foreground_action && ! last_foreground_action -> auto_cast )
    {
      if ( last_foreground_action -> ability_lag > timespan_t::zero )
      {
        timespan_t ability_lag = rngs.lag_ability -> gauss( last_foreground_action -> ability_lag, last_foreground_action -> ability_lag_stddev );
        timespan_t gcd_lag     = rngs.lag_gcd   -> gauss( sim ->   gcd_lag, sim ->   gcd_lag_stddev );
        timespan_t diff        = ( gcd_ready + gcd_lag ) - ( sim -> current_time + ability_lag );
        if ( diff > timespan_t::zero && sim -> strict_gcd_queue )
        {
          lag = gcd_lag;
        }
        else
        {
          lag = ability_lag;
          action_queued = true;
        }
      }
      else if ( last_foreground_action -> gcd() == timespan_t::zero )
      {
        lag = timespan_t::zero;
      }
      else if ( last_foreground_action -> channeled )
      {
        lag = rngs.lag_channel -> gauss( sim -> channel_lag, sim -> channel_lag_stddev );
      }
      else
      {
        timespan_t   gcd_lag = rngs.lag_gcd   -> gauss( sim ->   gcd_lag, sim ->   gcd_lag_stddev );
        timespan_t queue_lag = rngs.lag_queue -> gauss( sim -> queue_lag, sim -> queue_lag_stddev );

        timespan_t diff = ( gcd_ready + gcd_lag ) - ( sim -> current_time + queue_lag );

        if ( diff > timespan_t::zero && sim -> strict_gcd_queue )
        {
          lag = gcd_lag;
        }
        else
        {
          lag = queue_lag;
          action_queued = true;
        }
      }
    }

    if ( lag < timespan_t::zero ) lag = timespan_t::zero;

    delta_time += lag;
  }

  if ( last_foreground_action )
  {
    last_foreground_action -> stats -> total_execute_time += delta_time;
  }

  readying = new ( sim ) player_ready_event_t( sim, this, delta_time );

  if ( was_executing && was_executing -> gcd() > timespan_t::zero && ! was_executing -> background && ! was_executing -> proc && ! was_executing -> repeating )
  {
    // Record the last ability use time for cast_react
    cast_delay_occurred = readying -> occurs();
    cast_delay_reaction = rngs.lag_brain -> gauss( brain_lag, brain_lag_stddev );
    if ( sim -> debug )
    {
      log_t::output( sim, "%s %s schedule_ready(): cast_finishes=%f cast_delay=%f",
                     name_str.c_str(),
                     was_executing -> name_str.c_str(),
                     readying -> occurs().total_seconds(),
                     cast_delay_reaction.total_seconds() );
    }
  }
}

// player_t::arise ==========================================================

void player_t::arise()
{
  if ( sim -> log )
    log_t::output( sim, "%s arises.", name() );

  if ( ! initial_sleeping )
    sleeping = 0;

  if ( sleeping )
    return;

  init_resources( true );

  readying = 0;
  off_gcd = 0;

  arise_time = sim -> current_time;

  schedule_ready();
}

// player_t::demise =========================================================

void player_t::demise()
{
  // No point in demising anything if we're not even active
  if ( sleeping == 1 ) return;

  if ( sim -> log )
    log_t::output( sim, "%s demises.", name() );

  assert( arise_time >= timespan_t::zero );
  iteration_fight_length += sim -> current_time - arise_time;
  arise_time = timespan_t::min;

  sleeping = 1;
  if ( readying )
  {
    event_t::cancel( readying );
    readying = 0;
  }

  event_t::cancel( off_gcd );

  for ( buff_t* b = buff_list; b; b = b -> next )
  {
    b -> expire();
    // Dead actors speak no lies .. or proc aura delayed buffs
    if ( b -> delay ) event_t::cancel( b -> delay );
  }
  for ( action_t* a = action_list; a; a = a -> next )
  {
    a -> cancel();
  }

  //sim -> cancel_events( this );

  for ( pet_t* pet = pet_list; pet; pet = pet -> next_pet )
  {
    pet -> demise();
  }
}

// player_t::interrupt ======================================================

void player_t::interrupt()
{
  // FIXME! Players will need to override this to handle background repeating actions.

  if ( sim -> log ) log_t::output( sim, "%s is interrupted", name() );

  if ( executing  ) executing  -> interrupt_action();
  if ( channeling ) channeling -> interrupt_action();

  if ( buffs.stunned -> check() )
  {
    if ( readying ) event_t::cancel( readying );
    if ( off_gcd ) event_t::cancel( off_gcd );
  }
  else
  {
    if ( ! readying && ! sleeping ) schedule_ready();
  }
}

// player_t::halt ===========================================================

void player_t::halt()
{
  if ( sim -> log ) log_t::output( sim, "%s is halted", name() );

  interrupt();
}

// player_t::stun() =========================================================

void player_t::stun()
{
  halt();
}

// player_t::moving =========================================================

void player_t::moving()
{
  // FIXME! In the future, some movement events may not cause auto-attack to stop.

  halt();
}

// player_t::clear_debuffs===================================================

void player_t::clear_debuffs()
{
  // FIXME! At the moment we are just clearing DoTs

  if ( sim -> log ) log_t::output( sim, "%s clears debuffs from %s", name(), sim -> target -> name() );

  for ( action_t* a = action_list; a; a = a -> next )
  {
    if ( a -> action_dot && a -> action_dot -> ticking )
      a -> action_dot -> cancel();
  }

  for ( std::vector<targetdata_t*>::iterator i = targetdata.begin(); i != targetdata.end(); ++i )
  {
    if ( *i )
      ( *i )->clear_debuffs();
  }
}

// player_t::print_action_map ===============================================

std::string player_t::print_action_map( int iterations, int precision )
{
  std::ostringstream ret;
  ret.precision( precision );
  ret << "Label: Number of executes (Average number of executes per iteration)<br />\n";

  for ( std::map<std::string,int>::const_iterator it = action_map.begin(), end = action_map.end(); it != end; ++it )
  {
    ret << it -> first << ": " << it -> second;
    if ( iterations > 0 ) ret << " (" << ( ( double )it -> second ) / iterations << ')';
    ret << "<br />\n";
  }

  return ret.str();
}

// player_t::execute_action =================================================

action_t* player_t::execute_action()
{
  readying = 0;
  off_gcd = 0;

  action_t* action=0;

  for ( action = action_list; action; action = action -> next )
  {
    if ( action -> background ||
         action -> sequence )
      continue;

    if ( action -> ready() )
      break;

    if ( action -> wait_on_ready == 1 )
    {
      action = 0;
      break;
    }
  }

  last_foreground_action = action;

  if ( action )
  {
    action -> schedule_execute();
    iteration_executed_foreground_actions++;
    if ( action -> marker ) action_sequence += action -> marker;
    if ( ! action -> label_str.empty() )
      action_map[ action -> label_str ] += 1;
  }

  return action;
}

// player_t::regen ==========================================================

void player_t::regen( const timespan_t periodicity )
{
  int resource_type = primary_resource();

  if ( resource_type == RESOURCE_ENERGY )
  {
    double energy_regen = periodicity.total_seconds() * energy_regen_per_second();

    resource_gain( RESOURCE_ENERGY, energy_regen, gains.energy_regen );
  }

  else if ( resource_type == RESOURCE_FORCE )
  {
    double force_regen = periodicity.total_seconds() * force_regen_per_second();

    resource_gain( RESOURCE_FORCE, force_regen, gains.force_regen );
  }

  else if ( resource_type == RESOURCE_AMMO )
  {
    double ammo_regen = periodicity.total_seconds() * ammo_regen_per_second();

    resource_gain( RESOURCE_AMMO, ammo_regen, gains.ammo_regen );
  }

  int index = ( int ) ( sim -> current_time.total_seconds() );

  for ( int i = RESOURCE_NONE; i < RESOURCE_MAX; i++ )
  {
    if ( resource_max[ i ] == 0 ) continue;

    timeline_resource[ i ][ index ] += resource_current[ i ] * periodicity.total_seconds();
  }
}

// player_t::resource_loss ==================================================

double player_t::resource_loss( int       resource,
                                double    amount,
                                action_t* action )
{
  if ( amount == 0 )
    return 0;

  if ( sleeping )
    return 0;

  if ( resource == primary_resource() )
    primary_resource_cap -> update( false );

  double actual_amount;

  if ( infinite_resource[ resource ] == 0 || is_enemy() )
  {
    actual_amount = std::min( amount, resource_current[ resource ] );
    resource_current[ resource ] -= actual_amount;
    resource_lost[ resource ] += actual_amount;
  }
  else
  {
    actual_amount = amount;
    resource_current[ resource ] -= actual_amount;
    resource_lost[ resource ] += actual_amount;
  }

  action_callback_t::trigger( resource_loss_callbacks[ resource ], action, ( void* ) &actual_amount );

  if ( sim -> debug )
    log_t::output( sim, "Player %s loses %.2f (%.2f) %s. health pct: %.2f",
                   name(), actual_amount, amount, util_t::resource_type_string( resource ), health_percentage()  );

  return actual_amount;
}

// player_t::resource_gain ==================================================

double player_t::resource_gain( int       resource,
                                double    amount,
                                gain_t*   source,
                                action_t* action )
{
  if ( sleeping )
    return 0;

  double actual_amount = std::min( amount, resource_max[ resource ] - resource_current[ resource ] );

  if ( actual_amount > 0 )
  {
    resource_current[ resource ] += actual_amount;
    resource_gained [ resource ] += actual_amount;
  }

  if ( resource == primary_resource() && resource_max[ resource ] <= resource_current[ resource ] )
    primary_resource_cap -> update( true );

  if ( source )
  {
    if ( source -> type == RESOURCE_NONE )
      source -> type = ( resource_type ) resource;

    if ( resource != source -> type )
    {
      sim -> errorf( "player_t::resource_gain: player=%s gain=%s resource_gain type not identical to gain resource type..\n resource=%s gain=%s",
                     name(), source -> name_str.c_str(), util_t::resource_type_string( resource ), util_t::resource_type_string( source -> type ) );
      assert ( 0 );
    }

    source -> add( actual_amount, amount - actual_amount );
  }

  action_callback_t::trigger( resource_gain_callbacks[ resource ], action, ( void* ) &actual_amount );

  if ( sim -> log )
  {
    log_t::output( sim, "%s gains %.2f (%.2f) %s from %s (%.2f/%.2f)",
                   name(), actual_amount, amount,
                   util_t::resource_type_string( resource ),
                   source ? source -> name() : action ? action -> name() : "unknown",
                   resource_current[ resource ], resource_max[ resource ] );
  }

  return actual_amount;
}

// player_t::resource_available =============================================

bool player_t::resource_available( int    resource,
                                   double cost ) const
{
  if ( resource == RESOURCE_NONE || cost <= 0 || infinite_resource[ resource ] == 1 )
  {
    return true;
  }

  return resource_current[ resource ] >= cost;
}

// player_t::recalculate_resource_max =======================================

void player_t::recalculate_resource_max( int resource )
{
  // The first 20pts of intellect/stamina only provide 1pt of mana/health.

  resource_max[ resource ] = resource_base[ resource ] +
                             gear.resource[ resource ] +
                             enchant.resource[ resource ] +
                             ( is_pet() ? 0 : sim -> enchant.resource[ resource ] );

  switch ( resource )
  {
  case RESOURCE_HEALTH:
  {
    double adjust = ( is_pet() || is_enemy() || is_add() ) ? 0 : std::min( 20, ( int ) floor( endurance() ) );
    resource_max[ resource ] += ( floor( endurance() ) - adjust ) * health_per_endurance + adjust;

    break;
  }
  default: break;
  }
}

// player_t::primary_tab ====================================================

int player_t::primary_tab() const
{
  int spec = TALENT_TAB_NONE;
  int max_points = 0;

  for ( int i=0; i < MAX_TALENT_TREES; i++ )
  {
    if ( talent_tab_points[ i ] > max_points )
    {
      spec = i;
      max_points = talent_tab_points[ i ];
    }
  }

  return spec;
}

// player_t::primary_role ===================================================

int player_t::primary_role() const
{
  return role;
}

// player_t::primary_tree_name ==============================================

const char* player_t::primary_tree_name() const
{
  return util_t::talent_tree_string( primary_tree() );
}

// player_t::primary_tree ===================================================

int player_t::primary_tree() const
{
  if ( specialization == TALENT_TAB_NONE )
    return TREE_NONE;

  return tree_type[ specialization ];
}

// player_t::normalize_by ===================================================

int player_t::normalize_by() const
{
  if ( sim -> normalized_stat != STAT_NONE )
    return sim -> normalized_stat;

  if ( primary_role() == ROLE_TANK )
    return STAT_ARMOR;

  return primary_attribute;
}

// player_t::health_percentage() ============================================

double player_t::health_percentage() const
{
  return resource_current[ RESOURCE_HEALTH ] / resource_max[ RESOURCE_HEALTH ] * 100 ;
}

// target_t::time_to_die ====================================================

timespan_t player_t::time_to_die() const
{
  // FIXME: Someone can figure out a better way to do this, for now, we NEED to
  // wait a minimum gcd before starting to estimate fight duration based on health,
  // otherwise very odd things happen with multi-actor simulations and time_to_die
  // expressions
  if ( resource_base[ RESOURCE_HEALTH ] > 0 && sim -> current_time >= timespan_t::from_seconds( 1.0 ) )
  {
    return sim -> current_time * ( resource_current[ RESOURCE_HEALTH ] / iteration_dmg_taken );
  }
  else
  {
    return ( sim -> expected_time - sim -> current_time );
  }
}

// player_t::total_reaction_time ============================================

timespan_t player_t::total_reaction_time() const
{
  return rngs.lag_reaction -> exgauss( reaction_mean, reaction_stddev, reaction_nu );
}

// player_t::stat_gain ======================================================

void player_t::stat_gain( int       stat,
                          double    amount,
                          gain_t*   gain,
                          action_t* action,
                          bool      temporary_stat )
{
  if ( amount <= 0 ) return;

  if ( sim -> log ) log_t::output( sim, "%s gains %.0f %s%s", name(), amount, util_t::stat_type_string( stat ), temporary_stat ? " (temporary)" : "" );

  int temp_value = temporary_stat ? 1 : 0;
  switch ( stat )
  {
  case STAT_STRENGTH:  stats.attribute[ ATTR_STRENGTH  ] += amount; attribute[ ATTR_STRENGTH  ] += amount; temporary.attribute[ ATTR_STRENGTH  ] += temp_value * amount; break;
  case STAT_AIM:       stats.attribute[ ATTR_AIM       ] += amount; attribute[ ATTR_AIM       ] += amount; temporary.attribute[ ATTR_AIM       ] += temp_value * amount; break;
  case STAT_CUNNING:   stats.attribute[ ATTR_CUNNING   ] += amount; attribute[ ATTR_CUNNING   ] += amount; temporary.attribute[ ATTR_CUNNING   ] += temp_value * amount; break;
  case STAT_WILLPOWER: stats.attribute[ ATTR_WILLPOWER ] += amount; attribute[ ATTR_WILLPOWER ] += amount; temporary.attribute[ ATTR_WILLPOWER ] += temp_value * amount; break;
  case STAT_ENDURANCE: stats.attribute[ ATTR_ENDURANCE ] += amount; attribute[ ATTR_ENDURANCE ] += amount; temporary.attribute[ ATTR_ENDURANCE ] += temp_value * amount; recalculate_resource_max( RESOURCE_HEALTH ); break;
  case STAT_PRESENCE:  stats.attribute[ ATTR_PRESENCE  ] += amount; attribute[ ATTR_PRESENCE  ] += amount; temporary.attribute[ ATTR_PRESENCE  ] += temp_value * amount; break;

  case STAT_MAX: for ( int i=0; i < ATTRIBUTE_MAX; i++ ) { stats.attribute[ i ] += amount; temporary.attribute[ i ] += temp_value * amount; attribute[ i ] += amount; } break;

  case STAT_HEALTH: resource_gain( RESOURCE_HEALTH, amount, gain, action ); break;
  case STAT_MANA:   resource_gain( RESOURCE_MANA,   amount, gain, action ); break;
  case STAT_RAGE:   resource_gain( RESOURCE_RAGE,   amount, gain, action ); break;
  case STAT_ENERGY: resource_gain( RESOURCE_ENERGY, amount, gain, action ); break;
  case STAT_AMMO:   resource_gain( RESOURCE_AMMO,   amount, gain, action ); break;

  case STAT_MAX_HEALTH: resource_max[ RESOURCE_HEALTH ] += amount; resource_gain( RESOURCE_HEALTH, amount, gain, action ); break;
  case STAT_MAX_MANA:   resource_max[ RESOURCE_MANA   ] += amount; resource_gain( RESOURCE_MANA,   amount, gain, action ); break;
  case STAT_MAX_RAGE:   resource_max[ RESOURCE_RAGE   ] += amount; resource_gain( RESOURCE_RAGE,   amount, gain, action ); break;
  case STAT_MAX_ENERGY: resource_max[ RESOURCE_ENERGY ] += amount; resource_gain( RESOURCE_ENERGY, amount, gain, action ); break;
  case STAT_MAX_AMMO:   resource_max[ RESOURCE_AMMO   ] += amount; resource_gain( RESOURCE_AMMO,   amount, gain, action ); break;

  case STAT_POWER:             stats.power             += amount; power                     += amount; break;
  case STAT_FORCE_POWER:       stats.force_power       += amount; force_power               += amount; break;
  case STAT_TECH_POWER:        stats.tech_power        += amount; tech_power                += amount; break;

  case STAT_HIT_RATING:
    stats.hit_rating += amount;
    temporary.hit_rating += temp_value * amount;
    accuracy_rating += amount;
    break;

  case STAT_CRIT_RATING:
    stats.crit_rating += amount;
    temporary.crit_rating += temp_value * amount;
    crit_rating += amount;
    break;

  case STAT_ALACRITY_RATING:
    stats.alacrity_rating += amount;
    temporary.alacrity_rating += temp_value * amount;
    alacrity_rating       += amount;
    break;

  case STAT_SURGE_RATING:
    stats.surge_rating += amount;
    temporary.surge_rating += temp_value * amount;
    surge_rating       += amount;
    recalculate_surge();
    break;

  case STAT_DEFENSE_RATING:
    stats.defense_rating += amount;
    temporary.defense_rating += temp_value * amount;
    defense_rating += amount;
    break;

  case STAT_ARMOR:          stats.armor          += amount; temporary.armor += temp_value * amount; armor       += amount;                  break;
  case STAT_BONUS_ARMOR:    stats.bonus_armor    += amount; bonus_armor += amount;                  break;
  case STAT_DODGE_RATING:   stats.dodge_rating   += amount; temporary.dodge_rating += temp_value * amount; dodge       += amount / rating.dodge;   break;
  case STAT_PARRY_RATING:   stats.parry_rating   += amount; temporary.parry_rating += temp_value * amount; parry       += amount / rating.parry;   break;

  case STAT_BLOCK_RATING: stats.block_rating += amount; temporary.block_rating += temp_value * amount; block       += amount / rating.block; break;

  default: assert( 0 );
  }
}

// player_t::stat_loss ======================================================

void player_t::stat_loss( int       stat,
                          double    amount,
                          action_t* action,
                          bool      temporary_buff )
{
  if ( amount <= 0 ) return;

  if ( sim -> log ) log_t::output( sim, "%s loses %.0f %s%s", name(), amount, util_t::stat_type_string( stat ), ( temporary_buff ) ? " (temporary)" : "" );

  int temp_value = temporary_buff ? 1 : 0;
  switch ( stat )
  {
  case STAT_STRENGTH:  stats.attribute[ ATTR_STRENGTH  ] -= amount; temporary.attribute[ ATTR_STRENGTH  ] -= temp_value * amount; attribute[ ATTR_STRENGTH  ] -= amount; break;
  case STAT_AIM:       stats.attribute[ ATTR_AIM       ] -= amount; temporary.attribute[ ATTR_AIM       ] -= temp_value * amount; attribute[ ATTR_AIM       ] -= amount; break;
  case STAT_CUNNING:   stats.attribute[ ATTR_CUNNING   ] -= amount; temporary.attribute[ ATTR_CUNNING   ] -= temp_value * amount; attribute[ ATTR_CUNNING   ] -= amount; break;
  case STAT_WILLPOWER: stats.attribute[ ATTR_WILLPOWER ] -= amount; temporary.attribute[ ATTR_WILLPOWER ] -= temp_value * amount; attribute[ ATTR_WILLPOWER ] -= amount; break;
  case STAT_ENDURANCE: stats.attribute[ ATTR_ENDURANCE ] -= amount; temporary.attribute[ ATTR_ENDURANCE ] -= temp_value * amount; attribute[ ATTR_ENDURANCE ] -= amount; stat_loss( STAT_MAX_HEALTH, floor( amount * composite_attribute_multiplier( ATTR_ENDURANCE ) ) * health_per_endurance, action ); break;
  case STAT_PRESENCE:  stats.attribute[ ATTR_PRESENCE  ] -= amount; temporary.attribute[ ATTR_PRESENCE  ] -= temp_value * amount; attribute[ ATTR_PRESENCE  ] -= amount; break;

  case STAT_MAX: for ( int i=0; i < ATTRIBUTE_MAX; i++ ) { stats.attribute[ i ] -= amount; temporary.attribute[ i ] -= temp_value * amount; attribute[ i ] -= amount; } break;

  case STAT_HEALTH: resource_loss( RESOURCE_HEALTH, amount, action ); break;
  case STAT_MANA:   resource_loss( RESOURCE_MANA,   amount, action ); break;
  case STAT_RAGE:   resource_loss( RESOURCE_RAGE,   amount, action ); break;
  case STAT_ENERGY: resource_loss( RESOURCE_ENERGY, amount, action ); break;
  case STAT_AMMO:   resource_loss( RESOURCE_AMMO,   amount, action ); break;

  case STAT_MAX_HEALTH:
  case STAT_MAX_MANA:
  case STAT_MAX_RAGE:
  case STAT_MAX_ENERGY:
  case STAT_MAX_AMMO:
  {
    int r = ( ( stat == STAT_MAX_HEALTH ) ? RESOURCE_HEALTH :
              ( stat == STAT_MAX_MANA   ) ? RESOURCE_MANA   :
              ( stat == STAT_MAX_RAGE   ) ? RESOURCE_RAGE   :
              ( stat == STAT_MAX_ENERGY ) ? RESOURCE_ENERGY : RESOURCE_AMMO );
    recalculate_resource_max( r );
    double delta = resource_current[ r ] - resource_max[ r ];
    if ( delta > 0 ) resource_loss( r, delta, action );
  }
  break;

  case STAT_POWER:             stats.power             -= amount; power                     -= amount; break;
  case STAT_FORCE_POWER:       stats.force_power       -= amount; force_power               -= amount; break;
  case STAT_TECH_POWER:        stats.tech_power        -= amount; tech_power                -= amount; break;

  case STAT_HIT_RATING:
    stats.hit_rating -= amount;
    temporary.hit_rating -= temp_value * amount;
    accuracy_rating       -= amount;
    break;

  case STAT_CRIT_RATING:
    stats.crit_rating -= amount;
    temporary.crit_rating -= temp_value * amount;
    crit_rating       -= amount;
    break;

  case STAT_ALACRITY_RATING:
    stats.alacrity_rating -= amount;
    temporary.alacrity_rating -= temp_value * amount;
    alacrity_rating       -= amount;
    break;

  case STAT_SURGE_RATING:
    stats.surge_rating -= amount;
    temporary.surge_rating -= temp_value * amount;
    surge_rating       -= amount;
    recalculate_surge();
    break;

  case STAT_DEFENSE_RATING:
    stats.defense_rating -= amount;
    temporary.defense_rating -= temp_value * amount;
    defense_rating       -= amount;
    break;

  case STAT_ARMOR:          stats.armor          -= amount; temporary.armor -= temp_value * amount; armor       -= amount;                  break;
  case STAT_BONUS_ARMOR:    stats.bonus_armor    -= amount; bonus_armor -= amount;                  break;
  case STAT_DODGE_RATING:   stats.dodge_rating   -= amount; temporary.dodge_rating -= temp_value * amount; dodge       -= amount / rating.dodge;   break;
  case STAT_PARRY_RATING:   stats.parry_rating   -= amount; temporary.parry_rating -= temp_value * amount; parry       -= amount / rating.parry;   break;

  case STAT_BLOCK_RATING: stats.block_rating -= amount; temporary.block_rating -= temp_value * amount; block       -= amount / rating.block; break;

  default: assert( 0 );
  }
}

// player_t::cost_reduction_gain ============================================

void player_t::cost_reduction_gain( int       school,
                                    double    amount,
                                    gain_t*   /* gain */,
                                    action_t* /* action */ )
{
  if ( amount <= 0 ) return;

  if ( sim -> log ) log_t::output( sim, "%s gains a cost reduction of %.0f on abilities of school %s", name(), amount, util_t::school_type_string( school ) );

  resource_reduction[ school ] += amount;
}

// player_t::cost_reduction_loss ============================================

void player_t::cost_reduction_loss( int       school,
                                    double    amount,
                                    action_t* /* action */ )
{
  if ( amount <= 0 ) return;

  if ( sim -> log ) log_t::output( sim, "%s loses a cost reduction %.0f on abilities of school %s", name(), amount, util_t::school_type_string( school ) );

  resource_reduction[ school ] -= amount;
}

// player_t::assess_damage ==================================================

double player_t::assess_damage( double            amount,
                                const school_type school,
                                int               dmg_type,
                                int               result,
                                action_t*         action )
{
  double mitigated_amount = target_mitigation( amount, school, dmg_type, result, action );

  size_t num_absorbs = absorb_buffs.size();
  double absorbed_amount = 0;
  if ( num_absorbs > 0 )
  {
    for ( size_t i = 0; i < num_absorbs; i++ )
    {
      double buff_value = absorb_buffs[ i ] -> value();
      double value = std::min( mitigated_amount - absorbed_amount, buff_value );
      absorbed_amount += value;
      if ( sim -> debug ) log_t::output( sim, "%s %s absorbs %.2f",
                                         name(), absorb_buffs[ i ] -> name(), value );
      if ( value == buff_value )
        absorb_buffs[ i ] -> expire();
      else
      {
        absorb_buffs[ i ] -> current_value -= value;
        if ( sim -> debug ) log_t::output( sim, "%s %s absorb remaining %.2f",
                                           name(), absorb_buffs[ i ] -> name(), absorb_buffs[ i ] -> current_value );
      }
    }
  }
  mitigated_amount -= absorbed_amount;

  iteration_dmg_taken += mitigated_amount;

  double actual_amount = resource_loss( RESOURCE_HEALTH, mitigated_amount, action );

  if ( resource_current[ RESOURCE_HEALTH ] <= 0 && !is_enemy() && infinite_resource[ RESOURCE_HEALTH ] == 0 )
  {
    // This can only save the target, if the damage is less than 200% of the target's health as of 4.0.6

    if ( ! sleeping )
    {
      if ( ! sleeping )
      {
        deaths.add( sim -> current_time.total_seconds() );
      }
      if ( sim -> log ) log_t::output( sim, "%s has died.", name() );
      demise();
    }
    if ( sim -> log ) log_t::output( sim, "%s has died.", name() );

    demise();
  }

  if ( vengeance_enabled )
  {
    vengeance_damage += actual_amount;
    vengeance_was_attacked = true;
  }

  return mitigated_amount;
}

// player_t::target_mitigation ==============================================

double player_t::target_mitigation( double            amount,
                                    const school_type school,
                                    int               /* dmg_type */,
                                    int               result,
                                    action_t*         action )
{
  if ( amount == 0 )
    return 0;

  double mitigated_amount = amount;

  // TODO: Add Force Valor damage reduction for internal and elemental schools. if ( buffs.force_valor -> up() )

  if ( result == RESULT_BLOCK )
  {
    mitigated_amount *= ( 1 - composite_tank_block_reduction() );
    if ( mitigated_amount < 0 ) return 0;
  }

  if ( result == RESULT_CRIT_BLOCK )
  {
    mitigated_amount *= ( 1 - 2 * composite_tank_block_reduction() );
    if ( mitigated_amount < 0 ) return 0;
  }

  if ( school == SCHOOL_KINETIC || school == SCHOOL_ENERGY )
  {

    // Armor
    if ( action )
    {
      double resist = action -> armor() / ( action -> armor() + armor_coeff );

      if ( resist < 0.0 )
        resist = 0.0;
      else if ( resist > 0.75 )
        resist = 0.75;
      mitigated_amount *= 1.0 - resist;


      if ( sim -> debug )
        log_t::output( sim, "%s %.0f armor mitigates %.1f%%", name(), action -> armor(), resist * 100.0 );
    }

  }

  return mitigated_amount;
}

// player_t::assess_heal ====================================================

player_t::heal_info_t player_t::assess_heal(  double            amount,
                                              const school_type /* school */,
                                              int               /* dmg_type */,
                                              int               /* result */,
                                              action_t*         action )
{
  heal_info_t heal;

  heal.amount = resource_gain( RESOURCE_HEALTH, amount, 0, action );
  heal.actual = amount;

  iteration_heal_taken += amount;

  return heal;
}

// player_t::summon_pet =====================================================

void player_t::summon_pet( const char* pet_name,
                           timespan_t  duration )
{
  for ( pet_t* p = pet_list; p; p = p -> next_pet )
  {
    if ( p -> name_str == pet_name )
    {
      p -> summon( duration );
      return;
    }
  }
  sim -> errorf( "Player %s is unable to summon pet '%s'\n", name(), pet_name );
}

// player_t::dismiss_pet ====================================================

void player_t::dismiss_pet( const char* pet_name )
{
  for ( pet_t* p = pet_list; p; p = p -> next_pet )
  {
    if ( p -> name_str == pet_name )
    {
      p -> dismiss();
      return;
    }
  }
  assert( 0 );
}

// player_t::register_callbacks =============================================

void player_t::register_callbacks()
{}

// player_t::register_resource_gain_callback ================================

void player_t::register_resource_gain_callback( int resource,
                                                action_callback_t* cb )
{
  resource_gain_callbacks[ resource ].push_back( cb );
}

// player_t::register_resource_loss_callback ================================

void player_t::register_resource_loss_callback( int resource,
                                                action_callback_t* cb )
{
  resource_loss_callbacks[ resource ].push_back( cb );
}

// player_t::register_attack_callback =======================================

void player_t::register_attack_callback( int64_t mask,
                                         action_callback_t* cb )
{
  for ( int64_t i=0; i < RESULT_MAX; i++ )
  {
    if ( ( i > 0 && mask < 0 ) || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      attack_callbacks[ i ].push_back( cb );
    }
  }
}

// player_t::register_spell_callback ========================================

void player_t::register_spell_callback( int64_t mask,
                                        action_callback_t* cb )
{
  for ( int64_t i=0; i < RESULT_MAX; i++ )
  {
    if ( ( i > 0 && mask < 0 ) || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      spell_callbacks[ i ].push_back( cb );
      heal_callbacks[ i ].push_back( cb );
    }
  }
}

// player_t::register_tick_callback =========================================

void player_t::register_tick_callback( int64_t mask,
                                       action_callback_t* cb )
{
  for ( int64_t i=0; i < RESULT_MAX; i++ )
  {
    if ( ( i > 0 && mask < 0 ) || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      tick_callbacks[ i ].push_back( cb );
    }
  }
}

// player_t::register_heal_callback =========================================

void player_t::register_heal_callback( int64_t mask,
                                       action_callback_t* cb )
{
  for ( int64_t i=0; i < RESULT_MAX; i++ )
  {
    if ( ( i > 0 && mask < 0 ) || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      heal_callbacks[ i ].push_back( cb );
    }
  }
}

// player_t::register_harmful_spell_callback ================================

void player_t::register_harmful_spell_callback( int64_t mask,
                                                action_callback_t* cb )
{
  for ( int64_t i=0; i < RESULT_MAX; i++ )
  {
    if ( ( i > 0 && mask < 0 ) || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      spell_callbacks[ i ].push_back( cb );
    }
  }
}

// player_t::register_tick_damage_callback ==================================

void player_t::register_tick_damage_callback( int64_t mask,
                                              action_callback_t* cb )
{
  for ( int64_t i=0; i < SCHOOL_MAX; i++ )
  {
    if ( mask < 0 || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      tick_damage_callbacks[ i ].push_back( cb );
    }
  }
}

// player_t::register_direct_damage_callback ================================

void player_t::register_direct_damage_callback( int64_t mask,
                                                action_callback_t* cb )
{
  for ( int64_t i=0; i < SCHOOL_MAX; i++ )
  {
    if ( mask < 0 || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      direct_damage_callbacks[ i ].push_back( cb );
    }
  }
}

// player_t::register_tick_heal_callback ====================================

void player_t::register_tick_heal_callback( int64_t mask,
                                            action_callback_t* cb )
{
  for ( int64_t i=0; i < SCHOOL_MAX; i++ )
  {
    if ( mask < 0 || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      tick_heal_callbacks[ i ].push_back( cb );
    }
  }
}

// player_t::register_direct_heal_callback ==================================

void player_t::register_direct_heal_callback( int64_t mask,
                                              action_callback_t* cb )
{
  for ( int64_t i=0; i < SCHOOL_MAX; i++ )
  {
    if ( mask < 0 || ( mask & ( int64_t( 1 ) << i ) ) )
    {
      direct_heal_callbacks[ i ].push_back( cb );
    }
  }
}

// player_t::alacrity =======================================================

double player_t::alacrity() const
{ return 1.0 - rating_t::alacrity_from_rating( alacrity_rating, level ); }

// player_t::recalculate_surge ==============================================

void player_t::recalculate_surge()
{ surge_bonus = rating_t::surge_from_rating( surge_rating, level ); }

// player_t::find_action ====================================================

action_t* player_t::find_action( const std::string& str )
{
  for ( action_t* a = action_list; a; a = a -> next )
    if ( str == a -> name_str )
      return a;

  return 0;
}

// player_t::aura_gain ======================================================

void player_t::aura_gain( const char* aura_name , double value )
{
  if ( sim -> log && ! sleeping )
  {
    log_t::output( sim, "%s gains %s ( value=%.2f )", name(), aura_name, value );
  }
}

// player_t::aura_loss ======================================================

void player_t::aura_loss( const char* aura_name , double /* value */ )
{
  if ( sim -> log && ! sleeping )
  {
    log_t::output( sim, "%s loses %s", name(), aura_name );
  }
}

// player_t::find_cooldown ==================================================

cooldown_t* player_t::find_cooldown( const std::string& name ) const
{
  for ( cooldown_t* c = cooldown_list; c; c = c -> next )
  {
    if ( c -> name_str == name )
      return c;
  }

  return 0;
}

// player_t::find_dot =======================================================

dot_t* player_t::find_dot( const std::string& name ) const
{
  for ( dot_t* d = dot_list; d; d = d -> next )
  {
    if ( d -> name_str == name )
      return d;
  }

  return 0;
}

// player_t::find_action_priority_list( const std::string& name ) ===========

action_priority_list_t* player_t::find_action_priority_list( const std::string& name ) const
{
  for ( unsigned int i = 0; i < action_priority_list.size(); i++ )
  {
    action_priority_list_t* a = action_priority_list[i];
    if ( a -> name_str == name )
      return a;
  }

  return 0;
}

// player_t::find_set_bonus =======================================================

set_bonus_t* player_t::find_set_bonus( const std::string& name )
{
  set_bonus_t* sb=0;

  for ( sb = set_bonus_list; sb; sb = sb -> next )
  {
    if ( sb -> name == name )
      return sb;
  }

  return 0;
}

// player_t::get_cooldown ===================================================

cooldown_t* player_t::get_cooldown( const std::string& name )
{
  cooldown_t* c=0;

  for ( c = cooldown_list; c; c = c -> next )
  {
    if ( c -> name_str == name )
      return c;
  }

  c = new cooldown_t( name, this );

  cooldown_t** tail = &cooldown_list;

  while ( *tail && name > ( ( *tail ) -> name_str ) )
  {
    tail = &( ( *tail ) -> next );
  }

  c -> next = *tail;
  *tail = c;

  return c;
}

// player_t::get_dot ========================================================

dot_t* player_t::get_dot( const std::string& name )
{
  dot_t* d=0;

  for ( d = dot_list; d; d = d -> next )
  {
    if ( d -> name_str == name )
      return d;
  }

  d = new dot_t( name, this );

  dot_t** tail = &dot_list;

  while ( *tail && name > ( ( *tail ) -> name_str ) )
  {
    tail = &( ( *tail ) -> next );
  }

  d -> next = *tail;
  *tail = d;

  return d;
}

// player_t::get_gain =======================================================

gain_t* player_t::get_gain( const std::string& name )
{
  gain_t* g=0;

  for ( g = gain_list; g; g = g -> next )
  {
    if ( g -> name_str == name )
      return g;
  }

  g = new gain_t( name );

  gain_t** tail = &gain_list;

  while ( *tail && name > ( ( *tail ) -> name_str ) )
  {
    tail = &( ( *tail ) -> next );
  }

  g -> next = *tail;
  *tail = g;

  return g;
}

// player_t::get_proc =======================================================

proc_t* player_t::get_proc( const std::string& name )
{
  proc_t* p=0;

  for ( p = proc_list; p; p = p -> next )
  {
    if ( p -> name_str == name )
      return p;
  }

  p = new proc_t( sim, name );

  proc_t** tail = &proc_list;

  while ( *tail && name > ( ( *tail ) -> name_str ) )
  {
    tail = &( ( *tail ) -> next );
  }

  p -> next = *tail;
  *tail = p;

  p -> player = this;

  return p;
}

// player_t::get_stats ======================================================

stats_t* player_t::get_stats( const std::string& n, action_t* a )
{
  stats_t* stats;

  for ( stats = stats_list; stats; stats = stats -> next )
  {
    if ( stats -> name_str == n )
      break;
  }

  if ( ! stats )
  {
    stats = new stats_t( n, this );

    stats_t** tail= &stats_list;
    while ( *tail && n > ( ( *tail ) -> name_str ) )
    {
      tail = &( ( *tail ) -> next );
    }
    stats -> next = *tail;
    *tail = stats;
  }

  assert( stats -> player == this );

  if ( a ) stats -> action_list.push_back( a );
  return stats;
}

// player_t::get_benefit =====================================================

benefit_t* player_t::get_benefit( const std::string& name )
{
  benefit_t* u=0;

  for ( u = benefit_list; u; u = u -> next )
  {
    if ( u -> name_str == name )
      return u;
  }

  u = new benefit_t( name );

  benefit_t** tail = &benefit_list;

  while ( *tail && name > ( ( *tail ) -> name_str ) )
  {
    tail = &( ( *tail ) -> next );
  }

  u -> next = *tail;
  *tail = u;

  return u;
}

// player_t::get_uptime =====================================================

uptime_t* player_t::get_uptime( const std::string& name )
{
  uptime_t* u=0;

  for ( u = uptime_list; u; u = u -> next )
  {
    if ( u -> name_str == name )
      return u;
  }

  u = new uptime_t( sim, name );

  uptime_t** tail = &uptime_list;

  while ( *tail && name > ( ( *tail ) -> name_str ) )
  {
    tail = &( ( *tail ) -> next );
  }

  u -> next = *tail;
  *tail = u;

  return u;
}

// player_t::get_rng ========================================================

rng_t* player_t::get_rng( const std::string& n, int type )
{
  assert( sim -> rng );

  if ( type == RNG_GLOBAL ) return sim -> rng;
  if ( type == RNG_DETERMINISTIC ) return sim -> deterministic_rng;

  if ( ! sim -> smooth_rng ) return ( sim -> deterministic_roll ? sim -> deterministic_rng : sim -> rng );

  rng_t* rng=0;

  for ( rng = rng_list; rng; rng = rng -> next )
  {
    if ( rng -> name_str == n )
      return rng;
  }

  if ( ! rng )
  {
    rng = rng_t::create( sim, n, type );
    rng -> next = rng_list;
    rng_list = rng;
  }

  return rng;
}

// player_t::get_set_bonus =======================================================

set_bonus_t* player_t::get_set_bonus( const std::string& name, std::string filter )
{
  set_bonus_t* sb=0;

  for ( sb = set_bonus_list; sb; sb = sb -> next )
  {
    if ( sb -> name == name )
      return sb;
  }

  sb = new set_bonus_t( name, filter );

  set_bonus_t** tail = &set_bonus_list;

  while ( *tail && name > ( ( *tail ) -> name ) )
  {
    tail = &( ( *tail ) -> next );
  }

  sb -> next = *tail;
  *tail = sb;

  return sb;
}

// player_t::get_position_distance ==========================================

double player_t::get_position_distance( double m, double v ) const
{
  // Square of Euclidean distance since sqrt() is slow
  double delta_x = this -> x_position - m;
  double delta_y = this -> y_position - v;
  return delta_x * delta_x + delta_y * delta_y;
}

// player_t::get_player_distance ============================================

double player_t::get_player_distance( const player_t* p ) const
{ return get_position_distance( p -> x_position, p -> y_position ); }

// player_t::get_action_priority_list( const std::string& name ) ============

action_priority_list_t* player_t::get_action_priority_list( const std::string& name )
{
  action_priority_list_t* a = find_action_priority_list( name );
  if ( ! a )
  {
    a = new action_priority_list_t( name, this );
    action_priority_list.push_back( a );
  }
  return a;
}

// player_t::debuffs_t::snared ==============================================

bool player_t::debuffs_t::snared()
{
  return false;
}

// Summon Pet Spell =========================================================

struct summon_companion_t : public action_t
{
  timespan_t summoning_duration;
  pet_t* pet;

private:
  void _init_summon_companion_t( const std::string& pet_name )
  {
    harmful = false;

    pet = player -> find_pet( pet_name.c_str() );
    if ( ! pet )
    {
      sim -> errorf( "Player %s unable to find pet %s for summons.\n", player -> name(), pet_name.c_str() );
      sim -> cancel();
    }
  }

public:
  summon_companion_t( player_t* p, const std::string& options_str ) :
    action_t( ACTION_OTHER, "summon_companion", p ), summoning_duration ( timespan_t::zero ), pet( 0 )
  {
    std::string pet_name;
    option_t options[] =
    {
      { "name", OPT_STRING,  &pet_name     },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    _init_summon_companion_t( pet_name );
  }

  virtual void execute()
  {
    pet -> summon( timespan_t::zero );

    player -> active_companion = pet;

    action_t::execute();
  }

  virtual void calculate_result()
  {
    result = RESULT_NONE;
  }

  virtual void schedule_execute()
  {
    action_t::schedule_execute();

    if ( player -> active_companion )
      player -> active_companion -> dismiss();
  }

  virtual bool ready()
  {
    if ( player -> active_companion == pet )
      return false;

    return action_t::ready();
  }

};

// Chosen Movement Actions ==================================================

struct start_moving_t : public action_t
{
  start_moving_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "start_moving", player )
  {
    parse_options( NULL, options_str );
    trigger_gcd = timespan_t::zero;
    cooldown -> duration = timespan_t::from_seconds( 0.5 );
    harmful = false;
  }

  virtual void execute()
  {
    player -> buffs.self_movement -> trigger();

    if ( sim -> log ) log_t::output( sim, "%s starts moving.", player -> name() );
    update_ready();
  }

  virtual bool ready()
  {
    if ( player -> buffs.self_movement -> check() )
      return false;

    return action_t::ready();
  }
};

struct stop_moving_t : public action_t
{
  stop_moving_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "stop_moving", player )
  {
    parse_options( NULL, options_str );
    trigger_gcd = timespan_t::zero;
    cooldown -> duration = timespan_t::from_seconds( 0.5 );
    harmful = false;
  }

  virtual void execute()
  {
    player -> buffs.self_movement -> expire();

    if ( sim -> log ) log_t::output( sim, "%s stops moving.", player -> name() );
    update_ready();
  }

  virtual bool ready()
  {
    if ( ! player -> buffs.self_movement -> check() )
      return false;

    return action_t::ready();
  }
};

// ===== Racial Abilities ===================================================


// Cycle Action =============================================================

struct cycle_t : public action_t
{
  action_t* current_action;

  cycle_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "cycle", player ), current_action( 0 )
  {
    parse_options( NULL, options_str );
  }

  virtual void reset()
  {
    action_t::reset();

    if ( ! current_action )
    {
      current_action = next;
      if ( ! current_action )
      {
        sim -> errorf( "Player %s has no actions after 'cycle'\n", player -> name() );
        sim -> cancel();
      }
      for ( action_t* a = next; a; a = a -> next ) a -> background = true;
    }
  }

  virtual void schedule_execute()
  {
    player -> last_foreground_action = current_action;
    current_action -> schedule_execute();
    current_action = current_action -> next;
    if ( ! current_action ) current_action = next;
  }

  virtual bool ready()
  {
    if ( ! current_action ) return false;

    return current_action -> ready();
  }
};

// Restart Sequence Action ==================================================

struct restart_sequence_t : public action_t
{
  sequence_t* seq;
  std::string seq_name_str;

  restart_sequence_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "restart_sequence", player ), seq( 0 )
  {
    seq_name_str = "default"; // matches default name for sequences
    option_t options[] =
    {
      { "name", OPT_STRING, &seq_name_str },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = timespan_t::zero;
  }

  virtual void execute()
  {
    if ( ! seq )
    {
      for ( action_t* a = player -> action_list; a; a = a -> next )
      {
        if ( a -> type != ACTION_SEQUENCE )
          continue;

        if ( ! seq_name_str.empty() )
          if ( seq_name_str != a -> name_str )
            continue;

        seq = dynamic_cast< sequence_t* >( a );
      }

      assert( seq );
    }

    seq -> restart();
  }

  virtual bool ready()
  {
    if ( seq ) return ! seq -> restarted;
    return action_t::ready();
  }
};

#if 0
// Restore Mana Action ======================================================

struct restore_mana_t : public action_t
{
  double mana;

  restore_mana_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "restore_mana", player ), mana( 0 )
  {
    option_t options[] =
    {
      { "mana", OPT_FLT, &mana },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = timespan_t::zero;
  }

  virtual void execute()
  {
    double mana_missing = player -> resource_max[ RESOURCE_MANA ] - player -> resource_current[ RESOURCE_MANA ];
    double mana_gain = mana;

    if ( mana_gain == 0 || mana_gain > mana_missing ) mana_gain = mana_missing;

    if ( mana_gain > 0 )
    {
      player -> resource_gain( RESOURCE_MANA, mana_gain, player -> gains.restore_mana );
    }
  }
};
#endif

// Snapshot Stats ===========================================================

struct snapshot_stats_t : public action_t
{
  bool completed;

  snapshot_stats_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "snapshot_stats", player ),
    completed( false )
  {
    parse_options( NULL, options_str );
    trigger_gcd = timespan_t::zero;
  }

  virtual void execute()
  {
    player_t* p = player;

    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );

    p -> buffed.alacrity  = p -> alacrity();

    for ( int i=ATTRIBUTE_NONE; i < ATTRIBUTE_MAX; ++i )
      p -> buffed.attribute[ i ] = floor( p -> get_attribute( static_cast<attribute_type>( i ) ) );

    range::copy( p -> resource_max, p -> buffed.resource );

    p -> buffed.power             = p -> composite_power();
    p -> buffed.force_power       = p -> composite_force_power();
    p -> buffed.tech_power        = p -> composite_tech_power();

    p -> buffed_surge             = p -> surge_bonus;

    p -> buffed.melee_hit         = p -> melee_hit_chance();
    p -> buffed.melee_crit        = p -> melee_crit_chance();
    p -> buffed.melee_avoidance   = p -> melee_avoidance();

    p -> buffed.range_hit         = p -> range_hit_chance();
    p -> buffed.range_crit        = p -> range_crit_chance();
    p -> buffed.range_avoidance   = p -> range_avoidance();

    p -> buffed.force_hit         = p -> force_hit_chance();
    p -> buffed.force_crit        = p -> force_crit_chance();
    p -> buffed.force_avoidance   = p -> force_avoidance();

    p -> buffed.tech_hit          = p -> tech_hit_chance();
    p -> buffed.tech_crit         = p -> tech_crit_chance();
    p -> buffed.tech_avoidance    = p -> tech_avoidance();

    p -> buffed_armor       = p -> composite_armor();
    p -> buffed_miss        = p -> composite_tank_miss( SCHOOL_KINETIC );
    p -> buffed_dodge       = p -> composite_tank_dodge();
    p -> buffed_parry       = p -> composite_tank_parry();
    p -> buffed_block       = p -> composite_tank_block();

    double spell_hit_extra=0, attack_hit_extra=0;

#if 0
    int role = p -> primary_role();
    int delta_level = sim -> target -> level - p -> level;

    if ( role == ROLE_SPELL || role == ROLE_HYBRID || role == ROLE_HEAL )
    {
      if ( ! spell ) spell = new spell_t( "snapshot_spell", p );
      spell -> background = true;
      spell -> player_buff();
      spell -> target_debuff( target, DMG_DIRECT );
      double chance = spell -> miss_chance( delta_level );
      if ( chance < 0 ) spell_hit_extra = -chance * p -> rating.spell_hit;
    }

    if ( role == ROLE_ATTACK || role == ROLE_HYBRID || role == ROLE_TANK )
    {
      if ( ! attack ) attack = new attack_t( "snapshot_attack", p );
      attack -> background = true;
      attack -> player_buff();
      attack -> target_debuff( target, DMG_DIRECT );
      double chance = attack -> miss_chance( delta_level );
      if ( p -> dual_wield() ) chance += 0.19;
      if ( chance < 0 ) attack_hit_extra = -chance * p -> rating.attack_hit;
      chance = attack -> dodge_chance(  delta_level );
      if ( chance < 0 ) expertise_extra = -chance * 4 * p -> rating.expertise;
    }
#endif

    p -> over_cap[ STAT_HIT_RATING ] = std::max( spell_hit_extra, attack_hit_extra );

    completed = true;
  }

  virtual bool ready()
  {
    if ( sim -> current_iteration > 0 ) return false;
    if ( completed ) return false;
    return action_t::ready();
  }
};

// Wait Fixed Action ========================================================

struct wait_fixed_t : public wait_action_base_t
{
  action_expr_t* time_expr;

  wait_fixed_t( player_t* player, const std::string& options_str ) :
    wait_action_base_t( player, "wait" )
  {
    std::string sec_str = "1.0";

    option_t options[] =
    {
      { "sec", OPT_STRING, &sec_str },
      { NULL,  OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    time_expr = action_expr_t::parse( this, sec_str );
  }

  virtual timespan_t execute_time() const
  {
    int result = time_expr -> evaluate();
    assert( result == TOK_NUM ); ( void )result;
    timespan_t wait = timespan_t::from_seconds( time_expr -> result_num );

    if ( wait <= timespan_t::zero ) wait = player -> available();

    return wait;
  }
};

// Wait Until Ready Action ==================================================

struct wait_until_ready_t : public wait_fixed_t
{
  wait_until_ready_t( player_t* player, const std::string& options_str ) :
    wait_fixed_t( player, options_str )
  {}

  virtual timespan_t execute_time() const
  {
    timespan_t wait = wait_fixed_t::execute_time();
    timespan_t remains = timespan_t::zero;

    for ( action_t* a = player -> action_list; a; a = a -> next )
    {
      if ( a -> background ) continue;

      remains = a -> cooldown -> remains();
      if ( remains > timespan_t::zero && remains < wait ) wait = remains;

      remains = a -> dot() -> remains();
      if ( remains > timespan_t::zero && remains < wait ) wait = remains;
    }

    if ( wait <= timespan_t::zero ) wait = player -> available();

    return wait;
  }
};

// Wait For Cooldown Action =================================================

wait_for_cooldown_t::wait_for_cooldown_t( player_t* player, const char* cd_name ) :
  wait_action_base_t( player, ( "wait_for_" + std::string( cd_name ) ).c_str() ),
  wait_cd( player -> get_cooldown( cd_name ) ), a( player -> find_action( cd_name ) )
{
  assert( a );
}

timespan_t wait_for_cooldown_t::execute_time() const
{ return wait_cd -> remains(); }

// Use Item Actions =========================================================

struct base_use_item_t : public action_t
{
  item_t* item;
  action_t* discharge;
  action_callback_t* trigger;
  stat_buff_t* buff;
  std::string use_name;

  base_use_item_t( player_t* player ) :
    action_t( ACTION_OTHER, "use_item", player ),
    item(), discharge(), trigger(), buff()
  {}

  virtual void init()
  {
    if ( ! item )
      return;

    if ( ! item -> use.active() )
    {
      sim -> errorf( "Player %s attempting 'use_item' action with item '%s' which has no 'use=' encoding.\n", player -> name(), item->name() );
      item = 0;
      return;
    }

    name_str += '_';
    name_str += item -> name();

    stats = player -> get_stats( name_str, this );

    item_t::special_effect_t& e = item -> use;

    use_name = e.name_str.empty() ? item -> name() : e.name_str;

    if ( e.trigger_type )
    {
      if ( e.cost_reduction && e.school && e.discharge_amount )
      {
        trigger = unique_gear_t::register_cost_reduction_proc( e.trigger_type, e.trigger_mask, use_name, player,
                                                               e.school, e.max_stacks, e.discharge_amount,
                                                               e.proc_chance, timespan_t::zero/*dur*/, timespan_t::zero/*cd*/, false, e.reverse, 0 );
      }
      else if ( e.stat )
      {
        trigger = unique_gear_t::register_stat_proc( e.trigger_type, e.trigger_mask, use_name, player,
                                                     e.stat, e.max_stacks, e.stat_amount,
                                                     e.proc_chance, timespan_t::zero/*dur*/, timespan_t::zero/*cd*/, e.tick, e.reverse, 0 );
      }
      else if ( e.school )
      {
        trigger = unique_gear_t::register_discharge_proc( e.trigger_type, e.trigger_mask, use_name, player,
                                                          e.max_stacks, e.school, e.discharge_amount, e.discharge_scaling,
                                                          e.proc_chance, timespan_t::zero/*cd*/, e.no_crit, e.no_player_benefits, e.no_debuffs );
      }

      if ( trigger ) trigger -> deactivate();
    }
    else if ( e.school )
    {
      struct discharge_spell_t : public action_t
      {
        discharge_spell_t( const std::string& n, player_t* p, double a, const school_type s ) :
          action_t( ACTION_ATTACK, n.c_str(), p, force_policy, RESOURCE_NONE, s )
        {
          trigger_gcd = timespan_t::zero;
          dd.base_min = dd.base_max = a;
          may_crit    = true;
          background  = true;
          init();
        }
      };

      discharge = new discharge_spell_t( use_name, player, e.discharge_amount, e.school );
    }
    else if ( e.stat )
    {
      if ( e.max_stacks  == 0 ) e.max_stacks  = 1;
      if ( e.proc_chance == 0 ) e.proc_chance = 1;

      buff = new stat_buff_t( player, use_name, e.stat, e.stat_amount, e.max_stacks, e.duration, timespan_t::zero, e.proc_chance, false, e.reverse );
    }
    else assert( false );

    std::string cooldown_name = use_name + '_' + item -> slot_name();

    cooldown = player -> get_cooldown( cooldown_name );
    cooldown -> duration = item -> use.cooldown;
    trigger_gcd = timespan_t::zero;

    if ( buff != 0 ) buff -> cooldown = cooldown;
  }

  void lockout( timespan_t duration )
  {
    if ( duration <= timespan_t::zero ) return;
    timespan_t ready = sim -> current_time + duration;
    for ( action_t* a = player -> action_list; a; a = a -> next )
    {
      if ( a -> name_str.substr(0, 8) == "use_item" )
      {
        if ( ready > a -> cooldown -> ready )
        {
          a -> cooldown -> ready = ready;
        }
      }
    }
  }

  virtual void execute()
  {
    if ( discharge )
    {
      discharge -> execute();
    }

    else if ( trigger )
    {
      if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), use_name.c_str() );

      trigger -> activate();

      if ( item -> use.duration != timespan_t::zero )
      {
        struct trigger_expiration_t : public event_t
        {
          action_callback_t* trigger;

          trigger_expiration_t( sim_t* sim, player_t* player, item_t* item, action_callback_t* t ) :
            event_t( sim, player ), trigger( t )
          {
            name = item -> name();
            sim -> add_event( this, item -> use.duration );
          }
          virtual void execute()
          {
            trigger -> deactivate();
          }
        };

        new ( sim ) trigger_expiration_t( sim, player, item, trigger );

        lockout( item -> use.duration );
      }
    }

    else if ( buff )
    {
      if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), use_name.c_str() );
      buff -> trigger();
      lockout( buff -> buff_duration );
    }

    else assert( false );

    // Enable to report use_item ability
    //if ( ! dual ) stats -> add_execute( time_to_execute );

    update_ready();
  }

  virtual void reset()
  {
    action_t::reset();
    if ( trigger ) trigger -> deactivate();
  }

  virtual bool ready()
  {
    if ( ! item ) return false;
    return action_t::ready();
  }
};

struct use_item_t : public base_use_item_t
{
  use_item_t( player_t* player, const std::string& options_str ) :
    base_use_item_t( player )
  {
    std::string item_name;
    option_t options[] =
    {
      { "name", OPT_STRING, &item_name },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( item_name.empty() )
    {
      sim -> errorf( "Player %s has 'use_item' action with no 'name=' option.\n", player -> name() );
      return;
    }

    item = player -> find_item( item_name );
    if ( ! item )
    {
      sim -> errorf( "Player %s attempting 'use_item' action with item '%s' which is not currently equipped.\n", player -> name(), item_name.c_str() );
      return;
    }
    if ( ! item -> use.active() )
    {
      sim -> errorf( "Player %s attempting 'use_item' action with item '%s' which has no 'use=' encoding.\n", player -> name(), item_name.c_str() );
      item = 0;
      return;
    }
  }
};

struct use_slot_t : public base_use_item_t
{
  use_slot_t( player_t* player, const std::string& options_str ) :
    base_use_item_t( player )
  {
    std::string slot_name;
    int quiet = 0;

    option_t options[] =
    {
      { "quiet", OPT_BOOL,   &quiet },
      { "slot",  OPT_STRING, &slot_name },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( slot_name.empty() )
    {
      sim -> errorf( "Player %s has 'use_slot' action with no 'slot=' option.\n", player -> name() );
      return;
    }

    int slot = util_t::parse_slot_type( slot_name );
    if ( slot == SLOT_NONE )
    {
      if ( ! quiet )
        sim -> errorf( "Player %s has 'use_slot' action with invalid slot name '%s'.\n", player -> name(), slot_name.c_str() );
      return;
    }

    for ( unsigned i = 0; i < player -> items.size(); ++i )
    {
      if ( player -> items[ i ].slot == slot )
      {
        item = &player -> items[ i ];
        break;
      }
    }

    if ( ! item )
    {
      if ( ! quiet )
      sim -> errorf( "Player %s attempting 'use_slot' action for slot '%s' with no item equipped.\n", player -> name(), slot_name.c_str() );
      return;
    }

    if ( ! item -> use.active() )
    {
      if ( !quiet )
        sim -> errorf( "Player %s attempting 'use_slot' action with item '%s' which has no 'use=' encoding.\n", player -> name(), item -> name() );
      item = 0;
      return;
    }
  }
};


struct use_relics_t : public action_t
{
  use_slot_t *relic1, *relic2;

  use_relics_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "use_relics", player ),
    relic1( new use_slot_t( player, "slot=relic1,quiet=1" ) ),
    relic2( new use_slot_t( player, "slot=relic2,quiet=1" ) )
  {
    parse_options( 0, options_str );

    harmful = false;
    trigger_gcd = timespan_t::zero;
  }

  virtual void execute()
  {
    if ( relic1 -> ready() )
      relic1 -> execute();
    else if ( relic2 -> ready() )
      relic2 -> execute();
  }

  virtual bool ready()
  {
    if ( ! relic1 -> ready() && ! relic2 -> ready () )
      return false;
    return action_t::ready();
  }
};

// Cancel Buff ==============================================================

struct cancel_buff_t : public action_t
{
  buff_t* buff;

  cancel_buff_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "cancel_buff", player ), buff( 0 )
  {
    std::string buff_name;
    option_t options[] =
    {
      { "name", OPT_STRING, &buff_name },
      { NULL,  OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( buff_name.empty() )
    {
      sim -> errorf( "Player %s uses cancel_buff without specifying the name of the buff\n", player -> name() );
      sim -> cancel();
    }

    buff = buff_t::find( player, buff_name );

    if ( ! buff )
    {
      sim -> errorf( "Player %s uses cancel_buff with unknown buff %s\n", player -> name(), buff_name.c_str() );
      sim -> cancel();
    }
    trigger_gcd = timespan_t::zero;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s cancels buff %s", player -> name(), buff -> name() );
    buff -> expire();
  }

  virtual bool ready()
  {
    if ( ! buff || ! buff -> check() )
      return false;
    return action_t::ready();
  }
};

// player_t::create_action ==================================================

action_t* player_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  if ( name == "cancel_buff"      ) return new      cancel_buff_t( this, options_str );
  if ( name == "cycle"            ) return new            cycle_t( this, options_str );
  if ( name == "restart_sequence" ) return new restart_sequence_t( this, options_str );
  //if ( name == "restore_mana"     ) return new     restore_mana_t( this, options_str );
  if ( name == "sequence"         ) return new         sequence_t( this, options_str );
  if ( name == "snapshot_stats"   ) return new   snapshot_stats_t( this, options_str );
  if ( name == "start_moving"     ) return new     start_moving_t( this, options_str );
  if ( name == "stop_moving"      ) return new      stop_moving_t( this, options_str );
  if ( name == "use_item"         ) return new         use_item_t( this, options_str );
  if ( name == "use_slot"         ) return new         use_slot_t( this, options_str );
  if ( name == "use_relics"       ) return new       use_relics_t( this, options_str );
  if ( name == "wait"             ) return new       wait_fixed_t( this, options_str );
  if ( name == "wait_until_ready" ) return new wait_until_ready_t( this, options_str );
  if ( name == "summon_companion" ) return new summon_companion_t( this, options_str );


  return consumable_t::create_action( this, name, options_str );
}

// player_t::find_pet =======================================================

pet_t* player_t::find_pet( const std::string& pet_name )
{
  for ( pet_t* p = pet_list; p; p = p -> next_pet )
    if ( p -> name_str == pet_name )
      return p;

  return 0;
}

// player_t::trigger_replenishment ==========================================

void player_t::trigger_replenishment()
{}

// player_t::parse_talent_trees =============================================

bool player_t::parse_talent_trees( const int encoding[ MAX_TALENT_SLOTS ] )
{
  int index=0;

  for ( int i=0; i < MAX_TALENT_TREES; i++ )
  {
    size_t tree_size = talent_trees[ i ].size();

    for ( size_t j=0; j < tree_size; j++ )
    {
      talent_trees[ i ][ j ] -> set_rank( encoding[ index++ ] );
    }
  }

  return true;
}

// player_t::parse_talents_armory ===========================================

bool player_t::parse_talents_armory( const std::string& talent_string )
{
  int encoding[ MAX_TALENT_SLOTS ];

  size_t i;
  size_t i_max = std::min( talent_string.size(),
                           static_cast< size_t >( MAX_TALENT_SLOTS ) );
  for ( i = 0; i < i_max; i++ )
  {
    char c = talent_string[ i ];
    if ( c < '0' || c > '5' )
    {
      sim -> errorf( "Player %s has illegal character '%c' in talent encoding.\n", name(), c );
      return false;
    }
    encoding[ i ] = c - '0';
  }

  while ( i < MAX_TALENT_SLOTS ) encoding[ i++ ] = 0;

  return parse_talent_trees( encoding );
}

// player_t::parse_talents_wowhead ==========================================

bool player_t::parse_talents_wowhead( const std::string& talent_string )
{
  // wowhead format: [tree_1]Z[tree_2]Z[tree_3] where the trees are character encodings
  // each character expands to a pair of numbers [0-5][0-5]
  // unused deeper talents are simply left blank instead of filling up the string with zero-zero encodings

  int encoding[ MAX_TALENT_SLOTS ];
  range::fill( encoding, 0 );

  int tree = 0;
  unsigned tree_count = 0;
  size_t count = 0;

  for ( unsigned int i=0; i < talent_string.length(); i++ )
  {
    if ( tree >= MAX_TALENT_TREES )
    {
      sim -> errorf( "Player %s has malformed wowhead talent string. Too many talent trees specified.\n", name() );
      return false;
    }

    char c = talent_string[ i ];

    if ( c == '.' ) break;

    if ( c == 'Z' )
    {
      count = 0;
      for ( int j=0; j <= tree; j++ )
        count += talent_trees[ j ].size();
      tree++;
      tree_count = 0;
      continue;
    }

    try
    {
      std::pair<int, int> decoding = wowhead_talent_decode_pair( c );

      encoding[ count++ ] = decoding.first;
      ++tree_count;

      if ( tree_count < talent_trees[ tree ].size() )
      {
        encoding[ count++ ] = decoding.second;
        ++tree_count;
      }

      if ( tree_count > talent_trees[ tree ].size() )
      {
        ++tree;
        tree_count = 0;
      }
    }

    catch ( wowhead_bad_talent_encoding )
    {
      sim -> errorf( "Player %s has malformed wowhead talent string. Translation for '%c' unknown.\n", name(), c );
      return false;
    }
  }

  if ( sim -> debug )
  {
    std::string str_out;
    for ( size_t i = 0; i < count; i++ ) str_out += '0' + encoding[i];
    log_t::output( sim, "%s Wowhead talent string translation: %s\n", name(), str_out.c_str() );
  }

  return parse_talent_trees( encoding );
}

// player_t::create_talents =================================================

void player_t::create_talents()
{}

// player_t::find_talent ====================================================

talent_t* player_t::find_talent( const std::string& n,
                                 int tree )
{
  for ( int i=0; i < MAX_TALENT_TREES; i++ )
  {
    if ( tree != TALENT_TAB_NONE && tree != i )
      continue;

    size_t size=talent_trees[ i ].size();
    for ( size_t j=0; j < size; j++ )
    {
      talent_t* t = talent_trees[ i ][ j ];

      if ( n == t -> name_cstr() )
      {
        return t;
      }
    }
  }

  sim -> errorf( "Player %s unable to find talent %s\n", name(), n.c_str() );
  return 0;
}

#if 0
// player_t::create_glyphs ==================================================

void player_t::create_glyphs()
{
  std::vector<unsigned> glyph_ids = dbc_t::glyphs( util_t::class_id( type ), dbc.ptr );

  size_t size=glyph_ids.size();
  for ( size_t i=0; i < size; i++ )
    glyphs.push_back( new glyph_t( this, spell_data_t::find( glyph_ids[ i ], dbc.ptr ) ) );
}

// player_t::find_glyph =====================================================

glyph_t* player_t::find_glyph( const std::string& n )
{
  size_t size=glyphs.size();
  for ( size_t i=0; i < size; i++ )
  {
    glyph_t* g = glyphs[ i ];
    if ( n == g -> sd -> name_cstr() ) return g;
    if ( n == g -> s_token ) return g; // Armory-ized
  }

  sim -> errorf( "\nPlayer %s unable to find glyph %s\n", name(), n.c_str() );

  return 0;
}
#endif

// player_t::create_expression ==============================================

action_expr_t* player_t::create_expression( action_t* a,
                                            const std::string& name_str )
{
  int resource_type = util_t::parse_resource_type( name_str );
  if ( resource_type != RESOURCE_NONE )
  {
    struct resource_expr_t : public action_expr_t
    {
      int resource_type;
      resource_expr_t( action_t* a, const std::string& n, int r ) : action_expr_t( a, n, TOK_NUM ), resource_type( r ) {}
      virtual int evaluate() { result_num = action -> player -> resource_current[ resource_type ]; return TOK_NUM; }
    };
    return new resource_expr_t( a, name_str, resource_type );
  }
  if ( name_str == "level" )
  {
    struct level_expr_t : public action_expr_t
    {
      level_expr_t( action_t* a ) : action_expr_t( a, "level", TOK_NUM ) {}
      virtual int evaluate() { player_t* p = action -> player; result_num = p -> level; return TOK_NUM; }
    };
    return new level_expr_t( a );
  }
  if ( name_str == "multiplier" )
  {
    struct multiplier_expr_t : public action_expr_t
    {
      multiplier_expr_t( action_t* a ) : action_expr_t( a, "multiplier", TOK_NUM ) {}
      virtual int evaluate() { player_t* p = action -> player; result_num = p -> composite_player_multiplier( action -> school, action ); return TOK_NUM; }
    };
    return new multiplier_expr_t( a );
  }
  if ( name_str == "mana_pct" )
  {
    struct mana_pct_expr_t : public action_expr_t
    {
      mana_pct_expr_t( action_t* a ) : action_expr_t( a, "mana_pct", TOK_NUM ) {}
      virtual int evaluate() { player_t* p = action -> player; result_num = 100 * ( p -> resource_current[ RESOURCE_MANA ] / p -> resource_max[ RESOURCE_MANA ] ); return TOK_NUM; }
    };
    return new mana_pct_expr_t( a );
  }
  if ( name_str == "health_pct" )
  {
    struct health_pct_expr_t : public action_expr_t
    {
      player_t* player;
      health_pct_expr_t( action_t* a, player_t* p ) : action_expr_t( a, "health_pct", TOK_NUM ), player( p ) {}
      virtual int evaluate() { result_num = player -> health_percentage(); return TOK_NUM; }
    };
    return new health_pct_expr_t( a, this );
  }
  if ( name_str == "mana_deficit" )
  {
    struct mana_deficit_expr_t : public action_expr_t
    {
      mana_deficit_expr_t( action_t* a ) : action_expr_t( a, "mana_deficit", TOK_NUM ) {}
      virtual int evaluate() { player_t* p = action -> player; result_num = ( p -> resource_max[ RESOURCE_MANA ] - p -> resource_current[ RESOURCE_MANA ] ); return TOK_NUM; }
    };
    return new mana_deficit_expr_t( a );
  }
  if ( name_str == "in_combat" )
  {
    struct in_combat_expr_t : public action_expr_t
    {
      in_combat_expr_t( action_t* a ) : action_expr_t( a, "in_combat", TOK_NUM ) {}
      virtual int evaluate() { result_num = ( action -> player -> in_combat  ? 1 : 0 ); return TOK_NUM; }
    };
    return new in_combat_expr_t( a );
  }
  if ( name_str == "alacrity" )
  {
    struct alacrity_expr_t : public action_expr_t
    {
      alacrity_expr_t( action_t* a ) : action_expr_t( a, "alacrity", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> player -> alacrity(); return TOK_NUM; }
    };
    return new alacrity_expr_t( a );
  }
  if ( name_str == "energy_regen" )
  {
    struct energy_regen_expr_t : public action_expr_t
    {
      energy_regen_expr_t( action_t* a ) : action_expr_t( a, "energy_regen", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> player -> energy_regen_per_second(); return TOK_NUM; }
    };
    return new energy_regen_expr_t( a );
  }
  if ( name_str == "ammo_regen" )
  {
    struct ammo_regen_expr_t : public action_expr_t
    {
      ammo_regen_expr_t( action_t* a ) : action_expr_t( a, "ammo_regen", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> player -> ammo_regen_per_second(); return TOK_NUM; }
    };
    return new ammo_regen_expr_t( a );
  }
  if ( name_str == "time_to_die" )
  {
    struct time_to_die_expr_t : public action_expr_t
    {
      player_t* player;
      time_to_die_expr_t( action_t* a, player_t* p ) :
        action_expr_t( a, "target_time_to_die", TOK_NUM ), player( p ) {}
      virtual int evaluate() { result_num = player -> time_to_die().total_seconds();  return TOK_NUM; }
    };
    return new time_to_die_expr_t( a, this );
  }
  if ( name_str == "time_to_max_energy" )
  {
    struct time_to_max_energy_expr_t : public action_expr_t
    {
      time_to_max_energy_expr_t( action_t* a ) : action_expr_t( a, "time_to_max_energy", TOK_NUM ) {}
      virtual int evaluate()
      {
        result_num = ( action -> player -> resource_max[ RESOURCE_ENERGY ] -
                       action -> player -> resource_current[ RESOURCE_ENERGY ] ) /
                     action -> player -> energy_regen_per_second(); return TOK_NUM;
      }
    };
    return new time_to_max_energy_expr_t( a );
  }
  if ( name_str == "time_to_max_ammo" )
  {
    struct time_to_max_ammo_expr_t : public action_expr_t
    {
      time_to_max_ammo_expr_t( action_t* a ) : action_expr_t( a, "time_to_max_ammo", TOK_NUM ) {}
      virtual int evaluate()
      {
        result_num = ( action -> player -> resource_max[ RESOURCE_AMMO ] -
                       action -> player -> resource_current[ RESOURCE_AMMO ] ) /
                     action -> player -> ammo_regen_per_second(); return TOK_NUM;
      }
    };
    return new time_to_max_ammo_expr_t( a );
  }
  if ( name_str == "max_energy" )
  {
    struct max_energy_expr_t : public action_expr_t
    {
      max_energy_expr_t( action_t* a ) : action_expr_t( a, "max_energy", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> player -> resource_max[ RESOURCE_ENERGY ]; return TOK_NUM; }
    };
    return new max_energy_expr_t( a );
  }
  if ( name_str == "max_ammo" )
  {
    struct max_ammo_expr_t : public action_expr_t
    {
      max_ammo_expr_t( action_t* a ) : action_expr_t( a, "max_ammo", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> player -> resource_max[ RESOURCE_AMMO ]; return TOK_NUM; }
    };
    return new max_ammo_expr_t( a );
  }
  if ( name_str == "max_rage" )
  {
    struct max_rage_expr_t : public action_expr_t
    {
      max_rage_expr_t( action_t* a ) : action_expr_t( a, "max_rage", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> player -> resource_max[ RESOURCE_RAGE ]; return TOK_NUM; }
    };
    return new max_rage_expr_t( a );
  }
  if ( name_str == "max_mana" )
  {
    struct max_mana_expr_t : public action_expr_t
    {
      max_mana_expr_t( action_t* a ) : action_expr_t( a, "max_mana", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> player -> resource_max[ RESOURCE_MANA ]; return TOK_NUM; }
    };
    return new max_mana_expr_t( a );
  }
  if ( name_str == "ptr" )
  {
    struct ptr_expr_t : public action_expr_t
    {
      ptr_expr_t( action_t* a ) : action_expr_t( a, "ptr", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> player -> dbc.ptr ? 1 : 0; return TOK_NUM; }
    };
    return new ptr_expr_t( a );
  }
  if ( name_str == "position_front" )
  {
    struct position_front_expr_t : public action_expr_t
    {
      position_front_expr_t( action_t* a ) : action_expr_t( a, "position_front", TOK_NUM ) {}
      virtual int evaluate() { result_num = ( action -> player -> position == POSITION_FRONT || action -> player -> position == POSITION_RANGED_FRONT ) ? 1 : 0; return TOK_NUM; }
    };
    return new position_front_expr_t( a );
  }
  if ( name_str == "position_back" )
  {
    struct position_back_expr_t : public action_expr_t
    {
      position_back_expr_t( action_t* a ) : action_expr_t( a, "position_back", TOK_NUM ) {}
      virtual int evaluate() { result_num = ( action -> player -> position == POSITION_BACK || action -> player -> position == POSITION_RANGED_BACK ) ? 1 : 0; return TOK_NUM; }
    };
    return new position_back_expr_t( a );
  }
  std::vector<std::string> splits;
  int num_splits = util_t::string_split( splits, name_str, "." );
  if ( splits[ 0 ] == "pet" )
  {
    pet_t* pet = find_pet( splits[ 1 ] );
    if ( pet )
    {
      if ( splits[ 2 ] == "active" )
      {
        struct pet_active_expr_t : public action_expr_t
        {
          pet_t* pet;
          pet_active_expr_t( action_t* a, pet_t* p ) : action_expr_t( a, "pet_active", TOK_NUM ), pet( p ) {}
          virtual int evaluate() { result_num = ( pet -> sleeping ) ? 0 : 1; return TOK_NUM; }
        };
        return new pet_active_expr_t( a, pet );
      }
      else if ( splits[ 2 ] == "remains" )
      {
        struct pet_remains_expr_t : public action_expr_t
        {
          pet_t* pet;
          pet_remains_expr_t( action_t* a, pet_t* p ) : action_expr_t( a, "pet_remains", TOK_NUM ), pet( p ) {}
          virtual int evaluate() { result_num = ( pet -> expiration && pet -> expiration-> remains() > timespan_t::zero ) ? pet -> expiration -> remains().total_seconds() : 0; return TOK_NUM; }
        };
        return new pet_remains_expr_t( a, pet );
      }
      else
      {
        return pet -> create_expression( a, name_str.substr( splits[ 1 ].length() + 5 ) );
      }
    }
  }
  else if ( splits[ 0 ] == "owner" )
  {
    if ( is_pet() )
    {
      pet_t* pet = ( pet_t* ) this;

      if ( pet-> owner )
      {
        return pet -> owner -> create_expression( a, name_str.substr( 6 ) );
      }
    }
  }
  else if ( splits[ 0 ] == "temporary_bonus" )
  {
    int stat = util_t::parse_stat_type( splits[ 1 ] );

    if ( stat != STAT_NONE )
    {
      double* p_stat = 0;
      int attr = -1;

      switch ( stat )
      {
        case STAT_STRENGTH:         p_stat = &( a -> player -> temporary.attribute[ ATTR_STRENGTH  ] ); attr = ATTR_STRENGTH;  break;
        case STAT_AIM:              p_stat = &( a -> player -> temporary.attribute[ ATTR_AIM       ] ); attr = ATTR_AIM;       break;
        case STAT_CUNNING:          p_stat = &( a -> player -> temporary.attribute[ ATTR_CUNNING   ] ); attr = ATTR_CUNNING;   break;
        case STAT_WILLPOWER:        p_stat = &( a -> player -> temporary.attribute[ ATTR_WILLPOWER ] ); attr = ATTR_WILLPOWER; break;
        case STAT_ENDURANCE:        p_stat = &( a -> player -> temporary.attribute[ ATTR_ENDURANCE ] ); attr = ATTR_ENDURANCE; break;
        case STAT_PRESENCE:         p_stat = &( a -> player -> temporary.attribute[ ATTR_PRESENCE  ] ); attr = ATTR_PRESENCE;  break;
        case STAT_EXPERTISE_RATING: p_stat = &( a -> player -> temporary.expertise_rating            ); break;
        case STAT_HIT_RATING:       p_stat = &( a -> player -> temporary.hit_rating                  ); break;
        case STAT_CRIT_RATING:      p_stat = &( a -> player -> temporary.crit_rating                 ); break;
        case STAT_ALACRITY_RATING:  p_stat = &( a -> player -> temporary.alacrity_rating             ); break;
        case STAT_ARMOR:            p_stat = &( a -> player -> temporary.armor                       ); break;
        case STAT_DODGE_RATING:     p_stat = &( a -> player -> temporary.dodge_rating                ); break;
        case STAT_PARRY_RATING:     p_stat = &( a -> player -> temporary.parry_rating                ); break;
        case STAT_BLOCK_RATING:     p_stat = &( a -> player -> temporary.block_rating                ); break;
        case STAT_POWER:            p_stat = &( a -> player -> temporary.power                       ); break;
        case STAT_FORCE_POWER:      p_stat = &( a -> player -> temporary.force_power                 ); break;
        case STAT_TECH_POWER:       p_stat = &( a -> player -> temporary.tech_power                  ); break;
        case STAT_SURGE_RATING:     p_stat = &( a -> player -> temporary.surge_rating                ); break;
        case STAT_DEFENSE_RATING:   p_stat = &( a -> player -> temporary.defense_rating              ); break;

        default: assert( 0 ); break;
      }

      if ( p_stat )
      {
        struct temporary_stat_expr_t : public action_expr_t
        {
          int attr;
          int stat_type;
          double& stat;
          temporary_stat_expr_t( action_t* a, double* p_stat, int stat_type, int attr ) :
            action_expr_t( a, "temporary_stat", TOK_NUM ), attr( attr ), stat_type( stat_type ), stat( *p_stat ) { }

          virtual int evaluate()
          {
            result_num = stat;
            if ( attr != -1 )
              result_num *= action -> player -> composite_attribute_multiplier( attr );

            return TOK_NUM;
          }
        };
        return new temporary_stat_expr_t( a, p_stat, stat, attr );
      }
    }
  }
  else if ( num_splits == 3 )
  {
    if ( splits[ 0 ] == "buff" || splits[ 0 ] == "debuff" || splits[ 0 ] == "aura" )
    {
      buff_t* buff;
      buff = sim->get_targetdata_aura( a -> player, this, splits[1] );
      if ( ! buff ) buff = buff_t::find( this, splits[ 1 ] );
      if ( ! buff ) buff = buff_t::find( sim, splits[ 1 ] );
      if ( ! buff ) return 0;
      return buff -> create_expression( a, splits[ 2 ] );
    }
    else if ( splits[ 0 ] == "cooldown" )
    {
      cooldown_t* cooldown = get_cooldown( splits[ 1 ] );
      if ( splits[ 2 ] == "remains" )
      {
        struct cooldown_remains_expr_t : public action_expr_t
        {
          cooldown_t* cooldown;
          cooldown_remains_expr_t( action_t* a, cooldown_t* c ) : action_expr_t( a, "cooldown_remains", TOK_NUM ), cooldown( c ) {}
          virtual int evaluate() { result_num = cooldown -> remains().total_seconds(); return TOK_NUM; }
        };
        return new cooldown_remains_expr_t( a, cooldown );
      }
    }
    else if ( splits[ 0 ] == "dot" )
    {
      dot_t* dot = 0;
      dot = sim->get_targetdata_dot( a -> player, this, splits[1] );
      if ( ! dot )
        dot = get_dot( splits[ 1 ] );
      if ( ! dot )
        return 0;
      if ( splits[ 2 ] == "duration" )
      {
        struct duration_expr_t : public action_expr_t
        {
          duration_expr_t( action_t* a ) : action_expr_t( a, "dot_duration", TOK_NUM ) {}
          virtual int evaluate() { result_num = action -> num_ticks * action -> tick_time().total_seconds(); return TOK_NUM; }
        };
        return new duration_expr_t( a );
      }
      if ( splits[ 2 ] == "multiplier" )
      {
        struct multiplier_expr_t : public action_expr_t
        {
          multiplier_expr_t( action_t* a ) : action_expr_t( a, "dot_multiplier", TOK_NUM ) {}
          virtual int evaluate() { result_num = action -> player_multiplier; return TOK_NUM; }
        };
        return new multiplier_expr_t( a );
      }
      if ( splits[ 2 ] == "remains" )
      {
        struct dot_remains_expr_t : public action_expr_t
        {
          dot_t* dot;
          dot_remains_expr_t( action_t* a, dot_t* d ) : action_expr_t( a, "dot_remains", TOK_NUM ), dot( d ) {}
          virtual int evaluate() { result_num = dot -> remains().total_seconds(); return TOK_NUM; }
        };
        return new dot_remains_expr_t( a, dot );
      }
      if ( splits[ 2 ] == "ticks_remain" )
      {
        struct dot_ticks_remain_expr_t : public action_expr_t
        {
          dot_t* dot;
          dot_ticks_remain_expr_t( action_t* a, dot_t* d ) : action_expr_t( a, "dot_ticks_remain", TOK_NUM ), dot( d ) {}
          virtual int evaluate() { result_num = dot -> ticks(); return TOK_NUM; }
        };
        return new dot_ticks_remain_expr_t( a, dot );
      }
      else if ( splits[ 2 ] == "ticking" )
      {
        struct dot_ticking_expr_t : public action_expr_t
        {
          dot_t* dot;
          dot_ticking_expr_t( action_t* a, dot_t* d ) : action_expr_t( a, "dot_ticking", TOK_NUM ), dot( d ) {}
          virtual int evaluate() { result_num = dot -> ticking; return TOK_NUM; }
        };
        return new dot_ticking_expr_t( a, dot );
      }
    }
    else if ( splits[ 0 ] == "action" )
    {
      std::vector<action_t*> in_flight_list;
      for ( action_t* action = action_list; action; action = action -> next )
      {
        if ( action -> name_str == splits[ 1 ] )
        {
          if ( splits[ 2 ] == "in_flight" )
          {
            in_flight_list.push_back( action );
          }
          else
          {
            return action -> create_expression( splits[ 2 ] );
          }
        }
      }
      if ( in_flight_list.size() > 0 )
      {
        struct in_flight_multi_expr_t : public action_expr_t
        {
          std::vector<action_t*> action_list;
          in_flight_multi_expr_t( std::vector<action_t*> al ) : action_expr_t( al[ 0 ], "in_flight", TOK_NUM ), action_list( al ) {}
          virtual int evaluate()
          {
            result_num = false;
            for ( int i = 0; i < ( int ) action_list.size(); i++ )
            {
              if ( action_list[ i ] -> travel_event != NULL ) result_num = true;
            }
            return TOK_NUM;
          }
        };
        return new in_flight_multi_expr_t( in_flight_list );
      }
    }
  }
  /*else if ( num_splits == 2 )
  {
    if ( splits[ 0 ] == "set_bonus" )
    {
      return a -> player -> set_bonus.create_expression( a, splits[ 1 ] );
    }
  }*/

  if ( num_splits >= 2 && splits[ 0 ] == "target" )
  {
    std::string rest = splits[1];
    for ( int i = 2; i < num_splits; ++i )
      rest += '.' + splits[i];
    return target -> create_expression( a, rest );
  }

  return sim -> create_expression( a, name_str );
}

// player_t::create_profile =================================================

bool player_t::create_profile( std::string& profile_str, int save_type, bool save_html )
{
  std::string term;

  if ( save_html )
    term = "<br>\n";
  else
    term = "\n";

  if ( save_type == SAVE_ALL )
  {
    profile_str += "#!./simc " + term + term;
  }

  if ( ! comment_str.empty() )
  {
    profile_str += "# " + comment_str + term;
  }

  if ( save_type == SAVE_ALL )
  {
    std::string pname = name_str;

    profile_str += util_t::player_type_string( type );
    profile_str += "=" + util_t::format_text( pname, sim -> input_is_utf8 ) + term;
    profile_str += "origin=\"" + origin_str + "\"" + term;
    profile_str += "level=" + util_t::to_string( level ) + term;
    profile_str += "race=" + race_str + term;
    profile_str += "position=" + position_str + term;
    profile_str += "role=";
    profile_str += util_t::role_type_string( primary_role() ) + term;
    profile_str += "use_pre_potion=" + util_t::to_string( use_pre_potion ) + term;

    if ( professions_str.size() > 0 )
    {
      profile_str += "professions=" + professions_str + term;
    };
  }

  if ( save_type == SAVE_ALL || save_type == SAVE_TALENTS )
  {
    talents_str = torhead_encode_talents( *this );
    if ( ! talents_str.empty() )
      profile_str += "talents=" + talents_str + term;
  }

  if ( save_type == SAVE_ALL || save_type == SAVE_ACTIONS )
  {
    if ( ! action_list_str.empty() )
    {
      int i = 0;
      for ( action_t* a = action_list; a; a = a -> next )
      {
        if ( a -> signature_str.empty() ) continue;
        profile_str += "actions";
        profile_str += i ? "+=/" : "=";
        std::string encoded_action = a -> signature_str;
        if ( save_html )
          report_t::encode_html( encoded_action );
        profile_str += encoded_action + term;
        i++;
      }
    }
  }

  if ( save_type == SAVE_ALL || save_type == SAVE_GEAR )
  {
    for ( int i=0; i < SLOT_MAX; i++ )
    {
      item_t& item = items[ i ];

      if ( item.active() )
      {
        profile_str += item.slot_name();
        profile_str += "=" + item.options_str + term;
      }
    }
    if ( ! items_str.empty() )
    {
      profile_str += "items=" + items_str + term;
    }

    profile_str += "# Gear Summary" + term;
    for ( int i=0; i < STAT_MAX; i++ )
    {
      double value = initial_stats.get_stat( i );
      if ( value != 0 )
      {
        profile_str += "# gear_";
        profile_str += util_t::stat_type_string( i );
        profile_str += "=" + util_t::to_string( value, 0 ) + term;
      }
    }
    profile_str += "# Set Bonuses" + term;
    for ( set_bonus_t* sb = set_bonus_list; sb; sb = sb -> next )
    {
      if ( sb -> two_pc() )  profile_str += "# set_bonus=" + sb -> name + "_2pc" + term ;
      if ( sb -> four_pc() ) profile_str += "# set_bonus=" + sb -> name + "_4pc" + term ;
    }

    for ( int i=0; i < SLOT_MAX; i++ )
    {
      item_t& item = items[ i ];
      if ( ! item.active() ) continue;
      if ( item.unique || item.unique_enchant || item.unique_addon || ! item.encoded_weapon_str.empty() )
      {
        profile_str += "# ";
        profile_str += item.slot_name();
        profile_str += "=";
        profile_str += item.name();
        if ( item.heroic() ) profile_str += ",heroic=1";
        if ( ! item.encoded_weapon_str.empty() ) profile_str += ",weapon=" + item.encoded_weapon_str;
        if ( item.unique_enchant ) profile_str += ",enchant=" + item.encoded_enchant_str;
        if ( item.unique_addon   ) profile_str += ",addon="   + item.encoded_addon_str;
        profile_str += term;
      }
    }

    if ( enchant.attribute[ ATTR_STRENGTH  ] != 0 )  profile_str += "enchant_strength="         + util_t::to_string( enchant.attribute[ ATTR_STRENGTH  ] ) + term;
    if ( enchant.attribute[ ATTR_AIM       ] != 0 )  profile_str += "enchant_aim="              + util_t::to_string( enchant.attribute[ ATTR_AIM       ] ) + term;
    if ( enchant.attribute[ ATTR_CUNNING   ] != 0 )  profile_str += "enchant_cunning="          + util_t::to_string( enchant.attribute[ ATTR_CUNNING   ] ) + term;
    if ( enchant.attribute[ ATTR_WILLPOWER ] != 0 )  profile_str += "enchant_willpower="        + util_t::to_string( enchant.attribute[ ATTR_WILLPOWER ] ) + term;
    if ( enchant.attribute[ ATTR_ENDURANCE ] != 0 )  profile_str += "enchant_endurance="        + util_t::to_string( enchant.attribute[ ATTR_ENDURANCE ] ) + term;
    if ( enchant.attribute[ ATTR_PRESENCE  ] != 0 )  profile_str += "enchant_presence="         + util_t::to_string( enchant.attribute[ ATTR_PRESENCE  ] ) + term;
    if ( enchant.power                       != 0 )  profile_str += "enchant_power="            + util_t::to_string( enchant.power ) + term;
    if ( enchant.force_power                 != 0 )  profile_str += "enchant_force_power="      + util_t::to_string( enchant.force_power ) + term;
    if ( enchant.tech_power                  != 0 )  profile_str += "enchant_tech_power="       + util_t::to_string( enchant.tech_power ) + term;
    if ( enchant.expertise_rating            != 0 )  profile_str += "enchant_expertise_rating=" + util_t::to_string( enchant.expertise_rating ) + term;
    if ( enchant.armor                       != 0 )  profile_str += "enchant_armor="            + util_t::to_string( enchant.armor ) + term;
    if ( enchant.alacrity_rating             != 0 )  profile_str += "enchant_alacrity_rating="  + util_t::to_string( enchant.alacrity_rating ) + term;
    if ( enchant.hit_rating                  != 0 )  profile_str += "enchant_hit_rating="       + util_t::to_string( enchant.hit_rating ) + term;
    if ( enchant.crit_rating                 != 0 )  profile_str += "enchant_crit_rating="      + util_t::to_string( enchant.crit_rating ) + term;
    if ( enchant.resource[ RESOURCE_HEALTH ] != 0 )  profile_str += "enchant_health="           + util_t::to_string( enchant.resource[ RESOURCE_HEALTH ] ) + term;
    if ( enchant.resource[ RESOURCE_MANA   ] != 0 )  profile_str += "enchant_mana="             + util_t::to_string( enchant.resource[ RESOURCE_MANA   ] ) + term;
    if ( enchant.resource[ RESOURCE_RAGE   ] != 0 )  profile_str += "enchant_rage="             + util_t::to_string( enchant.resource[ RESOURCE_RAGE   ] ) + term;
    if ( enchant.resource[ RESOURCE_ENERGY ] != 0 )  profile_str += "enchant_energy="           + util_t::to_string( enchant.resource[ RESOURCE_ENERGY ] ) + term;
    if ( enchant.resource[ RESOURCE_AMMO   ] != 0 )  profile_str += "enchant_ammo="             + util_t::to_string( enchant.resource[ RESOURCE_AMMO   ] ) + term;
}

  return true;
}

// player_t::copy_from ======================================================

void player_t::copy_from( player_t* source )
{
  origin_str = source -> origin_str;
  level = source -> level;
  race_str = source -> race_str;
  role = source -> role;
  professions_str = source -> professions_str;

  position = source -> position;
  position_str = source -> position_str;

  use_pre_potion = source -> use_pre_potion;

  talents_str = "http://www.torhead.com/skill-calc#";
  talents_str += util_t::player_type_string( type );
  talents_str += "-";
  // This is necessary because sometimes the talent trees change shape between live/ptr.
  for ( int i=0; i < MAX_TALENT_TREES; i++ )
  {
    for ( unsigned j = 0; j < talent_trees[ i ].size(); j++ )
    {
      talent_t* t = talent_trees[ i ][ j ];
      talent_t* source_t = source -> find_talent( t -> name_cstr() );
      if ( source_t ) t -> set_rank( source_t -> rank() );
      std::stringstream ss;
      ss << t -> rank();
      talents_str += ss.str();
    }
  }

  action_list_str = source -> action_list_str;
  action_priority_list.clear();
  for ( unsigned int i = 0; i < source -> action_priority_list.size(); i++ )
  {
    action_priority_list.push_back( source -> action_priority_list[ i ] );
  }

  int num_items = ( int ) items.size();
  for ( int i=0; i < num_items; i++ )
  {
    items[ i ] = source -> items[ i ];
    items[ i ].player = this;
  }
  gear = source -> gear;
  enchant = source -> enchant;
}

// player_t::create_options =================================================

void player_t::create_options()
{
  option_t player_options[] =
  {
    // General
    { "name",                                 OPT_STRING,   &( name_str                               ) },
    { "origin",                               OPT_STRING,   &( origin_str                             ) },
    { "region",                               OPT_STRING,   &( region_str                             ) },
    { "server",                               OPT_STRING,   &( server_str                             ) },
    { "id",                                   OPT_STRING,   &( id_str                                 ) },
    { "talents",                              OPT_FUNC,     ( void* ) ::parse_talent_url                },
    //{ "glyphs",                               OPT_STRING,   &( glyphs_str                             ) },
    { "race",                                 OPT_STRING,   &( race_str                               ) },
    { "level",                                OPT_INT,      &( level                                  ) },
    { "use_pre_potion",                       OPT_INT,      &( use_pre_potion                         ) },
    { "role",                                 OPT_FUNC,     ( void* ) ::parse_role_string               },
    { "set_bonus",                            OPT_FUNC,     ( void* ) ::parse_set_bonus                 },
    { "target",                               OPT_STRING,   &( target_str                             ) },
    { "skill",                                OPT_FLT,      &( initial_skill                          ) },
    { "distance",                             OPT_FLT,      &( distance                               ) },
    { "position",                             OPT_STRING,   &( position_str                           ) },
    { "professions",                          OPT_STRING,   &( professions_str                        ) },
    { "actions",                              OPT_STRING,   &( action_list_str                        ) },
    { "actions+",                             OPT_APPEND,   &( action_list_str                        ) },
    { "action_list",                          OPT_STRING,   &( choose_action_list                     ) },
    { "sleeping",                             OPT_BOOL,     &( initial_sleeping                       ) },
    { "quiet",                                OPT_BOOL,     &( quiet                                  ) },
    { "save",                                 OPT_STRING,   &( save_str                               ) },
    { "save_gear",                            OPT_STRING,   &( save_gear_str                          ) },
    { "save_talents",                         OPT_STRING,   &( save_talents_str                       ) },
    { "save_actions",                         OPT_STRING,   &( save_actions_str                       ) },
    { "comment",                              OPT_STRING,   &( comment_str                            ) },
    { "bugs",                                 OPT_BOOL,     &( bugs                                   ) },
    { "world_lag",                            OPT_FUNC,     ( void* ) ::parse_world_lag                 },
    { "world_lag_stddev",                     OPT_FUNC,     ( void* ) ::parse_world_lag_stddev          },
    { "brain_lag",                            OPT_FUNC,     ( void* ) ::parse_brain_lag                 },
    { "brain_lag_stddev",                     OPT_FUNC,     ( void* ) ::parse_brain_lag_stddev          },
    { "scale_player",                         OPT_BOOL,     &( scale_player                           ) },
    // Items
    { "items",                                OPT_STRING,   &( items_str                              ) },
    { "items+",                               OPT_APPEND,   &( items_str                              ) },
    { "head",                                 OPT_STRING,   &( items[ SLOT_HEAD      ].options_str    ) },
    { "ear",                                  OPT_STRING,   &( items[ SLOT_EAR       ].options_str    ) },
    { "shoulders",                            OPT_STRING,   &( items[ SLOT_SHOULDERS ].options_str    ) },
    { "shoulder",                             OPT_STRING,   &( items[ SLOT_SHOULDERS ].options_str    ) },
    { "shirt",                                OPT_STRING,   &( items[ SLOT_SHIRT     ].options_str    ) },
    { "chest",                                OPT_STRING,   &( items[ SLOT_CHEST     ].options_str    ) },
    { "waist",                                OPT_STRING,   &( items[ SLOT_WAIST     ].options_str    ) },
    { "legs",                                 OPT_STRING,   &( items[ SLOT_LEGS      ].options_str    ) },
    { "leg",                                  OPT_STRING,   &( items[ SLOT_LEGS      ].options_str    ) },
    { "feet",                                 OPT_STRING,   &( items[ SLOT_FEET      ].options_str    ) },
    { "foot",                                 OPT_STRING,   &( items[ SLOT_FEET      ].options_str    ) },
    { "wrists",                               OPT_STRING,   &( items[ SLOT_WRISTS    ].options_str    ) },
    { "wrist",                                OPT_STRING,   &( items[ SLOT_WRISTS    ].options_str    ) },
    { "hands",                                OPT_STRING,   &( items[ SLOT_HANDS     ].options_str    ) },
    { "hand",                                 OPT_STRING,   &( items[ SLOT_HANDS     ].options_str    ) },
    { "implant1",                             OPT_STRING,   &( items[ SLOT_IMPLANT_1 ].options_str    ) },
    { "implant2",                             OPT_STRING,   &( items[ SLOT_IMPLANT_2 ].options_str    ) },
    { "relic1",                               OPT_STRING,   &( items[ SLOT_RELIC_1   ].options_str    ) },
    { "relic2",                               OPT_STRING,   &( items[ SLOT_RELIC_2   ].options_str    ) },
    { "back",                                 OPT_STRING,   &( items[ SLOT_BACK      ].options_str    ) },
    { "main_hand",                            OPT_STRING,   &( items[ SLOT_MAIN_HAND ].options_str    ) },
    { "off_hand",                             OPT_STRING,   &( items[ SLOT_OFF_HAND  ].options_str    ) },
    { "ranged",                               OPT_STRING,   &( items[ SLOT_RANGED    ].options_str    ) },
    { "tabard",                               OPT_STRING,   &( items[ SLOT_TABARD    ].options_str    ) },

    // Gear Stats
    { "gear_strength",                        OPT_FLT,  &( gear.attribute[ ATTR_STRENGTH  ]           ) },
    { "gear_aim",                             OPT_FLT,  &( gear.attribute[ ATTR_AIM       ]           ) },
    { "gear_cunning",                         OPT_FLT,  &( gear.attribute[ ATTR_CUNNING   ]           ) },
    { "gear_willpower",                       OPT_FLT,  &( gear.attribute[ ATTR_WILLPOWER ]           ) },
    { "gear_endurance",                       OPT_FLT,  &( gear.attribute[ ATTR_ENDURANCE ]           ) },
    { "gear_presence",                        OPT_FLT,  &( gear.attribute[ ATTR_PRESENCE  ]           ) },
    { "gear_power",                           OPT_FLT,  &( gear.power                                 ) },
    { "gear_force_power",                     OPT_FLT,  &( gear.force_power                           ) },
    { "gear_expertise_rating",                OPT_FLT,  &( gear.expertise_rating                      ) },
    { "gear_alacrity_rating",                 OPT_FLT,  &( gear.alacrity_rating                       ) },
    { "gear_hit_rating",                      OPT_FLT,  &( gear.hit_rating                            ) },
    { "gear_crit_rating",                     OPT_FLT,  &( gear.crit_rating                           ) },
    { "gear_health",                          OPT_FLT,  &( gear.resource[ RESOURCE_HEALTH ]           ) },
    { "gear_mana",                            OPT_FLT,  &( gear.resource[ RESOURCE_MANA   ]           ) },
    { "gear_rage",                            OPT_FLT,  &( gear.resource[ RESOURCE_RAGE   ]           ) },
    { "gear_energy",                          OPT_FLT,  &( gear.resource[ RESOURCE_ENERGY ]           ) },
    { "gear_ammo",                            OPT_FLT,  &( gear.resource[ RESOURCE_AMMO   ]           ) },
    { "gear_armor",                           OPT_FLT,  &( gear.armor                                 ) },
    // Stat Enchants
    { "enchant_strength",                     OPT_FLT,  &( enchant.attribute[ ATTR_STRENGTH  ]        ) },
    { "enchant_aim",                          OPT_FLT,  &( enchant.attribute[ ATTR_AIM       ]        ) },
    { "enchant_cunning",                      OPT_FLT,  &( enchant.attribute[ ATTR_CUNNING   ]        ) },
    { "enchant_willpower",                    OPT_FLT,  &( enchant.attribute[ ATTR_WILLPOWER ]        ) },
    { "enchant_endurance",                    OPT_FLT,  &( enchant.attribute[ ATTR_ENDURANCE ]        ) },
    { "enchant_presence",                     OPT_FLT,  &( enchant.attribute[ ATTR_PRESENCE  ]        ) },
    { "enchant_power",                        OPT_FLT,  &( enchant.power                              ) },
    { "enchant_force_power",                  OPT_FLT,  &( enchant.force_power                        ) },
    { "enchant_expertise_rating",             OPT_FLT,  &( enchant.expertise_rating                   ) },
    { "enchant_armor",                        OPT_FLT,  &( enchant.armor                              ) },
    { "enchant_alacrity_rating",              OPT_FLT,  &( enchant.alacrity_rating                    ) },
    { "enchant_hit_rating",                   OPT_FLT,  &( enchant.hit_rating                         ) },
    { "enchant_crit_rating",                  OPT_FLT,  &( enchant.crit_rating                        ) },
    { "enchant_health",                       OPT_FLT,  &( enchant.resource[ RESOURCE_HEALTH ]        ) },
    { "enchant_mana",                         OPT_FLT,  &( enchant.resource[ RESOURCE_MANA   ]        ) },
    { "enchant_rage",                         OPT_FLT,  &( enchant.resource[ RESOURCE_RAGE   ]        ) },
    { "enchant_energy",                       OPT_FLT,  &( enchant.resource[ RESOURCE_ENERGY ]        ) },
    { "enchant_ammo",                         OPT_FLT,  &( enchant.resource[ RESOURCE_AMMO   ]        ) },
    // Regen
    { "infinite_energy",                      OPT_BOOL,   &( infinite_resource[ RESOURCE_ENERGY ]     ) },
    { "infinite_ammo",                        OPT_BOOL,   &( infinite_resource[ RESOURCE_AMMO   ]     ) },
    { "infinite_health",                      OPT_BOOL,   &( infinite_resource[ RESOURCE_HEALTH ]     ) },
    { "infinite_mana",                        OPT_BOOL,   &( infinite_resource[ RESOURCE_MANA   ]     ) },
    { "infinite_rage",                        OPT_BOOL,   &( infinite_resource[ RESOURCE_RAGE   ]     ) },
    // Misc
    { "skip_actions",                         OPT_STRING, &( action_list_skip                         ) },
    { "modify_action",                        OPT_STRING, &( modify_action                            ) },
    { "reaction_time_mean",                   OPT_TIMESPAN, &( reaction_mean                          ) },
    { "reaction_time_stddev",                 OPT_TIMESPAN, &( reaction_stddev                        ) },
    { "reaction_time_nu",                     OPT_TIMESPAN, &( reaction_nu                            ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, player_options );
}

// player_t::create =========================================================

player_t* player_t::create( sim_t*             sim,
                            const std::string& type,
                            const std::string& name,
                            race_type r )
{
  if ( type == "jedi_sage" )
  {
    return player_t::create_jedi_sage( sim, name, r );
  }
  else if ( type == "sith_sorcerer" )
  {
    return player_t::create_sith_sorcerer( sim, name, r );
  }
  else if ( type == "jedi_shadow" )
  {
    return player_t::create_jedi_shadow( sim, name, r );
  }
  else if ( type == "sith_assassin" )
  {
    return player_t::create_sith_assassin( sim, name, r );
  }

  else if ( type == "enemy" )
  {
    return player_t::create_enemy( sim, name, r );
  }
  return 0;
}

targetdata_t* player_t::new_targetdata( player_t* source, player_t* target )
{
  return new targetdata_t( source, target );
}

// ==========================================================================
// Target data
// ==========================================================================

targetdata_t* targetdata_t::get( player_t* source, player_t* target )
{
  int id = source->targetdata_id;
  if ( id < 0 )
    source -> targetdata_id = id = source -> sim -> num_targetdata_ids++;

  if ( id >= ( int ) target -> targetdata.size() )
    target -> targetdata.resize( id + 1 );

  targetdata_t* p = target->targetdata[id];
  if ( ! p )
    target -> targetdata[id] = p = source -> new_targetdata( source, target );

  return p;
}

targetdata_t::targetdata_t( player_t* source, player_t* target )
  : source( ( player_t* )source ), target( ( player_t* )target ), dot_list( NULL )
{
  std::vector<std::pair<size_t, std::string> >& v = source->sim->targetdata_dots[source->type];
  for ( std::vector<std::pair<size_t, std::string> >::iterator i = v.begin(); i != v.end(); ++i )
  {
    *( dot_t** )( ( char* )this + i->first ) = add_dot( new dot_t( i->second, this->target ) );
  }
}

targetdata_t::~targetdata_t()
{
  while ( dot_t* d = dot_list )
  {
    dot_list = d -> next;
    delete d;
  }
}

void targetdata_t::reset()
{
  for ( dot_t* d = dot_list; d; d = d->next )
    d -> reset();
}

void targetdata_t::clear_debuffs()
{
  // FIXME: should clear debuffs as well according to similar FIXME in player_t::clear_debuffs()

  for ( dot_t* d = dot_list; d; d = d->next )
    d -> cancel();
}

dot_t* targetdata_t::add_dot( dot_t* d )
{
  d -> next = dot_list;
  dot_list = d;

  return d;
}

aura_t* targetdata_t::add_aura( aura_t* a )
{
  assert( a->player == this->target );
  assert( a->initial_source == this->source );
  return a;
}
