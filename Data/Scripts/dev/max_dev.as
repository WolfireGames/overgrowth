#include "music_load.as"

MusicLoad ml("Data/Music/challengelevel.xml");

void Init(string p_level_name)
{
    DebugDrawText(vec3(0,20,-10),GetBuildVersionShort(),1.0f, true, _persistent);
    DebugDrawText(vec3(0,20,10),GetBuildTimestamp(), 30.0f, false, _persistent);
}

bool HasFocus()
{
    return false;
}

void Reset()
{
}

void ReceiveMessage(string msg)
{
}

void DrawGUI() 
{
}

bool t = true;
int c = 0;

string randSong()
{
    array<string> songs = {"sub_arena_loop", "challengelevel_ambient-tense", "challengelevel_ambient-happy", "challengelevel_combat", "challengelevel_sad" };

    c = (c+1)%songs.size();

    return songs[c];
}

void Update()
{
    if( IsKeyDown( GetCodeForKey("f6") ) )
    {
        if( t )
        { 
            string song = randSong();
            if( song != "" ) {
                PlaySong( song );
            }
            t = false;
        }
    }
    else
    {
        t = true;
    }

    DebugDrawPoint( vec3(0,20,0), vec4(0,1.0f,0,1.0f), _delete_on_update );
    /*
    mat4 meshTransform;
    meshTransform.SetTranslationPart( vec3(0,20,0) );
    DebugDrawWireMesh( "Data/Models/box.obj", meshTransform, vec4(1,0,0,1), _delete_on_update );
    DebugDrawCircle( meshTransform, vec4(0,1,0,1), _delete_on_update);
    */
    /*
    DebugDrawWireBox( vec3( 10, 20, 0 ),  vec3(10,10,10), vec3(1.0f,0,0), _delete_on_update );
    DebugDrawWireSphere( vec3( -10, 20, 0 ), 10, vec3(1.0f,0,0), _delete_on_update);
    DebugDrawWireCylinder( vec3( 0, 30, 0 ), 10, 10, vec3(1.0f,0,0), _delete_on_update);
    DebugDrawLine( vec3( 10, 20, 0 ), vec3( 0, 30, 0 ), vec3( 0.0f,1.0f,0 ), _delete_on_update);
    DebugDrawLine( vec3( 20, 20, 0 ), vec3( 10, 30, 0 ), vec3( 0.0f,1.0f,0 ),vec3( 0.0f,0.0f,1.0f ), _delete_on_update);

    array<vec3> vertices;

    vertices.push_back( vec3( 20, 25, 5 ) );
    vertices.push_back( vec3( 20, 20, 2 ) );
    vertices.push_back( vec3( 20, 20, 2 ) );
    vertices.push_back( vec3( 10, 21, 5 ) );
    vertices.push_back( vec3( 10, 21, 5 ) );
    vertices.push_back( vec3( 15, 20, 5 ) );

    DebugDrawLines( vertices ,vec4( 0.0f,0.0f,1.0f, 1.0f ), _delete_on_update);
    */
}
