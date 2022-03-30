//-----------------------------------------------------------------------------
//           Name: asfuncs.h
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

#include <string>
#include <map>
#include <cstdint>

class asIScriptContext;
struct asSMessageInfo;

void PrintString(std::string &str);
void MessageCallback(const asSMessageInfo *msg, void *param);

class SceneGraph;
class Engine;
class ASContext;
class MovementObject;

void AttachASNetwork(ASContext* context);
void AttachStringConvert(ASContext *context);
void AttachUIQueries(ASContext *context);
void AttachMathFuncs(ASContext *context);
void Attach3DMathFuncs(ASContext *context);
void AttachActiveCamera(ASContext *context);
void AttachMovementObjectCamera(ASContext *context, MovementObject* mo);
void AttachTimer(ASContext *context);
void AttachPhysics(ASContext *context);
void AttachSound( ASContext *context );
void AttachParticles( ASContext *context );
void AttachDebugDraw( ASContext *context );
void AttachDecals( ASContext *context );
void AttachEngine( ASContext *context );
void AttachScenegraph( ASContext *context, SceneGraph *scenegraph );
void AttachLevel( ASContext *context );
void AttachInterlevelData( ASContext *context );
void AttachIMGUI( ASContext *context );
void AttachIMGUIModding( ASContext *context );
void AttachNavMesh(ASContext *context);
void AttachMessages(ASContext *context);
void AttachScreenWidth(ASContext *context);
void AttachObject(ASContext *context);
void AttachPlaceholderObject(ASContext *context);
void AttachTokenIterator(ASContext *context);
void AttachError(ASContext *context);
void AttachSky( ASContext *context );
void AttachSimpleFile( ASContext *context );
void AttachStopwatch( ASContext *context );
void AttachProfiler( ASContext* context );
void AttachTelemetry( ASContext *context );
void AttachUndo( ASContext *context );
void AttachLog( ASContext *context );
void AttachInfo( ASContext *context );
void AttachJSON( ASContext *context );
void AttachConfig( ASContext *context );
void AttachStringUtil( ASContext *context );
void AttachIO( ASContext *context );
void AttachLevelDetails(ASContext *context);
void AttachModding( ASContext* context );
void AttachStorage( ASContext* context );
void AttachDebug( ASContext *context );
void AttachOnline( ASContext * context);

void ReloadConfigValues();
void SetSettingsToPreset( std::string preset_name );

int ASCreateObject(const std::string& path, bool exclude_from_save);
class Object* ReadObjectFromID(int id);
