#include <iostream>
#include <string>
#include <string.h> // strstr()
#include <assert.h>
#include <math.h>
#include <angelscript.h>
#include "../../../add_on/scriptstdstring/scriptstdstring.h"
#include "../../../add_on/scripthelper/scripthelper.h"

using namespace std;

// Function prototypes
void PrintHelp();
void ExecString(asIScriptEngine *engine, string &arg);
void AddVariable(asIScriptEngine *engine, string &arg);
void DeleteVariable(asIScriptEngine *engine, string &arg);
void AddFunction(asIScriptEngine *engine, string &arg);
void DeleteFunction(asIScriptEngine *engine, string &arg);
void ListVariables(asIScriptEngine *engine);
void ListFunctions(asIScriptEngine *engine);
void ConfigureEngine(asIScriptEngine *engine);
void print(const string &s);
void grab(int);
void grab(asUINT);
void grab(bool);
void grab(float);
void grab(double);
void grab(const string&);
void grab(void);

// Some global variables that the script can access
float          g_gravity;
asUINT         p_health;
asUINT         r_fov;
bool           r_shadow;
string         p_name = "player";

int main(int argc, char **argv)
{
	// Create the script engine
	asIScriptEngine *engine = asCreateScriptEngine();
	if( engine == 0 )
	{
		cout << "Failed to create script engine." << endl;
		return -1;
	}

	// Configure the script engine with all the functions,
	// and variables that the script should be able to use.
	ConfigureEngine(engine);

	// Print some useful information and start the input loop
	cout << "Sample console using AngelScript " << asGetLibraryVersion() << " to perform scripted tasks." << endl;
	cout << "Type 'help' for more information." << endl;

	// TODO: Allow multiple lines per command by ending each line with a backslash.

	for(;;)
	{
		string input;
		input.resize(256);
		string cmd, arg;

		cout << "> ";
		cin.getline(&input[0], 256);

		// Trim unused characters
		input.resize(strlen(input.c_str()));

		size_t pos;
		if( (pos = input.find(" ")) != string::npos )
		{
			cmd = input.substr(0, pos);
			arg = input.substr(pos+1);
		}
		else
		{
			cmd = input;
			arg = "";
		}

		// Interpret the command
		if( cmd == "exec" )
			ExecString(engine, arg);
		else if( cmd == "addfunc" )
			AddFunction(engine, arg);
		else if( cmd == "delfunc" )
			DeleteFunction(engine, arg);
		else if( cmd == "addvar" )
			AddVariable(engine, arg);
		else if( cmd == "delvar" )
			DeleteVariable(engine, arg);
		else if( cmd == "help" )
			PrintHelp();
		else if( cmd == "listfuncs" )
			ListFunctions(engine);
		else if( cmd == "listvars" )
			ListVariables(engine); 
		else if( cmd == "quit" )
			break;
		else
			cout << "Unknown command." << endl;
	}

	// Shut down the engine
	engine->ShutDownAndRelease();

	return 0;
}

void PrintHelp()
{
	cout << "Commands:" << endl;
	cout << " addfunc [decl] - adds a user function" << endl;
	cout << " addvar [decl]  - adds a user variable" << endl; 
	cout << " delfunc [name] - removes a user function" << endl;
	cout << " delvar [name]  - removes a user variable" << endl;
	cout << " exec [script]  - executes script statement and prints the result" << endl;
	cout << " help           - this command" << endl;
	cout << " listfuncs      - list functions" << endl;
	cout << " listvars       - list variables" << endl;
	cout << " quit           - end application" << endl;
}

void print(const string &s)
{
	cout << s << endl;
}

void MessageCallback(const asSMessageInfo *msg, void *param)
{
	const char *type = "ERR ";
	if( msg->type == asMSGTYPE_WARNING )
		type = "WARN";
	else if( msg->type == asMSGTYPE_INFORMATION )
		type = "INFO";

	cout << msg->section << " (" << msg->row << ", " << msg->col << ") : " << type << " : " << msg->message << endl;
}

void ConfigureEngine(asIScriptEngine *engine)
{
	int r;

	// Tell the engine to output any error messages to printf
	engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);

	// Register the script string type
	// Look at the implementation for this function for more information
	// on how to register a custom string type, and other object types.
	RegisterStdString(engine);

	// Register the global variables
	r = engine->RegisterGlobalProperty("float g_gravity", &g_gravity); assert( r >= 0 );
	r = engine->RegisterGlobalProperty("uint p_health", &p_health);    assert( r >= 0 );
	r = engine->RegisterGlobalProperty("uint r_fov", &r_fov);          assert( r >= 0 );
	r = engine->RegisterGlobalProperty("bool r_shadow", &r_shadow);    assert( r >= 0 );
	r = engine->RegisterGlobalProperty("string p_name", &p_name);      assert( r >= 0 );

	// Register some useful functions
	r = engine->RegisterGlobalFunction("float sin(float)", asFUNCTION(sinf), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("float cos(float)", asFUNCTION(cosf), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(print), asCALL_CDECL); assert( r >= 0 );

	// Register special function with overloads to catch any type.
	// This is used by the exec command to output the resulting value from the statement.
	r = engine->RegisterGlobalFunction("void _grab(bool)", asFUNCTIONPR(grab, (bool), void), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void _grab(int)", asFUNCTIONPR(grab, (int), void), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void _grab(uint)", asFUNCTIONPR(grab, (asUINT), void), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void _grab(float)", asFUNCTIONPR(grab, (float), void), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void _grab(double)", asFUNCTIONPR(grab, (double), void), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void _grab()", asFUNCTIONPR(grab, (void), void), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void _grab(const string &in)", asFUNCTIONPR(grab, (const string&), void), asCALL_CDECL); assert( r >= 0 );

	// Do not output anything else to printf
	engine->ClearMessageCallback();
}

void ExecString(asIScriptEngine *engine, string &arg)
{
	string script;

	// Wrap the expression in with a call to _grab, which allow us to print the resulting value
	script = "_grab(" + arg + ")";

	// TODO: Add a time out to the script, so that never ending scripts doesn't freeze the application

	int r = ExecuteString(engine, script.c_str(), engine->GetModule("console"));
	if( r < 0 )
		cout << "Invalid script statement. " << endl;
	else if( r == asEXECUTION_EXCEPTION )
		cout << "A script exception was raised." << endl;
}

void AddVariable(asIScriptEngine *engine, string &arg)
{
	asIScriptModule *mod = engine->GetModule("console", asGM_CREATE_IF_NOT_EXISTS);

	// Add a semi-colon to end the statement (if not already there)
	if( arg.length() > 0 && arg[arg.length()-1] != ';' )
		arg += ";";

	int r = mod->CompileGlobalVar("addvar", arg.c_str(), 0);
	if( r < 0 )
	{
		// TODO: Add better description of error (invalid declaration, name conflict, etc)
		cout << "Failed to add variable. " << endl;
	}
	else
		cout << "Variable added. " << endl;
}

void DeleteVariable(asIScriptEngine *engine, string &arg)
{
	asIScriptModule *mod = engine->GetModule("console");
	if( mod == 0 || mod->GetGlobalVarCount() == 0 ) 
	{
		cout << "No variables have been added. " << endl;
		return;
	}

	// trim the string to find the variable name
	size_t p1 = arg.find_first_not_of(" \n\r\t");
	if( p1 != string::npos )
		arg = arg.substr(p1, -1);
	size_t p2 = arg.find_last_not_of(" \n\r\t");
	if( p2 != string::npos )
		arg = arg.substr(0, p2+1);

	int index = mod->GetGlobalVarIndexByName(arg.c_str());
	if( index >= 0 )
	{
		mod->RemoveGlobalVar(index);
		cout << "Variable removed. " << endl;
	}
	else
		cout << "No such variable. " << endl;
}

void AddFunction(asIScriptEngine *engine, string &arg)
{
	asIScriptModule *mod = engine->GetModule("console", asGM_CREATE_IF_NOT_EXISTS);

	asIScriptFunction *func = 0;
	int r = mod->CompileFunction("addfunc", arg.c_str(), 0, asCOMP_ADD_TO_MODULE, &func);
	if( r < 0 )
	{
		// TODO: Add better description of error (invalid declaration, name conflict, etc)
		cout << "Failed to add function. " << endl;
	}
	else
	{
		// The script engine supports function overloads, but to simplify the 
		// console we'll disallow multiple functions with the same name.
		// We know the function was added, so if GetFunctionByName() fails it is
		// because there already was another function with the same name.
		if( mod->GetFunctionByName(func->GetName()) == 0 )
		{
			mod->RemoveFunction(func);
			cout << "Another function with that name already exists." << endl;
		}
		else
			cout << "Function added. " << endl;
	}

	// We must release the function object
	if( func )
		func->Release();
}

void DeleteFunction(asIScriptEngine *engine, string &arg)
{
	asIScriptModule *mod = engine->GetModule("console");
	if( mod == 0 || mod->GetFunctionCount() == 0 ) 
	{
		cout << "No functions have been added. " << endl;
		return;
	}

	// trim the string to find the variable name
	size_t p1 = arg.find_first_not_of(" \n\r\t");
	if( p1 != string::npos )
		arg = arg.substr(p1, -1);
	size_t p2 = arg.find_last_not_of(" \n\r\t");
	if( p2 != string::npos )
		arg = arg.substr(0, p2+1);

	asIScriptFunction *func = mod->GetFunctionByName(arg.c_str());
	if( func )
	{
		mod->RemoveFunction(func);
		cout << "Function removed. " << endl;
	}
	else
		cout << "No such function. " << endl;

	// Since functions can be recursive, we'll call the garbage
	// collector to make sure the object is really freed
	engine->GarbageCollect();
}

void ListVariables(asIScriptEngine *engine)
{
	asUINT n;

	// List the application registered variables
	cout << "Application variables:" << endl;
	for( n = 0; n < (asUINT)engine->GetGlobalPropertyCount(); n++ )
	{
		const char *name;
		int typeId;
		bool isConst;
		engine->GetGlobalPropertyByIndex(n, &name, 0, &typeId, &isConst);
		string decl = isConst ? " const " : " ";
		decl += engine->GetTypeDeclaration(typeId);
		decl += " ";
		decl += name;
		cout << decl << endl;
	}

	// List the user variables in the module
	asIScriptModule *mod = engine->GetModule("console");
	if( mod )
	{
		cout << endl;
		cout << "User variables:" << endl;
		for( n = 0; n < (asUINT)mod->GetGlobalVarCount(); n++ )
		{
			cout << " " << mod->GetGlobalVarDeclaration(n) << endl;
		}
	}
}

void ListFunctions(asIScriptEngine *engine)
{
	asUINT n;
	
	// List the application registered functions
	cout << "Application functions:" << endl;
	for( n = 0; n < (asUINT)engine->GetGlobalFunctionCount(); n++ )
	{
		asIScriptFunction *func = engine->GetGlobalFunctionByIndex(n);

		// Skip the functions that start with _ as these are not meant to be called explicitly by the user
		if( func->GetName()[0] != '_' )
			cout << " " << func->GetDeclaration() << endl;
	}

	// List the user functions in the module
	asIScriptModule *mod = engine->GetModule("console");
	if( mod )
	{
		cout << endl;
		cout << "User functions:" << endl;
		for( n = 0; n < (asUINT)mod->GetFunctionCount(); n++ )
		{
			asIScriptFunction *func = mod->GetFunctionByIndex(n);
			cout << " " << func->GetDeclaration() << endl;
		}
	}
}

void grab(int v)
{
	cout << v << endl;
}

void grab(asUINT v)
{
	cout << v << endl;
}

void grab(bool v)
{
	cout << boolalpha << v << endl;
}

void grab(float v)
{
	cout << v << endl;
}

void grab(double v)
{
	cout << v << endl;
}

void grab(const string &v)
{
	cout << v << endl;
}

void grab()
{
	// There is no value
}
