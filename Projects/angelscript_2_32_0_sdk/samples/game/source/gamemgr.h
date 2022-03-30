#ifndef GAMEMGR_H
#define GAMEMGR_H

#include <vector>
#include <string>

class CGameObj;

class CGameMgr
{
public:
	CGameMgr();
	~CGameMgr();

	int StartGame();
	void Run();
	void EndGame(bool win);

	CGameObj *GetGameObjAt(int x, int y);

	bool GetActionState(int action);

	CGameObj *FindGameObjByName(const std::string &name);

protected:
	void Render();
	void GetInput();
	CGameObj *SpawnObject(const std::string &type, char dispChar, int x, int y);

	std::vector<CGameObj*> gameObjects;

	bool actionStates[4];

	bool gameOn;
};

extern CGameMgr *gameMgr;

#endif
