//-----------------------------------------------------------------------------
//           Name: arena_meta_persistance_sanity_check.as
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

/***
 * This function verifies that the campaign data id references are valid.
 *
 * First it verifies that the data is following strucutural rules.
 * Then the script verifies that links between different objects are correct.
 */
bool ArenaCampaignSanityCheck( GlobalArenaData@ gad )
{
    bool is_ok = true;

    is_ok = VerifyStructure( gad );

    if( is_ok )
    {
        is_ok = VerifyReferences( gad );
    }

    if( !is_ok )
    {
        DisplayError( "Errors in arena scripts", "There were one or more errors in the arena script, read the logs");
    }

    return is_ok;
}

/**********************************************/
/* Structure verification */
/**********************************************/
bool VerifyStructure( GlobalArenaData@ gad)
{
    bool is_ok = true;
    JSONValue root = gad.campaignJSON.getRoot();

    /*
    array<string> rootIds = JSON.getMembers( root );

    for( uint i = 0; i < rootIds.length(); i++ )
    {
        JSONValue 
        switch( 
    }
    */

    //TODO: Implement
    //
    return is_ok;
}

/**********************************************/
/* Reference verification */
/**********************************************/
bool VerifyReferences(GlobalArenaData@ gad)
{
    bool is_ok = true;

    is_ok = VerifyActionReferences( gad ) && is_ok; 
    is_ok = VerifyArenaInstanceReferences( gad ) && is_ok;
    is_ok = VerifyCharacterReferences( gad ) && is_ok;
    is_ok = VerifyWorldNodeReferences( gad ) && is_ok;
    is_ok = VerifyStateReferences( gad ) && is_ok;

    return is_ok;
}

bool VerifyStateReferences( GlobalArenaData@ gad )
{
    bool is_ok = true;
    JSONValue states = gad.getStates();

    for( uint i = 0; i < states.size(); i++ )
    {
        JSONValue state = states[i];

        if( !FileExists( "Data/" + state["glyph"].asString() ) )
        {
            Log(error, "Missing referenced state glyph: " + "Data/" + state["glyph"].asString() ); 
            is_ok = false;
        }   
    }
    return is_ok;
}


bool VerifyWorldNodeReferences( GlobalArenaData@ gad ) 
{
    bool is_ok = true;  

    JSONValue world_nodes = gad.getWorldNodes();

    for( uint i = 0; i < world_nodes.size(); i++ )
    {
        JSONValue world_node = world_nodes[i];  

        //Log( info, "Evaluating world_node id:" + world_node["id"].asString() );
        JSONValue type = world_node["type"];
        JSONValue pre_actions = world_node["pre_actions"];
        JSONValue post_actions = world_node["post_actions"];

        string type_s = type.asString();
        string id = world_node["id"].asString();

        for( uint j = 0; j < pre_actions.size(); j++ )
        {
            JSONValue action = pre_actions[j];
            
            is_ok = VerifyActionReferenceValue(gad,action) && is_ok;
        } 

        for( uint j = 0; j < post_actions.size(); j++ )
        {
            JSONValue action = post_actions[j];
            
            is_ok = VerifyActionReferenceValue(gad,action) && is_ok;
        } 
    } 

    return is_ok;
}

bool VerifyCharacterReferences( GlobalArenaData@ gad )
{
    bool is_ok = true;

    JSONValue characters = gad.getCharacters();

    for( uint i = 0; i < characters.size(); i++ )
    {
        JSONValue character = characters[i];

        string id = character["id"].asString();

        JSONValue portrait =        character["portrait"];
        JSONValue states =          character["states"];
        JSONValue world_node_id =   character["world_node_id"];
        JSONValue global_pre_actions =  character["global_pre_actions"];
        JSONValue global_post_actions =  character["global_post_actions"];
        JSONValue intro_pages =     character["intro"]["pages"];

        if( !FileExists( "Data/" + portrait.asString() ) )
        {
            Log( error, "character " + id + " has invalid portrait: " + portrait.asString() );
            is_ok = false;  
        }

        for( uint j = 0; j < states.size(); j++ )
        {
            JSONValue state = states[j];

            if( gad.getState(state.asString()).type() != JSONobjectValue )
            {
                Log( error, "character " + id + " has invalid state: " + state.asString() );
                is_ok = false;
            }
        }
    
        if( gad.getWorldNode( world_node_id.asString() ).type() != JSONobjectValue )
        {
            Log( error, "character " + id + " world_node_id is invalid " + world_node_id.asString() );
            is_ok = false;  
        }

        for( uint j = 0; j < global_pre_actions.size(); j++ )
        {
            is_ok = VerifyActionReferenceValue(gad,global_pre_actions[j]) && is_ok;
        }

        for( uint j = 0; j < global_post_actions.size(); j++ )
        {
            is_ok = VerifyActionReferenceValue(gad,global_post_actions[j]) && is_ok;
        }

        for( uint j = 0; j < intro_pages.size(); j++ )
        {
            JSONValue page = intro_pages[j];

            if( !FileExists( "Data/" + page["glyph"].asString() ) )
            {
                Log( error, "character " + id + " has invalid page glyph: " + page["glyph"].asString() );
                is_ok = false;
            }
        }
    }

    return is_ok;     
}

bool VerifyAction(GlobalArenaData@ gad,JSONValue action)
{
    bool is_ok = true;
    is_ok = RecursivelyCheckActionIfReferences(gad,action["if"]) && is_ok;
    is_ok = VerifyActionClause(gad,action["then"]) && is_ok;
    is_ok = VerifyActionClause(gad,action["else"]) && is_ok;
    return is_ok;
}

bool VerifyActionReferenceValue(GlobalArenaData@ gad,JSONValue action)
{
    if( action.type() == JSONobjectValue )
    {            
        return VerifyAction(gad,action);
    }
    else if( action.type() == JSONstringValue )
    {
        if( gad.getAction( action.asString() ).type() != JSONobjectValue )
        {
            Log( error, "the action id " + action.asString() + " is being referenced, but it doesn't exist.");
            return false;
        }
    }

    return true;
}

bool VerifyArenaInstanceReferences( GlobalArenaData@ gad )
{
    bool is_ok = true;

    JSONValue arena_instances = gad.getArenaInstances();

    for( uint i = 0; i < arena_instances.size(); i++ )
    {
        JSONValue arena_instance = arena_instances[i];

        JSONValue level = arena_instance["level"];
        JSONValue battle = arena_instance["battle"];
        
        if( !FileExists( "Data/Levels/" + level.asString() ) )
        {
            Log( error, "arena_instance missing level:" + arena_instance["level"].asString() );
            is_ok = false;
        }

    }

    return is_ok;     
}

bool VerifyActionReferences( GlobalArenaData@ gad )
{
    bool is_ok = true; 

    JSONValue actions = gad.getActions(); 

    for( uint i = 0; i < actions.size(); i++ )
    {
        JSONValue action = actions[i];
        
        bool v_is_ok = VerifyAction(gad,action);

        if( !v_is_ok  )
        {
            Log( error, "Previous errors came from action: " + action["id"].asString() );            
        }

        is_ok =  v_is_ok && is_ok;
        
    }
    
    return is_ok;
}

bool VerifyActionClause( GlobalArenaData@ gad, JSONValue clause )
{
    bool is_ok = true;
    if( clause.type() == JSONobjectValue )
    {
        JSONValue set_world_node                = clause["set_world_node"];
        JSONValue add_states                    = clause["add_states"];
        JSONValue lose_states                   = clause["lose_states"];
        JSONValue add_hidden_states             = clause["add_hidden_states"];
        JSONValue lose_hidden_states            = clause["lose_hidden_states"];
        JSONValue actions                       = clause["actions"]; 
        JSONValue set_arena_instance            = clause["set_arena_instance"];
        JSONValue set_meta_choice               = clause["set_meta_choice"];
        JSONValue set_message                   = clause["set_message"];
        JSONValue set_world_map                 = clause["set_world_map"];

        JSONValue add_world_map_nodes           = clause["add_world_map_nodes"];
        JSONValue remove_world_map_nodes        = clause["remove_world_map_nodes"];
        JSONValue visit_world_map_nodes         = clause["visit_world_map_nodes"];
        JSONValue unvisit_world_map_nodes       = clause["unvisit_world_map_nodes"];
        JSONValue available_world_map_nodes     = clause["available_world_map_nodes"];
        JSONValue unavailable_world_map_nodes   = clause["unavailable_world_map_nodes"];

        JSONValue set_world_map_node            = clause["set_world_map_node"];

        JSONValue add_world_map_connections     = clause["add_world_map_connections"];
        JSONValue remove_world_map_connections  = clause["remove_world_map_connections"];

        if( set_world_node.type() == JSONstringValue )
        {
            if( gad.getWorldNode( set_world_node.asString() ).type() != JSONobjectValue )
            {
                Log(error, "set_world_node is set to non-existant node: " + set_world_node.asString() );
                is_ok = false;
            }
        }
        else if( set_world_node.type() != JSONnullValue )
        {
            Log( error, "set_world_node has non-string value" );
            is_ok = false;
        }

        if( add_states.type() == JSONarrayValue )
        {
            for( uint j = 0; j < add_states.size(); j++ )
            {
                string state = add_states[j].asString(); 

                if( gad.getState(state).type() != JSONobjectValue )
                {
                    Log( error, "add_states lists invalid state: " + state);
                    is_ok = false;
                }
            }
        }
        else if( add_states.type() != JSONnullValue )
        {
            Log( error, "add_states has non-array value" );
            is_ok = false;
        }

        if( lose_states.type() == JSONarrayValue )
        {
            for( uint j = 0; j < lose_states.size(); j++ )
            {
                string state = lose_states[j].asString(); 

                if( gad.getState(state).type() != JSONobjectValue )
                {
                    Log( error, "lose_states lists invalid state: " + state);
                    is_ok = false;
                }
            }
        }
        else if( lose_states.type() != JSONnullValue )
        {
            Log( error, "lose_states has non-array value" );
            is_ok = false;
        }

        if( add_hidden_states.type() == JSONarrayValue )
        {
            for( uint j = 0; j < add_hidden_states.size(); j++ )
            {
                string state = add_hidden_states[j].asString(); 

                if( gad.getHiddenState(state).type() != JSONobjectValue )
                {
                    Log( error, "add_hidden_states lists invalid state: " + state);
                    is_ok = false;
                }
            }
        }
        else if( add_hidden_states.type() != JSONnullValue )
        {
            Log( error, "add_hidden_states has non-array value" );
            is_ok = false;
        }

        if( lose_hidden_states.type() == JSONarrayValue )
        {
            for( uint j = 0; j < lose_hidden_states.size(); j++ )
            {
                string state = lose_hidden_states[j].asString(); 

                if( gad.getHiddenState(state).type() != JSONobjectValue )
                {
                    Log( error, "lose_hidden_states lists invalid state: " + state);
                    is_ok = false;
                }
            }
        }
        else if( lose_hidden_states.type() != JSONnullValue )
        {
            Log( error, "lose_hidden_states has non-array value" );
            is_ok = false;
        }

        if( actions.type() == JSONarrayValue )
        {
            for( uint j = 0; j < actions.size(); j++ )
            {
                is_ok = VerifyActionReferenceValue(gad,actions[j]) && is_ok;
            }
        }
        else if( actions.type() != JSONnullValue )
        {
            Log( error, "actions has non-array value" );
            is_ok = false;
        }

        if( set_arena_instance.type() == JSONstringValue )
        {
            if( gad.getArenaInstance(set_arena_instance.asString()).type() != JSONobjectValue ) 
            {
                Log( error, "set_arena_instance refers to invalid set_arena_instance: " + set_arena_instance.asString());
                is_ok = false;
            }
        }
        else if( set_arena_instance.type() != JSONnullValue )
        {
            Log( error, "set_arena_instance has non-string value" );
            is_ok = false;
        }

        if( set_meta_choice.type() == JSONstringValue )
        {
            if( gad.getMetaChoice(set_meta_choice.asString()).type() != JSONobjectValue )
            {
                Log(error, "set_meta_choice referes to invalid meta_choice: " + set_meta_choice.asString() );
                is_ok = false;
            }
        }
        else if( set_meta_choice.type() != JSONnullValue )
        {
            Log( error, "set_meta_choice has non-string value" );
            is_ok = false;
        }

        if( set_message.type() == JSONstringValue )
        {
            if( gad.getMessage( set_message.asString() ).type() != JSONobjectValue )
            {
                Log(error, "set_message referes to invalid message: " + set_message.asString() );
                is_ok = false;
            }
        }
        else if( set_message.type() != JSONnullValue )
        {
            Log( error, "set_message has non-string value" );
            is_ok = false;
        }

        if( set_world_map.type() == JSONstringValue )
        {
            if( gad.getWorldMap( set_world_map.asString() ).type() != JSONobjectValue )
            {
                Log(error, "set_world_map referes to invalid message: " + set_world_map.asString() );
                is_ok = false;
            }
        }
        else if( set_world_map.type() != JSONnullValue )
        {
            Log( error, "set_world_map has non-string value" );
            is_ok = false;
        }

        if( add_world_map_nodes.type() == JSONarrayValue )
        {
            for( uint i = 0; i < add_world_map_nodes.size(); i++ )
            {
                if( gad.getWorldMapNode( add_world_map_nodes[i].asString() ).type() != JSONobjectValue )
                {
                    Log( error, "add_world_map_nodes refers to invalid world_map_node: " + add_world_map_nodes[i].asString() );
                    is_ok = false;
                } 
            }
        } 
        else if( add_world_map_nodes.type() != JSONnullValue )
        {
            Log( error, "add_world_map_nodes has non array value" );
            is_ok = false;
        }

        if( remove_world_map_nodes.type() == JSONarrayValue )
        {
            for( uint i = 0; i < remove_world_map_nodes.size(); i++ )
            {
                if( gad.getWorldMapNode( remove_world_map_nodes[i].asString() ).type() != JSONobjectValue )
                {
                    Log( error, "remove_world_map_nodes refers to invalid world_map_node: " + remove_world_map_nodes[i].asString() );
                    is_ok = false;
                } 
            }
        } 
        else if( remove_world_map_nodes.type() != JSONnullValue )
        {
            Log( error, "remove_world_map_nodes has non array value" );
            is_ok = false;
        }

        if( visit_world_map_nodes.type() == JSONarrayValue )
        {
            for( uint i = 0; i < visit_world_map_nodes.size(); i++ )
            {
                if( gad.getWorldMapNode( visit_world_map_nodes[i].asString() ).type() != JSONobjectValue )
                {
                    Log( error, "visit_world_map_nodes refers to invalid world_map_node: " + visit_world_map_nodes[i].asString() );
                    is_ok = false;
                } 
            }
        } 
        else if( visit_world_map_nodes.type() != JSONnullValue )
        {
            Log( error, "visit_world_map_nodes has non array value" );
            is_ok = false;
        }

        if( unvisit_world_map_nodes.type() == JSONarrayValue )
        {
            for( uint i = 0; i < unvisit_world_map_nodes.size(); i++ )
            {
                if( gad.getWorldMapNode( unvisit_world_map_nodes[i].asString() ).type() != JSONobjectValue )
                {
                    Log( error, "unvisit_world_map_nodes refers to invalid world_map_node: " + unvisit_world_map_nodes[i].asString() );
                    is_ok = false;
                } 
            }
        } 
        else if( unvisit_world_map_nodes.type() != JSONnullValue )
        {
            Log( error, "unvisit_world_map_nodes has non array value" );
            is_ok = false;
        }

        if( available_world_map_nodes.type() == JSONarrayValue )
        {
            for( uint i = 0; i < available_world_map_nodes.size(); i++ )
            {
                if( gad.getWorldMapNode( available_world_map_nodes[i].asString() ).type() != JSONobjectValue )
                {
                    Log( error, "available_world_map_nodes refers to invalid world_map_node: " + available_world_map_nodes[i].asString() );
                    is_ok = false;
                } 
            }
        } 
        else if( available_world_map_nodes.type() != JSONnullValue )
        {
            Log( error, "available_world_map_nodes has non array value" );
            is_ok = false;
        }

        if( unavailable_world_map_nodes.type() == JSONarrayValue )
        {
            for( uint i = 0; i < unavailable_world_map_nodes.size(); i++ )
            {
                if( gad.getWorldMapNode( unavailable_world_map_nodes[i].asString() ).type() != JSONobjectValue )
                {
                    Log( error, "unavailable_world_map_nodes refers to invalid world_map_node: " + unavailable_world_map_nodes[i].asString() );
                    is_ok = false;
                } 
            }
        } 
        else if( unavailable_world_map_nodes.type() != JSONnullValue )
        {
            Log( error, "unavailable_world_map_nodes has non array value" );
            is_ok = false;
        }

        if( set_world_map_node.type() == JSONstringValue )
        {
            if( gad.getWorldMapNode( set_world_map_node.asString() ).type() != JSONobjectValue )
            {
                Log( error, "set_world_map_node refers to invalid world_map_node: " + set_world_map_node.type() );
                is_ok = false;
            }
        }
        else if( set_world_map_node.type() != JSONnullValue )
        {
            Log( error, "set_world_map_node has non array value" );
            is_ok = false;
        }

        if( add_world_map_connections.type() == JSONarrayValue )
        {
            for( uint i = 0; i < add_world_map_connections.size(); i++ )
            {
                if( gad.getWorldMapConnection( add_world_map_connections[i].asString() ).type() != JSONobjectValue )
                {
                    Log( error, "add_world_map_connections refers to invalid world_map_node: " + add_world_map_connections[i].asString() );
                    is_ok = false;
                } 
            }
        } 
        else if( add_world_map_connections.type() != JSONnullValue )
        {
            Log( error, "add_world_map_connections has non array value" );
            is_ok = false;
        }

        if( remove_world_map_connections.type() == JSONarrayValue )
        {
            for( uint i = 0; i < remove_world_map_connections.size(); i++ )
            {
                if( gad.getWorldMapConnection( remove_world_map_connections[i].asString() ).type() != JSONobjectValue )
                {
                    Log( error, "remove_world_map_connections refers to invalid world_map_node: " + remove_world_map_connections[i].asString() );
                    is_ok = false;
                } 
            }
        } 
        else if( remove_world_map_connections.type() != JSONnullValue )
        {
            Log( error, "remove_world_map_connections has non array value" );
            is_ok = false;
        }
    }

    return is_ok;
}

bool RecursivelyCheckActionIfReferences( GlobalArenaData@ gad, JSONValue node )
{
    bool is_ok = true;
    if( node.type() == JSONarrayValue )
    {
        for( uint i = 0; i < node.size(); i++ )
        {
            is_ok = RecursivelyCheckActionIfReferences(gad,node[i]) && is_ok;
        }
    }
    else if( node.type() == JSONobjectValue )
    {
        JSONValue result_match_any           = node["result_match_any"]; //Untested
        JSONValue eq                         = node["eq"]; //Untested
        JSONValue gt                         = node["gt"]; //Untested
        JSONValue chance                     = node["chance"]; 
        JSONValue has_state                  = node["has_state"];
        JSONValue has_hidden_state           = node["has_hidden_state"];
        JSONValue current_world_node_is      = node["current_world_node_is"];

        if( chance.type() == JSONrealValue )
        {
            double c = chance.asDouble();
            if( c < 0 || c > 1.0 )
            {
                Log( error, "chance is outside the valid range [0,1]: " + c );
                is_ok = false;
            }
        }

        if( has_state.type() == JSONstringValue )
        {
            if( gad.getState( has_state.asString() ).type() != JSONobjectValue )
            {
                Log( error, "has_state refers to invalid state: " + has_state.asString());
                is_ok = false;
            }
        }

        if( has_hidden_state.type() == JSONstringValue )
        {
            if( gad.getHiddenState( has_hidden_state.asString() ).type() != JSONobjectValue )
            {
                Log( error, "has_hidden_state refers to invalid state: " + has_hidden_state.asString() );
                is_ok = false;
            }
        }
        
        if( current_world_node_is.type() == JSONstringValue )
        {
            if( gad.getWorldNode( current_world_node_is.asString() ).type() != JSONobjectValue )
            {
                Log( error, "current_world_node_is refers to invalid world_node: " + current_world_node_is.asString() );
                is_ok = false;
            }
        }

    }
    return is_ok;
}
