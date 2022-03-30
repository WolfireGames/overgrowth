//-----------------------------------------------------------------------------
//           Name: pxdebugdraw.cpp
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
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
#include "pxdebugdraw.h"

#include <Graphics/graphics.h>
#include <Graphics/geometry.h>
#include <Graphics/camera.h>
#include <Graphics/textures.h>
#include <Graphics/shaders.h>
#include <Graphics/model.h>
#include <Graphics/Billboard.h>
#include <Graphics/font_renderer.h>
#include <Graphics/text.h>
#include <Graphics/vboringcontainer.h>

#include <Internal/timer.h>
#include <Internal/profiler.h>
#include <Internal/common.h>

#include <Math/vec3math.h>
#include <Math/vec4math.h>

#include <Logging/logdata.h>

#include <set>

extern Timer game_timer;

extern TextAtlas g_text_atlas[kNumTextAtlas];
extern TextAtlasRenderer g_text_atlas_renderer;

extern bool g_debug_runtime_disable_debug_draw;
extern bool g_debug_runtime_disable_debug_ribbon_draw;

DebugDraw::~DebugDraw() {
    Dispose();
}

void DebugDraw::Dispose() {
    DebugDrawElementMap::iterator iter = elements.begin();
    for(;iter != elements.end(); ++iter){
        DebugDrawElement* element = (*iter).second;
        delete element;
    }
    elements.clear();
}

void DebugDraw::Draw() {
    // Can't return early for g_debug_runtime_disable_debug_draw without memory usage ramping up every frame

    bool editor_draw = true;
    if(Graphics::Instance()->media_mode() || 
       ActiveCameras::Instance()->Get()->GetFlags() == Camera::kPreviewCamera ||
       elements.empty())
    {
        editor_draw = false;
    }

    CHECK_GL_ERROR();
    GLState gl_state;
    gl_state.blend = true;
    gl_state.cull_face = false;
    gl_state.depth_test = true;
    gl_state.depth_write = false;
    Graphics::Instance()->setGLState(gl_state);

    if (!g_debug_runtime_disable_debug_draw) {
        delete_on_update.Draw();
        delete_on_draw.Draw();
    }
    delete_on_draw.verts.clear();

    int old_key = -1;
    DebugDrawElementMap::iterator iter = elements.begin();
    for(;iter != elements.end();){
        DebugDrawElement* element = (*iter).second;
        CHECK_GL_ERROR();
		if(!g_debug_runtime_disable_debug_draw && (element->type == kRibbon || editor_draw) && element->visible){
			PROFILER_GPU_ZONE(g_profiler_ctx, "Draw element");
			Graphics::Instance()->setDepthTest(true);
            element->Draw();
        }
        CHECK_GL_ERROR();
        if(element->GetLifespan() == _delete_on_draw){
            LOGS << "Removing element" << std::endl;
            delete element;
            elements.erase(iter);
            if(old_key == -1){
                iter = elements.begin();
            } else {
                iter = ++elements.find(old_key);
            }
        } else {
            old_key = (*iter).first;
             ++iter;
        }
    }
    CHECK_GL_ERROR();
}

void DebugDraw::Update(float timestep) {
    delete_on_update.verts.clear();
    int old_key = -1;
    DebugDrawElementMap::iterator iter = elements.begin();
    for(;iter != elements.end();){
        DebugDrawElement* element = (*iter).second;
        if(element->GetLifespan() == _fade){
            element->fade_amount += timestep;
        }
        bool to_delete = element->GetLifespan() == _delete_on_update ||
                         element->fade_amount >= 1.0f;
        if(to_delete){
            delete element;
            elements.erase(iter);
            if(old_key == -1){
                iter = elements.begin();
            } else {
                iter = ++elements.find(old_key);
            }
        } else {
            old_key = (*iter).first;
             ++iter;
        }
    }
}

void DebugDraw::Remove( int which )
{
    LOGS << "Erasing element" << std::endl;
    DebugDrawElementMap::iterator iter;
    iter = elements.find(which);
    if(iter != elements.end()){
        free_ids.push((*iter).first);
        DebugDrawElement* element = (*iter).second;
        delete element;
        elements.erase(iter);
    }
}

int DebugDraw::AddElement( DebugDrawElement* element,
                          const DDLifespan lifespan)
{
    LOGS << "Adding element" << std::endl;
    int new_id;
    if(free_ids.empty()){
        new_id = id_index++;
    } else {
        new_id = free_ids.front();
        free_ids.pop();
    }
    element->SetLifespan(lifespan);
    elements.insert(std::pair<int, DebugDrawElement*> (new_id, element));
    return new_id;
}

DebugDraw::DebugDraw():
    id_index(0)
{}

int DebugDraw::AddLine( const vec3 &start, 
                        const vec3 &end, 
                        const vec4 &start_color, 
                        const vec4 &end_color, 
                        const DDLifespan lifespan,
                        const DDFlag& flags)
{
    if(lifespan == _delete_on_update && flags == _DD_NO_FLAG){
        size_t start_count = delete_on_update.verts.size();
        delete_on_update.verts.resize(start_count + 14);
        memcpy(&delete_on_update.verts[start_count + 0], &start[0], sizeof(vec3));
        memcpy(&delete_on_update.verts[start_count + 3], &start_color[0], sizeof(vec4));
        memcpy(&delete_on_update.verts[start_count + 7], &end[0], sizeof(vec3));
        memcpy(&delete_on_update.verts[start_count + 10], &end_color[0], sizeof(vec4));
        return -1;
    } else if(lifespan == _delete_on_draw && flags == _DD_NO_FLAG){
        size_t start_count = delete_on_draw.verts.size();
        delete_on_draw.verts.resize(start_count + 14);
        memcpy(&delete_on_draw.verts[start_count + 0], &start[0], sizeof(vec3));
        memcpy(&delete_on_draw.verts[start_count + 3], &start_color[0], sizeof(vec4));
        memcpy(&delete_on_draw.verts[start_count + 7], &end[0], sizeof(vec3));
        memcpy(&delete_on_draw.verts[start_count + 10], &end_color[0], sizeof(vec4));
        return -1;
    } else {
        DebugDrawElement* element = new DebugDrawLine(start, end, start_color, end_color, flags);
        return AddElement(element, lifespan);
    }
}

int DebugDraw::AddLine( const vec3 &start, 
                        const vec3 &end, 
                        const vec4 &color, 
                        const DDLifespan lifespan,
                        const DDFlag& flags)
{
	// sync_attach_3 vore coolt att skicka över detta 
	// om vi kommer från sync_attach_1
    return AddLine(start,end,color,color,lifespan,flags);
}


int DebugDraw::AddRibbon(const vec3 &start, const vec3 &end, const vec4 &start_color, const vec4 &end_color, const float start_width, const float end_width, const DDLifespan lifespan, const DDFlag& flags /*= _DD_NO_FLAG*/) {
    DebugDrawElement* element = new DebugDrawRibbon(start, end, start_color, end_color, start_width, end_width, flags);
    return AddElement(element, lifespan);
}

int DebugDraw::AddRibbon(const DDLifespan lifespan, const DDFlag& flags /*= _DD_NO_FLAG*/) {
    DebugDrawElement* element = new DebugDrawRibbon(flags);
    return AddElement(element, lifespan);
}

int DebugDraw::AddPoint( const vec3 &point,
                        const vec4 &color, 
                        const DDLifespan lifespan,
                        const DDFlag& flags)
{
    DebugDrawElement* element = new DebugDrawPoint(point, color, flags);
    return AddElement(element, lifespan);
}

int DebugDraw::AddWireSphere( const vec3 &position, 
                              const float radius, 
                              const vec4 &color, 
                              const DDLifespan lifespan )
{
    if(radius == 0.0f){
        DisplayError("Error", "Creating debug draw sphere with radius zero");
    }
    DebugDrawElement* element = new DebugDrawWireSphere(position, radius, vec3(1.0f), color);
    return AddElement(element, lifespan);
}

int DebugDraw::AddText( const vec3 &position, const std::string &content, const float& scale, const DDLifespan lifespan, const DDFlag& flags, const vec4& color )
{
    DebugDrawElement* element = new DebugDrawText(position, scale, content, flags, color);
    return AddElement(element, lifespan);
}

bool DebugDraw::SetPosition( int id, const vec3 &position )
{
    DebugDrawElement* element = GetElement( id );
    if( element ) {
        return element->SetPosition(position);
    } else {
        LOGE << "No element with id " << id << std::endl;
        return false;
    }
}

bool DebugDraw::SetVisible(int id, bool visible)
{
    DebugDrawElement* element = GetElement( id );
    if( element ) {
        element->visible = visible;
        return true;
    } else {
        LOGE << "No element with id " << id << std::endl;
        return false;
    }
}

DebugDrawElement* DebugDraw::GetElement( int id )
{
    DebugDrawElementMap::iterator it = elements.find(id);

    if( it != elements.end() ) {
        return it->second;
    } else {
        return NULL;
    }
}

int DebugDraw::AddWireBox( const vec3 &position, const vec3 &dimensions, const vec4 &color, const DDLifespan lifespan )
{    
    DebugDrawElement* element = new DebugDrawWireBox(position, 
                                                     dimensions, 
                                                     color);
    return AddElement(element, lifespan);
}

int DebugDraw::AddWireCylinder( const vec3 &position, const float radius, const float height, const vec4 &color, const DDLifespan lifespan )
{
    DebugDrawElement* element = new DebugDrawWireCylinder(position,
                                                        radius, 
                                                        height, 
                                                        color);
    return AddElement(element, lifespan);
}

int DebugDraw::AddWireScaledSphere( const vec3 &position, const float radius, const vec3 &scale, const vec4 &color, const DDLifespan lifespan )
{
    DebugDrawElement* element = new DebugDrawWireSphere(position, radius, scale, color);
    return AddElement(element, lifespan);
}

int DebugDraw::AddWireMesh( const std::string &path, const mat4 &transform, const vec4 &color, const DDLifespan lifespan )
{
    RC_VBOContainer vbo;
    std::vector<vec3> vertices;

    std::map<std::string, RC_VBOContainer>::iterator me_iter = mesh_edges_map.find(path);

    if(me_iter == mesh_edges_map.end())
    {
        Model model;
        model.LoadObj(path, _MDL_SIMPLE);
        model.RemoveDuplicatedVerts();


        for(unsigned i=0; i < (model.faces.size()/3)*3; i +=3)
        {
            for( unsigned j=0; j < 3; j++ )
            {
                int f = model.faces[i+j];
                int s = model.faces[i+((j+1)%3)];

                vertices.push_back(vec3(model.vertices[f*3+0], 
                                        model.vertices[f*3+1],
                                        model.vertices[f*3+2]));
                vertices.push_back(vec3(model.vertices[s*3+0], 
                                        model.vertices[s*3+1],
                                        model.vertices[s*3+2]));
            }
        }
        
        //One of the few cases in this codebase where kVBOStream makes sense
        //(Creating the buffer, filling it once, reading a couple of times then destroying, as per spec)
        vbo->Fill( kVBOFloat | kVBOStream, vertices.size() * sizeof(vertices[0]), &vertices[0] );
        
        mesh_edges_map[path] = vbo;
    } 
    else 
    {
        vbo = me_iter->second;
    } 
    
    DebugDrawElement* element = new DebugDrawWireMesh(vbo, transform, color); 

    return AddElement(element, lifespan);
}

int DebugDraw::AddLineObject(const RC_VBOContainer &vbo,
        const mat4 &transform,
        const vec4 &color, 
        const DDLifespan lifespan)
{
    DebugDrawElement* element = new DebugDrawWire( vbo, transform, color, _DD_NO_FLAG );
    return AddElement(element,lifespan);
}

int DebugDraw::AddStippleMesh(const RC_VBOContainer &vbo,
        const mat4 &transform,
        const vec4 &color, 
        const DDLifespan lifespan)
{
    DebugDrawElement* element = new DebugDrawStippleMesh(vbo, transform, color, _DD_NO_FLAG);
    return AddElement(element, lifespan);
}

int DebugDraw::AddLines( const std::vector<float> &vertices, const std::vector<unsigned> &indices, const vec4 &color, const DDLifespan lifespan, const DDFlag& flags ) {
    DebugDrawElement* element = new DebugDrawLines(vertices, indices, color, flags);
    return AddElement(element, lifespan);
}

int DebugDraw::AddLines( const std::vector<vec3> &vertices, const vec4 &color, const DDLifespan lifespan, const DDFlag& flags ) {
    DebugDrawElement* element = new DebugDrawLines(vertices, color, flags);
    return AddElement(element, lifespan);
}

int DebugDraw::AddCircle( const mat4 &transform, const vec4 &color, const DDLifespan lifespan, const DDFlag& flags ) {
    DebugDrawElement* element = new DebugDrawCircle(transform, color, flags);
    return AddElement(element, lifespan);
}

int DebugDraw::AddBillboard(const TextureRef &ref, const vec3 &position, float scale, const vec4& color, AlphaMode mode, const DDLifespan lifespan) {
    DebugDrawElement* element = new DebugDrawBillboard(ref, position, scale, color, mode);
    return AddElement(element, lifespan);
}

int DebugDraw::AddTransformedWireScaledSphere( const mat4 &transform, const vec4 &color, const DDLifespan lifespan ) {
  
    DebugDrawWireSphere* element = new DebugDrawWireSphere(transform, color);
    return AddElement((DebugDrawElement*)element, lifespan);
}

DebugDrawLine::DebugDrawLine(
        const vec3 &start, 
        const vec3 &end, 
        const vec4 &start_color,
        const vec4 &end_color,
        const DDFlag& _flags ) :
DebugDrawWire((DDFlag)(_flags | _DD_COLOR))
{

    if( _flags & _DD_DASHED )
    {
        const float segment_length = 0.2f;
        std::vector<GLfloat> line_data;

        vec3 diff =  end - start;
        vec3 dir = normalize(diff);
        float len2 = length_squared(diff);
        
        vec3 cur = start;
        while( length_squared((cur+dir*segment_length)-start) < len2 )
        {
            vec4 mix_color = lerp(start_color, end_color, (len2-length_squared(cur-start))/len2);
            line_data.push_back(cur[0]);
            line_data.push_back(cur[1]);
            line_data.push_back(cur[2]);
            line_data.push_back(mix_color[0]);
            line_data.push_back(mix_color[1]);
            line_data.push_back(mix_color[2]);
            line_data.push_back(mix_color[3]);

            vec3 next = cur+dir*segment_length;
            line_data.push_back(next[0]);
            line_data.push_back(next[1]);
            line_data.push_back(next[2]);
            line_data.push_back(mix_color[0]);
            line_data.push_back(mix_color[1]);
            line_data.push_back(mix_color[2]);
            line_data.push_back(mix_color[3]);

            cur = cur+dir*segment_length*2.0f;
        }

        if( length_squared(cur-start) < length_squared(end-start) )
        {
            line_data.push_back(cur[0]);
            line_data.push_back(cur[1]);
            line_data.push_back(cur[2]);

            line_data.push_back(end_color[0]);
            line_data.push_back(end_color[1]);
            line_data.push_back(end_color[2]);
            line_data.push_back(end_color[3]);

            line_data.push_back(end[0]);
            line_data.push_back(end[1]);
            line_data.push_back(end[2]);

            line_data.push_back(end_color[0]);
            line_data.push_back(end_color[1]);
            line_data.push_back(end_color[2]);
            line_data.push_back(end_color[3]);
        }

        vbo->Fill(kVBODynamic | kVBOFloat, sizeof(GLfloat)*line_data.size(), &line_data[0]);
    }
    else
    {
        GLfloat data[] = {
            start[0], start[1], start[2], 
            start_color[0], start_color[1], start_color[2], start_color[3],
            end[0], end[1], end[2],
            end_color[0], end_color[1], end_color[2], end_color[3]
        };

        vbo->Fill(kVBODynamic | kVBOFloat, sizeof(data), data);
    }
    type = kLine;
}

DebugDrawLines::DebugDrawLines(
const std::vector<float> &vertices, 
const std::vector<unsigned> &indices, 
const vec4 &_color, 
const DDFlag& _flags):
DebugDrawWire(_color, _flags)
{
    std::vector<vec3> data;
    data.reserve( indices.size() );  

    for( unsigned i = 0; i < indices.size(); i++ )
    {
        data.push_back( vec3( 
            vertices[i*3+0],
            vertices[i*3+1],
            vertices[i*3+2]));
    }

    vbo->Fill(kVBOFloat | kVBOStatic, data.size() * sizeof(data[0]), &data[0]);
    type = kLines;
}

DebugDrawLines::DebugDrawLines(
const std::vector<vec3> &vertices, 
const vec4 &_color, 
const DDFlag& _flags):
DebugDrawWire(_color, _flags)
{
    vbo->Fill(kVBOFloat | kVBOStatic, vertices.size() * sizeof(vertices[0]), (void*)&vertices[0]);
    type = kLines;
}

void DebugDrawWire::DecreaseOpacity( float how_much )
{
    opacity -= how_much; 
    if( opacity < 0.0f ) opacity = 0.0f;
}

bool DebugDrawWire::IsInvisible()
{
    return opacity<=0;
}

DDLifespan LifespanFromInt(int lifespan_int) {
    DDLifespan lifespan;
    switch(lifespan_int){
    case _delete_on_draw:
        lifespan = _delete_on_draw;
        break;
    case _delete_on_update:
        lifespan = _delete_on_update;
        break;
    case _persistent:
        lifespan = _persistent;
        break;
    case _fade:
        lifespan = _fade;
        break;
    default:
        std::ostringstream os;
        os << "Invalid lifespan int: " << lifespan_int;
        DisplayError("Error", os.str().c_str());
        return _delete_on_update;
    }
    return lifespan;
}

RC_VBOContainer DebugDrawWireCylinder::wire_cylinder_vbo;

DebugDrawWireCylinder::DebugDrawWireCylinder( const vec3 &_position,
                                              const float _radius, 
                                              const float _height, 
                                              const vec4 &_color ):
    DebugDrawWire(_color, _DD_NO_FLAG)
{
    transform.AddTranslation( _position );
    transform.AddRotation(vec3(0,0,0));

    mat4 scaleMatrix( 1.0f );
    scaleMatrix.SetScale( vec3( _radius, _height, _radius ) );
    
    transform *= scaleMatrix;

    if(!wire_cylinder_vbo->valid()){
        //TODO: fix cleanup by correct referencecounting.
        std::vector<vec3> vert_array;
        GetWireCylinderVertArray(12,vert_array); 
        wire_cylinder_vbo->Fill(kVBOFloat | kVBOStatic, vert_array.size() * sizeof(vert_array[0]), &vert_array[0]);
    }

    vbo = wire_cylinder_vbo;
    type = kWireCylinder;
}

DebugDrawWireCylinder::~DebugDrawWireCylinder()
{
}

RC_VBOContainer DebugDrawWireSphere::wire_sphere_vbo;

DebugDrawWireSphere::DebugDrawWireSphere( const vec3 &_position,
                                          const float _radius, 
                                          const vec3 &_scale,
                                          const vec4 &_color ):
    DebugDrawWire(_color, _DD_NO_FLAG)
{
    if(!wire_sphere_vbo->valid()){
        std::vector<float> vert_array;
        GetWireSphereVertArray(1.0f,12,12,vert_array); 
        wire_sphere_vbo->Fill(kVBOFloat | kVBOStatic, vert_array.size() * sizeof(vert_array[0]), &vert_array[0]);
    }
    vbo = wire_sphere_vbo;

    transform.AddTranslation( _position );
    transform.AddRotation(vec3(PI_f/2.0f,0,0));
    vec3 swizzle_scale = _scale;
    std::swap(swizzle_scale[2], swizzle_scale[1]);


    mat4 scaleMatrix( 1.0f );
    scaleMatrix.SetScale( swizzle_scale * _radius );
    transform *= scaleMatrix;
    type = kWireSphere;
}


DebugDrawWireSphere::DebugDrawWireSphere(const mat4 _transform,
                    const vec4 &_color ):
    DebugDrawWire(_transform, _color, _DD_NO_FLAG)
{
    if(!wire_sphere_vbo->valid()){
        std::vector<float> vert_array;
        GetWireSphereVertArray(1.0f,12,12,vert_array); 
        wire_sphere_vbo->Fill(kVBOFloat | kVBOStatic, vert_array.size() * sizeof(vert_array[0]), &vert_array[0]);
    }
    vbo = wire_sphere_vbo;
    type = kWireSphere;
}

DebugDrawWireSphere::~DebugDrawWireSphere() {
}

RC_VBOContainer DebugDrawText::vbo;

DebugDrawText::DebugDrawText( const vec3 &_position, 
                              const float &_scale,
                              const std::string &_content,
                              const DDFlag& _flags,
                              const vec4& _color ):
    DebugDrawElement(_flags),
    scale(_scale),
    color(_color)
{
    if( !vbo->valid())
    {
        const float origo[] = {
            0.0f,0.0f,0.0f,
        };

        vbo->Fill(kVBOFloat | kVBOStatic, sizeof(origo), (void*)&origo[0]);
    }

    transform = mat4(1.0f);
    transform.SetTranslation( _position );
    
    FormatString(text, kBufSize, "%s", _content.c_str());
    type = kText;
}

DebugDrawText::~DebugDrawText()
{
}

bool DebugDrawText::SetPosition(const vec3& position)
{
    transform = mat4(1.0f);
    transform.SetTranslation( position );
    return true;
}

void DebugDrawText::Draw() 
{
	PROFILER_GPU_ZONE(g_profiler_ctx, "DebugDrawText::Draw()");
    vec3 screen_point = ActiveCameras::Get()->worldToScreen(transform.GetTranslationPart());
    if(screen_point[2] < 1.0){
        int pos[] = {(int)screen_point[0], (int)(Graphics::Instance()->window_dims[1] - screen_point[1])};
        g_text_atlas_renderer.num_characters = 0;
        CHECK_GL_ERROR();
        FontRenderer* font_renderer = FontRenderer::Instance();
        TextMetrics metrics = g_text_atlas_renderer.GetMetrics(
            &g_text_atlas[kTextAtlasMono], 
            text, 
            font_renderer, 
            512);
        pos[0] -= metrics.bounds[2] / 2;
        g_text_atlas_renderer.AddText(
            &g_text_atlas[kTextAtlasMono], 
            text, 
            pos, font_renderer,
            512);
        CHECK_GL_ERROR();
        vec4 gamma_corrected_color = vec4(powf(color.r(), 2.2f), powf(color.g(), 2.2f), powf(color.b(), 2.2f), color.a());
        g_text_atlas_renderer.Draw(&g_text_atlas[kTextAtlasMono], Graphics::Instance(), TextAtlasRenderer::kTextShadow, gamma_corrected_color);
        Textures::Instance()->InvalidateBindCache();
    }
}

DebugDrawElement::DebugDrawElement(const DDFlag& _flags):
    flags(_flags),
    fade_amount(0.0f),
    type(kUndefined),
    visible(true)
    {}

DebugDrawElement::~DebugDrawElement( )
{
}

bool DebugDrawElement::SetPosition(const vec3 &position)
{
    LOGW << "Function not implemented for current DebugDraw element" << std::endl;
    return false;
}

DebugDrawWire::DebugDrawWire( 
    const DDFlag& _flags ) : 
DebugDrawElement(_flags),
transform(1.0f),
color(vec4(1.0f,0,0,1.0f)),
opacity(1.0f)
{
    type = kWire;
}

DebugDrawWire::DebugDrawWire( 
    const vec4 &_color, 
    const DDFlag& _flags ) : 
DebugDrawElement(_flags),
transform(1.0f),
color(_color),
opacity(1.0f)
{
    type = kWire;
}

DebugDrawWire::DebugDrawWire( 
    const mat4 &_transform, 
    const vec4 &_color,
    const DDFlag& _flags ) : 
DebugDrawElement( _flags ),
transform(_transform),
color(_color),
opacity(1.0f)
{
    type = kWire;
}

DebugDrawWire::DebugDrawWire(
    const RC_VBOContainer& _vbo, 
    const mat4 &_transform, 
    const vec4 &_color,
    const DDFlag& _flags):
DebugDrawElement( _flags ),
vbo(_vbo),
transform(_transform),
color(_color),
opacity(1.0f)
{
    type = kWire;     
}

RC_VBOContainer DebugDrawWireBox::wire_box_vbo;

DebugDrawWireBox::DebugDrawWireBox( const vec3 &pos, 
                                    const vec3 &dim, 
                                    const vec4 &color ):
    DebugDrawWire(color, _DD_NO_FLAG)
{
    transform.AddTranslation( pos );

    mat4 scaleMatrix( 1.0f );
    scaleMatrix.SetScale( dim );
    transform *= scaleMatrix;

    if( !wire_box_vbo->valid() )
    {
        //TODO: fix cleanup by correct referencecounting.
        std::vector<vec3> vert_array;
        GetWireBoxVertArray(vert_array); 
        wire_box_vbo->Fill(kVBOFloat | kVBOStatic, vert_array.size() * sizeof(vert_array[0]), &vert_array[0]);
    }    
    vbo = wire_box_vbo;
    type = kWireBox;
}

RC_VBOContainer DebugDrawPoint::vbo;

DebugDrawPoint::DebugDrawPoint( const vec3 &_point, 
                                const vec4 &_color, 
                                const DDFlag& _flags ):
    DebugDrawElement(_flags),
    color(_color)
{
    if( !vbo->valid() )
    {
        vec3 origin(0.0f);
        vbo->Fill( kVBOFloat | kVBOStatic, sizeof(origin), (void*)&origin );
    }
    transform.SetTranslation( _point );
    type = kPoint;
}

void DebugDrawPoint::Draw()
{
    Graphics* graphics = Graphics::Instance();
    Shaders* shaders = Shaders::Instance();
    Camera* cam = ActiveCameras::Get();

    graphics->SetLineWidth(2);
    
    int shader_id = shaders->returnProgram("3d_color #COLOR_UNIFORM #NO_VELOCITY_BUF");
    shaders->setProgram(shader_id);
    int vert_attrib_id = shaders->returnShaderAttrib("vert_attrib", shader_id);
    shaders->SetUniformMat4("mvp", cam->GetProjMatrix() * cam->GetViewMatrix() * transform );
    shaders->SetUniformVec4("color_uniform", color);
    shaders->SetUniformFloat("opacity", (1.0f-fade_amount) ); 

    graphics->EnableVertexAttribArray(vert_attrib_id);

    vbo->Bind();
    glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, sizeof(float) * 3, (const void*)0);

    graphics->DrawArrays( GL_POINTS, 0, vbo->size()/sizeof(float)/3 );

    graphics->ResetVertexAttribArrays();
}

void DebugDrawAux::Update(float timestep) {
}

DebugDrawWireMesh::DebugDrawWireMesh( const RC_VBOContainer& _vbo, const mat4 &_transform, const vec4 &_color ):
DebugDrawWire( _vbo, _transform, _color, _DD_NO_FLAG )
{
    type = kWireMesh;         
}

DebugDrawWireMesh::~DebugDrawWireMesh() {
}

RC_VBOContainer DebugDrawCircle::circle_vert_vbo;

DebugDrawCircle::DebugDrawCircle( 
    const mat4 &_transform, 
    const vec4 &_color, 
    const DDFlag& _flags ):
DebugDrawWire( _transform, _color, _flags )
{
    static const unsigned numLines = 50;
    if(!circle_vert_vbo->valid()){
        std::vector<vec3> circle_verts;
        for(unsigned i=0; i<numLines; ++i){
            float angle = (float)i/(float)(numLines) * PI_f * 2.0f;
            float angle2 = (float)(i+1)/(float)(numLines) * PI_f * 2.0f;

            circle_verts.push_back(vec3(
                sinf(angle),
                cosf(angle),
                0.0f));

            circle_verts.push_back(vec3(
                sinf(angle2),
                cosf(angle2),
                0.0f));
        }
        circle_vert_vbo->Fill(kVBOFloat | kVBOStatic, sizeof(circle_verts[0]) * circle_verts.size(), (void*)&circle_verts[0]);
    }

    vbo = circle_vert_vbo;
    type = kCircle;
}

DebugDrawBillboard::DebugDrawBillboard(const TextureRef& ref, const vec3 &position, float scale, const vec4& color, AlphaMode mode)
:DebugDrawElement(_DD_NO_FLAG) {
    ref_ = ref;
    position_ = position;
    scale_ = scale;
    mode_ = mode;
    color_ = color;
    type = kBillboard;
}

bool DebugDrawBillboard::SetPosition(const vec3 &position) {
    position_ = position;
    return true;
}

void DebugDrawBillboard::SetScale(float scale) {
    scale_ = scale;
}

void DebugDrawBillboard::Draw() {
    DrawBillboard(ref_, position_, scale_, color_, mode_);
    Graphics::Instance()->setBlend(true);
}

void DebugDrawWire::Draw()
{
    Graphics* graphics = Graphics::Instance();

    if((flags & _DD_XRAY) == _DD_XRAY )
    {
        graphics->setDepthTest(false);
    }

    if((flags & _DD_COLOR) == _DD_COLOR )
    {
        DrawVertColor();
    } 
    else
    {
        DrawVert();
    }

    if((flags & _DD_XRAY) == _DD_XRAY )
    {
        graphics->setDepthTest(true);
    }
}

void DebugDrawWire::DrawVert()
{
    Graphics* graphics = Graphics::Instance();
    Shaders* shaders = Shaders::Instance();
    Camera* cam = ActiveCameras::Get();

    graphics->SetLineWidth(1);
    
    int shader_id = shaders->returnProgram("3d_color #COLOR_UNIFORM #NO_VELOCITY_BUF");
    shaders->setProgram(shader_id);
    int vert_attrib_id = shaders->returnShaderAttrib("vert_attrib", shader_id);
    shaders->SetUniformMat4("mvp", cam->GetProjMatrix() * cam->GetViewMatrix() * transform );
    shaders->SetUniformVec4("color_uniform", color);
    shaders->SetUniformFloat("opacity", opacity * (1.0f-fade_amount) ); 

    graphics->EnableVertexAttribArray(vert_attrib_id);

    vbo->Bind();
    glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, sizeof(float) * 3, (const void*)0);

    graphics->DrawArrays( GL_LINES, 0, vbo->size()/sizeof(float)/3 );

    graphics->ResetVertexAttribArrays();
}

void DebugDrawWire::DrawVertColor()
{
    Graphics* graphics = Graphics::Instance();
    Shaders* shaders = Shaders::Instance();
    Camera* cam = ActiveCameras::Get();

    graphics->SetLineWidth(1);
    
    int shader_id = shaders->returnProgram("3d_color #COLOR_ATTRIB #NO_VELOCITY_BUF");
    shaders->setProgram(shader_id);
    int vert_attrib_id = shaders->returnShaderAttrib("vert_attrib", shader_id);
    int color_attrib_id = shaders->returnShaderAttrib("color_attrib", shader_id);
    shaders->SetUniformMat4("mvp", cam->GetProjMatrix() * cam->GetViewMatrix() * transform );
    shaders->SetUniformFloat("opacity", opacity * (1.0f-fade_amount)); 

    graphics->EnableVertexAttribArray(vert_attrib_id);
    graphics->EnableVertexAttribArray(color_attrib_id);

    vbo->Bind();
    glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, sizeof(float) * 7, (const void*)0);
    glVertexAttribPointer(color_attrib_id, 4, GL_FLOAT, false, sizeof(float) * 7, (const void*)(sizeof(float)*3));

    graphics->DrawArrays( GL_LINES, 0, vbo->size()/sizeof(float)/7 );

    graphics->ResetVertexAttribArrays();
}

DebugDrawStippleMesh::DebugDrawStippleMesh(const RC_VBOContainer& _vbo,
                         const mat4 &_transform,
                         const vec4 &_color,
                         const DDFlag& _flags):
DebugDrawElement(_DD_NO_FLAG),
vbo(_vbo),
transform(_transform),
color(_color)
{
    type = kStippleMesh;
}

void DebugDrawStippleMesh::Draw()
{
    Shaders* shaders = Shaders::Instance();
    Camera* cam = ActiveCameras::Get();
    Graphics* graphics = Graphics::Instance();

    int shader_id = shaders->returnProgram("debug_fill #STIPPLING #NO_VELOCITY_BUF");
    shaders->setProgram(shader_id);
    int vert_attrib_id = shaders->returnShaderAttrib("vert_attrib", shader_id);

    shaders->SetUniformMat4("mvp", cam->GetProjMatrix() * cam->GetViewMatrix() * transform);
    shaders->SetUniformVec3("close_stipple_color", vec3(0.1f, 0.8f, 1.0f));
    shaders->SetUniformVec3("far_stipple_color", vec3(0.25f, 0.29f, 0.3f));
    shaders->SetUniformVec3("camera_forward", cam->GetFacing());
    shaders->SetUniformVec3("camera_position", cam->GetPos());

    graphics->EnableVertexAttribArray(vert_attrib_id);
    vbo->Bind();
    glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, 0, 0);

    graphics->DrawArrays(GL_TRIANGLES, 0, vbo->size() / (sizeof(float)*3));

    graphics->ResetVertexAttribArrays();
}

void DebugDrawRibbon::Draw() {
    if (g_debug_runtime_disable_debug_ribbon_draw) {
        return;
    }

    PROFILER_GPU_ZONE(g_profiler_ctx, "DebugDrawRibbon::Draw()");
    if(ribbon_points.size() < 2){
        return;
    }

    Graphics* graphics = Graphics::Instance();
    Shaders* shaders = Shaders::Instance();
    Camera* cam = ActiveCameras::Get();

    int shader_id = shaders->returnProgram("3d_color #COLOR_ATTRIB #FIRE");
    shaders->setProgram(shader_id);

    shaders->SetUniformMat4("mvp", cam->GetProjMatrix() * cam->GetViewMatrix());

    std::vector<vec3> right;
    right.resize(ribbon_points.size()-1);
    for(int i=0, len=ribbon_points.size()-1; i<len; ++i) {
        vec3 vec = ribbon_points[i+1].pos - ribbon_points[i].pos;
        right[i] = normalize(cross(cam->GetFacing(), vec));
    }

    std::vector<GLfloat> data;
    data.resize(7*ribbon_points.size()*2);
    int index = 0;
    for(int i=0, len=ribbon_points.size(); i<len; ++i) {
        vec3 temp_right;
        if(i==0) {
            temp_right = right[i];
        } else if(i==len-1){
            temp_right = right[i-1];
        } else {
            temp_right = normalize(right[i] + right[i-1]);
        }
        ribbon_points[i].color[3] = 0.0f;
        for(int j=0; j<4; ++j){
            data[index++] = ribbon_points[i].color[j];
        }
        for(int j=0; j<3; ++j){
            data[index++] = ribbon_points[i].pos[j] + temp_right[j] * ribbon_points[i].width;
        }
        ribbon_points[i].color[3] = 1.0f;
        for(int j=0; j<4; ++j){
            data[index++] = ribbon_points[i].color[j];
        }
        for(int j=0; j<3; ++j){
            data[index++] = ribbon_points[i].pos[j] - temp_right[j] * ribbon_points[i].width;
        }
    }

    vec2 viewport_dims;
    for(int i=0; i<2; ++i){
        viewport_dims[i] = (float)Graphics::Instance()->viewport_dim[i+2];
    }
    shaders->SetUniformVec2("viewport_dims",viewport_dims);
    shaders->SetUniformVec3("cam_pos",cam->GetPos());
    shaders->SetUniformFloat("time",game_timer.GetRenderTime());
    Textures::Instance()->bindTexture(Graphics::Instance()->screen_depth_tex, 5);

	static VBORingContainer data_vbo(V_MIBIBYTE, kVBODynamic | kVBOFloat);
    data_vbo.Fill(data.size() * sizeof(GLfloat), &data[0]);
    data_vbo.Bind();
    int vert_attrib_id = shaders->returnShaderAttrib("vert_attrib", shader_id);
    int color_attrib_id = shaders->returnShaderAttrib("color_attrib", shader_id);
    graphics->EnableVertexAttribArray(vert_attrib_id);
    graphics->EnableVertexAttribArray(color_attrib_id);
    glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, 7*sizeof(GLfloat), (const void*)(data_vbo.offset() + sizeof(GLfloat) * 4));
    glVertexAttribPointer(color_attrib_id, 4, GL_FLOAT, false, 7*sizeof(GLfloat), (const void*)data_vbo.offset());
    Graphics::Instance()->DrawArrays(GL_TRIANGLE_STRIP, 0, ribbon_points.size()*2);
    graphics->ResetVertexAttribArrays();

    /*glEnableVertexAttribArray(vert_attrib_id);
    vbo->Bind();
    glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, 0, 0);

    glDrawArrays(GL_TRIANGLES, 0, vbo->size() / (sizeof(float)*3));

    glDisableVertexAttribArray(vert_attrib_id);*/
}

DebugDrawRibbon::DebugDrawRibbon(const vec3 &start, const vec3 &end, const vec4 &start_color, const vec4 &end_color, const float start_width, const float end_width, const DDFlag& flags)
    :DebugDrawElement(flags)
{
    ribbon_points.resize(2);
    ribbon_points[0].pos = start;
    ribbon_points[1].pos = end;
    ribbon_points[0].color = start_color;
    ribbon_points[1].color = end_color;
    ribbon_points[0].width = start_width;
    ribbon_points[1].width = end_width;
    type = kRibbon;
}

DebugDrawRibbon::DebugDrawRibbon(const DDFlag& flags)
    :DebugDrawElement(flags)
{
    type = kRibbon;
}

void DebugDrawRibbon::AddPoint(const vec3 &pos, const vec4 &color, float width) {
    RibbonPoint point;
    point.pos = pos;
    point.color = color;
    point.width = width;
    ribbon_points.push_back(point);
}

void TempLines::Draw() {
	PROFILER_GPU_ZONE(g_profiler_ctx, "Draw TempLines");
    if(!verts.empty()){
        vbo.Fill(verts.size() * sizeof(verts[0]), &verts[0]);

        Graphics* graphics = Graphics::Instance();
        Shaders* shaders = Shaders::Instance();
        Camera* cam = ActiveCameras::Get();

        graphics->SetLineWidth(1);

        int shader_id = shaders->returnProgram("3d_color #COLOR_ATTRIB #NO_VELOCITY_BUF");
        shaders->setProgram(shader_id);
        int vert_attrib_id = shaders->returnShaderAttrib("vert_attrib", shader_id);
        int color_attrib_id = shaders->returnShaderAttrib("color_attrib", shader_id);
        shaders->SetUniformMat4("mvp", cam->GetProjMatrix() * cam->GetViewMatrix() );
        shaders->SetUniformFloat("opacity", 1.0f); 

        graphics->EnableVertexAttribArray(vert_attrib_id);
        graphics->EnableVertexAttribArray(color_attrib_id);

        vbo.Bind();
        glVertexAttribPointer(vert_attrib_id, 3, GL_FLOAT, false, sizeof(float) * 7, (const void*)(vbo.offset()));
        glVertexAttribPointer(color_attrib_id, 4, GL_FLOAT, false, sizeof(float) * 7, (const void*)(vbo.offset()+sizeof(float)*3));

        graphics->DrawArrays( GL_LINES, 0, vbo.size()/sizeof(float)/7 );

        graphics->ResetVertexAttribArrays();
    }
}
