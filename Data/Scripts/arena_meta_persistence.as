//-----------------------------------------------------------------------------
//           Name: arena_meta_persistance.as
//      Developer: Wolfire Games LLC
//    Script Type:
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
//   Copyright 2022 Wolfire Games LLC
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//-----------------------------------------------------------------------------

#include "arena_funcs.as"
#include "utility/string_json_injector.as"
#include "utility/json_help.as"
#include "utility/array.as"
#include "arena_meta_persistence_sanity_check.as"

const float MIN_PLAYER_SKILL = 0.5f;
const float MAX_PLAYER_SKILL = 1.9f;

const int SAVEVERSION = 17;  // So we can keep track of older versions

// Just for fun let's have some random name -- total non-cannon 
array<string> firstNames = {"P'teth", "Greah", "Smugli", "Mec", "Jinx", "Malchi", 
"Fetla", "Qil", "Fet", "Vri", "Tenda", "Kwell", "Kanata", "Poi", "Wit", "Scar", "Trip", 
"Dreda", "Leki", "Yog", "Te-te", "Pela", "Quor", "Ando", "Imon", "Flip", "Goty", "Tril",
"Dede", "Menta", "Farren", "Gilt", "Gam", "Jer", "Pex", "Prim" };

array<string> lastNameFirsts = { "Bright", "Dark", "Golden", "Swift", "Still", "Quiet", 
"Hard", "Hidden", "Torn", "Silver", "Steel", "Rising" };
array<string> lastNameSeconds = { "water", "dawn", "leaf", "runner", "moon", "sky", 
"rain", "blood", "wind", "river" };

enum ActionIfOperator
{
    ActionOperatorAnd,
    ActionOperatorOr,
    ActionOperatorNot
};

class WorldMapNodeInstance
{
    WorldMapNodeInstance( string _id )
    {
        id = _id;
        is_available = true;
        is_visited = false;
    }

    WorldMapNodeInstance( string _id, bool _available, bool _visited )
    {
        id = _id;
        is_available = _available;
        is_visited = _visited;
    }

    WorldMapNodeInstance( JSONValue node )
    {
        id = node["id"].asString();
        is_available = node["available"].asBool();
        is_visited = node["visited"].asBool();
    }

    JSONValue toJSON()
    {
        JSONValue node = JSONValue( JSONobjectValue );
        node["id"] = JSONValue( id );
        node["available"] = JSONValue( is_available );
        node["visited"] = JSONValue( is_visited ); 
        return node;
    }

    void unavailable()
    {
        is_available = false;
    }

    void available()
    {
        is_available = true;
    }

    void visit()
    {
        is_visited = true;
    }

    void unvisit()
    {
        is_visited = false;
    }

    string id;
    bool is_visited;
    bool is_available;
};

class WorldMapConnectionInstance
{
    WorldMapConnectionInstance( string _id )
    {
        id = _id;
    }

    WorldMapConnectionInstance( JSONValue node )
    {
        id = node["id"].asString();
    }

    JSONValue toJSON()
    {
        JSONValue node = JSONValue( JSONobjectValue );
        node["id"] = JSONValue( id );
        return node;
    }

    string id;
};

class GlobalArenaData { 
    //Info about campaigns/world maps
    JSON campaignJSON;

    // Info about the save file
    bool dataLoaded = false;    // Have we loaded the data yet?
    bool dataOutdated = false;  // Did the version numbers match?
    JSON profileData;           // Data stored

    // Info about the player
    int profileId = -1;         // Which profile are we working with
    int fan_base;               // How big is the fan base?
    float player_skill;         // What's the player skill?
    array<vec3> player_colors;  // What colors has the player selected
    array<string> states;       // Current states of the player
    array<string> hidden_states; // Non visualized states

    array<WorldMapNodeInstance@> world_map_nodes; //World map nodes that are rendered
    array<WorldMapConnectionInstance@> world_map_connections; //List of connections between world map nodes;

    int player_wins;            // Lifetime wins
    int player_loses;           // Lifetime loses
    int player_kills;           // Number of individuals murdered
    int player_kos;             // Number of individuals knocked out
    int player_deaths;          // Number of deaths.
    string character_name;      // Name for this character
    string character_id;        // Character type chosen

    string world_map_id;        // Current world map

    string world_map_node_id;   // Current world map node
    string meta_choice_id;      // Id for current meta choice
    string message_id;          // Id for currently shown message
    string arena_instance_id;   // Current arena instance id.

    string world_node_id;       // Current node.
    int play_time;              // Play time in seconds
    /**************************/
    //Other data which isn't stored
    string queued_world_node_id;
    int meta_choice_option;
    bool arena_victory;
    bool done_with_current_node;

    /*******************************************************************************************/
    /**
     * @brief  Constructor
     *
     */
    GlobalArenaData() {
        // Make sure there's some values for non-arena campaign mode
        fan_base = 0;
        player_skill = MIN_PLAYER_SKILL;
        player_wins = 0;
        player_loses = 0;
        player_kills = 0;
        player_kos = 0;
        player_deaths = 0;
        character_name = generateRandomName();
        character_id = "";
        world_node_id = "";
        world_map_id = "";
        world_map_node_id = "";
        meta_choice_id = "";
        message_id = "";
        arena_instance_id = "";
        play_time = 0;

        player_colors.resize(4);

        // Now add the colors
        for( uint i = 0; i < 4; i++ ) {
            player_colors[i] = GetRandomFurColor();
        }

        queued_world_node_id = "";
        meta_choice_option = -1;
        arena_victory = false;
        done_with_current_node = false;

        ReloadJSON();
    }

    SavedLevel@ GetSave() {
        return save_file.GetSave(GetCurrCampaignID(),"challenge_level",level_name);
    }

    void ReloadJSON()
    {
        dictionary filesAndRoots = 
        {
            {"characters",              "Data/Campaign/ArenaMode/StandardCampaign/characters.json"},
            {"states",                  "Data/Campaign/ArenaMode/StandardCampaign/states.json"},
            {"hidden_states",           "Data/Campaign/ArenaMode/StandardCampaign/hidden_states.json"},
            {"world_maps",              "Data/Campaign/ArenaMode/StandardCampaign/world_maps.json"},
            {"world_map_nodes",         "Data/Campaign/ArenaMode/StandardCampaign/world_map_nodes.json"},
            {"world_map_connections",   "Data/Campaign/ArenaMode/StandardCampaign/world_map_connections.json"},
            {"world_nodes",             "Data/Campaign/ArenaMode/StandardCampaign/world_nodes.json"},
            {"meta_choices",            "Data/Campaign/ArenaMode/StandardCampaign/meta_choices.json"},
            {"arena_instances",         "Data/Campaign/ArenaMode/StandardCampaign/arena_instances.json"},
            {"messages",                "Data/Campaign/ArenaMode/StandardCampaign/messages.json"},
            {"actions",                 "Data/Campaign/ArenaMode/StandardCampaign/actions.json"}
        };

        JSON j;

        array<string> keys = filesAndRoots.getKeys();

        for( uint i = 0; i < keys.length(); i++ )
        {
            string val;
            filesAndRoots.get(keys[i],val);
            j.parseFile( val );
            campaignJSON.getRoot()[keys[i]] = j.getRoot()[keys[i]];
        }
    } 

    /*******************************************************************************************/
    /**
     * @brief  Generates a random name from preset pieces
     * 
     * @returns name as a string
     *
     */
    string generateRandomName() { 

        return firstNames[ rand() % firstNames.length() ] + " " + 
               lastNameFirsts[ rand() % lastNameFirsts.length() ] +
               lastNameSeconds[ rand() % lastNameSeconds.length() ];
    }

    /*******************************************************************************************/
    /**
     * @brief  Produce a new blank set of profiles, etc
     *
     * @returns A JSON object with the minimum fields filled in
     *
     */
    JSON generateNewProfileSet() {

        JSON newProfileSet;

        newProfileSet.getRoot()["version"] = JSONValue( SAVEVERSION );
        newProfileSet.getRoot()["profiles"] =  JSONValue( JSONarrayValue );
        return newProfileSet; 

    }

    /*******************************************************************************************/
    /**
     * @brief Deletes all the profile data
     *
     */
    void clearProfiles() {
        if(!dataLoaded) {
            return;
        }

        profileData.getRoot()["version"] = JSONValue( SAVEVERSION );
        profileData.getRoot()["profiles"] =  JSONValue( JSONarrayValue );

    }

    /*******************************************************************************************/
    /**
     * @brief  Produce a new blank profile
     *
     * @returns A JSON value object with the minimum fields filled in
     *
     */
    JSONValue generateNewProfile() {

        if(!dataLoaded) {
            DisplayError("Persistence Error", "Can't create a profile without loading profiles first.");
        }
        
        JSONValue newProfile( JSONobjectValue );

        // generate a unique id
        bool idFound = false;
        int newId;

        JSONValue profiles = profileData.getRoot()["profiles"];

        while( !idFound ) {

            // Generate a random id
            newId = rand() % 10000;

            // See if it's being used already
            bool idInList = false;
            for( uint i = 0; i < profiles.size(); ++i ) {
                if( profiles[ i ]["id"].asInt() == newId ) {
                    idInList = true;
                }
            }

            // if not, we're good to go
            if( !idInList ) idFound = true;

        }

        // Write in some default value
        newProfile["id"]                        = JSONValue( newId );
        newProfile["fan_base"]                  = JSONValue( 0 );
        newProfile["player_skill"]              = JSONValue( MIN_PLAYER_SKILL );
        newProfile["player_colors"]             = JSONValue( JSONarrayValue );
        newProfile["player_wins"]               = JSONValue( 0 );
        newProfile["player_loses"]              = JSONValue( 0 );
        newProfile["player_kills"]              = JSONValue( 0 );
        newProfile["player_kos"]                = JSONValue( 0 );
        newProfile["player_deaths"]             = JSONValue( 0 );
        newProfile["character_name"]            = JSONValue( generateRandomName() );
        newProfile["fans"]                      = JSONValue( 0 );
        newProfile["states"]                    = JSONValue( JSONarrayValue );
        newProfile["hidden_states"]             = JSONValue( JSONarrayValue );
        newProfile["world_map_nodes"]           = JSONValue( JSONarrayValue );
        newProfile["world_map_connections"]     = JSONValue( JSONarrayValue );
        newProfile["character_id"]              = JSONValue( "" );
        newProfile["world_node_id"]             = JSONValue( "" );
        newProfile["world_map_id"]              = JSONValue( "" );
        newProfile["world_map_node_id"]         = JSONValue( "" );
        newProfile["meta_choice_id"]            = JSONValue( "" );
        newProfile["message_id"]                = JSONValue( "" );
        newProfile["arena_instance_id"]         = JSONValue( "" );
        newProfile["play_time"]                 = JSONValue( 0 );
        newProfile["pronoun"]                   = JSONValue( JSONobjectValue );
        newProfile["pronoun"]["she"]            = JSONValue( "he" );
        newProfile["pronoun"]["her"]            = JSONValue( "his" );
        newProfile["pronoun"]["herself"]        = JSONValue( "himself" );

        // Now add the colors
        for( uint i = 0; i < 4; i++ ) {
            
            JSONValue colorTriplet( JSONarrayValue );

            vec3 newRandomColor = GetRandomFurColor();

            colorTriplet.append( JSONValue( newRandomColor.x ) );
            colorTriplet.append( JSONValue( newRandomColor.y ) );
            colorTriplet.append( JSONValue( newRandomColor.z ) );

            newProfile[ "player_colors" ].append( colorTriplet );   
        }

        return newProfile;
    
    }

    /*******************************************************************************************/
    /**
     * @brief  Copy from the JSON structure to the member variables
     * 
     * @param targetId Id to load 
     *
     */
    void setDataFrom( int targetId ) {

        JSONValue profiles = profileData.getRoot()["profiles"];

        bool profileFound = false;

        for( uint i = 0; i < profiles.size(); ++i ) {
            if( profiles[ i ]["id"].asInt() == targetId ) {
                profileFound = true;

                // Copy all the values back
                profileId = targetId;
                fan_base = profiles[ i ][ "fan_base" ].asInt();
                player_skill = profiles[ i ][ "player_skill" ].asFloat();
                player_wins = profiles[ i ][ "player_wins" ].asInt();
                player_loses = profiles[ i ][ "player_loses" ].asInt();
                player_kills = profiles[ i ][ "player_kills" ].asInt();
                player_kos = profiles[ i ][ "player_kos" ].asInt();
                player_deaths = profiles[ i ][ "player_deaths" ].asInt();
                character_name = profiles[ i ][ "character_name" ].asString();
                character_id = profiles[ i ][ "character_id" ].asString();
                world_node_id = profiles[ i ][ "world_node_id" ].asString(); 
                world_map_id = profiles[ i ][ "world_map_id" ].asString(); 
                world_map_node_id = profiles[ i ][ "world_map_node_id" ].asString(); 
                meta_choice_id = profiles[ i ][ "meta_choice_id" ].asString(); 
                message_id = profiles[ i ][ "message_id" ].asString(); 
                arena_instance_id = profiles[ i ][ "arena_instance_id" ].asString(); 
                play_time = profiles[ i ][ "play_time" ].asInt(); 

                // Combine the color vectors
                for( uint c = 0; c < 4; c++ ) {
                    player_colors[c].x = profiles[ i ][ "player_colors" ][c][0].asFloat();
                    player_colors[c].y = profiles[ i ][ "player_colors" ][c][1].asFloat();
                    player_colors[c].z = profiles[ i ][ "player_colors" ][c][2].asFloat();
                }
                
                states = array<string>();

                for( uint j = 0; j < profiles[i]["states"].size(); j++ )
                {
                    string state = profiles[i]["states"][j].asString();
                    if( getState( state ).type() == JSONobjectValue )
                    {
                        states.insertLast(state);
                    }
                    else
                    {
                        Log( warning, "Not loading invalid state \"" + state + "\"" );
                    }
                }

                hidden_states = array<string>();

                for( uint j = 0; j < profiles[i]["hidden_states"].size(); j++ )
                {
                    string state = profiles[i]["hidden_states"][j].asString();
                    if( getHiddenState( state ).type() == JSONobjectValue )
                    {
                        hidden_states.insertLast(state);
                    }
                    else
                    {
                        Log( warning, "Not loading invalid state \"" + state + "\"" );
                    }
                }

                world_map_nodes.resize(0);

                for( uint j = 0; j < profiles[i]["world_map_nodes"].size(); j++ )  
                {
                    world_map_nodes.insertLast(WorldMapNodeInstance(profiles[i]["world_map_nodes"][j]));
                }

                world_map_connections.resize(0);

                for( uint j = 0; j < profiles[i]["world_map_connections"].size(); j++ )  
                {
                    world_map_connections.insertLast(WorldMapConnectionInstance(profiles[i]["world_map_connections"][j]));
                }

                // We're done here
                break;  
            }
        }

        // Sanity checking
        if( !profileFound ) {
            DisplayError("Persistence Error", "1 Profile id " + targetId + " not found in store.");
        }
    }

    JSONValue serializeCurrentProfile()
    {
        JSONValue prof = JSONValue(JSONobjectValue);

        // Copy all the values back
        prof[ "id" ]              = JSONValue( profileId );
        prof[ "fan_base" ]        = JSONValue( fan_base );
        prof[ "player_skill" ]    = JSONValue( player_skill );
        prof[ "player_wins" ]     = JSONValue( player_wins );
        prof[ "player_loses" ]    = JSONValue( player_loses );
        prof[ "player_kills" ]    = JSONValue( player_kills );
        prof[ "player_kos" ]      = JSONValue( player_kos );
        prof[ "player_deaths" ]   = JSONValue( player_deaths );
        prof[ "character_name" ]  = JSONValue( character_name );
        prof[ "character_id" ]    = JSONValue( character_id );
        prof[ "world_node_id" ]   = JSONValue( world_node_id );
        prof[ "world_map_id" ]    = JSONValue( world_map_id );
        prof[ "world_map_node_id"]= JSONValue( world_map_node_id );
        prof[ "meta_choice_id"]   = JSONValue( meta_choice_id );
        prof[ "message_id"]       = JSONValue( message_id );
        prof[ "arena_instance_id"]= JSONValue( arena_instance_id );
        prof[ "play_time" ]       = JSONValue( play_time );

        // Unpack the color vectors
        for( uint c = 0; c < 4; c++ ) {
            prof[ "player_colors" ][c][0] = JSONValue( player_colors[c].x );
            prof[ "player_colors" ][c][1] = JSONValue( player_colors[c].y );
            prof[ "player_colors" ][c][2] = JSONValue( player_colors[c].z );
        }

        prof[ "states" ] = JSONValue( JSONarrayValue );
        for( uint j = 0; j < states.length(); j++ )
        {
            prof[ "states" ].append(JSONValue(states[j]));
        }

        prof[ "hidden_states" ] = JSONValue( JSONarrayValue );
        for( uint j = 0; j < hidden_states.length(); j++ )
        {
            prof[ "hidden_states" ].append(JSONValue(hidden_states[j]));
        }

        prof[ "world_map_nodes" ] = JSONValue( JSONarrayValue );
        for( uint j = 0; j < world_map_nodes.length(); j++ )
        {
            prof[ "world_map_nodes" ].append( world_map_nodes[j].toJSON() );
        }

        prof[ "world_map_connections" ] = JSONValue( JSONarrayValue );
        for( uint j = 0; j < world_map_connections.length(); j++ )
        {
            prof[ "world_map_connections" ].append(world_map_connections[j].toJSON());
        }

        return prof;
    }

    /*******************************************************************************************/
    /**
     * @brief  Copy from the JSON structure to the member variables
     *
     */
    void writeDataToProfiles() {
        // Make sure that the data is good
        if( profileId == -1  ) {
            DisplayError("Persistence Error", "Trying to store an uninitialized profile.");
        }

        bool profileFound = false;

        for( uint i = 0; i < profileData.getRoot()["profiles"].size(); ++i ) {
            if( profileData.getRoot()["profiles"][ i ]["id"].asInt() == profileId ) {
                profileFound = true;

                JSONValue partial_profile = serializeCurrentProfile();

                array<string> partial_profile_members = partial_profile.getMemberNames();

                for(uint j = 0; j < partial_profile_members.size(); j++ )
                {
                    profileData.getRoot()["profiles"][ i ][partial_profile_members[j]] 
                        = partial_profile[partial_profile_members[j]];
                }

                // We're done here
                break;  
            }
        }

        // Sanity checking
        if( !profileFound ) {
            DisplayError("Persistence Error", "2 Profile id " + profileId + " not found in store.");
        }
    }

    /*******************************************************************************************/
    /**
     * @brief Adds a newly created profile to the profile set
     *
     * Checks to see if the id already exists, if so replaces it
     *
     * Not that this function does not activate the newly added profile as the current one
     * 
     * @param newProfile JSON value of the new profile 
     *
     */
    void addProfile( JSONValue newProfile ) {
        // check to see if this profile already exists
        for( uint i = 0; i < profileData.getRoot()["profiles"].size(); ++i ) {
            if( newProfile["id"].asInt() == profileData.getRoot()["profiles"][ i ]["id"].asInt() ) {
                profileData.getRoot()["profiles"][ i ] = newProfile;
                return;
            }
        }

        // if we got this far, it's a new one, so just append it
        profileData.getRoot()["profiles"].append( newProfile );

        WritePersistentInfo( false );
    }

    /*******************************************************************************************/
    /**
     * @brief  Removes a profile from the store
     *  
     * @param targetProfileId id of the profile to remove
     *
     */
    void removeProfile( int targetProfileId ) {

        // Find the profile in the array
        int targetProfileIndex = -1;
        for( uint i = 0; i < profileData.getRoot()["profiles"].size(); ++i ) {
            if( profileData.getRoot()["profiles"][ i ]["id"].asInt() == targetProfileId ) {
                targetProfileIndex = i;
                break;
            }
        }

        // Throw an error if not found
        if( targetProfileIndex == -1 ) {
            DisplayError("Persistence Error", "Cannot find profile " + targetProfileId + " for deletion");
        }

        // Do the removal 
        profileData.getRoot()["profiles"].removeIndex(targetProfileIndex);

        WritePersistentInfo( false );
    }

    /*******************************************************************************************/
    /**
     * @brief  Get the profile information by id 
     * 
     * @param requestId the id of the profile 
     *
     * @returns array of profile ids (integers)
     *
     */
    array<int> getProfileIds() {
        array<int> profileIds;

        for( uint i = 0; i < profileData.getRoot()["profiles"].size(); ++i ) {
            profileIds.insertLast( profileData.getRoot()["profiles"][ i ]["id"].asInt() );
        }
        
        return profileIds;

    }

    int getProfileIndexFromId( int id )
    {
        array<int> profileIds = getProfileIds();
        return profileIds.find(id);
    }

    /*******************************************************************************************/
    /**
     * @brief  Gets a *copy* of the profile data 
     * 
     * @returns The profile data as a JSON object
     *
     */
    JSONValue getProfiles() {
       if(!dataLoaded) {
           DisplayError("Persistence Error", "Cannot get profiles if data is not loaded");
       }
       return profileData.getRoot()["profiles"];
    }

    /**
     * @brief Get a copy of the currently selection profile
     * 
     * @returns The currently active profiles JSON object, will write current cached state to JSONValue first.
     */
    JSONValue getCurrentProfile() {
        writeDataToProfiles();
        for( uint i = 0; i < profileData.getRoot()["profiles"].size(); ++i ) {
            if( profileData.getRoot()["profiles"][ i ]["id"].asInt() == profileId )
            {
                return profileData.getRoot()["profiles"][ i ];
            }
        }
        return JSONValue();
    }


    /*******************************************************************************************/
    /**
     * @brief  Read the data from disk and if blank, set things up
     * 
     */
    void ReadPersistentInfo() {
        SavedLevel @saved_level = save_file.GetSavedLevel("arena_progress");
        
        // First we determine if we have a session -- if not we're not going to read data 
        JSONValue sessionParams = getSessionParameters();

        if( !sessionParams.isMember("started") ) {
            // Nothing is started, so we shouldn't actually read the data
            return;
        }

        // read in campaign_started
        string profiles_str = saved_level.GetValue("arena_profiles");

        if( profiles_str == "" ) {
            profileData = generateNewProfileSet();
        }
        else {

            // Parse the JSON
            if( !profileData.parseString( profiles_str ) ) {
                DisplayError("Persistence Error", "Unable to parse profile information");
            }

            // Now check the version 
            if( profileData.getRoot()[ "version" ].asInt() == SAVEVERSION ) {
                dataOutdated = false;
            }
            else {
                // We have a save version from a previous incarnation 
                // For now we'll just nuke it and restart
                dataOutdated = true;
                profileData = generateNewProfileSet();
            }

        } 

        dataLoaded = true;

        // Now see if we have a profile in this session -- if so load it 
        if( sessionParams.isMember("profile_id") ) {
            // Get the id from the session and load it into the usable values
            int currentProfileId = sessionParams["profile_id"].asInt();
            setDataFrom( currentProfileId ); // This will throw an error if not found
        }

    }

    /*******************************************************************************************/
    /**
     * @brief  Save the profile data to disk 
     * 
     * @param moveDataToStore Is there data in the variables that should be moved to the store
     *
     */
    void WritePersistentInfo( bool moveDataToStore = true ) {
        Log( info, "Saving profile data to disk" );

        // Make sure we've got information to write -- this is not an error
        if( !dataLoaded ) return;

        // Make sure our current data has been written back to the JSON structure
        if( moveDataToStore ) {
            Log(info, "Writing data to profile");
            writeDataToProfiles(); // This'll do nothing if we haven't set a profile
        }

        SavedLevel @saved_level = save_file.GetSavedLevel("arena_progress");
        
        // Render the JSON to a string
        string profilesString = profileData.writeString(false);

        // Set the value and write to disk
        saved_level.SetValue( "arena_profiles", profilesString );        
        save_file.WriteInPlace();
    
    }

    /*******************************************************************************************/
    /**
     * @brief  Begins a new session — overwrites any data that is there
     * 
     */
    void startNewSession() {
        JSONValue thisSession = getSessionParameters();
        thisSession["started"] = JSONValue( "true" );
        setSessionParameters( thisSession );
    }

    /*******************************************************************************************/
    /**
     * @brief  Sets which profile this session is using 
     *   
     * @param _profileId Which profile id will be using for this session
     *
     */
    void setSessionProfile(int _profileId) {
        JSONValue thisSession = getSessionParameters();
        thisSession["profile_id"] = JSONValue( _profileId );
        setSessionParameters( thisSession );
    }    

    /*******************************************************************************************/
    /**
     * @brief  Gets which profile this session is using 
     *   
     * @returns The profile id for this session
     *
     */
    int getSessionProfile() {
        JSONValue thisSession = getSessionParameters();
        if( !thisSession.isMember("profile_id") ) {
            return -1;
        }
        else {
            return thisSession["profile_id"].asInt();
        }
    }

    void clearSessionProfile()
    {
        WritePersistentInfo( false );

        JSONValue thisSession = getSessionParameters();
        thisSession["profile_id"] = JSONValue(-1); 
        setSessionParameters( thisSession );
    }

    void clearArenaSession()
    {
        WritePersistentInfo( false );

        JSONValue thisSession = JSONValue( JSONobjectValue );
        setSessionParameters( thisSession );
    }

    /*******************************************************************************************/
    /**
     * @brief  Gets the JSON object representing the current arena session
     *  
     */
    JSONValue getSessionParameters() {
        
        SavedLevel @saved_level = save_file.GetSavedLevel("arena_progress");
        
        // read in the text for the session object
        string arena_session_str = saved_level.GetValue("arena_session");

        // sanity check 
        if( arena_session_str == "" ) {
            arena_session_str = "{}";

            // write it back
            saved_level.SetValue("arena_session", arena_session_str );
        }

        JSON sessionJSON;

        // sanity check
        if( !sessionJSON.parseString( arena_session_str ) ) {
            DisplayError("Persistence Error", "Unable to parse session information");
        }

        return sessionJSON.getRoot();
    }

    /*******************************************************************************************/
    /**
     * @brief Stores the JSON representation for the current session
     *  
     */
    void setSessionParameters( JSONValue session ) {
        SavedLevel @saved_level = save_file.GetSavedLevel("arena_progress");
        
        // set the value to the stringified JSON
        JSON sessionJSON;
        sessionJSON.getRoot() = session;
        string arena_session_str = sessionJSON.writeString(false);
        saved_level.SetValue("arena_session", arena_session_str );

        // write out the changes
        save_file.WriteInPlace();
    }


    bool EvaluateActionIfNode( JSONValue node, string result )
    {
        //Array value means that it's an operator first, then one or more subevals.
        if( node.type() == JSONarrayValue )
        {
            if( node.size() >= 2 )
            {
                bool val = false;
                ActionIfOperator op = ActionOperatorAnd;

                if( node[0].type() == JSONstringValue )
                {
                    string str = node[0].asString();

                    if( str == "or" )
                    {
                        op = ActionOperatorOr;
                    }
                    else if( str == "and" )
                    {
                        op = ActionOperatorAnd;
                    }
                    else if( str == "not" )
                    {
                        op = ActionOperatorNot;
                    }
                    else
                    {
                        Log( error, "Unknown action-if operator " + str );
                    }
                }
                else
                {
                    Log( error, "First value in if-action-array isn't a string, as expected" );
                }
                
                if( op == ActionOperatorNot )
                {
                    if( node.size() == 2 )
                    {
                        val = !EvaluateActionIfNode(node[1],result);
                    }
                    else
                    {
                        DisplayError("Invalid not operator", "not operator is followed by more than one value or no value, has to be 1.");
                    }
                }
                else
                {
                    for( uint i = 1;  i < node.size(); i++ )
                    {
                        if( i == 1 )
                        {
                            val = EvaluateActionIfNode(node[i], result);
                        }   
                        else
                        {
                            if( op == ActionOperatorAnd )
                            {
                                val = val && EvaluateActionIfNode(node[i], result);
                            }
                            else if( op == ActionOperatorOr )
                            {
                                val = val || EvaluateActionIfNode(node[i], result);
                            }
                            else
                            {
                                Log( error, "Unknown if-action operator state" ); 
                            }
                        }

                        //We can do some potential early outs due to the nature of these operators.
                        if( op == ActionOperatorAnd )
                        {
                            if( !val )
                                break;
                        }
                        else if( op == ActionOperatorOr )
                        {
                            if( val )
                                break;
                        }
                    }
                }
                return val;
            }
            else
            {
                Log( error, "if-action-array doesn't contain enough elements" );
                return false;
            }

        }
        else if( node.type() == JSONbooleanValue )
        {
            return node.asBool();
        }
        else if( node.type() == JSONobjectValue )
        {
            array<string>@ memberNames = node.getMemberNames();

            if( memberNames.length() == 1 )
            { 
                string memb = memberNames[0];
                bool ret_value = false;

                if( memb == "result_match_any" ) {
                    ret_value = false;
                    JSONValue result_match_any = node["result_match_any"];
                    for( uint j = 0; j < result_match_any.size(); j++ )
                    {
                        if( result_match_any[j].asString() == result )
                        {
                            Log( info, "Match on result " + result_match_any[j].asString() + " and " + result );
                            ret_value = true;
                        }
                    }
                } else if(memb == "eq" ) {
                    JSONValue eq = node["eq"];
                    for( uint k = 0; k < eq.size(); k++ )
                    {
                        for( uint j = 1; j < eq[k].size(); j++  )
                        {
                            string s1 = resolveString(eq[k][j-1].asString());
                            string s2 = resolveString(eq[k][j].asString());
                            if( s1 == s2 )
                            {
                                Log( info, "Evaluated \"" + s1 + "\" == \"" + s2 + "\" to true" );
                                ret_value = true;
                            }
                            else
                            {
                                Log( info, "Evaluated \"" + s1 + "\" == \"" + s2 + "\" to false" );
                                ret_value = false;
                            }
                        }
                    }
                } else if( memb == "gt" ) {
                    JSONValue gt = node["gt"];
                    for( uint k = 0; k < gt.size(); k++ )
                    {
                        for( uint j = 1; j < gt[k].size(); j++  )
                        {
                            string s1 = resolveString(gt[k][j-1].asString());
                            string s2 = resolveString(gt[k][j].asString());

                            uint c1 = 0;
                            double v1 = parseFloat(s1,c1);
                            uint c2 = 0;
                            double v2 = parseFloat(s2,c2);
                            
                            if( c1 == s1.length() && c2 == s2.length() )
                            {
                                if( v1 > v2 )
                                {
                                    Log( info, "Evaluated \"" + s1 + "\" > \"" + s2 + "\" to true" );
                                    ret_value = true;
                                }
                                else
                                {
                                    Log( info, "Evaluated \"" + s1 + "\" > \"" + s2 + "\" to false" );
                                    ret_value = false;
                                }
                            }
                            else
                            {
                                Log( error, "Malformed input for evaluating \"" + s1 + "\" > \"" + s2 + "\"" );
                            }
                        }
                    }
                } else if( memb ==  "chance" ) {
                        JSONValue chance = node["chance"];
                        double r = ((rand()%1001)/1000.0f);
                        ret_value = ( r <= chance.asDouble());
                } else if( memb == "has_state" ) {
                        ret_value = global_data.hasState( node["has_state"].asString() );
                } else if( memb == "has_hidden_state" ) {
                        ret_value = global_data.hasHiddenState( node["has_hidden_state"].asString() );
                } else if( memb == "current_world_node_is" ) {
                        ret_value = (global_data.world_node_id == node["current_world_node_is"].asString());
                } else if( memb == "current_world_map_node_is" ) {
                        ret_value = (global_data.world_map_node_id == node["current_world_map_node_is"].asString());
                } else if( memb == "visited_world_map_node" ) {
                        WorldMapNodeInstance@ wmni = global_data.getWorldMapNodeInstance( node["visited_world_map_node"].asString() );
                        ret_value = (wmni !is null && wmni.is_visited);
                } else {
                    DisplayError("Unknown action if-clause", "Unknown action if-clause \"" + memberNames[0] + "\"");
                }
                return ret_value;
            }
            else
            {
                DisplayError( "Action data error", "Invalid number of objects in action if-clause, only one per object is allowed." );
                return false;
            }
        }
        else if( node.type() == JSONnullValue )
        {
            //Assume true if we are missing the if clause
            return true;
        } 
        else
        {
            DisplayError( "Action data error", "Invalid node in action if statement for node " + node["id"].asString() );
            return false;
        }
    }

    void ApplyWorldNodeActionClause(JSONValue action_clause, string result)
    {
        JSONValue set_world_node = action_clause["set_world_node"];
        if( set_world_node.type() != JSONnullValue )
        {
            if( set_world_node_to == "" )
            {
                Log( info, "Setting next world_node to " + world_node_id );
                queued_world_node_id = set_world_node.asString();
            }
            else
            {
                Log( error, "Previously set world node to " + set_world_node_to + " won't set again" );
            }
        }
        
        JSONValue add_states = action_clause["add_states"];
        if( add_states.type() != JSONnullValue )
        {
            for( uint j = 0; j < add_states.size(); j++ )
            {
                Log( info, "add_states: " + add_states[j].asString() );
                addState( add_states[j].asString() );
            }
        }

        JSONValue lose_states = action_clause["lose_states"];
        if( lose_states.type() != JSONnullValue )
        {
            for( uint j = 0; j < lose_states.size(); j++ )
            {
                Log( info, "lose_states: " + lose_states[j].asString() );
                removeState( lose_states[j].asString() );
            }
        }

        JSONValue add_hidden_states = action_clause["add_hidden_states"];
        if( add_hidden_states.type() != JSONnullValue )
        {
            for( uint j = 0; j < add_hidden_states.size(); j++ )
            {
                Log( info, "add_hidden_states: " + add_hidden_states[j].asString() );
                addHiddenState( add_hidden_states[j].asString() );
            }
        }

        JSONValue lose_hidden_states = action_clause["lose_hidden_states"];
        if( lose_hidden_states.type() != JSONnullValue )
        {
            for( uint j = 0; j < lose_hidden_states.size(); j++ )
            {
                Log( info, "lose_hidden_states: " + lose_hidden_states[j].asString() );
                removeHiddenState( lose_hidden_states[j].asString() );
            }
        }

        JSONValue actions = action_clause["actions"];
        if( actions.type() != JSONnullValue )
        {
            ProcessActionNodeArray( actions, result );
        }

        JSONValue set_meta_choice = action_clause["set_meta_choice"];
        if( set_meta_choice.type() != JSONnullValue )
        {
            Log( info, "set_meta_choice: " + set_meta_choice.asString() );
            meta_choice_id = set_meta_choice.asString();
        }  

        JSONValue set_message = action_clause["set_message"];
        if( set_message.type() != JSONnullValue )
        {
            Log( info, "set_message: " + set_message.asString() );
            message_id = set_message.asString(); 
        }
         
        JSONValue set_arena_instance = action_clause["set_arena_instance"];
        if( set_arena_instance.type() != JSONnullValue )
        {
            Log( info, "set_arena_instance: " + set_arena_instance.asString() );
            arena_instance_id = set_arena_instance.asString();
        }

        JSONValue set_world_map = action_clause["set_world_map"];
        if( set_world_map.type() != JSONnullValue )
        {
            Log( info, "set_world_map: " + set_world_map.asString() );
            world_map_id = set_world_map.asString();
        }

        JSONValue add_world_map_nodes = action_clause["add_world_map_nodes"];
        if( add_world_map_nodes.type() != JSONnullValue )
        {
            for( uint i = 0; i < add_world_map_nodes.size(); i++ )
            {
                Log( info, "add_world_map_nodes: " + add_world_map_nodes[i].asString() );
                addWorldMapNodeInstance( add_world_map_nodes[i].asString() );
            }
        }

        JSONValue remove_world_map_nodes = action_clause["remove_world_map_nodes"];
        if( remove_world_map_nodes.type() != JSONnullValue )
        {
            for( uint i = 0; i < remove_world_map_nodes.size(); i++ )
            {
                Log( info, "remove_world_map_nodes: " + remove_world_map_nodes[i].asString() );
                removeWorldMapNodeInstance( remove_world_map_nodes[i].asString() );
            }
        }
        
        JSONValue visit_world_map_nodes = action_clause["visit_world_map_nodes"];
        if( visit_world_map_nodes.type() != JSONnullValue )
        {
            for( uint i = 0; i < visit_world_map_nodes.size(); i++ )
            {
                WorldMapNodeInstance@ n = getWorldMapNodeInstance( visit_world_map_nodes[i].asString() );

                if( n is null )
                {
                    Log(error, "Trying to visit non-activated node \"" + visit_world_map_nodes[i].asString() + "\""  );
                }
                else
                {
                    Log( info, "visit_world_map_nodes: " + visit_world_map_nodes[i].asString( ));
                    n.visit();
                }
            }
        }

        JSONValue unvisit_world_map_nodes = action_clause["unvisit_world_map_nodes"];
        if( unvisit_world_map_nodes.type() != JSONnullValue )
        {
            for( uint i = 0; i < unvisit_world_map_nodes.size(); i++ )
            {
                WorldMapNodeInstance@ n = getWorldMapNodeInstance( unvisit_world_map_nodes[i].asString() );

                if( n is null )
                {
                    Log(error, "Trying to unvisit non-activated node \"" + unvisit_world_map_nodes[i].asString() + "\""  );
                }
                else
                {
                    Log( info, "unvisit_world_map_nodes: " + unvisit_world_map_nodes[i].asString( ));
                    n.unvisit();
                }
            }
        }

        JSONValue available_world_map_nodes = action_clause["available_world_map_nodes"];
        if( available_world_map_nodes.type() != JSONnullValue )
        {
            for( uint i = 0; i < available_world_map_nodes.size(); i++ )
            {
                WorldMapNodeInstance@ n = getWorldMapNodeInstance( available_world_map_nodes[i].asString() );

                if( n is null )
                {
                    Log(error, "Trying to available non-activated node \"" + available_world_map_nodes[i].asString() + "\""  );
                }
                else
                {
                    Log( info, "available_world_map_nodes: " + available_world_map_nodes[i].asString( ));
                    n.available();
                }
            }
        }

        JSONValue unavailable_world_map_nodes = action_clause["unavailable_world_map_nodes"];
        if( unavailable_world_map_nodes.type() != JSONnullValue )
        {
            for( uint i = 0; i < unavailable_world_map_nodes.size(); i++ )
            {
                WorldMapNodeInstance@ n = getWorldMapNodeInstance( unavailable_world_map_nodes[i].asString() );

                if( n is null )
                {
                    Log(error, "Trying to unavailable non-activated node \"" + unavailable_world_map_nodes[i].asString() + "\""  );
                }
                else
                {
                    Log( info, "unavailable_world_map_nodes: " + unavailable_world_map_nodes[i].asString( ));
                    n.unavailable();
                }
            }
        }

        JSONValue set_world_map_node = action_clause["set_world_map_node"];
        if( set_world_map_node.type() != JSONnullValue )
        {
            if( !hasWorldMapNodeInstance(set_world_map_node.asString() ) )
            {
                Log( info, "set_world_map_node: " + set_world_map_node.asString() );
                addWorldMapNodeInstance( set_world_map_node.asString() );
            }

            world_map_node_id = set_world_map_node.asString();
        }

        JSONValue add_world_map_connections = action_clause["add_world_map_connections"];
        if( add_world_map_connections.type() != JSONnullValue )
        {
            for( uint i = 0; i < add_world_map_connections.size(); i++ )
            {
                Log( info, "add_world_map_connections: " + add_world_map_connections[i].asString() );
                addWorldMapConnectionInstance( add_world_map_connections[i].asString() );
            }
        }

        JSONValue remove_world_map_connections = action_clause["remove_world_map_connections"];
        if( remove_world_map_connections.type() != JSONnullValue )
        {
            for( uint i = 0; i < remove_world_map_connections.size(); i++ )
            {
                Log( info, "remove_world_map_connections: " + remove_world_map_connections[i].asString() );
                removeWorldMapConnectionInstance( remove_world_map_connections[i].asString() );
            }
        }
    }

    void ProcessActionNode( JSONValue action_node, string result )
    {
        Log(info, "Processing action node: \"" + action_node["id"].asString() + "\"");

        JSONValue action_if = action_node["if"];
        JSONValue action_then = action_node["then"];
        JSONValue action_else = action_node["else"];

        if( EvaluateActionIfNode( action_if, result ) )
        {
            if( action_then.type() != JSONnullValue )
            {
                Log( info, "Running " + action_node["id"].asString() + " then clause" );
                ApplyWorldNodeActionClause(action_then,result);
            }
            else
            {
                Log( info, "Won't run " + action_node["id"].asString() );
            }
        }
        else
        {
            if( action_else.type() != JSONnullValue )
            {
                Log( info, "Running " + action_node["id"].asString() + " else clause" );
                ApplyWorldNodeActionClause(action_else,result);
            }
            else
            {
                Log( info, "Won't run " + action_node["id"].asString() );
            }
        }
    }

    void ProcessActionNodeArray( JSONValue actions, string result )
    {
        if( actions.type() == JSONarrayValue )
        {
            for( uint i = 0; i < actions.size(); i++ )
            {
                JSONValue action = actions[i];

                if( action.type() == JSONstringValue )
                {
                    ProcessActionNode(getAction(action.asString()),result);
                }
                else if( action.type() == JSONobjectValue )
                {
                    ProcessActionNode(action,result);
                }
            }
        }
        else
        {
            Log( error, "*_actions is not an array." );
        }
    }

    string set_world_node_to;

    void ResetDuplicateWarnings()
    {
        set_world_node_to = "";
    }

    void ResolveWorldNode( bool top_level = true )
    {
        JSONValue character = getCurrentCharacter();

        ResetDuplicateWarnings();

        if( queued_world_node_id != "" )
        {
            world_node_id = queued_world_node_id;

            JSONValue world_node = getWorldNode( world_node_id );
            queued_world_node_id = "";

            Log(info, "Trying to resolve pre_action for node: \"" + world_node["id"].asString() + "\"");
            ProcessActionNodeArray(character["global_pre_actions"], "");
            ProcessActionNodeArray(world_node["pre_actions"], "");
            ProcessActionNodeArray(character["global_post_actions"], "");

            done_with_current_node = false;
            ResolveWorldNode(false);
        }
        else if( done_with_current_node )
        {
            JSONValue world_node = getWorldNode( world_node_id );
            Log(info, "Trying to resolve post_action for node: \"" + world_node["id"].asString() + "\"");
            if( world_node["type"].asString() == "meta_choice" )
            { 
                JSONValue meta_choice = getMetaChoice(meta_choice_id);

                if( meta_choice_option >= 0 && meta_choice_option < int(meta_choice["options"].size()) )
                {
                    JSONValue result = meta_choice["options"][meta_choice_option];
                    Log( info, "Got result " + result["result"].asString() + "\n" );

                    ProcessActionNodeArray(character["global_pre_actions"], result["result"].asString());
                    ProcessActionNodeArray(world_node["post_actions"], result["result"].asString());
                    ProcessActionNodeArray(character["global_post_actions"], result["result"].asString());
                }
                else
                {
                    Log(error, "Invalid meta choice option" );
                }
            }
            else  if( world_node["type"].asString() == "message" ) 
            {
                ProcessActionNodeArray(character["global_pre_actions"], "continue");
                ProcessActionNodeArray(world_node["post_actions"], "continue");
                ProcessActionNodeArray(character["global_post_actions"], "continue");
            }
            else if( world_node["type"].asString() == "arena_instance" )
            {
                ProcessActionNodeArray(character["global_pre_actions"], arena_victory ? "win" : "loss");
                ProcessActionNodeArray(world_node["post_actions"], arena_victory ? "win" : "loss");
                ProcessActionNodeArray(character["global_post_actions"], arena_victory ? "win" : "loss");
            }
            else if( world_node["type"].asString() == "world_map" )
            {
                ProcessActionNodeArray(character["global_pre_actions"], "continue");
                ProcessActionNodeArray(world_node["post_actions"], "continue");
                ProcessActionNodeArray(character["global_post_actions"], "continue");
            }
            else
            {
                Log(error, "Unknown world_node type \""  + world_node["type"].asString() + "\"" );
            }

            if( queued_world_node_id == "" )
            { 
                DisplayError( "Arena Mode", "We were not given a new world_node from actions this means we are stuck here.");
            }

            done_with_current_node = false;

            ResolveWorldNode(false);
        }

        if( top_level )
        {
            WritePersistentInfo();
        }
    }

    WorldMapNodeInstance@ getWorldMapNodeInstance( string id )
    {
        for( uint i = 0; i < world_map_nodes.length(); i++ )
        {
            if( world_map_nodes[i].id == id )
                return @world_map_nodes[i];
        }
        return null;
    }

    bool hasWorldMapNodeInstance( string id )
    {
        if( getWorldMapNodeInstance(id) is null  )
            return false;
        else
            return true;
    }

    void addWorldMapNodeInstance( string id )
    {
        if( !hasWorldMapNodeInstance( id ) )
        {
            if( getWorldMapNode( id ).type() == JSONobjectValue )
            {
                world_map_nodes.insertLast( WorldMapNodeInstance( id ) );
            }
            else
            {
                DisplayError( "Error adding world map node", "Can't add world_map_node \"" + id + "\" because it isn't declared:" );
            }
        }
        else
        {
            Log( warning, "Tried to add world map node which already exists." );
        }
    }

    void removeWorldMapNodeInstance( string id )
    {
        for( uint i = 0; i < world_map_nodes.length(); i++ )
        {
            if( world_map_nodes[i].id == id )
            {
                world_map_nodes.removeAt(i);
                break;
            }
        }
    }

    WorldMapConnectionInstance@ getWorldMapConnectionInstance( string id )
    {
        for( uint i = 0; i < world_map_connections.length(); i++ )
        {
            if( world_map_connections[i].id == id )
                return @world_map_connections[i];
        }
        return null;
    }

    bool hasWorldMapConnectionInstance( string id )
    {
        if( getWorldMapConnectionInstance(id) is null  )
            return false;
        else
            return true;
    }

    void addWorldMapConnectionInstance( string id )
    {
        if( !hasWorldMapConnectionInstance( id ) )
        {
            if( getWorldMapConnection( id ).type() == JSONobjectValue )
            {
                world_map_connections.insertLast( WorldMapConnectionInstance( id ) );
            }
            else
            {
                DisplayError( "Error adding world map connection", "Can't add world_map_connection \"" + id + "\" because it isn't declared:" );
            }
        }
        else
        {
            Log( warning, "Tried to add world map connection which already exists." );
        }
    }

    void removeWorldMapConnectionInstance( string id )
    {
        for( uint i = 0; i < world_map_connections.length(); i++ )
        {
            if( world_map_connections[i].id == id )
            {
                world_map_connections.removeAt(i);
                break;
            }
        }
    }

    bool hasState( string state )
    {
        for( uint i = 0; i < states.length(); i++ )
        {
            if( states[i] == state )
                return true;
        }
        return false;
    }

    void removeState( string state )
    {
        if(hasState(state))
        {
            for( uint i = 0; i < states.length(); i++ )
            {
                if( states[i] == state )
                {
                    states.removeAt(i);
                    i--;
                }
            }
        }

    }

    void addState( string state )
    {
        if( getState( state ).type() != JSONnullValue )
        {
            if( not hasState( state ) )
            {
                states.insertLast(state);
            } 
        }
        else
        {
            Log( error, "State " + state + " isn't declared\n") ;
        }
    }

    bool hasHiddenState( string hidden_state )
    {
        for( uint i = 0; i < hidden_states.length(); i++ )
        {
            if( hidden_states[i] == hidden_state )
                return true;
        }
        return false;
    }

    void removeHiddenState( string hidden_state )
    {
        if(hasHiddenState(hidden_state))
        {
            for( uint i = 0; i < hidden_states.length(); i++ )
            {
                if( hidden_states[i] == hidden_state )
                {
                    hidden_states.removeAt(i);
                    i--;
                }
            }
        }
    }

    void addHiddenState( string hidden_state )
    {
        if( getHiddenState( hidden_state ).type() != JSONnullValue )
        {
            if( not hasHiddenState( hidden_state ) )
            {
                hidden_states.insertLast(hidden_state); 
            }
        }
        else
        {
            Log( error, "Hidde state " + hidden_state + " isn't declared\n") ;
        }
    }

    /**
    * @brief Contains the character type data 
    *
    */
    JSONValue getCharacters()
    {
        return campaignJSON.getRoot()["characters"]; 
    }

    /**
    * @brief Contains state information
    */
    JSONValue getStates()
    {
        return campaignJSON.getRoot()["states"];
    }

    JSONValue getHiddenStates()
    {
        return campaignJSON.getRoot()["hidden_states"];
    }

    JSONValue getMetaChoices()
    {
        return campaignJSON.getRoot()["meta_choices"];
    }

    JSONValue getWorldMaps()
    {
        return campaignJSON.getRoot()["world_maps"];
    }

    JSONValue getWorldMapNodes()
    {
        return campaignJSON.getRoot()["world_map_nodes"];
    }

    JSONValue getWorldMapConnections()
    {
        return campaignJSON.getRoot()["world_map_connections"];
    }

    JSONValue getArenaInstances()
    {
        return campaignJSON.getRoot()["arena_instances"];
    }

    JSONValue getMessages()
    {
        return campaignJSON.getRoot()["messages"];
    }
    
    JSONValue getActions()
    {
        return campaignJSON.getRoot()["actions"];
    }

    JSONValue getWorldNodes()
    {
        return campaignJSON.getRoot()["world_nodes"]; 
    }

    JSONValue getState( string id )
    {
        JSONValue states = getStates();
        for( uint i = 0; i < states.size(); i++ )
        {
            if( states[i]["id"].asString() == id )
            {
                return states[i];
            }
        } 
        return JSONValue();
    }

    JSONValue getHiddenState( string id )
    {
        JSONValue hidden_states = getHiddenStates();
        for( uint i = 0; i < hidden_states.size(); i++ )
        {
            if( hidden_states[i]["id"].asString() == id )
            {
                return hidden_states[i];
            }
        } 
        return JSONValue();
    }

    JSONValue getWorldMap( string id )
    {
        JSONValue worldmaps = getWorldMaps();

        for( uint i = 0; i < worldmaps.size(); i++ )
        {
            if( worldmaps[i]["id"].asString() == id )
            {
                return worldmaps[i];
            } 
        }
        return JSONValue();
    }

    JSONValue getWorldMapNode( string id )
    {
        JSONValue worldmapnodes = getWorldMapNodes();

        for( uint i = 0; i < worldmapnodes.size(); i++ )
        {
            if( worldmapnodes[i]["id"].asString() == id )
            {
                return worldmapnodes[i];
            }
        }
        return JSONValue();
    }

    JSONValue getWorldMapConnection( string id )
    {
        JSONValue worldmapconnections = getWorldMapConnections();

        for( uint i = 0; i < worldmapconnections.size(); i++ )
        {
            if( worldmapconnections[i]["id"].asString() == id )
            {
                return worldmapconnections[i];
            }
        }
        return JSONValue();
    }

    JSONValue getCharacter( string id )
    {
        JSONValue characters = getCharacters();

        for( uint i = 0; i < characters.size(); i++ )
        {
            if( characters[i]["id"].asString() == id )
            {
                return characters[i];
            }
        }
        return JSONValue();
    } 

    JSONValue getMessage( string id )
    {
        JSONValue messages = getMessages();
    
        for( uint i = 0; i < messages.size(); i++ )
        {
            if( messages[i]["id"].asString() == id )
            {
                return messages[i];
            }
        }
        return JSONValue();
    }

    JSONValue getCurrentCharacter()
    {
        return getCharacter(character_id);
    }

    JSONValue getCurrentWorldMap()
    {
        return getWorldMap(world_map_id);
    }

    JSONValue getWorldNode(string node_id )
    {
        JSONValue world_nodes = getWorldNodes();
        for( uint i = 0; i < world_nodes.size(); i++ )
        {
            if( world_nodes[i]["id"].asString() == node_id )
            {
                return world_nodes[i];
            }
        }

        return JSONValue();
    }

    JSONValue getMetaChoice(string id)
    {
        JSONValue meta_choices = getMetaChoices();

        for( uint i = 0; i < meta_choices.size(); i++ )
        {
            if( meta_choices[i]["id"].asString() == id )
            {
                return meta_choices[i];
            }
        } 

        return JSONValue();
    }

    JSONValue getArenaInstance( string id )
    {
        JSONValue arena_instances = getArenaInstances(); 

        for( uint i = 0; i < arena_instances.size(); i++ )
        {
            if( arena_instances[i]["id"].asString() == id )
            {
                return arena_instances[i];
            }
        }

        return JSONValue();
    }

    JSONValue getAction( string id )
    {
        JSONValue actions = getActions(); 

        for( uint i = 0; i < actions.size(); i++ )
        {
            if( actions[i]["id"].asString() == id )
            {
                return actions[i];
            }
        }

        return JSONValue();
    }

    JSONValue getCurrentWorldNode()
    {
        return getWorldNode( world_node_id );
    }

    /**
     * Resolve a string and fill it with data from the json data structures
     */
    string resolveString( string input )
    {
        StringJSONInjector sjsoni;    

        sjsoni.setRoot( "profile", getCurrentProfile() );

        return sjsoni.evaluate(input);
    }
}

GlobalArenaData global_data;
