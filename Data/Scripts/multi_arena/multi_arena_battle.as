/*******
 *
 * Classes and helpers defining the entities in an arena battle and the battle
 *  itself
 *
 */

#include "arena_meta_persistence.as"
#include "multi_arena/multi_arena_player_spawn.as"
#include "multi_arena/multi_arena_enemy_spawn.as"
#include "multi_arena/multi_arena_weapon_spawn.as"

class SpawnLocation {
    SpawnLocation() {
        name = "";
        id = -1;
    }

    SpawnLocation( string _name, int _id ) {
        name = _name;
        id = _id;
    }

    string name;
    int id;
}

class SpawnLocations {
    array<SpawnLocation> locations;

    void deleteAll() {
        locations.removeRange(0,locations.length());
    }

    bool exists(string name) {
        for( uint i = 0; i < locations.length(); i++ ) {
            if( locations[i].name == name ) {
                return true;
            }
        }
        return false;
    }

    int Get(string name) {
        for( uint i = 0; i < locations.length(); i++ ) {
            if( locations[i].name == name ) {
                return locations[i].id;
            }
        }
        return 0;
    }

    void Set(string name, int val) {
        bool found = false;
        for( uint i = 0; i < locations.length(); i++ ) {
            if( locations[i].name == name ) {
                locations[i].id = val;
                found = true;
            }
        }
        if( found == false ) {
            locations.insertLast( SpawnLocation(name,val) );
        }
    }
}

/**
 * All the necessary bookkeeping for a battle in the arena
 **/
class BattleInstance {

    array<BattleEntitySpawn@> spawnedEntities; // All entities that will be spawned
    array<array<int>> teamsByIndex; // The entities in each team by index of spawnedEntities
    array<array<MovementObject@>> teams; // The actual entities in each team
    array<int> spawnedAgents; // The AI agents (by index of spawnedEntities)
    int playerIndexId; // The spawned player (by index of spawnedEntities)
    int playerObjectId; // The object id of the payer
    MovementObject@ player; // The actual player id

    uint numTeams; // How many teams are there?

    SpawnLocations spawnLocations; // All the spawn locations map from name to object id

    float battleDifficulty; // Difficulty rating for this battle

    bool weaponsInUse; // Does anybody have a weapon?

    string gamemode;
    int current_wave;

    /**
     * Return everything to factory defaults
     **/
    void reset() {

        // Reset the bookkeeping
        weaponsInUse = false;
        numTeams = 0;
        spawnedEntities.resize(0);
        teams.resize(0);
        teamsByIndex.resize(0);
        spawnedAgents.resize(0);
        playerIndexId = -1;
        playerObjectId = -1;
        current_wave = 1;
        battleDifficulty = global_data.player_skill;
        @player = null;

        // Go through and record all possible spawn locations, store them by name
        spawnLocations.deleteAll();
        array<int> @allObjectIds = GetObjectIDs();
        for( uint objectIndex = 0; objectIndex < allObjectIds.length(); objectIndex++ ) {
            Object @obj = ReadObjectFromID( allObjectIds[ objectIndex ] );
            ScriptParams@ params = obj.GetScriptParams();
            if(params.HasParam("Name") && params.GetString("Name") == "arena_spawn" ) {
                if(params.HasParam("LocName") ) {
                    string LocName = params.GetString("LocName");
                    if( LocName != "" ) {
                        if( spawnLocations.exists( LocName ) ) {
                            DisplayError("Error", "Duplicate spawn location " + LocName );
                        }
                        else {
                            spawnLocations.Set(LocName, allObjectIds[ objectIndex ]);
                            obj.SetEditorLabel(LocName);
                        }
                    }
                }
            }
        }
    }

    /**
     * Constructor
     **/
    BattleInstance() {
        reset();
    }

    /**
     * Destructor
     **/
    ~BattleInstance() {
        reset();
    }

    void initialize( string _gamemode, JSONValue battleParams ) {
        Log(info,"Starting initialize battle");

        JSONValue newTeams = battleParams["teams"];
        JSONValue newItems = battleParams["items"];

        reset();

        gamemode = _gamemode;

        // Make sure there's enough teams to make this interesting
        int teamsWithMembers = 0;
        for( uint teamCounter = 0; teamCounter < newTeams.size(); teamCounter++ )
        {
            if( newTeams[ teamCounter ]["members"].size() > 0 ) {
                teamsWithMembers++;
            }
        }

        if( teamsWithMembers < 2 ) {
            DisplayError( "error", "Arena battle needs at least two teams with at least one member" );
        }

        if( newTeams.size() > 4 ) {
            DisplayError( "error", "Arena battle currently supports at most four teams" );
        }

        // First make sure there is exactly one player
        bool playerFound = false;
        uint totalChars = 0; // While we're at it, count the chars
        uint totalNotPlayerChars = 0; // Count the characters that can't be the player
        array<int> possiblePlayerJSONIds;

        for( uint teamIndex = 0; teamIndex < newTeams.size(); ++teamIndex ) {
            for( uint charIndex = 0; charIndex < newTeams[ teamIndex ]["members"].size(); ++charIndex ) {
                if( newTeams[ teamIndex ]["members"][ charIndex ].isMember( "player" ) ) {
                    string playerValue = newTeams[ teamIndex ]["members"][ charIndex ]["player"].asString();
                    // Todo -- readd player force?
                    if( playerValue == "maybe" ) {
                        int playerJSONId =  newTeams[ teamIndex ]["members"][ charIndex ]["id"].asInt();
                        playerFound = true;
                        possiblePlayerJSONIds.insertLast( playerJSONId );
                    }
                    else {
                        totalNotPlayerChars++;
                    }
                }
                totalChars++;
            }
        }

        if( possiblePlayerJSONIds.length() == 0 ) {
            DisplayError("Error", "All characters specified as non-player");
        }

        // If a specific player spawn hasn't been defined then randomly choose one


        // Come up with a random number of characters to skip over
        //  before we choose a player (respecting the characters marked as non-player)
        int chosenPlayerJSONId = possiblePlayerJSONIds[rand()%(possiblePlayerJSONIds.length())];


        // Finally we can go through the structure and create the new objects
        for( uint teamIndex = 0; teamIndex < newTeams.size(); ++teamIndex ) {
            // Initialize a new team
            startTeam();
            // Setup the spawns for all the characters in that team
            for( uint charIndex = 0; charIndex < newTeams[ teamIndex ]["members"].size(); ++charIndex ) {
                if( newTeams[ teamIndex ]["members"][ charIndex ].isMember( "player" ) and
                  newTeams[ teamIndex ]["members"][ charIndex ]["id"].asInt() == chosenPlayerJSONId ) {
                    addPlayer( newTeams[ teamIndex ]["members"][ charIndex ] );
                }
                else {
                    addEnemy( newTeams[ teamIndex ]["members"][ charIndex ] );
                }
            }
        }

        // Now, much simpler, setup all the item spawns
        for( uint itemIndex = 0; itemIndex < newItems.size(); ++itemIndex ) {
            addWeapon( newItems[ itemIndex ] );
        }

    }

    /**
     * Get the number of teams
     **/
     uint getNumberOfTeams() {
        return numTeams;
     }

    /**
     * Signal the start of a new team
     **/
    void startTeam() {

        if( numTeams == 4 )
        {
            DisplayError("Error", "More than four teams not current supported" );
        }

        if( numTeams != 0 && teamsByIndex[ numTeams - 1 ].length() == 0 )
        {
            DisplayError("Error", "Cannot have an empty team" );
        }

        numTeams++;
        teamsByIndex.resize( numTeams );

    }

    /**
     * Wrap the dictionary to do a little error checking
     **/
    int getObjectIdFromLocationName( string name ) {
        if( !spawnLocations.exists( name ) ) {
            DisplayError("error", "Location name " + name + " not found in level");
        }
        return spawnLocations.Get( name );
    }


    /**
     * Add an enemy to the current team
     **/
    void addEnemy( JSONValue params ) {

        if( !params.isMember( "location" ) ) {
            DisplayError("error", "Location not defined");
        }
        string spawnLocation = params[ "location" ].asString();

        string enemyType = "";
        if( params.isMember( "type" ) ) {
            enemyType = params[ "type" ].asString();
        }

        string weaponType = "";
        if( params.isMember( "weapon" ) ) {
            weaponType = params[ "weapon" ].asString();
        }

        int wave = 1;
        if( params.isMember( "wave" ) ) {
            wave = params[ "wave" ].asInt();
            Log( info, "got wave: " + wave );
        }

        if( numTeams == 0 ) {
            DisplayError("Error", "No teams defined when adding enemy" );
        }

        // Create a new entity
        BattleEnemySpawn enemySpawn;
        enemySpawn.team = numTeams - 1;
        enemySpawn.difficulty =  battleDifficulty - 0.5f;
        enemySpawn.spawnPointOjectId = getObjectIdFromLocationName( spawnLocation );
        enemySpawn.locationName = spawnLocation;
        enemySpawn.wave = wave;

        // if an enemy type wasn't specified, chose one
        if( enemyType == "" or enemyType == "any" ) {
            switch( rand()%2+1 ) {
                case 0:
                    enemySpawn.fur_channel = -1;
                    enemySpawn.entityPath = level.GetPath("char_civ");
                    break;
                case 1:
                    enemySpawn.fur_channel = 1;
                    enemySpawn.entityPath = level.GetPath("char_guard");
                    break;
                case 2:
                    enemySpawn.fur_channel = 0;
                    enemySpawn.entityPath = level.GetPath("char_raider");
                    break;
            }
        }
        else {
            if( enemyType == "civ" ) {
                enemySpawn.fur_channel = -1;
                enemySpawn.entityPath = level.GetPath("char_civ");
            }
            else if ( enemyType == "guard" ) {
                enemySpawn.fur_channel = 1;
                enemySpawn.entityPath = level.GetPath("char_guard");
            }
            else if ( enemyType == "raider" ) {
                enemySpawn.fur_channel = 0;
                enemySpawn.entityPath = level.GetPath("char_raider");
            }
            else {
                DisplayError("Error", "Unknown enemy type " + enemyType );
            }
        }

        // Now determine the color
        // Set palette colors randomly, darkening based on skill
        for(int i=0; i<4; ++i) {
            vec3 color = FloatTintFromByte(RandReasonableColor());
            float tintAmount = 0.5f;

            uint teamNumber = numTeams - 1;

            if( teamNumber == 0 ) {
                color = mix(color, vec3(1,0,0), tintAmount);
            } else if( teamNumber == 1 ) {
                color = mix(color, vec3(0,0,1), tintAmount);
            } else if( teamNumber == 2 ) {
                color = mix(color, vec3(1,0.5f,0), tintAmount);
            } else if( teamNumber == 3) {
                color = mix(color, vec3(0,1,1), tintAmount);
            }
            enemySpawn.color[i] = mix(color, vec3(1.0-enemySpawn.difficulty), 0.5f);
        }

        // See if the enemy gets a weapon
        if( weaponType == "any" ) {
            switch( rand()%4 ) {
                case 0:
                    enemySpawn.useWeaponPath = level.GetPath("weap_knife");
                    weaponsInUse = true;
                    break;
                case 1:
                    enemySpawn.useWeaponPath = level.GetPath("weap_big_sword");
                    weaponsInUse = true;
                    break;
                case 2:
                    enemySpawn.useWeaponPath = level.GetPath("weap_sword");
                    weaponsInUse = true;
                    break;
                case 3:
                    enemySpawn.useWeaponPath = level.GetPath("weap_spear");
                    weaponsInUse = true;
                    break;
            }
        }
        else {
            if( weaponType == "knife" ) {
                enemySpawn.useWeaponPath = level.GetPath("weap_knife");
                weaponsInUse = true;
            }
            else if ( weaponType == "big_sword" ) {
                enemySpawn.useWeaponPath = level.GetPath("weap_big_sword");
                weaponsInUse = true;
            }
            else if ( weaponType == "sword" ) {
                enemySpawn.useWeaponPath = level.GetPath("weap_sword");
                weaponsInUse = true;
            }
            else if ( weaponType == "spear" ) {
                enemySpawn.useWeaponPath = level.GetPath("weap_spear");
                weaponsInUse = true;
            }
        }

        // Finally, store the enemy
        teamsByIndex[ numTeams - 1 ].insertLast( spawnedEntities.length() );
        spawnedAgents.insertLast( spawnedEntities.length() );
        spawnedEntities.insertLast( enemySpawn );

    }

    /**
     * Add the player to the current team
     **/
    void addPlayer( JSONValue params ) {

        if( !params.isMember( "location" ) ) {
            DisplayError("error", "Location not defined");
        }
        string spawnLocation = params[ "location" ].asString();

        string weaponType = "";
        if( params.isMember( "weapon" ) ) {
            weaponType = params[ "weapon" ].asString();
        }

        if( numTeams == 0 ) {
            DisplayError("Error", "No teams defined when adding player" );
        }

        // Create a new entity
        BattlePlayerSpawn playerSpawn;
        playerSpawn.team = numTeams - 1;
        playerSpawn.spawnPointOjectId = getObjectIdFromLocationName( spawnLocation );
        playerSpawn.locationName = spawnLocation;

        // See if the player gets a weapon
        if( weaponType == "any" ) {
            switch( rand()%4 ) {
                case 0:
                    playerSpawn.useWeaponPath = level.GetPath("weap_knife");
                    weaponsInUse = true;
                    break;
                case 1:
                    playerSpawn.useWeaponPath = level.GetPath("weap_big_sword");
                    weaponsInUse = true;
                    break;
                case 2:
                    playerSpawn.useWeaponPath = level.GetPath("weap_sword");
                    weaponsInUse = true;
                    break;
                case 3:
                    playerSpawn.useWeaponPath = level.GetPath("weap_spear");
                    weaponsInUse = true;
                    break;
            }
        }
        else {
            if( weaponType == "knife" ) {
                playerSpawn.useWeaponPath = level.GetPath("weap_knife");
                weaponsInUse = true;
            }
            else if ( weaponType == "big_sword" ) {
                playerSpawn.useWeaponPath = level.GetPath("weap_big_sword");
                weaponsInUse = true;
            }
            else if ( weaponType == "sword" ) {
                playerSpawn.useWeaponPath = level.GetPath("weap_sword");
                weaponsInUse = true;
            }
            else if ( weaponType == "spear" ) {
                playerSpawn.useWeaponPath = level.GetPath("weap_spear");
                weaponsInUse = true;
            }
        }

        // Finally, store the player
        playerIndexId = spawnedEntities.length();
        teamsByIndex[ numTeams - 1 ].insertLast( spawnedEntities.length() );
        spawnedAgents.insertLast( spawnedEntities.length() );
        spawnedEntities.insertLast( playerSpawn );
    }


    /**
     * Add a weapon to the battle
     **/
    void addWeapon( JSONValue params ) {

        if( !params.isMember( "location" ) ) {
            DisplayError("error", "Location not defined");
        }
        string spawnLocation = params[ "location" ].asString();

        string weaponType = "";
        if( params.isMember( "type" ) ) {
            weaponType = params[ "type" ].asString();
        }

        // Create a new entity
        BattleWeaponSpawn weaponSpawn;
        weaponSpawn.spawnPointOjectId = getObjectIdFromLocationName( spawnLocation );
        weaponSpawn.locationName = spawnLocation;

        // if an weapon type wasn't specified, chose one
        if( weaponType == "" or weaponType == "any" ) {
            switch( rand()%4 ) {
                case 0:
                    weaponSpawn.entityPath = level.GetPath("weap_knife");
                    break;
                case 1:
                    weaponSpawn.entityPath = level.GetPath("weap_big_sword");
                    break;
                case 2:
                    weaponSpawn.entityPath = level.GetPath("weap_sword");
                    break;
                case 3:
                    weaponSpawn.entityPath = level.GetPath("weap_spear");
                    break;
            }
        }
        else {
            if( weaponType == "knife" ) {
                weaponSpawn.entityPath = level.GetPath("weap_knife");
            }
            else if ( weaponType == "big_sword" ) {
                weaponSpawn.entityPath = level.GetPath("weap_big_sword");
            }
            else if ( weaponType == "sword" ) {
                weaponSpawn.entityPath = level.GetPath("weap_sword");
            }
            else if ( weaponType == "spear" ) {
                weaponSpawn.entityPath = level.GetPath("weap_spear");
            }
            else {
                DisplayError("Error", "Unknown weapon type " + weaponType );
            }
        }

        Log(info, "weaponSpawn.entityPath: " + weaponSpawn.entityPath);

        // Finally, store the weapon
        weaponsInUse = true;
        spawnedEntities.insertLast( weaponSpawn );

    }


    /**
     * Get this whole show on the road
     **/
    void spawn() {
        //Reset wave counter.
        current_wave = 0;
        spawnNextWave();
    }

    bool spawnNextWave()
    {
        bool spawned_something = false;
        current_wave++;

        for( uint entityIndex = 0; entityIndex < spawnedEntities.length(); ++entityIndex ) {
            if( spawnedEntities[ entityIndex ].wave == current_wave )
            {
                spawnedEntities[ entityIndex ].spawn();
                spawned_something = true;
            }
        }

        // Get the player object
        if( playerIndexId != -1 ) {
            playerObjectId = spawnedEntities[ playerIndexId ].spawnedObjectId;
            @player = ReadCharacterID( playerObjectId );
        }
        else {
            DisplayError("Error", "No player set!");
        }

        // Get the objects for all characters
        teams.resize( numTeams );
        for( uint teamIndex = 0; teamIndex < numTeams; ++teamIndex ) {
            for( uint charIndex = 0; charIndex < teamsByIndex.length(); ++charIndex ) {
                if( spawnedEntities[ charIndex ].wave == current_wave )
                {
                    int objectId = spawnedEntities[ charIndex ].spawnedObjectId;
                    teams[ teamIndex ].insertLast( ReadCharacterID( objectId ) );
                }
            }
        }

        return spawned_something;
    }
}

BattleInstance battle; // global battle instance



// // Attach a specific preview path to a given placeholder object
// void SetSpawnPointPreview(Object@ spawn, string &in path) {
//     PlaceholderObject@ placeholder_object = cast<PlaceholderObject@>(spawn);
//     placeholder_object.SetPreview(path);
// }

// // Find spawn points and set which object is displayed as a preview
// void SetPlaceholderPreviews() {
//     array<int> @object_ids = GetObjectIDs();
//     int num_objects = object_ids.length();
//     for(int i=0; i<num_objects; ++i) {
//         Object @obj = ReadObjectFromID(object_ids[i]);
//         ScriptParams@ params = obj.GetScriptParams();
//         if(params.HasParam("Name")) {
//             string name_str = params.GetString("Name");
//             if("character_spawn" == name_str) {
//                 SetSpawnPointPreview(obj,level.GetPath("spawn_preview"));
//             }
//             if("weapon_spawn" == name_str) {
//                 SetSpawnPointPreview(obj,level.GetPath("weap_preview"));
//             }
//         }
//     }
// }

