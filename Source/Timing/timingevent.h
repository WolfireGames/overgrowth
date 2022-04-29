//-----------------------------------------------------------------------------
//           Name: timingevent.h
//      Developer: Wolfire Games LLC
//         Author: Micah J Best
//           Date: Thursday, April 1, 2016
//    Description: Lightweight timing tools
//        License: Read below
//-----------------------------------------------------------------------------
//
//   Copyright 2022 Wolfire Games LLC
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//-----------------------------------------------------------------------------
/*******************************************************************************************/
/**
 * "Slim Timing" is a system for taking high accuracy timings of various events in the system
 * and reporting them by various means.  It has too modes: fine and coarse grained.
 * Coarse grained is always compiled in and is basically for measuring fairly large engine
 * features such as the time it takes to update all the AI agents and feed the realtime
 * visualization.  Fine grained mode needs to be compiled in (-DSLIM_TIMING) and is
 * designed to take significantly finer grained timings and then output them in a nice
 * easy to crunch .csv file.  Both systems use the same commands with a different macro for
 * each timing mode.
 *
 * The system revolves around a set of discrete event categories.  Like most other timing
 * systems the code is marked up with a 'begin event' and 'end event' macro.  The list of
 * timing events is in the SlimTimingEvents enum below (there is also a corresponding array
 * of strings (EVENT_NAMES) for data output).  The total amount of time spent inside these
 * events is tabulated at the end of the frame.  The system deals with nested timing of the
 * same event type by disregarding all but the outermost, so there is no need to worry
 * about double counting with reentrant code.
 *
 * An example setup would look something like this:
 * 	STIMING_INIT( GetWritePath(CoreGameModID).c_str() );
 *
 * 	STIMING_ADDVISUALIZATION( STUpdate,     vec3( 0.7, 0.3, 0.3 ),
 *                            STDraw,       vec3( 0.3, 0.7, 0.3 ) );
 *
 * 	STIMING_ADDVISUALIZATION( STDrawSwap,   vec3( 0.3, 0.3, 0.7 ) );
 *
 *  STIMING_SETVISUALIZATIONSCALE(16,32);
 *
 *  STIMING_INITVISUALIZATION();
 *
 * This should be called sometime during program initialization (something like this should
 * already be in the main program).  The STIMING_INIT starts things off and specifies where
 * the output (if any) should be written.  The STIMING_ADDVISUALIZATION lines add events
 * to the realtime visualization (which is bound to the "show_timing" key which at the time
 * of this writing is given the default value of F2).  These lines above say to display the
 * STUpdata and STDraw event times in a stacked bar plot and the STDrawSwap in single bar
 * plot.   Each will be rendered with the RGB color values provided in the second parameter
 * The system keeps a certain number of values as history (number determined by the
 * HISTORY_FRAMES #define below) and displays all those values to show how the timing is changing
 * frame by frame.
 *
 * The STIMING_SETVISUALIZATIONSCALE macro sets up the visual scale of the timing.  Essentially
 * this is saying "put markers at 16ms and 32ms in the visualization" and scale the results
 * appropriately. (If you were timing something smaller, you may for example use 2,4).  The
 * defaults are 16 and 32 as these are the thresholds for 60 and 30 FPS respectively.
 *
 * Finally, STIMING_INITVISUALIZATION, allocated memory and gets ready (you can change what
 * is being visualized dynamically, but that's out of the scope of this text).
 *
 * To demonstrate actual timings, let's look at simplified version of the main game loop:
 *
 * while( !engine->quitting_ ) {
 *
 *     STIMING_STARTFRAME();
 *
 *     STIMING_START_COARSE( STUpdate );
 *     engine->Update();
 *     STIMING_END_COARSE( STUpdate );
 *
 *     if(!engine->quitting_){
 *         STIMING_START_COARSE( STDraw );
 *         engine->Draw();
 *         STIMING_END_COARSE( STDraw );
 *     }
 *     STIMING_START_COARSE( STDrawSwap );
 *     Graphics::Instance()->SwapToScreen();
 *     STIMING_END_COARSE( STDrawSwap );
 *
 *     STIMING_ENDFRAME();
 *
 * }
 *
 * The STIMING_STARTFRAME and STIMING_ENDFRAME should be self-explanatory (note that the
 * system assumes that the visualization is done *during* a frame I thought about timing this
 * to compensate, but I think that might be going a little too far).
 *
 * Similarly, the STIMING_START_COARSE and STIMING_END_COARSE should also be self explanatory.
 * This is where the difference between find and coarse mode come into play.  There is the
 * corresponding pair STIMING_START and STIMING_END which take exactly the same parameters,
 * but without being in fine grained mode (compiling with -DSLIM_TIMING) they will be compiled
 * to no-ops. The timings from the _COARSE markup will be used in *both* modes.
 *
 * UPDATE: due to strange crashing on Windows 7 machines, I'm going to
 *         disable this unless SLIM_TIMING is explicitly defined
 *
 *
 */
#pragma once

//These needs to be included even when slim timing is disabled, because they contain zeroed macros.
#include <Timing/gl_query_perf.h>
#include <Timing/intel_gl_perf.h>
#include <Timing/timestamp.h>

#ifdef SLIM_TIMING

#ifndef MEASURE_TIME_LEVEL
#define MEASURE_TIME_LEVEL 1
#endif

#include <Internal/integer.h>
#include <Wrappers/glm.h>
#include <Graphics/text.h>
#include <Graphics/vboringcontainer.h>

#if defined(PLATFORM_WINDOWS) && _MSC_VER >= 1600 
#include <intrin.h>
#endif

#include <string>
#include <vector>
#include <stdint.h>
#include <stdio.h>

// Total hack, but add new timing event classes here
// (and don't forgot to add their name as a string below)
enum SlimTimingEvents {
    STUpdate,
    STDraw,
    STDrawSwap,
    STLastEvent // Leave this last or Bad Things will happen
};

const std::string EVENT_NAMES[STLastEvent] = {
    "Update",
    "Draw",
    "DrawSwap"
};

#ifdef SLIM_TIMING
#define ST_MAX_EVENTS 16384
#else 
#define ST_MAX_EVENTS 256
#endif //SLIM_TIMING

// How man frames should we skip to avoid throwing our averages off with setup/warming
#define WARMUP_FRAMES 10
#define HISTORY_FRAMES 120

#define ST_MAX_THREADS 4

#define ST_EVENT_START_CODE 0
#define ST_EVENT_END_CODE 1

class EventRecorder {
    
    // Internal structure for timing events
    struct Event {
        uint64_t tsc;       // Cycle count at the time of this event
        uint16_t eventType; // Start, end, etc
        uint16_t event_id;  // Which event is being recorded?
        uint32_t thread_id; // For future use -- and to pad out the structure for better reading
    };
    
    // Holding the derived values
    struct FrameTotals {
        uint64_t eventTimes[STLastEvent];   // Total for each event
        uint64_t frameTimeCycles;           // Total cycles used in this frame
        uint32_t frameTimeMS;               // Elapsed frame time in ms
    };
    
    struct NormalizedFrameTotals {
        float eventTimes[ STLastEvent ];
    };
    
    int nextIndex;  // Where to write the next event?
    Event events[ ST_MAX_EVENTS ];  //  Storage for all events
    
    uint64_t frameStartCycle;   // When did we start this frame (cycles)
    uint32_t frameStartTick;    // When did we start this frame (ms)?
    
    FrameTotals timingHistory[HISTORY_FRAMES]; // Aggregate time for all events
    NormalizedFrameTotals normalizedFrames[HISTORY_FRAMES];
    
    bool writeFile;             // Are we writing to a file?
    std::string outputFileName; // What file are we writing to/if any?
    FILE * fp;                  // File handle for the above
    int frameCount;             // Which frame are we processing
    
    struct VisualizerEvent {
        uint16_t eventID;
        vec3 color;
        
        VisualizerEvent() {}
        
        VisualizerEvent( uint16_t _eventID, vec3 _color ) :
            eventID( _eventID ),
            color( _color )
        {}
    };
    
    bool showVisualization; // Should we be displaying the visualizationx
    
    GLfloat* visData;   // OpenGL vertex data for the visualization
    GLuint* visIndices; // OpenGL index data for the visualization
    
    VBORingContainer data_vbo;  //VBOs for the above
    VBORingContainer index_vbo;

    int visDataSize;    // Sizes of the above (in bytes)
    int visIndexSize;
    
    float visBaseMS;    // How many miliseconds does the 'main' area of the visualization represent
    float visHighMS;    // How many miliseconds does should the 'high' area of the visualization represent
    
    std::string baseMSLabel; // Label for the base level maker
    std::string highMSLabel; // Label for the high marker
    
    
    std::vector< std::vector<VisualizerEvent> > visualizerEvents; // What events are being visualized
    
    GLState gl_state;   // State for the visualization
    
    /*******************************************************************************************/
    /**
     * @brief  Clear all current records
     *
     */
    void resetData();
    
    /*******************************************************************************************/
    /**
     * @brief  Get the number of history frames available
     *
     */
    int getAvailableHistorySize();
    
    
    /*******************************************************************************************/
    /**
     * @brief  Internal — fill in the coordinates for a box
     *
     */
    void buildBoxAt( vec2 const& UL, vec2 const& LR, vec4 const& color, int boxNumber );
    
public:
    
    EventRecorder();
    
    void init( std::string filePath );
        
    
    inline void startEvent( int const& event_id ) {
        events[ nextIndex ].tsc = GetTimestamp();
        events[ nextIndex ].eventType = ST_EVENT_START_CODE;
        events[ nextIndex ].event_id = event_id;
        events[ nextIndex ].thread_id = 0;
        nextIndex++;
    }
    
    inline void endEvent( int const& event_id ) {
        events[ nextIndex ].tsc = GetTimestamp();
        events[ nextIndex ].eventType = ST_EVENT_END_CODE;
        events[ nextIndex ].event_id = event_id;
        events[ nextIndex ].thread_id = 0;
        nextIndex++;
    }
    
    void startFrame();
    
    void endFrame();
    
    void finalize();
    
    
    /*******************************************************************************************/
    /**
     * @brief  Determine the scale of the presentation by giving a ms value to the ‘main box’ and an optional hight marker
     *
     *
     * @param baseMS how many ms does the 'base' part of the visualization represent?
     * @param highMS how many ms does the 'high' part of the visualization represent? (optional)
     *
     */
    void setVisualizationScale( float baseMS, float highMS = -1.0 );
    
    /*******************************************************************************************/
    /**
     * @brief  Adds a new timing event to the visualziation (add multiple events to stack)
     *
     *
     * @param event1 ID of the event we want to visualization (required)
     * @param color1 color for the first event (required)
     * @param event2 ID of the event we want to visualization (optional)
     * @param color2 color for the first event (optional)
     * @param event3 ID of the event we want to visualization (optional)
     * @param color3 color for the first event (optional)
     * @param event4 ID of the event we want to visualization (optional)
     * @param color4 color for the first event (optional)
     * @param event5 ID of the event we want to visualization (optional)
     * @param color5 color for the first event (optional)
     *
     */
    void addNewVisualization( uint16_t event1, vec3 color1,
                              int32_t event2 = -1, vec3 color2 = vec3(0.0),
                              int32_t event3 = -1, vec3 color3 = vec3(0.0),
                              int32_t event4 = -1, vec3 color4 = vec3(0.0),
                              int32_t event5 = -1, vec3 color5 = vec3(0.0) );
    
    /*******************************************************************************************/
    /**
     * @brief  Allocate memory, etc for the visualization
     *
     */
    void initVisualization();

    /*******************************************************************************************/
    /**
     * @brief  Draw the various graphs requested for timing visualizing
     *
     * @param text_atlas_renderer Reference to a text atlas renderer for drawing text
     * @param font_renderer Reference to a font renderer for drawing text
     * @param text_atlas Text atlas to use for displaying text
     *
     */
    void drawVisualization( TextAtlasRenderer& text_atlas_renderer,
                            FontRenderer& font_renderer,
                            TextAtlas& text_atlas );
    
    /*******************************************************************************************/
    /**
     * @brief  Display (or not) the visualization
     *
     * @param show Should we show the visualization or not?
     *
     */
    void displayVisualization( bool show );
    
    /*******************************************************************************************/
    /**
     * @brief  Toggles displaying the visualization
     *
     */
    void toggleVisualization() {
        displayVisualization( !showVisualization );
    }
    
    /*******************************************************************************************/
    /**
     * @brief  Get rid of the current visualization parameters, you'll have to reset it up if you
     *         want it again
     *
     */
    void clearVisualization();
    

};

extern EventRecorder* slimTimingEvents;

#ifdef SLIM_TIMING


#define STIMING_INIT( FILEPATH ) \
slimTimingEvents->init( FILEPATH )

#else

// Don't write out a file if only doing coarse timing
#define STIMING_INIT( FILEPATH ) \
slimTimingEvents->init( "" )

#endif //SLIM_TIMING

#define STIMING_ADDVISUALIZATION(...)\
slimTimingEvents->addNewVisualization(__VA_ARGS__)

#define STIMING_SETVISUALIZATIONSCALE(... )\
slimTimingEvents->setVisualizationScale(__VA_ARGS__)

#define STIMING_INITVISUALIZATION() \
slimTimingEvents->initVisualization()

#define STIMING_STARTFRAME() \
slimTimingEvents->startFrame()

#define STIMING_START_COARSE( EVENT ) \
slimTimingEvents->startEvent( EVENT )

#define STIMING_END_COARSE( EVENT ) \
slimTimingEvents->endEvent( EVENT )

#ifdef SLIM_TIMING

#define STIMING_START( EVENT ) \
slimTimingEvents->startEvent( EVENT )

#define STIMING_END( EVENT ) \
slimTimingEvents->endEvent( EVENT )

#else

// Don't take the timing of events not marked 'coarse' unless asked
#define STIMING_START( EVENT )

#define STIMING_END( EVENT )

#endif //SLIM_TIMING

#define STIMING_ENDFRAME() \
slimTimingEvents->endFrame()

#define STIMING_FINALIZE() \
slimTimingEvents->finalize()

#else

// To prevent an error on Windows, just disable everything if not defined

#define STIMING_INIT( FILEPATH ) 

#define STIMING_ADDVISUALIZATION(...)

#define STIMING_SETVISUALIZATIONSCALE(... )

#define STIMING_INITVISUALIZATION()

#define STIMING_STARTFRAME()

#define STIMING_START_COARSE( EVENT )

#define STIMING_END_COARSE( EVENT )

#define STIMING_START( EVENT )

#define STIMING_END( EVENT )

#define STIMING_ENDFRAME()

#define STIMING_FINALIZE()

#endif //SLIM_TIMING

#define GL_PERF_INIT( ) \
INTEL_GL_PERF_INIT(); \
GL_TIMER_QUERY_INIT()

#define GL_PERF_START( ) \
INTEL_GL_PERF_START(); \
GL_TIMER_QUERY_START()

#define GL_PERF_END( ) \
INTEL_GL_PERF_END(); \
GL_TIMER_QUERY_END()

#define GL_SWAP() \
INTEL_GL_PERF_SWAP(); \
GL_TIMER_QUERY_SWAP()

#define GL_PERF_FINALIZE() \
INTEL_GL_PERF_FINALIZE(); \
GL_TIMER_QUERY_FINALIZE()
