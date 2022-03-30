/*******
 *  
 * Representation for spawning a player to an arena battle 
 *
 */

#include "multi_arena/multi_arena_base_spawn.as"

class BattlePlayerSpawn : BattleEntitySpawn {

    string useWeaponPath;  // weapon for the player, empty string for none
    array<vec3> color(4);  // color of the player

    BattlePlayerSpawn() {
        entityPath = level.GetPath("char_player"); 
        useWeaponPath = "";
    }

    void spawn() override {

        Object@ char_obj = spawnObjectAtSpawnPoint();
        
        // Update the bookkeeping
        char_obj.SetPlayer(true);
        ScriptParams@ char_params = char_obj.GetScriptParams();
        
        char_params.SetString("Teams", ""+team);

        // Set the color
        vec3 color = FloatTintFromByte(RandReasonableColor());
        float tint_amount = 0.5f;
        if(team == 0) {
            color = mix(color, vec3(1,0,0), tint_amount);
        } else if(team == 1) {
            color = mix(color, vec3(0,0,1), tint_amount);
        } else if(team == 2) {
            color = mix(color, vec3(1,0.5f,0), tint_amount);
        } else if(team == 3) {
            color = mix(color, vec3(0,1,1), tint_amount);
        }

        color = mix(color, vec3(1.0-(global_data.player_skill-0.5f)), 0.5f);
        global_data.player_colors[2] = color;
        
        for(int j=0; j<4; ++j) {
            char_obj.SetPaletteColor(j, global_data.player_colors[j]);
        }

        // Check to see there's a weapon to be spawned 
        if( useWeaponPath != "" ) {
            attachWeapon( useWeaponPath );
        }

    }
}
