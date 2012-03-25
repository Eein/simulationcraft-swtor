// ==========================================================================
// SimulationCraft for Star Wars: The Old Republic
// http://code.google.com/p/simulationcraft-swtor/
// ==========================================================================

#include "simulationcraft.hpp"
#include <boost/uuid/string_generator.hpp>

namespace mrrobot { // ======================================================

namespace { // ANONYMOUS ====================================================

const bool USE_TEST_API = true;
const bool DEBUG_ITEMS = false;

// Encoding used by askmrrobot's talent builder.
const base36_t::encoding_t talent_encoding =
{
  '0', '1', '2', '3', '4', '5',
  '6', '7', '8', '9', 'a', 'b',
  'c', 'd', 'e', 'f', 'g', 'h',
  'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't',
  'u', 'v', 'w', 'x', 'y', 'z',
};

const base36_t decoder( talent_encoding );

void parse_profession( js_node_t* profile,
                       const std::string& path,
                       std::string& player_profession_string )
{
  std::string crafting_skill;
  if ( js_t::get_value( crafting_skill, profile, path ) )
  {
    util_t::format_name( crafting_skill );
    if ( ! player_profession_string.empty() )
      player_profession_string += '/';
    player_profession_string += crafting_skill;
  }
}
// parse_skills =============================================================

void parse_skill_tree( std::vector<talent_t*>& tree, std::string& s )
{
  for ( unsigned i = 0; i < s.size() && i < tree.size(); ++i )
  {
    signed char c = s[ s.size() - i - 1 ] - '0';

    if ( unlikely( c < 0 || c > 5 ) )
    {
      // FIXME: Report something.
      continue;
    }

    tree[ i ] -> set_rank( c );
  }
}

void parse_skills( player_t* p, js_node_t* profile )
{
  std::string skill_string;
  if ( !js_t::get_value( skill_string, profile, "SkillString" ) )
    return;

  std::vector<std::string> tree_strings = split( skill_string, '-' );

  unsigned max = std::min( static_cast<unsigned>( MAX_TALENT_TREES ),
                           tree_strings.size() );

  for ( unsigned tree = 0; tree < max; ++tree )
    parse_skill_tree( p -> talent_trees[ tree ], tree_strings[ tree ] );
}

slot_type translate_slot_name( const std::string& name )
{
  static const char* const slot_map[] =
  {
    "Helm",
    "Ears",
    nullptr,
    nullptr,
    "Chest",
    "Belt",
    "Leg",
    "Feet",
    "Wrist",
    "Glove",
    "Implant1",
    "Implant2",
    "Relic1",
    "Relic2",
    nullptr,
    "MainHand",
    "OffHand",
  };

  for ( unsigned i = 0; i < sizeof_array( slot_map ); ++i )
  {
    if ( slot_map[ i ] && util_t::str_compare_ci( name, slot_map[ i ] ) )
      return static_cast<slot_type>( i );
  }

  return SLOT_INVALID;
}

// decode_weapon_type =======================================================

weapon_type decode_weapon_type( const std::string& s )
{
  static const struct {
    const char* name;
    weapon_type type;
  } weapon_map[] = {
  };

  weapon_type wt = util_t::parse_weapon_type( s );
  if ( wt != WEAPON_NONE )
    return wt;

  for ( auto const& i : weapon_map )
    if ( util_t::str_compare_ci( s, i.name ) )
      return i.type;

  return WEAPON_NONE;
}

// decode_stats =============================================================

std::string decode_stats( js_node_t* node )
{
  static const struct {
    const char* name;
    const char* abbrv;
  } stat_mapping[] = {
    { "Armor", "armor" },
    { "MinDamage", "min" },
    { "MaxDamage", "max" },
    { "Endurance", "endurance" },
    { "Strength", "strength" },
    { "Aim", "aim" },
    { "Cunning", "cunning" },
    { "Will", "willpower" },
    { "Presence", "presence" },
    { "Expertise", "expertise" },
    { "Power", "power" },
    { "ForcePower", "forcepower" },
    { "TechPower", "techpower" },
    { "Defense", "defense" },
    { "Shielding", "shield" },
    { "Absorb", "absorb" },
    { "Accuracy", "accuracy" },
    { "Crit", "crit" },
    { "Surge", "surge" },
    { "Alacrity", "alacrity" },
  };

  std::stringstream ss;
  bool first = true;

  for ( auto const& i : stat_mapping )
  {
    int value;
    if ( ! js_t::get_value( value, node, i.name ) )
      continue;
    if ( first )
      first = false;
    else
      ss << '_';
    ss << value << i.abbrv;
  }

  return ss.str();
}

// parse_items ==============================================================

void parse_items( player_t* p, js_node_t* items )
{
  if ( !items ) return;

  for ( js_node_t* item : js_t::children( items ) )
  {
    std::stringstream item_encoding;

    std::string slot_name = js_t::get_name( item );
    slot_type slot = translate_slot_name( slot_name );
    if ( slot == SLOT_INVALID )
    {
      // FIXME: Report weirdness.
      continue;
    }

    std::string name;
    if ( ! js_t::get_value( name, item, "Name" ) )
    {
      // FIXME: Report weirdness.
      continue;
    }
    item_encoding << util_t::format_name( name );

#if 0
    int id;
    if ( js_t::get_value( level, item, "Id" ) )
      item_encoding << ",id=" << id;
#endif

    int level;
    if ( js_t::get_value( level, item, "ItemLevel" ) )
      item_encoding << ",ilevel=" << level;

    std::string quality;
    if ( js_t::get_value( quality, item, "Quality" ) )
      item_encoding << ",quality=" << util_t::format_name( quality );

    std::string weapon_type_str;
    weapon_type wt = WEAPON_NONE;
    if ( js_t::get_value( weapon_type_str, item, "WeaponType" ) )
    {
      wt = decode_weapon_type( weapon_type_str );
      if ( wt != WEAPON_NONE )
        item_encoding << ",weapon=" << util_t::weapon_type_string( wt );
    }

    if ( js_node_t* stats = js_t::get_child( item, "Stats" ) )
    {
      std::string s = decode_stats( stats );
      if ( ! s.empty() )
        item_encoding << ",stats=" << s;
    }

    p -> items[ slot ].options_str = item_encoding.str();
  }
}

#if 0
// parse_profession =========================================================

void parse_profession( std::string& professions_str, js_node_t* profile, int index )
{
  std::string key = "professions/primary/" + util_t::to_string( index );
  if ( js_node_t* profession = js_t::get_node( profile, key ) )
  {
    int id;
    std::string rank;
    if ( js_t::get_value( id, profession, "id" ) && js_t::get_value( rank, profession, "rank" ) )
    {
      if ( professions_str.length() > 0 )
        professions_str += '/';
      professions_str += util_t::profession_type_string( util_t::translate_profession_id( id ) );
      professions_str += '=' + rank;
    }
  }
}

struct item_info_t : public item_data_t
{
  std::string name_str, icon_str;
  item_info_t() : item_data_t() {}
};

bool download_item_data( item_t& item, item_info_t& item_data,
                         const std::string& item_id, cache::behavior_t caching )
{
  // BCP API doesn't currently provide enough information to describe items completely.
  if ( ! DEBUG_ITEMS )
    return false;

  js_node_t* js = download_id( item.sim, item.sim -> default_region_str, item_id, caching );
  if ( ! js )
  {
    if ( caching != cache::ONLY )
    {
      item.sim -> errorf( "BCP API: Player '%s' unable to download item id '%s' at slot %s.\n",
                          item.player -> name(), item_id.c_str(), item.slot_name() );
    }
    return false;
  }
  if ( item.sim -> debug ) js_t::print( js, item.sim -> output_file );

  try
  {
    {
      int id;
      if ( ! js_t::get_value( id, js, "id" ) ) throw( "id" );
      item_data.id = id;
    }

    if ( ! js_t::get_value( item_data.name_str, js, "name" ) ) throw( "name" );
    item_data.name = item_data.name_str.c_str();

    if ( js_t::get_value( item_data.icon_str, js, "icon" ) )
      item_data.icon = item_data.icon_str.c_str();

    if ( ! js_t::get_value( item_data.level, js, "itemLevel" ) ) throw( "level" );

    js_t::get_value( item_data.req_level, js, "requiredLevel" );
    js_t::get_value( item_data.req_skill, js, "requiredSkill" );
    js_t::get_value( item_data.req_skill_level, js, "requiredSkillRank" );

    if ( ! js_t::get_value( item_data.quality, js, "quality" ) ) throw( "quality" );

    if ( ! js_t::get_value( item_data.inventory_type, js, "inventoryType" ) ) throw( "inventory type" );
    if ( ! js_t::get_value( item_data.item_class, js, "itemClass" ) ) throw( "item class" );
    if ( ! js_t::get_value( item_data.item_subclass, js, "itemSubClass" ) ) throw( "item subclass" );
    js_t::get_value( item_data.bind_type, js, "itemBind" );

    if ( js_node_t* w = js_t::get_child( js, "weaponInfo" ) )
    {
      int minDmg, maxDmg;
      double speed;
      if ( ! js_t::get_value( speed, w, "weaponSpeed" ) ) throw( "weapon speed" );
      if ( ! js_t::get_value( minDmg, w, "damage/minDamage" ) ) throw( "weapon minimum damage" );
      if ( ! js_t::get_value( maxDmg, w, "damage/maxDamage" ) ) throw( "weapon maximum damage" );
      item_data.delay = speed * 1000.0;
      item_data.dmg_range = maxDmg - minDmg;
    }

    if ( js_node_t* classes = js_t::get_child( js, "allowableClasses" ) )
    {
      std::vector<js_node_t*> nodes;
      js_t::get_children( nodes, classes );
      for ( size_t i = 0, n = nodes.size(); i < n; ++i )
      {
        int cid;
        if ( js_t::get_value( cid, nodes[ i ] ) )
          item_data.class_mask |= 1 << ( cid - 1 );
      }
    }
    else
      item_data.class_mask = -1;

    if ( js_node_t* races = js_t::get_child( js, "allowableRaces" ) )
    {
      std::vector<js_node_t*> nodes;
      js_t::get_children( nodes, races );
      for ( size_t i = 0, n = nodes.size(); i < n; ++i )
      {
        int rid;
        if ( js_t::get_value( rid, nodes[ i ] ) )
          item_data.race_mask |= 1 << ( rid - 1 );
      }
    }
    else
      item_data.race_mask = -1;

    if ( js_node_t* stats = js_t::get_child( js, "bonusStats" ) )
    {
      std::vector<js_node_t*> nodes;
      js_t::get_children( nodes, stats );
      for ( size_t i = 0, n = std::min( nodes.size(), sizeof_array( item_data.stat_type ) ); i < n; ++i )
      {
        if ( ! js_t::get_value( item_data.stat_type[ i ], nodes[ i ], "stat" ) ) throw( "bonus stat" );
        if ( ! js_t::get_value( item_data.stat_val[ i ], nodes[ i ], "amount" ) ) throw( "bonus stat amount" );
      }
    }

    if ( js_node_t* sockets = js_t::get_node( js, "socketInfo/sockets" ) )
    {
      std::vector<js_node_t*> nodes;
      js_t::get_children( nodes, sockets );
      for ( size_t i = 0, n = std::min( nodes.size(), sizeof_array( item_data.socket_color ) ); i < n; ++i )
      {
        std::string color;
        if ( js_t::get_value( color, nodes[ i ], "type" ) )
        {
          if ( color == "META" )
            item_data.socket_color[ i ] = SOCKET_COLOR_META;
          else if ( color == "RED" )
            item_data.socket_color[ i ] = SOCKET_COLOR_RED;
          else if ( color == "YELLOW" )
            item_data.socket_color[ i ] = SOCKET_COLOR_RED;
          else if ( color == "BLUE" )
            item_data.socket_color[ i ] = SOCKET_COLOR_BLUE;
          else if ( color == "PRISMATIC" )
            item_data.socket_color[ i ] = SOCKET_COLOR_PRISMATIC;
          else if ( color == "COGWHEEL" )
            item_data.socket_color[ i ] = SOCKET_COLOR_COGWHEEL;
        }
      }
    }

    js_t::get_value( item_data.id_set, js, "itemSet" );

    // heroic tag is not available from BCP API.
    {
      // FIXME: set item_data.flags_1 to ITEM_FLAG_HEROIC as appropriate.
    }

    // FIXME: LFR tag is not available from BCP API.

    // socket bonus is not available from BCP API.
    {
      // FIXME: set item_data.id_socket_bonus appropriately.
    }
  }
  catch ( const char* fieldname )
  {
    item.sim -> errorf( "BCP API: Player '%s' unable to parse item '%s' %s at slot '%s'.\n",
                        item.player -> name(), item_id.c_str(), fieldname, item.slot_name() );
    return false;
  }

  assert( 0 );
  return false;
}

// download_roster ==========================================================

js_node_t* download_roster( sim_t* sim,
                            const std::string& region,
                            const std::string& server,
                            const std::string& name,
                            cache::behavior_t  caching )
{
  std::string url = "http://" + region + ".battle.net/api/wow/guild/" + server + '/' +
                    name + "?fields=members";

  std::string result;
  if ( ! http_t::get( result, url, caching, "\"members\"" ) )
  {
    sim -> errorf( "BCP API: Unable to download guild %s|%s|%s.\n", region.c_str(), server.c_str(), name.c_str() );
    return 0;
  }
  js_node_t* js = js_t::create( sim, result );
  if ( ! js || ! ( js = js_t::get_child( js, "members" ) ) )
  {
    sim -> errorf( "BCP API: Unable to get members of guild %s|%s|%s.\n", region.c_str(), server.c_str(), name.c_str() );
    return 0;
  }

  return js;
}
#endif // 0

void canonical_class_name( std::string& name )
{
  util_t::format_name( name );
  name = util_t::player_type_string( util_t::translate_class_str( name ) );
}

void canonical_race_name( std::string& name )
{
  util_t::format_name( name );
  name = util_t::race_type_string( util_t::parse_race_type( name ) );
}
} // ANONYMOUS namespace ====================================================

// mrrobot::download_player =================================================

player_t* download_player( sim_t*             sim,
                           const std::string& id,
                           cache::behavior_t  caching )
{
  if ( id != "test")
  {
    // Check form validity of the provided profile id before even starting to access the profile
    try
    {
      boost::uuids::string_generator()( id );
    }
    catch( std::runtime_error& )
    {
      sim -> errorf( "'%s' is not a valid Mr. Robot profile identifier.\n", id.c_str() );
      return nullptr;
    }
  }

  sim -> current_name = id;
  sim -> current_slot = 0;

  std::string url = "http://swtor.askmrrobot.com/api/";
  if ( USE_TEST_API ) url += "test/";
  url += "character/v1/";
  url += id;

  std::string result;

  if ( ! http_t::get( result, url, caching ) )
  {
    sim -> errorf( "Unable to download player from '%s'\n", url.c_str() );
    return nullptr;
  }

  // if ( sim -> debug ) util_t::fprintf( sim -> output_file, "%s\n%s\n", url.c_str(), result.c_str() );
  js_node_t* profile = js_t::create( sim, result );
  if ( ! profile )
  {
    sim -> errorf( "Unable to parse player profile from '%s'\n", url.c_str() );
    return nullptr;
  }
  if ( sim -> debug ) js_t::print( profile, sim -> output_file );

  std::string name;
  if ( ! js_t::get_value( name, profile, "Name" ) &&
       ! js_t::get_value( name, profile, "ProfileName" ) )
  {
    sim -> errorf( "Unable to extract player name from '%s'.\n", url.c_str() );
    return nullptr;
  }
  if ( ! name.empty() )
    sim -> current_name = name;

  int level;
  if ( ! js_t::get_value( level, profile, "Level"  ) )
  {
    sim -> errorf( "Unable to extract player level from '%s'.\n", url.c_str() );
    return nullptr;
  }

  std::string class_name;
  if ( ! js_t::get_value( class_name, profile, "AdvancedClass" ) )
  {
    sim -> errorf( "Unable to extract player class from '%s'.\n", url.c_str() );
    return nullptr;
  }
  canonical_class_name( class_name );

  std::string race_name;
  race_type race = RACE_NONE;
  if ( js_t::get_value( race_name, profile, "Race" ) )
    race = util_t::parse_race_type( race_name );

  player_t* p = player_t::create( sim, class_name, name, race );
  sim -> active_player = p;
  if ( ! p )
  {
    sim -> errorf( "Unable to build player with class '%s' and name '%s' from '%s'.\n",
                   class_name.c_str(), name.c_str(), url.c_str() );
    return nullptr;
  }

  p -> level = level;

  js_t::get_value( p -> server_str, profile, "Server" );
  js_t::get_value( p -> region_str, profile, "Region" );

  p -> origin_str = "http://swtor.askmrrobot.com/character/" + id;

  parse_profession( profile, "CraftingCrewSkill", p -> professions_str );
  if ( false )
  {
    parse_profession( profile, "CrewSkill2",      p -> professions_str );
    parse_profession( profile, "CrewSkill3",      p -> professions_str );
  }

  parse_skills( p, profile );

  parse_items( p, js_t::get_child( profile, "GearSet" ) );

  if ( js_node_t* datacrons = js_t::get_child( profile, "Datacrons" ) )
  {
    for ( js_node_t* node : js_t::children( datacrons ) )
    {
      // FIXME: Do something.
      ( void )node;
    }
  }

  return p;
}

// mrrobot::parse_talents ===================================================

bool parse_talents( player_t& p, const std::string& talent_string )
{
  // format: [tree_1]-[tree_2]-[tree_3] where each tree is a
  // sum over all talents of [# of points] * 6 ^ [0-based talent index]
  // in base 36.

  int encoding[ MAX_TALENT_SLOTS ];
  boost::fill( encoding, 0 );

  std::vector<std::string> tree_strings;
  util_t::string_split( tree_strings, talent_string, "-" );

  for ( unsigned tree=0; tree < MAX_TALENT_TREES && tree < tree_strings.size(); ++tree )
  {
    unsigned count = 0;
    for ( unsigned j=0; j < tree; j++ )
      count += p.talent_trees[ j ].size();
    unsigned tree_size = p.talent_trees[ tree ].size();
    std::string::size_type pos = tree_strings[ tree ].length();
    unsigned tree_count = 0;
    while ( pos-- > 0 )
    {
      try
      {
        base36_t::pair_t point_pair = decoder( tree_strings[ tree ][ pos ] );
        if ( ++tree_count >= tree_size )
          break;
        encoding[ count++ ] = point_pair.second;
        if ( ++tree_count >= tree_size )
          break;
        encoding[ count++ ] = point_pair.first;
      }

      catch ( base36_t::bad_char& bc )
      {
        p.sim -> errorf( "Player %s has malformed wowhead talent string. Translation for '%c' unknown.\n",
                         p.name(), bc.c );
        return false;
      }
    }
  }

  if ( p.sim -> debug )
  {
    std::string str_out;
    for ( size_t i = 0; i < MAX_TALENT_SLOTS; ++i )
      str_out += '0' + encoding[i];
    std::string::size_type pos = str_out.find_last_not_of( '0' );
    if ( pos != str_out.npos )
    {
      str_out.resize( pos );
      log_t::output( p.sim, "%s MrRobot talent string translation: %s\n", p.name(), str_out.c_str() );
    }
  }

  return p.parse_talent_trees( encoding );
}


#if 0
// bcp_api::download_item() =================================================

bool download_item( item_t& item, const std::string& item_id, cache::behavior_t caching )
{
  // BCP API doesn't currently provide enough information to describe items completely.
  if ( ! DEBUG_ITEMS )
    return false;

  item_info_t item_data;
  return download_item_data( item, item_data, item_id, caching );
}

// bcp_api::download_slot() =================================================

bool download_slot( item_t& item,
                    const std::string& item_id,
                    const std::string& enchant_id,
                    const std::string& addon_id,
                    const std::string& reforge_id,
                    const std::string& rsuffix_id,
                    const std::string gem_ids[ 3 ],
                    cache::behavior_t caching )
{
  // BCP API doesn't currently provide enough information to describe items completely.
  if ( ! DEBUG_ITEMS )
    return false;

  item_info_t item_data;
  if ( ! download_item_data( item, item_data, item_id, caching ) )
    return false;

  if ( ! enchant_t::download_addon( item, addon_id ) )
  {
    item.sim -> errorf( "Player %s unable to parse addon id %s for item \"%s\" at slot %s.\n",
                        item.player -> name(), addon_id.c_str(), item.name(), item.slot_name() );
  }

  if ( ! enchant_t::download_reforge( item, reforge_id ) )
  {
    item.sim -> errorf( "Player %s unable to parse reforge id %s for item \"%s\" at slot %s.\n",
                        item.player -> name(), reforge_id.c_str(), item.name(), item.slot_name() );
  }

  if ( ! enchant_t::download_rsuffix( item, rsuffix_id ) )
  {
    item.sim -> errorf( "Player %s unable to determine random suffix '%s' for item '%s' at slot %s.\n",
                        item.player -> name(), rsuffix_id.c_str(), item.name(), item.slot_name() );
  }

  return true;
}

// mrrobot::download_guild ==================================================

bool download_guild( sim_t* sim, const std::string& region, const std::string& server, const std::string& name,
                     const std::vector<int>& ranks, int player_filter, int max_rank, cache::behavior_t caching )
{
  js_node_t* js = download_roster( sim, region, server, name, caching );
  if ( !js ) return false;

  std::vector<std::string> names;
  std::vector<js_node_t*> characters;
  js_t::get_children( characters, js );

  for ( std::size_t i = 0, n = characters.size(); i < n; ++i )
  {
    int level;
    if ( ! js_t::get_value( level, characters[ i ], "character/level" ) || level < 85 )
      continue;

    int cid;
    if ( ! js_t::get_value( cid, characters[ i ], "character/class" ) ||
         ( player_filter != PLAYER_NONE && player_filter != util_t::translate_class_id( cid ) ) )
      continue;

    int rank;
    if ( ! js_t::get_value( rank, characters[ i ], "rank" ) ||
         ( ( max_rank > 0 ) && ( rank > max_rank ) ) ||
         ( ! ranks.empty() && range::find( ranks, rank ) == ranks.end() ) )
      continue;

    std::string cname;
    if ( ! js_t::get_value( cname, characters[ i ], "character/name" ) ) continue;
    names.push_back( cname );
  }

  if ( names.empty() ) return true;

  range::sort( names );

  for ( std::size_t i = 0, n = names.size(); i < n; ++i )
  {
    const std::string& cname = names[ i ];
    util_t::printf( "Downloading character: %s\n", cname.c_str() );
    player_t* player = download_player( sim, region, server, cname, "active", caching );
    if ( !player )
    {
      sim -> errorf( "BCP API: Failed to download player '%s'\n", cname.c_str() );
      // Just ignore invalid players
    }
  }

  return true;
}
#endif // 0

} // namespace mrrobot ======================================================
