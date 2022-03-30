#ifndef SCRIPTMGR_H
#define SCRIPTMGR_H

#include <string>
#include <vector>
#include <angelscript.h>
#include "../../../add_on/scripthandle/scripthandle.h"

class CGameObj;

class CScriptMgr
{
public:
	CScriptMgr();
	~CScriptMgr();

	int Init();

	asIScriptObject *CreateController(const std::string &type, CGameObj *obj);
	void CallOnThink(asIScriptObject *object);
	void CallOnMessage(asIScriptObject *object, CScriptHandle &msg, CGameObj *caller);

	bool hasCompileErrors;

protected:
	void MessageCallback(const asSMessageInfo &msg);
	asIScriptContext *PrepareContextFromPool(asIScriptFunction *func);
	void ReturnContextToPool(asIScriptContext *ctx);
	int ExecuteCall(asIScriptContext *ctx);

	struct SController
	{
		SController() : type(0), factoryFunc(0), onThinkMethod(0), onMessageMethod(0) {}
		std::string        module;
		asITypeInfo       *type;
		asIScriptFunction *factoryFunc;
		asIScriptFunction *onThinkMethod;
		asIScriptFunction *onMessageMethod;
	};

	SController *GetControllerScript(const std::string &type);

	asIScriptEngine  *engine;

	// Our pool of script contexts. This is used to avoid allocating
	// the context objects all the time. The context objects are quite
	// heavy weight and should be shared between function calls.
	std::vector<asIScriptContext *> contexts;

	// This is the cache of function ids etc that we use to avoid having
	// to search for the function ids everytime we need to call a function.
	// The search is quite time consuming and should only be done once.
	std::vector<SController *> controllers; 
};

extern CScriptMgr *scriptMgr;

#endif