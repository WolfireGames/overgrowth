// This sample shows how to use co-routines with AngelScript. Co-routines
// are threads that work together. When one yields the next one takes over.
// This way they are always synchronized, which makes them much easier to
// use than threads that run in parallel.

#include <iostream>  // cout
#include <assert.h>  // assert()
#include <string.h>  // strstr()
#ifdef _LINUX_
	#include <sys/time.h>
	#include <stdio.h>
	#include <termios.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <string.h>
#else
	#include <conio.h>   // kbhit(), getch()
	#include <windows.h> // timeGetTime()
	#include <crtdbg.h>  // debugging routines
#endif
#include <list>
#include <angelscript.h>
#include "../../../add_on/scriptstdstring/scriptstdstring.h"
#include "../../../add_on/scriptarray/scriptarray.h"
#include "../../../add_on/scriptdictionary/scriptdictionary.h"
#include "../../../add_on/contextmgr/contextmgr.h"

using namespace std;

#ifdef _LINUX_

#define UINT unsigned int 
typedef unsigned int DWORD;

#define Sleep usleep
// kbhit() for linux
int kbhit() 
{
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);

	if(ch != EOF) 
	{
		ungetc(ch, stdin);
		return 1;
	}

	return 0;
}

#endif

// Function prototypes
void ConfigureEngine(asIScriptEngine *engine);
int  CompileScript(asIScriptEngine *engine);
void PrintString(string &str);



void MessageCallback(const asSMessageInfo *msg, void *param)
{
	const char *type = "ERR ";
	if( msg->type == asMSGTYPE_WARNING ) 
		type = "WARN";
	else if( msg->type == asMSGTYPE_INFORMATION ) 
		type = "INFO";

	printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
}

CContextMgr contextMgr;
asIScriptEngine *engine = 0;
int main(int argc, char **argv)
{
	// Perform memory leak validation in debug mode
	#if defined(_MSC_VER)
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF);
	_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
	#endif

	int r;

	// Create the script engine
	engine = asCreateScriptEngine();
	if( engine == 0 )
	{
		cout << "Failed to create script engine." << endl;
		return -1;
	}

	// The script compiler will send any compiler messages to the callback function
	engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);

	// Configure the script engine with all the functions, 
	// and variables that the script should be able to use.
	ConfigureEngine(engine);

	// Compile the script code
	r = CompileScript(engine);
	if( r < 0 ) return -1;

	contextMgr.AddContext(engine, engine->GetModule("script")->GetFunctionByDecl("void main()"));
	
	// Print some useful information and start the input loop
	cout << "This sample shows how to use co-routines with AngelScript. Co-routines" << endl; 
	cout << "are threads that work together. When one yields the next one takes over." << endl;
	cout << "This way they are always synchronized, which makes them much easier to" << endl;
	cout << "use than threads that run in parallel." << endl;
	cout << "Press any key to abort execution." << endl;

	for(;;)
	{
		// Check if any key was pressed
		if( kbhit() )
		{
			contextMgr.AbortAll();
			break;
		}

		// Allow the contextManager to determine which script to execute next
		contextMgr.ExecuteScripts();

		// Slow it down a little so that we can see what is happening
		Sleep(100);
	}

	// Shut down the engine
	engine->ShutDownAndRelease();

	return 0;
}

void ConfigureEngine(asIScriptEngine *engine)
{
	int r;

	// Register the script string type
	// Look at the implementation for this function for more information  
	// on how to register a custom string type, and other object types.
	RegisterStdString(engine);

	// Register the script array type
	RegisterScriptArray(engine, false);

	// Register the script dictionary type
	// This type will allow the script to pass a dictionary of arguments to the function
	// thus making the CreateCoRoutine much more flexible in how necessary values are 
	// passed to the new function. The implementation is in "/add_on/scriptdictionary/scriptdictionary.cpp"
	RegisterScriptDictionary(engine);

	// Register the functions that the scripts will be allowed to use
	r = engine->RegisterGlobalFunction("void Print(string &in)", asFUNCTION(PrintString), asCALL_CDECL); assert( r >= 0 );

	// Add the support for co-routines
	contextMgr.RegisterCoRoutineSupport(engine);
}

int CompileScript(asIScriptEngine *engine)
{
	int r;

	const char *script = 
	"void main()                                \n"
	"{                                          \n"
    "  for(;;)                                  \n"
	"  {                                        \n"
	"    int count = 10;                        \n"
	"    createCoRoutine(thread2,               \n"
	"      dictionary = {{'count', 3},          \n"
	"                    {'str', ' B'}});       \n"
	"    while( count-- > 0 )                   \n"
	"    {                                      \n"
	"      Print('A :' + count + '\\n');        \n"
	"      yield();                             \n"
	"    }                                      \n"
	"  }                                        \n"
	"}                                          \n"
	"void thread2(dictionary @args)             \n"
	"{                                          \n"
	"  int count = int(args['count']);          \n"
	"  string str = string(args['str']);        \n"
	"  while( count-- > 0 )                     \n"
	"  {                                        \n"
	"    Print(str + ':' + count + '\\n');      \n"
	"    yield();                               \n"
	"  }                                        \n"
	"}                                          \n";
	
	// Build the two script into separate modules. This will make them have
	// separate namespaces, which allows them to use the same name for functions
	// and global variables.
	asIScriptModule *mod = engine->GetModule("script", asGM_ALWAYS_CREATE);
	r = mod->AddScriptSection("script", script, strlen(script));
	if( r < 0 ) 
	{
		cout << "AddScriptSection() failed" << endl;
		return -1;
	}
	
	r = mod->Build();
	if( r < 0 )
	{
		cout << "Build() failed" << endl;
		return -1;
	}

	return 0;
}

void PrintString(string &str)
{
	cout << str;
}


