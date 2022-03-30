/*******
 *  
 * Representation for spawning an enemy to an arena battle 
 *
 */

 #include "multi_arena/multi_arena_base_spawn.as"

class BattleEnemySpawn : BattleEntitySpawn {

    int fur_channel;   // Which tint mask channel corresponds to fur
    array<vec3> color(4);     // color of the enemy
    float difficulty;  // difficulty of this enemy
    string useWeaponPath;  // weapon for the enemy, empty string for none

    BattleEnemySpawn() {
        useWeaponPath = "";
    }


    /**
     * Spawn the entity into the world
     **/
    void spawn() override {
        // Spawn actor
        Object@ charObject = spawnObjectAtSpawnPoint();
        
        // Set palette colors randomly, darkening based on skill
        for(int i=0; i<4; ++i) {
            charObject.SetPaletteColor(i, color[i]);
        }
        charObject.SetPaletteColor(fur_channel, GetRandomFurColor());
        
        // Set character parameters based on difficulty
        ScriptParams@ params = charObject.GetScriptParams();
        params.SetString("Teams", ""+team);
        params.SetFloat("Character Scale", mix(RangedRandomFloat(0.9f,1.0f), RangedRandomFloat(1.0f,1.1f), difficulty));
        params.SetFloat("Fat", mix(RangedRandomFloat(0.4f,0.7f), RangedRandomFloat(0.4f,0.5f), difficulty));
        params.SetFloat("Muscle", mix(RangedRandomFloat(0.3f,0.5f), RangedRandomFloat(0.5f,0.7f), difficulty));
        params.SetFloat("Ear Size", RangedRandomFloat(0.5f,2.5f));
        params.SetFloat("Block Follow-up", mix(RangedRandomFloat(0.01f,0.25f), RangedRandomFloat(0.75f,1.0f), difficulty));
        params.SetFloat("Block Skill", mix(RangedRandomFloat(0.01f,0.25f), RangedRandomFloat(0.5f,0.8f), difficulty));
        params.SetFloat("Movement Speed", mix(RangedRandomFloat(0.8f,1.0f), RangedRandomFloat(0.9f,1.1f), difficulty));
        params.SetFloat("Attack Speed", mix(RangedRandomFloat(0.8f,1.0f), RangedRandomFloat(0.9f,1.1f), difficulty));
        float damage = mix(RangedRandomFloat(0.3f,0.5f), RangedRandomFloat(0.9f,1.1f), difficulty);
        params.SetFloat("Attack Knockback", damage);
        params.SetFloat("Attack Damage", damage);
        params.SetFloat("Aggression", RangedRandomFloat(0.25f,0.75f));
        params.SetFloat("Ground Aggression", mix(0.0f, 1.0f, difficulty));
        params.SetFloat("Damage Resistance", mix(RangedRandomFloat(0.6f,0.8f), RangedRandomFloat(0.9f,1.1f), difficulty));
        params.SetInt("Left handed", (rand()%5==0)?1:0);
        
        if(rand()%2==0) {
            params.SetString("Unarmed Stance Override", level.GetPath("alt_stance_anim"));
        }
        
        charObject.UpdateScriptParams();

        // Check to see there's a weapon to be spawned 
        if( useWeaponPath != "" ) {
            attachWeapon( useWeaponPath );
        }

    }
}
