//-----------------------------------------------------------------------------
//           Name: timingevent.cpp
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

#ifdef SLIM_TIMING

#include <Graphics/graphics.h>
#include <Graphics/shaders.h>
#include <Graphics/vbocontainer.h>

#include <Timing/timingevent.h>
#include <Logging/logdata.h>
#include <Compat/fileio.h>
#include <Threading/sdl_wrapper.h>

#include <sstream>

EventRecorder::EventRecorder() : frameCount(0),
                                 showVisualization(false),
                                 visData(NULL),
                                 visIndices(NULL),
                                 data_vbo(V_KIBIBYTE, kVBOFloat | kVBODynamic),
                                 index_vbo(V_KIBIBYTE / 4, kVBOElement | kVBODynamic) {
    setVisualizationScale(16.0, 32.0);

    gl_state.blend = true;
    gl_state.blend_src = GL_SRC_ALPHA;
    gl_state.blend_dst = GL_ONE_MINUS_SRC_ALPHA;
    gl_state.cull_face = false;
    gl_state.depth_test = false;
    gl_state.depth_write = false;

    resetData();
}

void EventRecorder::resetData() {
    memset(&events, 0, ST_MAX_EVENTS * sizeof(Event));
    nextIndex = 0;
}

void EventRecorder::init(std::string filePath) {
    // See if we have a file to open
    if (filePath == "") {
        writeFile = false;
    } else {
        writeFile = true;

        // Open the .csv file and write out the heading
        outputFileName = filePath + "/frameTimings.csv";

        LOGI << "Opening timing file: " << filePath << std::endl;

        fp = my_fopen(outputFileName.c_str(), "w");

        if (fp == NULL) {
            LOGF << "Unable to open timing file " << outputFileName << std::endl;
            exit(1);
        }

        fprintf(fp, "frame,ms,cycles");
        for (int i = 0; i < STLastEvent; ++i) {
            fprintf(fp, ",%s", EVENT_NAMES[i].c_str());
        }
        fprintf(fp, "\n");
    }
}

void EventRecorder::startFrame() {
    frameStartCycle = GetTimestamp();
    frameStartTick = SDL_TS_GetTicks();
}

void EventRecorder::endFrame() {
#ifndef SLIM_TIMING
    // If we're not doing fine grained timing and not displaying anything, we don't need to do anything
    if (!showVisualization) {
        // Just make sure that there's no overrun
        nextIndex = 0;
        frameCount++;
        // .. and get out of here
        return;
    }
#endif  // not SLIM_TIMING

    const int historyIndex = frameCount % HISTORY_FRAMES;

    timingHistory[historyIndex].frameTimeCycles = GetTimestamp() - frameStartCycle;
    timingHistory[historyIndex].frameTimeMS = SDL_TS_GetTicks() - frameStartTick;

    frameCount++;

    // For now we're just doing totals
    // (note -- I'm aware that this is not the most effecient way to do this
    //  if it becomes a burden it can always be optimized)

    // Do each event class in order
    for (int eventIndex = 0; eventIndex < STLastEvent; ++eventIndex) {
        uint64_t totalTime = 0;
        uint64_t startTime = 0;
        uint64_t endTime = 0;
        int openCount = 0;  // In the case of nested timings, we ignore everything but the outermost

        for (int i = 0; i < nextIndex; ++i) {
            if (events[i].event_id != eventIndex) continue;

            if (events[i].eventType == ST_EVENT_START_CODE) {
                openCount++;
                if (openCount > 1) continue;  // Skip over nested events

                startTime = events[i].tsc;

                continue;
            }

            if (events[i].eventType == ST_EVENT_END_CODE) {
                openCount--;
                if (openCount > 0) continue;  // Skip over nested events

                if (openCount < 0) {
                    std::string errorMsg = "Unterminated or extra timing event: " + EVENT_NAMES[eventIndex];

                    LOGW << errorMsg << std::endl;
                } else {
                    endTime = events[i].tsc;
                    // I'm also aware that there is a small small chance of wraparound here, but it's more than 1 in a billion and
                    // I'm in a hurry

                    totalTime += (endTime - startTime);
                }
            }
        }
        timingHistory[historyIndex].eventTimes[eventIndex] = totalTime;
    }

    // finally write out the times, if nesseary
    if (writeFile && frameCount > WARMUP_FRAMES) {
        fprintf(fp, "%d", frameCount);
        fprintf(fp, ",%d", timingHistory[historyIndex].frameTimeMS);
        fprintf(fp, ",%llu", (long long unsigned int)timingHistory[historyIndex].frameTimeCycles);

        for (int eventIndex = 0; eventIndex < STLastEvent; ++eventIndex) {
            fprintf(fp, ",%llu", (long long unsigned int)timingHistory[historyIndex].eventTimes[eventIndex]);
        }

        fprintf(fp, "\n");
    }

    resetData();
}

void EventRecorder::finalize() {
    if (fp) {
        fclose(fp);
        fp = NULL;
    }
}

int EventRecorder::getAvailableHistorySize() {
    if (frameCount > HISTORY_FRAMES) {
        return HISTORY_FRAMES;
    } else {
        return frameCount;
    }
}

void EventRecorder::setVisualizationScale(float baseMS, float highMS) {
    visBaseMS = baseMS;
    visHighMS = highMS;

    std::ostringstream ss;
    ss << baseMS << "ms";
    baseMSLabel = ss.str();
    ss.str("");
    ss.clear();
    ss << highMS << "ms";
    highMSLabel = ss.str();
}

void EventRecorder::addNewVisualization(uint16_t event1, vec3 color1,
                                        int32_t event2, vec3 color2,
                                        int32_t event3, vec3 color3,
                                        int32_t event4, vec3 color4,
                                        int32_t event5, vec3 color5) {
    // I'm aware this isn't the most effecient code -- but it's called a few times per program execution

    std::vector<VisualizerEvent> newVisualization;

    newVisualization.push_back(VisualizerEvent(event1, color1));

    if (event2 != -1) {
        newVisualization.push_back(VisualizerEvent(event2, color2));
    }

    if (event3 != -1) {
        newVisualization.push_back(VisualizerEvent(event3, color3));
    }

    if (event4 != -1) {
        newVisualization.push_back(VisualizerEvent(event4, color4));
    }

    if (event5 != -1) {
        newVisualization.push_back(VisualizerEvent(event5, color5));
    }

    visualizerEvents.push_back(newVisualization);
}

void EventRecorder::initVisualization() {
    // Now we allocate the memory, etc for our visualization based on this new count

    // See if we already have buffers allocated
    if (visData != NULL && visIndices != NULL) {
        delete visData;
        delete visIndices;
    }

    int boxCount = 0;
    for (std::vector<std::vector<VisualizerEvent> >::iterator it = visualizerEvents.begin();
         it != visualizerEvents.end();
         ++it) {
        boxCount += (*it).size();
    }

    visDataSize = 6 * (((HISTORY_FRAMES * boxCount) + 4) * 4);
    visIndexSize = 6 * (((HISTORY_FRAMES * boxCount) + 4));

    visData = new GLfloat[visDataSize];
    visIndices = new GLuint[visIndexSize];
}

// Fills in the box coordinates and color in the buffer
// Box number starts at 0
void EventRecorder::buildBoxAt(vec2 const& UL, vec2 const& LR, vec4 const& color, int boxNumber) {
    int vertOffset = boxNumber * 6 * 4;

    visData[vertOffset] = UL.x();
    visData[vertOffset + 1] = LR.y();

    visData[vertOffset + 2] = color.r();
    visData[vertOffset + 3] = color.g();
    visData[vertOffset + 4] = color.b();
    visData[vertOffset + 5] = color.a();

    visData[vertOffset + 6] = LR.x();
    visData[vertOffset + 7] = LR.y();

    visData[vertOffset + 8] = color.r();
    visData[vertOffset + 9] = color.g();
    visData[vertOffset + 10] = color.b();
    visData[vertOffset + 11] = color.a();

    visData[vertOffset + 12] = LR.x();
    visData[vertOffset + 13] = UL.y();

    visData[vertOffset + 14] = color.r();
    visData[vertOffset + 15] = color.g();
    visData[vertOffset + 16] = color.b();
    visData[vertOffset + 17] = color.a();

    visData[vertOffset + 18] = UL.x();
    visData[vertOffset + 19] = UL.y();

    visData[vertOffset + 20] = color.r();
    visData[vertOffset + 21] = color.g();
    visData[vertOffset + 22] = color.b();
    visData[vertOffset + 23] = color.a();

    int indexOffset = boxNumber * 6;
    int startIndex = boxNumber * 4;

    visIndices[indexOffset] = startIndex;
    visIndices[indexOffset + 1] = startIndex + 1;
    visIndices[indexOffset + 2] = startIndex + 2;

    visIndices[indexOffset + 3] = startIndex;
    visIndices[indexOffset + 4] = startIndex + 2;
    visIndices[indexOffset + 5] = startIndex + 3;
}

void EventRecorder::drawVisualization(TextAtlasRenderer& text_atlas_renderer,
                                      FontRenderer& font_renderer,
                                      TextAtlas& text_atlas) {
    // See if we're displaying and do a sanity check
    if (!showVisualization || visualizerEvents.empty() || visIndices == NULL || visData == NULL) return;

    Graphics* graphics = Graphics::Instance();
    Shaders* shaders = Shaders::Instance();

    const float winWidth = (float)graphics->window_dims[0];
    const float winHeight = (float)graphics->window_dims[1];

    // use non-premultiplied alpha
    graphics->SetBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Prep the data

    // Figure out the (rough) factor betweeen ms and cycles on this timing
    int currentHistory = (frameCount) % HISTORY_FRAMES;

    float cyclesInBaseBar;

    // prevent a divide by zero possibility (it'll look wonky... but it won't crash)
    if (timingHistory[currentHistory].frameTimeMS == 0) {
        cyclesInBaseBar = visBaseMS;
    } else {
        cyclesInBaseBar = (timingHistory[currentHistory].frameTimeCycles / timingHistory[currentHistory].frameTimeMS) * visBaseMS;
    }

    // Now figure out the 'normalized' version of the current timings ( in terms of a factor of the timing bar)
    for (int eventID = 0; eventID < STLastEvent; ++eventID) {
        normalizedFrames[currentHistory].eventTimes[eventID] = (float)timingHistory[currentHistory].eventTimes[eventID] / cyclesInBaseBar;
    }

    // Now we get to build some triangles!

    int shader_id = shaders->returnProgram("simple_2d #COLOREDVERTICES");
    shaders->setProgram(shader_id);
    int vert_coord_id = shaders->returnShaderAttrib("vert_coord", shader_id);
    int color_id = shaders->returnShaderAttrib("vert_color", shader_id);
    int mvp_mat_id = shaders->returnShaderVariable("mvp_mat", shader_id);

    graphics->setGLState(gl_state);
    glm::mat4 proj_mat;

    const vec4 backgroundColor(0.26, 0.27, 0.29, 0.5);
    const vec4 backgroundBarColor(0.5, 0.27, 0.29, 0.9);
    const vec4 highareaColor(0.26, 0.27, 0.29, 0.3);
    const vec4 highareaBarColor(0.5, 0.27, 0.29, 0.7);

    const float backgroundHeight = winHeight * 0.20;
    const float highmarkHeight = (visHighMS / visBaseMS) * backgroundHeight;

    // Set up the background panel
    buildBoxAt(vec2(0.0, backgroundHeight), vec2(winWidth, 0.0), backgroundColor, 0);
    buildBoxAt(vec2(0.0, backgroundHeight + 2), vec2(winWidth, backgroundHeight), backgroundBarColor, 1);

    // Set up the 'high' background area (if needed)
    if (visHighMS > visBaseMS) {
        buildBoxAt(vec2(0.0, highmarkHeight), vec2(winWidth, backgroundHeight), highareaColor, 2);
        buildBoxAt(vec2(0.0, highmarkHeight + 2), vec2(winWidth, highmarkHeight), highareaBarColor, 3);
    } else {
        // Just draw nothing ( just to keep the buffer the same size )
        buildBoxAt(vec2(0.0, backgroundHeight), vec2(winWidth, 0.0), vec4(0.0), 2);
        buildBoxAt(vec2(0.0, backgroundHeight + 2), vec2(winWidth, backgroundHeight), vec4(0.0), 3);
    }

    // Figure out how wide the bars should be
    float barWidth = ((winWidth - 10) - (float)(10.0 * visualizerEvents.size())) /
                     (float)(visualizerEvents.size() * HISTORY_FRAMES);

    float xOffset = 10.0;
    int boxCount = 4;

    for (std::vector<std::vector<VisualizerEvent> >::iterator outerIt = visualizerEvents.begin();
         outerIt != visualizerEvents.end();
         ++outerIt) {
        // Go through all the available history
        int histSize = getAvailableHistorySize();
        int histIndex;

        if (histSize < HISTORY_FRAMES) {
            histIndex = 0;
        } else {
            histIndex = (frameCount + 1) % HISTORY_FRAMES;
        }

        float alpha = 0.3;
        float alphaStep = 0.7 / (float)histSize;

        for (int i = 0; i < histSize; ++i) {
            float yOffset = 0;  // For stacking

            for (std::vector<VisualizerEvent>::iterator innerIt = (*outerIt).begin();
                 innerIt != (*outerIt).end();
                 ++innerIt) {
                const uint16_t eventID = innerIt->eventID;

                float upperHeight = yOffset + (backgroundHeight * normalizedFrames[histIndex].eventTimes[eventID]);

                vec2 UL(xOffset, upperHeight);
                vec2 LR((xOffset + barWidth), yOffset);
                vec4 color(innerIt->color, alpha);

                buildBoxAt(UL, LR, color, boxCount);

                boxCount++;
                yOffset = upperHeight;
            }
            alpha += alphaStep;
            histIndex = (histIndex + 1) % HISTORY_FRAMES;
            xOffset += barWidth;
        }
        xOffset += 10.0;
    }

    // Set up the model matrix
    glm::mat4 mvp_mat = glm::ortho(0.0f, winWidth, 0.0f, winHeight);

    glUniformMatrix4fv(mvp_mat_id, 1, false, (GLfloat*)&mvp_mat);
    Graphics::Instance()->EnableVertexAttribArray(vert_coord_id);
    Graphics::Instance()->EnableVertexAttribArray(color_id);

    index_vbo.SetHint(boxCount * 6 * sizeof(GLuint), kVBODynamic);
    index_vbo.Fill(boxCount * 6 * sizeof(GLuint), (void*)visIndices);

    data_vbo.SetHint(boxCount * 4 * 6 * sizeof(GLfloat), kVBODynamic);
    data_vbo.Fill(boxCount * 4 * 6 * sizeof(GLfloat), (void*)visData);
    index_vbo.Bind();
    data_vbo.Bind();

    glVertexAttribPointer(vert_coord_id, 2, GL_FLOAT, false, 6 * sizeof(GLfloat), (const void*)(data_vbo.offset()));
    glVertexAttribPointer(color_id, 4, GL_FLOAT, false, 6 * sizeof(GLfloat), (const void*)(data_vbo.offset() + 2 * sizeof(GLfloat)));

    Graphics::Instance()->DrawElements(GL_TRIANGLES, boxCount * 6, GL_UNSIGNED_INT, (const void*)index_vbo.offset());

    Graphics::Instance()->ResetVertexAttribArrays();

    int basepos[] = {5, (int)(graphics->window_dims[1] - (backgroundHeight + 4))};
    text_atlas_renderer.num_characters = 0;
    text_atlas_renderer.AddText(&text_atlas, baseMSLabel.c_str(), basepos, &font_renderer, UINT32MAX);

    if (visHighMS > visBaseMS) {
        int highpos[] = {5, (int)(graphics->window_dims[1] - (highmarkHeight + 4))};
        text_atlas_renderer.AddText(&text_atlas, highMSLabel.c_str(), highpos, &font_renderer, UINT32MAX);
    }

    text_atlas_renderer.Draw(&text_atlas, graphics, TextAtlasRenderer::kTextShadow, vec4(0.96, 0.91, 0.59, 0.8));
    Textures::Instance()->InvalidateBindCache();
}

void EventRecorder::displayVisualization(bool show) {
    // See if we're switching from not displaying to displaying
    if (!showVisualization && show) {
        // Set all the history structures back to zero
        memset(&normalizedFrames, 0, HISTORY_FRAMES * sizeof(NormalizedFrameTotals));
        memset(&timingHistory, 0, HISTORY_FRAMES * sizeof(FrameTotals));
    }

    showVisualization = show;
}

void EventRecorder::clearVisualization() {
    if (visData != NULL && visIndices != NULL) {
        delete visData;
        delete visIndices;
    }

    visData = NULL;
    visIndices = NULL;

    visualizerEvents.clear();
}

EventRecorder TimingEvents;
EventRecorder* slimTimingEvents = &TimingEvents;

#endif  // SLIM_TIMING
