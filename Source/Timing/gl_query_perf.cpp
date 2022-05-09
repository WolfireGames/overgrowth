//-----------------------------------------------------------------------------
//           Name: gl_query_perf.cpp
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
#include "gl_query_perf.h"

#ifdef TIMER_QUERY_TIMING

#include <Logging/logdata.h>
#include <Graphics/graphics.h>
#include <Utility/assert.h>
#include <Internal/filesystem.h>

#include <opengl.h>
#include <string>

extern GLTimerQueryPerf* glTimingQuery;

void GLTimerQueryPerf::Init() {
    if (GLAD_GL_ARB_timer_query) {
        perf_available = true;

        query_ids_used = 0;
        query_count = 0;
        frame_counter = 0;

        memset(queries, 0x0, sizeof(PerfQuery) * MAX_QUERY_COUNT);

        glGenQueries(MAX_QUERY_COUNT, query_ids);
        CHECK_GL_ERROR();

        std::string path = std::string(GetWritePath(CoreGameModID).c_str()) + "/gl_query_perf.perf.dat";
        my_ofstream_open(csv_output, path.c_str());

        begun_perf = false;
    }
}

void GLTimerQueryPerf::Finalize() {
    if (perf_available) {
        glDeleteQueries(MAX_QUERY_COUNT, query_ids);
        CHECK_GL_ERROR();
        csv_output.close();

        std::string path = std::string(GetWritePath(CoreGameModID).c_str()) + "/gl_query_perf_filenames.txt";
        LOGI << "Saved perf output names to " << path << std::endl;
        std::ofstream f;
        my_ofstream_open(f, path);

        std::set<const char*>::iterator nm_it;

        for (nm_it = file_names.begin();
             nm_it != file_names.end();
             nm_it++) {
            f << (uint64_t)(*nm_it) << "," << *nm_it << std::endl;
        }
        f.close();
    }
}

void GLTimerQueryPerf::PerfGPUBegin(const char* file, const int line) {
    if (perf_available) {
        if (queries[query_count].query_id == 0) {
            GLuint query_id = query_ids[query_ids_used];
            LOG_ASSERT(begun_perf == false);
            glBeginQuery(GL_TIME_ELAPSED, query_id);
            CHECK_GL_ERROR();

            queries[query_count].query_id = query_id;
            queries[query_count].line = line;
            queries[query_count].file = file;
            queries[query_count].frame = frame_counter;

            query_ids_used++;
            query_count++;
            begun_perf = true;

            if (query_ids_used >= MAX_QUERY_COUNT) {
                query_ids_used = 0;
                query_count = 0;
            }
        } else {
            LOGW << "Ran out of queries" << std::endl;
        }
    }
}

void GLTimerQueryPerf::PerfGPUEnd() {
    if (perf_available) {
        if (begun_perf) {
            glEndQuery(GL_TIME_ELAPSED);
            CHECK_GL_ERROR();
            begun_perf = false;
        }
    }
}

void GLTimerQueryPerf::PostFrameSwap() {
    if (perf_available) {
        GLint available = 0;

        /*
        while (!available) {
            glGetQueryObjectiv(query_ids[query_ids_used-1], GL_QUERY_RESULT_AVAILABLE, &available);
            CHECK_GL_ERROR();
        }
        */
        static int i = 0;
        do {
            PerfQuery& query = queries[i];

            if (query.query_id != 0) {
                glGetQueryObjectiv(query.query_id, GL_QUERY_RESULT_AVAILABLE, &available);
            } else {
                available = false;
            }

            if (available) {
                uint64_t result;
                glGetQueryObjectui64v(query.query_id, GL_QUERY_RESULT, &result);
                CHECK_GL_ERROR();

                // LOGI << "Query " << i << " " << query.file << ":" << query.line << " " << result << std::endl;

                file_names.insert(query.file);
                csv_output << query.frame << "," << 0 << "," << (uint64_t)query.file << "," << query.line << "," << result << std::endl;

                query.query_id = 0;

                i++;
                if (i >= MAX_QUERY_COUNT) {
                    i = 0;
                }
            }
        } while (available);
        frame_counter++;
    }
}

GLTimerQueryPerf GlTimingQuery;
GLTimerQueryPerf* glTimingQuery = &GlTimingQuery;

#endif
