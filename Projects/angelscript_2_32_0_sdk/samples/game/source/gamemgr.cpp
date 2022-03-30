#include "gamemgr.h"
#include "gameobj.h"
#include "scriptmgr.h"
#include <string.h>  // strcpy
#include <stdio.h>   // rand
#include <stdlib.h>  // rand
#include <iostream>  // cout

using namespace std;

CGameMgr::CGameMgr()
{
	gameOn = false;
}

CGameMgr::~CGameMgr()
{
	for( unsigned int n = 0; n < gameObjects.size(); n++ )
		gameObjects[n]->DestroyAndRelease();
}

int CGameMgr::StartGame()
{
	// Set up the game level

	// In a real game this would probably be loaded from a file, that would define
	// the position of each object and other things that would be placed in the game world.

	// The properties for each object type would probably also be loaded from a file.
	// The map file, would only have to specify the position and the type of the object.
	// Based on the type, the game manager would retrieve the graphics object and script
	// controller that should be used.

	// Create some stones
	for( unsigned int n = 0; n < 10; n++ )
		SpawnObject("stone", '0', rand()%10, rand()%10);

	// Create some zombies
	for( unsigned int n = 0; n < 3; n++ )
		SpawnObject("zombie", 'z', rand()%10, rand()%10);

	// Create the player
	CGameObj *obj = SpawnObject("player", 'p', rand()%10, rand()%10);
	if( obj )
		obj->name = "player";

	// Check if there were any compilation errors during the script loading
	if( scriptMgr->hasCompileErrors )
		return -1;

	return 0;
}

CGameObj *CGameMgr::SpawnObject(const std::string &type, char dispChar, int x, int y)
{
	CGameObj *obj = new CGameObj(dispChar, x, y);
	gameObjects.push_back(obj);

	// Set the controller based on type
	obj->controller = scriptMgr->CreateController(type, obj);

	return obj;
}

void CGameMgr::Run()
{
	gameOn = true;
	while( gameOn )
	{
		// Render the frame
		Render();

		// Get input from user
		GetInput();

		// Call the onThink method on each game object
		for( unsigned int n = 0; n < gameObjects.size(); n++ )
			gameObjects[n]->OnThink();

		// Kill the objects that have been queued for killing
		for( unsigned int n = 0; n < gameObjects.size(); n++ )
		{
			if( gameObjects[n]->isDead )
			{
				// We won't actually delete the memory here, as we do not know 
				// exactly who might still be referencing the object, but we
				// make sure to destroy the internals in order to avoid
				// circular references. 
				gameObjects[n]->DestroyAndRelease();
				gameObjects.erase(gameObjects.begin()+n);
				n--;
			}
		}
	}
}

void CGameMgr::EndGame(bool win)
{
	gameOn = false;

	if( win )
		cout << "Congratulations, you've defeated the zombies!" << endl;
	else
		cout << "Too bad, the zombies ate your brain!" << endl;
	
	// Get something to let the player see the message before exiting
	char buf[2];
	cin.getline(buf, 1);
}

void CGameMgr::Render()
{
	// Clear the buffer
	char buf[10][11];
	for( int y = 0; y < 10; y++ )
		memcpy(buf[y], "..........\0", 11);

	// Render each object into the buffer
	for( unsigned int n = 0; n < gameObjects.size(); n++ )
		buf[gameObjects[n]->y][gameObjects[n]->x] = gameObjects[n]->displayCharacter;

	// Clear the screen
#ifdef _WIN32
	system("cls");
#else
	system("clear");
#endif

	// Print some useful information and start the input loop
	cout << "Sample game using AngelScript " << asGetLibraryVersion() << "." << endl;
	cout << "Type u(p), d(own), l(eft), r(ight) to move the player." << endl;
	cout << "Type q(uit) to exit the game." << endl;
	cout << "Try to avoid getting eaten by the zombies, hah hah." << endl;
	cout << endl;

	// Present the buffer
	for( int y = 0; y < 10; y++ )
		cout << buf[y] << endl;
}

void CGameMgr::GetInput()
{
	cout << "> ";

	char buf[10];
	cin.getline(buf, 10);

	memset(actionStates, 0, sizeof(actionStates));

	switch( buf[0] )
	{
	case 'u':
		actionStates[0] = true;
		break;
	case 'd':
		actionStates[1] = true;
		break;
	case 'l':
		actionStates[2] = true;
		break;
	case 'r':
		actionStates[3] = true;
		break;
	case 'q':
		gameOn = false;
		break;
	}
}

bool CGameMgr::GetActionState(int action)
{
	if( action < 0 || action >= 4 ) return false;

	return actionStates[action];
}

CGameObj *CGameMgr::GetGameObjAt(int x, int y)
{
	for( unsigned int n = 0; n < gameObjects.size(); n++ )
		if( gameObjects[n]->x == x && gameObjects[n]->y == y )
			return gameObjects[n];

	return 0;
}

CGameObj *CGameMgr::FindGameObjByName(const string &name)
{
	for( unsigned int n = 0; n < gameObjects.size(); n++ )
	{
		if( gameObjects[n]->name == name )
			return gameObjects[n];
	}

	return 0;
}
