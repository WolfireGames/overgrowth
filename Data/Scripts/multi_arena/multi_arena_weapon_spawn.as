/*******
 *  
 * Representation for spawning a weapon to an arena battle 
 *
 */

 #include "multi_arena/multi_arena_base_spawn.as"

/**
 * Represent a weapon spawn
 **/
class BattleWeaponSpawn : BattleEntitySpawn {

    /**
     * Spawn the entity into the world
     **/
    void spawn() override {
        // Spawn object
        spawnObjectAtSpawnPoint();
    }

}

