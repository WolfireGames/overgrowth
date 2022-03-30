// Include the shared code
#include 'shared.as'

// This script implements the logic for the player character
class CPlayer : IController
{
	// The constructor must take a CGameObj handle as input,
	// this is the object that the script will control
	CPlayer(CGameObj @obj)
	{
		// Keep the owner for later reference
		@self = obj;
	}

	void OnThink()
	{
		// What do the player want to do?
		int dx = 0, dy = 0;
		if( game.actionState[UP] )
			dy--;
		if( game.actionState[DOWN] )
			dy++;
		if( game.actionState[LEFT] )
			dx--;
		if( game.actionState[RIGHT] )
			dx++;
		if( !self.Move(dx,dy) )
		{
			// It wasn't possible to move there.
			// Is there a zombie in front of us?
			// TODO:
		}
	}
	
	void OnMessage(ref @m, const CGameObj @sender)
	{
		CMessage @msg = cast<CMessage>(m);
		if( msg !is null && msg.txt == 'Attack' )
		{
			// The zombie got us
			self.Kill();
			game.EndGame(false);
		}
	}
	
	CGameObj @self;
}


// These are the actions that the user can do
enum EAction
{
	UP = 0,
	DOWN = 1,
	LEFT = 2,
	RIGHT = 3  
}
