#Rationale

The old logging utilities were spread out, logging to file was separate to console logging, little information is conveyed to the console and there was no support 
for logging levels, meaning that either the output is overly verbose per default or the developer needs to persistently remove possibly useful logging output.

To fix this we implement a multiplexed, log-level based, section based logging utility.

#Basic Design

#Output Format

Following is an excerpt of the logging utility when running the game. These strings are packed with a lot of information.

~~~
[i][__]:filehandler.cpp:  52: Preserving 6085209 of data from previous log file, starting at pos 0
[i][__]:       main.cpp:  66: Starting program. Version a209-695-gf1d72af 2015-10-19 08:10:31 UTC
[i][al]:    alAudio.cpp: 179: Available sound devices: OpenAL Soft.
[i][al]:    alAudio.cpp: 180: Opening default audio device: OpenAL Soft.
[i][__]:      input.cpp:  32: Current controllers:
[i][__]: modloading.cpp: 400: Added mod: /home/max/dev/c++/Overgrowth/BuildDebug/Data/Mods/example_mod
[i][__]:   graphics.cpp: 684: There are 2 available video drivers.
[i][__]:   graphics.cpp: 685: Video drivers are following:
[i][__]:   graphics.cpp: 689: Video driver 0:x11
[i][__]:   graphics.cpp: 689: Video driver 1:dummy
[i][__]:   graphics.cpp: 696: Initialized video driver: x11
[i][__]:   graphics.cpp: 752: RGBA bits, depth: 0 0 0 0 0
[i][__]:   graphics.cpp: 759: Anti-aliasing samples: 0
[i][__]:   graphics.cpp: 977: 192 texture units supported.
[i][__]:   graphics.cpp: 993: GL_MAX_VERTEX_UNIFORM_COMPONENTS: 4096
[i][__]:     engine.cpp:2941: Showing main menu...
[i][__]:       view.cpp: 535: Finished loading mainmenu/menu.html in 146ms.
~~~

The first indicator [i] says that the lines are all coming from the log level Info. The second cell is the prefix, usually reserved for different kind of systems. In this output most rows are coming from the default unnamed prefix and two are coming from [al] meaning the OpenAL audio subsystem. The third cell indicates which file the logging is coming from and the fourth what line in that file. The final cell is the string given to the logging utility.

#Basic Usage

To log the header <Logging/logdata.h> is required. By default the header defines six shorthand macros for simplified logging, they the following:

1. LOGF -- Fatal
2. LOGE -- Error
3. LOGW -- Warning
4. LOGI -- Info
5. LOGD -- Debug
6. LOGS -- Spam

To log something it's possible to either use the c++ streams syntax.

    LOGE << "Something went very wrong with the object " << obj << std::endl;

Or a more classic printf style output.

    LOGE.Format( "Something went very wrong witht the object %s\n", obj.toString().c_str() );

The c++ style i recommended for most outputs. There are some cases where the printf format is easier to read and construct for strict data, but generally the streams version is safer to use.

#Advanced Usage

It's possible to define your own label on your debugging output, this is useful for dividing and filtering the output to specific systems when doing spam level och debug level output. Following is an example of <Scripting/scriptlogging.h> redefining the label "as" for all logging output in the systems including this header.

~~~{ .cpp }
#pragma once

#include "Logging/logdata.h"

#undef LOGF 
#undef LOGE 
#undef LOGW 
#undef LOGI 
#undef LOGD 

#define LOGF LogSystem::LogData( LogSystem::fatal,	    "as",__FILE__,__LINE__ )
#define LOGE LogSystem::LogData( LogSystem::error,	    "as",__FILE__,__LINE__ )
#define LOGW LogSystem::LogData( LogSystem::warning,	    "as",__FILE__,__LINE__ )
#define LOGI LogSystem::LogData( LogSystem::info,	    "as",__FILE__,__LINE__ )
#define LOGD LogSystem::LogData( LogSystem::debug,	    "as",__FILE__,__LINE__ )
~~~