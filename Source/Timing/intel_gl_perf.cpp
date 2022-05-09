//-----------------------------------------------------------------------------
//           Name: intel_gl_perf.cpp
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
#include "intel_gl_perf.h"
#ifdef INTEL_TIMING

#include <Internal/integer.h>
#include <Internal/filesystem.h>
#include <Internal/snprintf.h>

#include <Memory/allocation.h>
#include <Logging/logdata.h>
#include <Utility/assert.h>

#include <opengl.h>

#include <cstdlib>

IntelGLPerfRequest::IntelGLPerfRequest(std::string query_name, int type_id, std::vector<std::string> query_counter_names) : query_name(query_name),
                                                                                                                            type_id(type_id),
                                                                                                                            query_counter_names(query_counter_names) {
}

IntelGLPerf::IntelGLPerf() : max_query_size(0),
                             data_dest(NULL) {
    std::vector<std::string> query_counter_names;
    query_counter_names.push_back("GpuTime");

    perf_requests.push_back(
        IntelGLPerfRequest(
            "Intel_HD_MD_Render_Basic_Hardware_Counters",
            kIntelGLPerf_TypeId_Standard,
            query_counter_names));
}

IntelGLPerf::~IntelGLPerf() {
    if (data_dest)
        OG_FREE(data_dest);

    if (csv_output.is_open()) {
        csv_output.close();
    }

    std::map<std::string, GLuint>::iterator query_it = active_query_handles.begin();

    for (; query_it != active_query_handles.end(); query_it++) {
        glDeletePerfQueryINTEL(query_it->second);
    }
}

void IntelGLPerf::Init() {
    LOGI << "Initializing IntelGLPerf" << std::endl;

    // Check if this plugin is available.
    if (GLAD_GL_INTEL_performance_query) {
        perf_available = true;

        GLuint query_id;
        glGetFirstPerfQueryIdINTEL(&query_id);
        while (query_id != 0) {
            const GLuint query_name_len = 64;
            GLchar query_name[query_name_len];
            GLuint data_size;
            GLuint no_counters;
            GLuint no_instances;
            GLuint caps_mask;

            glGetPerfQueryInfoINTEL(query_id,
                                    query_name_len, query_name,
                                    &data_size, &no_counters,
                                    &no_instances, &caps_mask);

            for (unsigned i = 0; i < perf_requests.size(); i++) {
                IntelGLPerfRequest& pr = perf_requests[i];
                if (pr.query_name == std::string(query_name)) {
                    IntelGLPerfQuery iglpq;

                    iglpq.id = query_id;
                    iglpq.name = std::string(query_name);
                    iglpq.size = data_size;
                    iglpq.type_id = pr.type_id;
                    queries.push_back(iglpq);

                    if (data_size > max_query_size)
                        max_query_size = data_size;
                }
            }

            data_dest = static_cast<char*>(OG_MALLOC(max_query_size));

            LOGI << "Have query id: " << query_id << std::endl
                 << "name:" << query_name << std::endl
                 << "data_size:" << data_size << std::endl
                 << "no_counters: " << no_counters << std::endl
                 << "no_instances: " << no_instances << std::endl
                 << "caps_mask: " << caps_mask << std::endl
                 << std::endl;

            for (unsigned counter_id = 1; counter_id <= no_counters; counter_id++) {
                const GLuint counter_name_len = 32;
                GLchar counter_name[counter_name_len];
                const GLuint counter_desc_len = 256;
                GLchar counter_desc[counter_desc_len];
                GLuint counter_offset;
                GLuint counter_data_size;
                GLuint counter_type_enum;
                GLuint counter_data_type_enum;
                uint64_t raw_counter_max_value;

                glGetPerfCounterInfoINTEL(
                    query_id,
                    counter_id,
                    counter_name_len,
                    counter_name,
                    counter_desc_len,
                    counter_desc,
                    &counter_offset,
                    &counter_data_size,
                    &counter_type_enum,
                    &counter_data_type_enum,
                    &raw_counter_max_value);

                for (unsigned i = 0; i < perf_requests.size(); i++) {
                    IntelGLPerfRequest& pr = perf_requests[i];
                    if (pr.query_name == std::string(query_name)) {
                        for (unsigned k = 0; k < pr.query_counter_names.size(); k++) {
                            if (pr.query_counter_names[k] == std::string(counter_name)) {
                                IntelGLPerfQueryCounter iglpqc;

                                iglpqc.query_id = query_id;
                                iglpqc.id = counter_id;
                                iglpqc.data_type = counter_data_type_enum;
                                iglpqc.name = std::string(counter_name);
                                iglpqc.offset = counter_offset;
                                iglpqc.size = counter_data_size;

                                query_counters.push_back(iglpqc);
                            }
                        }
                    }
                }

                LOGI << "Query counter info, query_id: " << query_id << std::endl
                     << "counter_id: " << counter_id << std::endl
                     << "counter_name: " << counter_name << std::endl
                     << "counter_desc: " << counter_desc << std::endl
                     << "counter_offset: " << counter_offset << std::endl
                     << "counter_data_size: " << counter_data_size << std::endl
                     << "counter_type_enum: " << std::hex << counter_type_enum << std::endl
                     << "counter_dta_type_enum: " << std::hex << counter_data_type_enum << std::endl
                     << "raw_counter_max_value: " << raw_counter_max_value << std::endl;
            }

            glGetNextPerfQueryIdINTEL(query_id, &query_id);
        }
        std::string path = std::string(GetWritePath(CoreGameModID).c_str()) + "/intel_gl_perf.csv";
        my_ofstream_open(csv_output, path.c_str());
    } else {
        LOGI << "Intel performance query is not available on this machine" << std::endl;
    }
}

void IntelGLPerf::Finalize() {
    if (perf_available) {
        // TODO: Dispose all available handles
        // TODO: dispose of all actively used query handles
    }
}

void IntelGLPerf::PerfGPUBegin(int type, int group, const char* file, const int line) {
    if (perf_available) {
        GLuint query_id = 0;

        int offset = 0;
        int slash_pos = 0;
        while (*(file + offset) != '\0') {
            if (*(file + offset) == '/' || *(file + offset) == '\\')
                slash_pos = offset;
            offset++;
        }

        char counter_name[256];
        snprintf(counter_name, 256, "%s_%d", file + slash_pos + 1, line);

        std::map<std::string, GLuint>::iterator query_it = active_query_handles.find(std::string(counter_name));

        for (unsigned i = 0; i < queries.size(); i++) {
            if (queries[i].type_id == type) {
                query_id = queries[i].id;
            }
        }

        GLuint query_handle;
        if (query_it != active_query_handles.end()) {
            query_handle = query_it->second;
        } else {
            glCreatePerfQueryINTEL(query_id, &query_handle);
            active_query_handles[std::string(counter_name)] = query_handle;
        }

        PerfInstance perf;
        perf.instance_id = query_handle;
        glBeginPerfQueryINTEL(perf.instance_id);

        perf.group = group;
        perf.file = file + slash_pos + 1;
        perf.line = line;
        perf.query_id = query_id;

        perf_query_instances.push_back(perf);
        query_handle_stack.push(perf.instance_id);
    }
}

void IntelGLPerf::PerfGPUEnd() {
    if (perf_available) {
        glEndPerfQueryINTEL(query_handle_stack.top());
        query_handle_stack.pop();
    }
}

void IntelGLPerf::PostFrameSwap() {
    if (perf_available) {
        for (unsigned i = 0; i < perf_query_instances.size(); i++) {
            PerfInstance& p = perf_query_instances[i];
            GLuint bytes_written;
            glGetPerfQueryDataINTEL(
                p.instance_id,
                GL_PERFQUERY_WAIT_INTEL,
                max_query_size,
                data_dest,
                &bytes_written);

            for (unsigned k = 0; k < query_counters.size(); k++) {
                if (query_counters[k].query_id == p.query_id) {
                    char* data = data_dest + query_counters[k].offset;
                    switch (query_counters[k].data_type) {
                        case GL_PERFQUERY_COUNTER_DATA_UINT32_INTEL:
                            csv_output << p.file << ":" << p.line << "," << query_counters[k].name << "," << *((uint32_t*)data) << std::endl;
                            break;
                        case GL_PERFQUERY_COUNTER_DATA_UINT64_INTEL:
                            csv_output << p.file << ":" << p.line << "," << query_counters[k].name << "," << *((uint64_t*)data) << std::endl;
                            break;
                        case GL_PERFQUERY_COUNTER_DATA_FLOAT_INTEL:
                            csv_output << p.file << ":" << p.line << "," << query_counters[k].name << "," << *((float*)data) << std::endl;
                            break;
                        case GL_PERFQUERY_COUNTER_DATA_DOUBLE_INTEL:
                            csv_output << p.file << ":" << p.line << "," << query_counters[k].name << "," << *((double*)data) << std::endl;
                            break;
                        case GL_PERFQUERY_COUNTER_DATA_BOOL32_INTEL:
                            csv_output << p.file << ":" << p.line << "," << query_counters[k].name << "," << (*((uint32_t*)data) ? "true" : "false") << std::endl;
                            break;
                    }
                }
            }
        }
        perf_query_instances.clear();
    }
}

IntelGLPerf IntelGLTiming;
IntelGLPerf* intelGLTiming = &IntelGLTiming;

#endif
