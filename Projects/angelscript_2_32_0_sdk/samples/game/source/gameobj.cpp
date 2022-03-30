#include "gameobj.h"
#include "scriptmgr.h"
#include "gamemgr.h"

using namespace std;

CGameObj::CGameObj(char dispChar, int x, int y)
{
	// The first reference is already counted when the object is created
	refCount         = 1;

	isDead           = false;
	displayCharacter = dispChar;
	this->x          = x;
	this->y          = y;
	controller       = 0;

	weakRefFlag      = 0;
}

CGameObj::~CGameObj()
{
	if( weakRefFlag )
	{
		// Tell the ones that hold weak references that the object is destroyed
		weakRefFlag->Set(true);
		weakRefFlag->Release();
	}

	if( controller )
		controller->Release();
}

asILockableSharedBool *CGameObj::GetWeakRefFlag()
{
	if( !weakRefFlag )
		weakRefFlag = asCreateLockableSharedBool();

	return weakRefFlag;
}

int CGameObj::AddRef()
{
	return ++refCount;
}

int CGameObj::Release()
{
	if( --refCount == 0 )
	{
		delete this;
		return 0;
	}
	return refCount;
}

void CGameObj::DestroyAndRelease()
{
	// Since there might be other object's still referencing this one, we
	// cannot just delete it. Here we will release all other references that
	// this object holds, so it doesn't end up holding circular references.
	if( controller )
	{
		controller->Release();
		controller = 0;
	}

	Release();
}

void CGameObj::OnThink()
{
	// Call the script controller's OnThink method
	if( controller )
		scriptMgr->CallOnThink(controller);
}

bool CGameObj::Move(int dx, int dy)
{
	// Check if it is actually possible to move to the desired position
	int x2 = x + dx;
	if( x2 < 0 || x2 > 9 ) return false;

	int y2 = y + dy;
	if( y2 < 0 || y2 > 9 ) return false;

	// Check with the game manager if another object isn't occupying this spot
	CGameObj *obj = gameMgr->GetGameObjAt(x2, y2);
	if( obj ) return false;

	// Now we can make the move
	x = x2;
	y = y2;

	return true;
}

void CGameObj::Send(CScriptHandle msg, CGameObj *other)
{
	if( other && other->controller )
		scriptMgr->CallOnMessage(other->controller, msg, this);
}

void CGameObj::Kill()
{
	// Just flag the object as dead. The game manager will 
	// do the actual destroying at the end of the frame
	isDead = true;
}

int CGameObj::GetX() const
{
	return x;
}

int CGameObj::GetY() const
{
	return y;
}
