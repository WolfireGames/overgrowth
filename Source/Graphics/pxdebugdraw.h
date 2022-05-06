//-----------------------------------------------------------------------------
//           Name: pxdebugdraw.h
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
#pragma once

#include <Math/vec3.h>
#include <Math/vec4.h>
#include <Math/mat4.h>

#include <Graphics/textureref.h>
#include <Graphics/glstate.h>
#include <Graphics/vbocontainer.h>
#include <Graphics/vboringcontainer.h>

#include <Internal/referencecounter.h>
#include <Asset/Asset/texture.h>

#include <opengl.h>

#include <queue>
#include <map>
#include <string>

enum DDLifespan {
    _delete_on_draw,
    _delete_on_update,
    _fade,
    _persistent
};

enum DDFlag {
    _DD_NO_FLAG = 0,
    //Request disabled depth buffer.
    _DD_XRAY = (1<<0),
    _DD_STIPPLE = (1<<1),
    //Means there is a vec4(color) element interleaved in vbo after verts
    _DD_COLOR = (1<<2),
    //Whether or not text should be rendered at a set size or in-world.
    _DD_SCREEN_SPACE = (1<<3),
    //Dashed line segment.
    _DD_DASHED = (1<<4)
};

enum DDElement {
    kUndefined,
    kWire,
    kText,
    kBillboard,
    kRibbon,
    kLine,
    kLines,
    kWireMesh,
    kStippleMesh,
    kPoint,
    kWireSphere,
    kWireBox,
    kCircle,
    kWireCylinder
};

class DebugDrawElement {
        DDLifespan lifespan;
    protected:
        DDFlag flags;
        
    public:
        bool visible;
        DDElement type;
        float fade_amount;
        DebugDrawElement(const DDFlag& _flags ); 
        
        virtual ~DebugDrawElement(); 
        virtual void Draw()=0; 

        //Move the origin of the object.
        virtual bool SetPosition(const vec3& pos);

        inline void SetLifespan(const DDLifespan &_lifespan) 
            {lifespan = _lifespan;}
        inline const DDLifespan& GetLifespan() 
            {return lifespan;}
};

class DebugDrawWire : public DebugDrawElement {

private:
    void DrawVert();
    void DrawVertColor();

protected:
    RC_VBOContainer vbo;
    mat4 transform;
    vec4 color;
    float opacity;

public:
    void DecreaseOpacity(float how_much);
    bool IsInvisible();
    DebugDrawWire(const DDFlag& _flags);
    DebugDrawWire(const vec4 &_color, const DDFlag& _flags);
    DebugDrawWire(const mat4 &_transform, const vec4 &_color, const DDFlag& _flags);
    DebugDrawWire(const RC_VBOContainer& _vbo, const mat4 &_transform, const vec4 &_color, const DDFlag& _flags);
    void Draw() override;
};

class LabelText;
class DebugDrawText : public DebugDrawElement {
        static RC_VBOContainer vbo;
        static const int kBufSize = 256;
        char text[kBufSize];
        vec4 color;
        mat4 transform;
        float scale;
    public:
        void Draw() override;
        DebugDrawText(const vec3 &_position,
                      const float &_scale,
                      const std::string &_content,
                      const DDFlag& _flags,
                      const vec4 & _color);
        ~DebugDrawText() override;
        bool SetPosition(const vec3& position) override;
};


class DebugDrawBillboard: public DebugDrawElement {
public:
    void Draw() override;
    bool SetPosition(const vec3 &position) override;
    void SetScale(float scale);
    DebugDrawBillboard(const TextureRef& ref, const vec3 &position, float scale, const vec4& color, AlphaMode mode);
private: 
    TextureRef ref_;
    vec3 position_;
    vec4 color_;
    float scale_;
    AlphaMode mode_;
};

class DebugDrawRibbon: public DebugDrawElement {
public:
    void Draw() override;
    DebugDrawRibbon(const DDFlag& flags);
    DebugDrawRibbon(const vec3 &start, 
                    const vec3 &end, 
                    const vec4 &start_color, 
                    const vec4 &end_color,
                    const float start_width, 
                    const float end_width,
                    const DDFlag& flags);
    void AddPoint( const vec3 &pos, const vec4 &color, float width );
private: 
    struct RibbonPoint {
        vec3 pos;
        vec4 color;
        float width;
    };
    std::vector<RibbonPoint> ribbon_points;
};

class DebugDrawLine : public DebugDrawWire {
public:
    DebugDrawLine(const vec3 &start, 
        const vec3 &end, 
        const vec4 &start_color,
        const vec4 &end_color,
        const DDFlag& flags = _DD_NO_FLAG);
};

class DebugDrawLines : public DebugDrawWire {
public:
    DebugDrawLines(
        const std::vector<float> &vertices,
        const std::vector<unsigned> &indices,
        const vec4 &color,
        const DDFlag& flags = _DD_NO_FLAG);

    DebugDrawLines(
        const std::vector<vec3> &vertices,
        const vec4 &color,
        const DDFlag& flags = _DD_NO_FLAG);
};

class DebugDrawWireMesh : public DebugDrawWire {
public:
    DebugDrawWireMesh(const RC_VBOContainer& _vbo,
                      const mat4 &_transform,
                      const vec4 &_color);
    ~DebugDrawWireMesh() override;
};

class DebugDrawStippleMesh : public DebugDrawElement {

protected:
    RC_VBOContainer vbo;
    mat4 transform;
    vec4 color;
public:
    DebugDrawStippleMesh(const RC_VBOContainer& _vbo,
                         const mat4 &_transform,
                         const vec4 &_color,
                         const DDFlag& _flags);
    void Draw() override;
};


class DebugDrawPoint : public DebugDrawElement {
        static RC_VBOContainer vbo;
        vec4 color;
        mat4 transform; 
    public:
        void Draw() override;
        DebugDrawPoint(const vec3 &point, 
                      const vec4 &color,
                      const DDFlag& _flags = _DD_NO_FLAG);
};

class DebugDrawWireSphere : public DebugDrawWire {
    private:
        static RC_VBOContainer wire_sphere_vbo;
    public:
        DebugDrawWireSphere(const mat4 _transform,
                            const vec4 &_color );

        DebugDrawWireSphere(const vec3 &position, 
                            const float radius,
                            const vec3 &scale = vec3(1.0f,1.0f,1.0f),
                            const vec4 &color = vec4(1.0f,1.0f,1.0f,1.0f));
        ~DebugDrawWireSphere() override;
};

class DebugDrawCircle : public DebugDrawWire {
private:
    static RC_VBOContainer circle_vert_vbo;
public:
    DebugDrawCircle(const mat4 &transform,
                    const vec4 &color = vec4(1.0f,1.0f,1.0f,1.0f),
                    const DDFlag& flags = _DD_NO_FLAG);
};

class DebugDrawWireCylinder : public DebugDrawWire {
    private:
        static RC_VBOContainer wire_cylinder_vbo;
    public:
        DebugDrawWireCylinder(const vec3 &position, 
                              const float radius,
                              const float height,
                              const vec4 &color);
        ~DebugDrawWireCylinder() override;
};


class DebugDrawWireBox : public DebugDrawWire {
    private:
        static RC_VBOContainer wire_box_vbo;
    public:
        DebugDrawWireBox(const vec3 &position, 
                         const vec3 &dimensions,
                         const vec4 &color);
};

typedef std::map<int,DebugDrawElement*> DebugDrawElementMap;

class TempLines {
public:
    TempLines():vbo(1024*1024, kVBOStream | kVBOFloat){}
    VBORingContainer vbo;
    std::vector<float> verts;
    void Draw();
};

class DebugDraw {
private:
    std::map<std::string, RC_VBOContainer> mesh_edges_map;
    TempLines delete_on_update;
    TempLines delete_on_draw;

    std::queue<int> free_ids;
    int id_index;
    DebugDrawElementMap elements;
    int AddElement( DebugDrawElement* object, const DDLifespan lifespan);
public:
    void Draw();
    void Update(float timestep);
    void Remove(int);
    DebugDrawElement* GetElement(int id);
    int AddLine(const vec3 &start, 
        const vec3 &end, 
        const vec4 &color, 
        const DDLifespan lifespan,
        const DDFlag& flags = _DD_NO_FLAG);
    int AddLine(const vec3 &start, 
        const vec3 &end, 
        const vec4 &start_color, 
        const vec4 &end_color, 
        const DDLifespan lifespan,
        const DDFlag& flags = _DD_NO_FLAG);
    int AddRibbon(const vec3 &start, 
        const vec3 &end, 
        const vec4 &start_color, 
        const vec4 &end_color, 
        const float start_width,
        const float end_width, 
        const DDLifespan lifespan,
        const DDFlag& flags = _DD_NO_FLAG);
    int AddRibbon( const DDLifespan lifespan,
        const DDFlag& flags = _DD_NO_FLAG);
    int AddPoint( const vec3 &point, 
        const vec4 &color, 
        const DDLifespan lifespan, 
        const DDFlag& flags);
    int AddWireSphere(const vec3 &position, 
        const float radius, 
        const vec4 &color, 
        const DDLifespan lifespan);
    int AddWireScaledSphere(const vec3 &position, 
        const float radius, 
        const vec3 &scale,
        const vec4 &color, 
        const DDLifespan lifespan);
    int AddTransformedWireScaledSphere(const mat4 &transform,
        const vec4 &color, 
        const DDLifespan lifespan);
    int AddWireCylinder(const vec3 &position, 
        const float radius, 
        const float height, 
        const vec4 &color, 
        const DDLifespan lifespan);
    int AddWireBox(const vec3 &position, 
        const vec3 &dimensions, 
        const vec4 &color, 
        const DDLifespan lifespan);
    int AddBillboard(const TextureRef &ref, 
        const vec3 &position, 
        float scale, 
        const vec4 &color, 
        AlphaMode mode, 
        const DDLifespan lifespan);
    int AddWireMesh(const std::string &path, 
        const mat4 &transform,
        const vec4 &color, 
        const DDLifespan lifespan);
    int AddLineObject(const RC_VBOContainer &vbo,
        const mat4 &transform,
        const vec4 &color, 
        const DDLifespan lifespan);
    int AddStippleMesh(const RC_VBOContainer &vbo,
        const mat4 &transform,
        const vec4 &color, 
        const DDLifespan lifespan);
    int AddCircle(const mat4 &transform,
                  const vec4 &color, 
                  const DDLifespan lifespan,
                  const DDFlag& flags = _DD_NO_FLAG);
    int AddText(const vec3 &position,
        const std::string &content, 
        const float& scale,
        const DDLifespan lifespan,
        const DDFlag& flags,
        const vec4& color = vec4(1.0f, 1.0f, 1.0f, 1.0f));
    bool SetPosition( int id,
        const vec3 &position );
    bool SetColor( int id,
        const vec4 &color );
    bool SetVisible( int id, bool visible );
    int AddLines( 
        const std::vector<float> &vertices, 
        const std::vector<unsigned> &indices, 
        const vec4 &color, 
        const DDLifespan lifespan, 
        const DDFlag& flags );
    int AddLines( 
        const std::vector<vec3> &vertices, 
        const vec4 &color, 
        const DDLifespan lifespan, 
        const DDFlag& flags );
    void Dispose();
    DebugDraw();
    ~DebugDraw(); 

    static DebugDraw* Instance() {
        static DebugDraw instance;
        return &instance;
    }
};

class DebugDrawAux {
public:
    void Update(float timestep);

    bool visible_sound_spheres;

    static DebugDrawAux* Instance() {
        static DebugDrawAux instance;
        return &instance;
    }
};

DDLifespan LifespanFromInt(int lifespan_int);
