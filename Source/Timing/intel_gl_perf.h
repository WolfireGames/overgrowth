//-----------------------------------------------------------------------------
//           Name: intel_gl_perf.h
//      Developer: Wolfire Games LLC
//    Description:
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
#pragma once

#ifdef INTEL_TIMING
#include <Internal/integer.h>
#include <Utility/disallow_copy_and_assign.h>

#include <opengl.h>

#include <vector>
#include <map>
#include <queue>
#include <string>
#include <stack>

//const char IntelGLPerf_basic_counters_name[] = "Intel_HD_MD_Render_Basic_Hardware_Counters";

//const char* IntelGLPerf_basic_counters[] = {"GpuTime", NULL};

const int kIntelGLPerf_TypeId_Standard = 1; 

struct IntelGLPerfQuery
{
	std::string name;
	int type_id;
	GLuint id;
	GLuint size;
};

struct IntelGLPerfQueryCounter
{
	GLuint query_id;
	std::string name;
	GLuint id;
	GLuint offset;
	GLenum data_type;
	GLuint size;
};

class IntelGLPerfRequest {
public:
	IntelGLPerfRequest( std::string query_name, int type_id, std::vector<std::string> query_counter_names );

	std::string query_name;
	int type_id;
	std::vector<std::string> query_counter_names;
};

struct PerfInstance {
	GLuint instance_id;
	GLuint query_id;
	int group;
	const char* file;
	int line;
};

class IntelGLPerf {
	bool perf_available;
	char* data_dest;

	std::vector<IntelGLPerfRequest> perf_requests;

	std::vector<IntelGLPerfQuery> queries;
	std::vector<IntelGLPerfQueryCounter> query_counters;

	std::map<std::string,GLuint> active_query_handles;

	std::vector<PerfInstance> perf_query_instances;

	GLuint max_query_size;

	std::stack<GLuint> query_handle_stack;

	std::ofstream csv_output;
public:
	IntelGLPerf();
	~IntelGLPerf();

	void Init();
	void Finalize();

	void PerfGPUBegin(int type, int group, const char* file, const int line);
	void PerfGPUEnd( );

	void PostFrameSwap();
};

extern IntelGLPerf* intelGLTiming;


#define INTEL_GL_PERF_INIT( ) \
    intelGLTiming->Init()

#define INTEL_GL_PERF_START( ) \
	if( intelGLTiming ) intelGLTiming->PerfGPUBegin( kIntelGLPerf_TypeId_Standard, 0, __FILE__, __LINE__ )

#define INTEL_GL_PERF_END( ) \
	if( intelGLTiming ) intelGLTiming->PerfGPUEnd()

#define INTEL_GL_PERF_SWAP() \
	intelGLTiming->PostFrameSwap()

#define INTEL_GL_PERF_FINALIZE() \
    intelGLTiming->Finalize()

#else

#define INTEL_GL_PERF_INIT()

#define INTEL_GL_PERF_START( )

#define INTEL_GL_PERF_END( )

#define INTEL_GL_PERF_SWAP()

#define INTEL_GL_PERF_FINALIZE()

#endif

