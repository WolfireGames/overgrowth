// Include the shared code
#include 'shared.as'

// This script implements the logic for the zombie
class CZombie : IController
{
	// The constructor must take a CGameObj handle as input,
	// this is the object that the script will control
	CZombie(CGameObj @obj)
	{
		// Keep the owner for later reference
		@self = obj;
		direction = UP;
	}

	void OnThink()
	{
		// Check if we already have a reference to the player
		const CGameObj @player = playerRef;
		if( player is null )
		{
			@player = game.FindObjByName('player');
			
			// Keep a weak ref to the player so we don't keep the object alive unnecessarily
			@playerRef = player;
		}
		
		// The zombie can either turn, move forward, or hit. 
		// It cannot do more than one of these, otherwise the 
		// player will not have any advantage.
		
		int dx = 0, dy = 0;
		switch( direction )
		{
		case UP:
			if( player.y < self.y )
				dy--;
			else if( player.x < self.x )
				direction--;
			else 
				direction++;
			break;
		case RIGHT:
			if( player.x > self.x )
				dx++;
			else if( player.y < self.y )
				direction--;
			else 
				direction++;
			break;
		case DOWN:
			if( player.y > self.y )
				dy++;
			else if( player.x > self.x )
				direction--;
			else 
				direction++;
			break;
		case LEFT:
			if( player.x < self.x )
				dx--;
			else if( player.y > self.y )
				direction--;
			else 
				direction++;
			break;
		}
		
		if( direction < 0 ) direction += 4;
		if( direction > 3 ) direction -= 4;
		
		if( dx != 0 || dy != 0 )
		{
			if( !self.Move(dx,dy) )
			{
				// It wasn't possible to move there.
				// Was the player in front of us?
				if( player.x == self.x + dx && player.y == self.y + dy )
				{
					// Hit the player
					self.Send(CMessage('Attack'), player);
				}
				else
				{
					// It was a stone. Turn towards the player
					switch( direction )
					{
					case UP:    if( player.x < self.x ) direction--; else direction++; break;
					case RIGHT: if( player.y < self.y ) direction--; else direction++; break;
					case DOWN:  if( player.x > self.x ) direction--; else direction++; break;
					case LEFT:  if( player.y > self.y ) direction--; else direction++; break;
					}
					
					if( direction < 0 ) direction += 4;
					if( direction > 3 ) direction -= 4;
				}
			}
		}
	}
	
	CGameObj @self;
	const_weakref<CGameObj> playerRef;
	int direction;
}

enum EDirection
{
	UP,
	RIGHT,
	DOWN,
	LEFT
}

