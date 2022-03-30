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
#include <angelscript.h>
#include "../../../add_on/scriptstdstring/scriptstdstring.h"

using namespace std;

#ifdef _LINUX_

#define UINT unsigned int 
typedef unsigned int DWORD;
int ch;
// Linux doesn't have timeGetTime(), this essentially does the same
// thing, except this is milliseconds since Epoch (Jan 1st 1970) instead
// of system start. It will work the same though...
DWORD timeGetTime()
{
	timeval time;
	gettimeofday(&time, NULL);
	return time.tv_sec*1000 + time.tv_usec/1000;
}

// Linux does have a getch() function in the curses library, but it doesn't
// work like it does on DOS. So this does the same thing, with out the need
// of the curses library.
int getch() 
{
	return ch;
}
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
void LineCallback(asIScriptContext *ctx, DWORD *timeOut);
void PrintString_Generic(asIScriptGeneric *gen);
void timeGetTime_Generic(asIScriptGeneric *gen);

void MessageCallback(const asSMessageInfo *msg, void *param)
{
	const char *type = "ERR ";
	if( msg->type == asMSGTYPE_WARNING ) 
		type = "WARN";
	else if( msg->type == asMSGTYPE_INFORMATION ) 
		type = "INFO";

	printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
}

int main(int argc, char **argv)
{
	int r;

	// Create the script engine
	asIScriptEngine *engine = asCreateScriptEngine();
	if( engine == 0 )
	{
		cout << "Failed to create script engine." << endl;
		return -1;
	}

	// The script compiler will send any compiler messages to the callback
	engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);

	// Configure the script engine with all the functions, 
	// and variables that the script should be able to use.
	ConfigureEngine(engine);
	
	// Compile the script code
	r = CompileScript(engine);
	if( r < 0 ) return -1;

	// Create two contexts, one will run the script thread 
	// that executes the main function, the other will other
	// be used to execute the event functions as needed.
	// Note that the global variables declared in the script
	// are shared between the contexts, so they are able to
	// communicate with each other this way.
	asIScriptContext *mainCtx = engine->CreateContext();
	if( mainCtx == 0 ) 
	{
		cout << "Failed to create the context." << endl;
		return -1;
	}

	asIScriptContext *eventCtx = engine->CreateContext();
	if( eventCtx == 0 )
	{
		cout << "Failed to create the context." << endl;
		return -1;
	}

	// Prepare the script context with the function we wish to execute
	r = mainCtx->Prepare(engine->GetModule(0)->GetFunctionByDecl("void main()"));
	if( r < 0 ) 
	{
		cout << "Failed to prepare the context." << endl;
		return -1;
	}

	// Get the function IDs for the event functions already
	asIScriptFunction *onKeyPressFunc = engine->GetModule(0)->GetFunctionByDecl("void OnKeyPress()");
	asIScriptFunction *onQuitFunc = engine->GetModule(0)->GetFunctionByDecl("void OnQuit()");

	// Set the line callback so that we can suspend the script execution
	// after a certain time. Before executing the script the timeOut variable
	// will be set to the time when the script must stop executing. This
	// way we will be able to do more than one thing, almost at the same time.
	DWORD timeOut;
	r = mainCtx->SetLineCallback(asFUNCTION(LineCallback), &timeOut, asCALL_CDECL);
	if( r < 0 )
	{
		cout << "Failed to set the line callback function." << endl;
		return -1;
	}

	// Print some useful information and start the input loop
	cout << "Sample event driven script execution using AngelScript " << asGetLibraryVersion() << "." << endl;
	cout << "The main script continuosly prints a short string." << endl;
	cout << "Press any key to fire an event that will print another string." << endl;
	cout << "Press ESC to terminate the application." << endl << endl;

	DWORD time = timeGetTime();
	DWORD quitTimeOut = 0;

	for(;;)
	{
		// Check if any key was pressed
		if( kbhit() )
		{
			int key = getch();
			if( key != 27 ) 
			{
				// Fire an event by calling the script function.
				eventCtx->Prepare(onKeyPressFunc);
				eventCtx->Execute();

				// Note, I'm being a little lazy here, since we don't 
				// verify the return codes. Neither do I add any safeguard 
				// against never ending scripts, which if the script is 
				// badly written could cause the program to hang, I do this 
				// for the main script though.
			}
			else
			{
				// Fire the quit event that will tell the main script to finish
				eventCtx->Prepare(onQuitFunc);
				eventCtx->Execute();
				
				// Let the script run for at most 1sec more, to give it time 
				// to quit graciously. If it does not finish in time the 
				// script will be aborted.
				quitTimeOut = timeGetTime() + 1000;
			}
		}

		// Allow the long running script to execute for 10ms
		timeOut = timeGetTime() + 10;
		r = mainCtx->Execute();
		if( r != asEXECUTION_SUSPENDED )
		{
			if( quitTimeOut == 0 )
				cout << "The script execution finished early." << endl;
			break;
		}
		
		if( quitTimeOut && quitTimeOut < timeGetTime() )
		{
			// Abort the long running script.
			// AngelScript makes sure everything is correctly freed.
			mainCtx->Abort();
			break;
		}
	}

	// We must release the contexts when no longer using them
	mainCtx->Release();
	eventCtx->Release();

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

	// Register the functions that the scripts will be allowed to use
	if( !strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		r = engine->RegisterGlobalFunction("void Print(string &in)", asFUNCTION(PrintString), asCALL_CDECL); assert( r >= 0 );
		r = engine->RegisterGlobalFunction("uint GetSystemTime()", asFUNCTION(timeGetTime), asCALL_STDCALL); assert( r >= 0 );
	}
	else
	{
		r = engine->RegisterGlobalFunction("void Print(string &in)", asFUNCTION(PrintString_Generic), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterGlobalFunction("uint GetSystemTime()", asFUNCTION(timeGetTime_Generic), asCALL_GENERIC); assert( r >= 0 );
	}
}

int CompileScript(asIScriptEngine *engine)
{
	int r;

	// This script prints a char approximately 10 times per second
	const char *scriptMain = 
	"string char = \"-\";               "
	"bool doQuit = false;               "
	"void main()                        "
	"{                                  "
	"  uint time = GetSystemTime();     "
	"  while( !doQuit )                 "
	"  {                                "
	"    uint t = GetSystemTime();      "
	"    if( t - time > 100 )           "
	"    {                              "
	"      time = t;                    "
	"      Print(char);                 "
	"    }                              "
	"  }                                "
	"}                                  ";
	
	const char *scriptEvents =
	"void OnKeyPress()                  "
	"{                                  "
	"  Print(\"A key was pressed\\n\"); "
	"  if( char == \"-\" )              "
	"    char = \"+\";                  "
	"  else                             "
	"    char = \"-\";                  "
	"}                                  "
	"void OnQuit()                      "
	"{                                  "
	"  doQuit = true;                   "
	"}                                  ";

	// Add the script sections that will be compiled into executable code
	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	r = mod->AddScriptSection("scriptMain", scriptMain, strlen(scriptMain));
	if( r < 0 ) 
	{
		cout << "AddScriptSection() failed" << endl;
		return -1;
	}
	
	r = mod->AddScriptSection("scriptEvents", scriptEvents, strlen(scriptEvents));
	if( r < 0 )
	{
		cout << "AddScriptSection() failed" << endl;
		return -1;
	}

	// Compile the script
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

void PrintString_Generic(asIScriptGeneric *gen)
{
	string *str = (string*)gen->GetArgAddress(0);
	PrintString(*str);
}

void timeGetTime_Generic(asIScriptGeneric *gen)
{
	gen->SetReturnDWord(timeGetTime());
}

void LineCallback(asIScriptContext *ctx, DWORD *timeOut)
{
	// If the time out is reached we suspend the script
	if( *timeOut < timeGetTime() )
		ctx->Suspend();
}

