/*******
 *  
 * Base class for spawned entity 
 *
 */

#include "arena_meta_persistence.as"

class BattleEntitySpawn {
    
    string locationName;   // which location to spawn at?
    int spawnedObjectId;   // what's my object id?
    int attachedObjectId;   // if we've spawned a object with this one
    int spawnPointOjectId; // what's the object id of my spawn point
    string entityPath; // Path to xml
    int team;   // what team am I on (-1 for none)
    int wave;

    /**
     * Constructor 
     **/
    BattleEntitySpawn() {
        team = -1;
        spawnedObjectId = -1;
        attachedObjectId = -1;
        wave = 1;
    }

    /**
     * Destructor
     **/
    ~BattleEntitySpawn() {
        // If we have a valid object id, free it
        // Start with any attached objects
        if( attachedObjectId != -1 ) {
            Log(info, "Deleting attached object: " + attachedObjectId  );
            DeleteObjectID( attachedObjectId );
        }
        // Now the object itself
        if( spawnedObjectId != -1 ) {
            Log(info, "Deleting object: " + spawnedObjectId );
            DeleteObjectID( spawnedObjectId );
        }
    }

    /**
     * Instantiate an object at the location of another object
     **/
    Object@ spawnObjectAtSpawnPoint() {
        
        // Create the new object
        spawnedObjectId = CreateObject( entityPath, true );
        Object @new_obj = ReadObjectFromID( spawnedObjectId );
        
        // Find the spawn point 
        Object@ spawn = ReadObjectFromID( spawnPointOjectId );
        
        // Move the new object to the correct location
        new_obj.SetTranslation(spawn.GetTranslation());
        vec4 rot_vec4 = spawn.GetRotationVec4();
        quaternion q(rot_vec4.x, rot_vec4.y, rot_vec4.z, rot_vec4.a);
        new_obj.SetRotation(q);

        // Update the bookkeeping
        ScriptParams@ params = new_obj.GetScriptParams();
        params.AddIntCheckbox("No Save", true);
        
        return new_obj;

    }

    /**
     * Attach a weapon to this object
     **/
     // This really should be a mixin, but one thing at a time
    void attachWeapon( string weaponPath ) {
    
        // Get the (actual) object we're attaching to
        Object@ charObj = ReadObjectFromID( spawnedObjectId );

        // Create the new item
        attachedObjectId = CreateObject( weaponPath, true );
        Object @itemObject = ReadObjectFromID( attachedObjectId );
        
        // Find the spawn point 
        Object@ spawn = ReadObjectFromID( spawnPointOjectId );
        
        // Move the new object to the correct location
        itemObject.SetTranslation(spawn.GetTranslation());
        vec4 rot_vec4 = spawn.GetRotationVec4();
        quaternion q(rot_vec4.x, rot_vec4.y, rot_vec4.z, rot_vec4.a);
        itemObject.SetRotation(q);

        // Update the bookkeeping
        ScriptParams@ itemParams = itemObject.GetScriptParams();
        itemParams.AddIntCheckbox("No Save", true);
       
        ScriptParams@ charParams = charObj.GetScriptParams();
        
        bool mirrored = false;
        if( charParams.HasParam("Left handed") && 
            charParams.GetInt("Left handed") != 0) {
            
            mirrored = true;
        
        }
        
        charObj.AttachItem(itemObject, _at_grip, mirrored);
    
    }


    /**
     * Spawn the entity into the world
     **/
    void spawn() {
        // This will be taken care of by the decedents
    }

}
