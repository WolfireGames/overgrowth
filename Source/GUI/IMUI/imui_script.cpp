//-----------------------------------------------------------------------------
//           Name: imui_script.cpp
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
#include "imui_script.h"

#include <GUI/IMUI/imgui.h>
#include <GUI/IMUI/imui.h>
#include <GUI/IMUI/im_container.h>
#include <GUI/IMUI/im_divider.h>
#include <GUI/IMUI/im_element.h>
#include <GUI/IMUI/im_image.h>
#include <GUI/IMUI/im_message.h>
#include <GUI/IMUI/im_selection_list.h>
#include <GUI/IMUI/im_spacer.h>
#include <GUI/IMUI/im_text.h>
#include <GUI/IMUI/im_behaviors.h>
#include <GUI/IMUI/imui_state.h>
#include <GUI/IMUI/im_support.h>
#include <GUI/IMUI/im_tween.h>

#include <Scripting/angelscript/ascontext.h>
#include <Scripting/angelscript/add_on/scriptarray/scriptarray.h>

static void IMUIContextDefaultConstructor(void *self) {
    new(self) IMUIContext();
}

static void IMUIContextCopyConstructor(IMUIImage *self, const IMUIContext& other ) {
    new(self) IMUIContext( other );
}

static const IMUIImage& IMUIContextAssign(IMUIImage *self, const IMUIImage& other) {
    return (*self) = other;
}

static void IMUIContextDestructor(IMUIContext *self ) {
    self->~IMUIContext();
}


static void IMUIImageDefaultConstructor(IMUIImage *self) {
    new(self) IMUIImage();
}

static void IMUIImageFileNameConstructor(IMUIImage *self, std::string filename) {
    new(self) IMUIImage( filename );
}

static void IMUIImageCopyConstructor(IMUIImage *self, const IMUIImage& other ) {
    new(self) IMUIImage( other );
}

static const IMUIImage& IMUIImageAssign(IMUIImage *self, const IMUIImage& other) {
    return (*self) = other;
}

static void IMUIImageDestructor(IMUIImage *self ) {
    self->~IMUIImage();
}

static void IMUITextDefaultConstructor(IMUIText *self) {
    new(self) IMUIText();
}

static void IMUITextCopyConstructor(IMUIText *self, const IMUIText& other ) {
    new(self) IMUIText( other );
}

static const IMUIText& IMUITextAssign(IMUIText *self, const IMUIText& other) {
    return (*self) = other;
}

static void IMUITextDestructor(IMUIText *self ) {
    self->~IMUIText();
}


float ASUNDEFINEDSIZE = UNDEFINEDSIZE;
int ASUNDEFINEDSIZEI = UNDEFINEDSIZEI;


// Example opCast behaviour
template<class A, class B>
B* refCast(A* a)
{
    // If the handle already is a null handle, then just return the null handle
    if( !a ) return 0;
    // Now try to dynamically cast the pointer to the wanted type
    B* b = dynamic_cast<B*>(a);
    if( b != 0 )
    {
        // Since the cast was made, we need to increase the ref counter for the returned handle
        b->AddRef();
    }

    return b;
}

static CScriptArray* AS_GetFloatingContents(IMContainer* element) {
    asIScriptContext *ctx = asGetActiveContext();
    asIScriptEngine *engine = ctx->GetEngine();
    asITypeInfo *arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<IMElement@>"));
    CScriptArray *array = CScriptArray::Create(arrayType, (asUINT)0);

    std::vector<IMElement*> vals = element->getFloatingContents();

    array->Reserve(vals.size());

    for(auto & val : vals) {
        array->InsertLast((void*)&val);
    }

    return array;
}

static CScriptArray* AS_GetContainers(IMDivider* element) {
    asIScriptContext *ctx = asGetActiveContext();
    asIScriptEngine *engine = ctx->GetEngine();
    asITypeInfo *arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<IMContainer@>"));
    CScriptArray *array = CScriptArray::Create(arrayType, (asUINT)0);

    unsigned int count = element->getContainerCount();
    array->Reserve(count);

    for( unsigned i = 0; i < count; i++ ) {
        IMContainer* container = element->getContainerAt(i);
        container->Release();
        array->InsertLast((void*)&container);
    }

    return array;
}

template<class T>
void registerASIMElement( ASContext *ctx, std::string const& className ) {

    ctx->RegisterObjectType(className.c_str(), 0, asOBJ_REF );

    ctx->RegisterObjectProperty(className.c_str(), "CItem controllerItem", asOFFSET(T, controllerItem));

    ctx->RegisterObjectBehaviour(className.c_str(), asBEHAVE_ADDREF, "void f()", asMETHOD(T,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour(className.c_str(), asBEHAVE_RELEASE, "void f()", asMETHOD(T,Release), asCALL_THISCALL);

    ctx->RegisterObjectMethod(className.c_str(), "string getElementTypeName()", asMETHOD(T, getElementTypeName), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setColor( vec4 _color )", asMETHOD(T, setColor), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "vec4 getColor()", asMETHOD(T, getColor), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "vec4 getBaseColor()", asMETHOD(T, getBaseColor), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setEffectColor( vec4 _color )", asMETHOD(T, setEffectColor), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "vec4 getEffectColor()", asMETHOD(T, getEffectColor), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void clearColorEffect()", asMETHOD(T, clearColorEffect), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setR( float value )", asMETHOD(T, setR), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "float getR()", asMETHOD(T, getR), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setG( float value )", asMETHOD(T, setG), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "float getG()", asMETHOD(T, getG), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setB( float value )", asMETHOD(T, setB), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "float getB()", asMETHOD(T, getB), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setAlpha( float value )", asMETHOD(T, setAlpha), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "float getAlpha()", asMETHOD(T, getAlpha), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setEffectR( float value )", asMETHOD(T, setEffectR), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "float getEffectR()", asMETHOD(T, getEffectR), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void clearEffectR()", asMETHOD(T, clearEffectR), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setEffectG( float value )", asMETHOD(T, setEffectG), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "float getEffectG()", asMETHOD(T, getEffectG), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void clearEffectG()", asMETHOD(T, clearEffectG), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setEffectB( float value )", asMETHOD(T, setEffectB), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "float getEffectB()", asMETHOD(T, getEffectB), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void clearEffectB()", asMETHOD(T, clearEffectB), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setEffectAlpha( float value )", asMETHOD(T, setEffectAlpha), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "float getEffectAlpha()", asMETHOD(T, getEffectAlpha), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void clearEffectAlpha()", asMETHOD(T, clearEffectAlpha), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void showBorder( bool _border = true )", asMETHOD(T, showBorder), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setBorderSize( float _borderSize )", asMETHOD(T, setBorderSize), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setBorderColor( vec4 _color )", asMETHOD(T, setBorderColor), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "vec4 getBorderColor()", asMETHOD(T, getBorderColor), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setBorderR( float value )", asMETHOD(T, setBorderR), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "float getBorderR()", asMETHOD(T, getBorderR), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setBorderG( float value )", asMETHOD(T, setBorderG), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "float getBorderG()", asMETHOD(T, getBorderG), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setBorderB( float value )", asMETHOD(T, setBorderB), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "float getBorderB()", asMETHOD(T, getBorderB), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setBorderAlpha( float value )", asMETHOD(T, setBorderAlpha), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "float getBorderAlpha()", asMETHOD(T, getBorderAlpha), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setZOrdering( int z )", asMETHOD(T, setZOrdering), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "int getZOrdering()", asMETHOD(T, getZOrdering), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void renderAbove( IMElement@ element )", asMETHOD(T, renderAbove), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void renderBelow( IMElement@ element )", asMETHOD(T, renderBelow), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setVisible( bool _show )", asMETHOD(T, setVisible), asCALL_THISCALL);
	ctx->RegisterObjectMethod(className.c_str(), "void setClip( bool _clip )", asMETHOD(T, setClip), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "vec2 getScreenPosition()", asMETHOD(T, getScreenPosition), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void addUpdateBehavior( IMUpdateBehavior@ behavior, const string &in behaviorName )", asMETHOD(T, addUpdateBehavior), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "bool removeUpdateBehavior( const string &in behaviorName )", asMETHOD(T, removeUpdateBehavior), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "bool hasUpdateBehavior(const string &in behaviorName)", asMETHOD(T, hasUpdateBehavior), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void clearUpdateBehaviors()", asMETHOD(T, clearUpdateBehaviors), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void addMouseOverBehavior( IMMouseOverBehavior@ behavior, const string &in behaviorName )", asMETHOD(T, addMouseOverBehavior), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "bool removeMouseOverBehavior( const string &in behaviorName )", asMETHOD(T, removeMouseOverBehavior), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void clearMouseOverBehaviors()", asMETHOD(T, clearMouseOverBehaviors), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void addLeftMouseClickBehavior( IMMouseClickBehavior@ behavior, const string &in behaviorName )", asMETHOD(T, addLeftMouseClickBehavior), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "bool removeLeftMouseClickBehavior( const string &in behaviorName )", asMETHOD(T, removeLeftMouseClickBehavior), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void clearLeftMouseClickBehaviors()", asMETHOD(T, clearLeftMouseClickBehaviors), asCALL_THISCALL);

    ctx->RegisterObjectMethod(className.c_str(), "void sendMouseDownToChildren( bool send = true )", asMETHOD(T, sendMouseDownToChildren), asCALL_THISCALL);

    ctx->RegisterObjectMethod(className.c_str(), "void sendMouseOverToChildren( bool send = true )", asMETHOD(T, sendMouseOverToChildren), asCALL_THISCALL);

    ctx->RegisterObjectMethod(className.c_str(), "void setPauseBehaviors( bool pause )", asMETHOD(T, setPauseBehaviors), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "bool isMouseOver()", asMETHOD(T, isMouseOver), asCALL_THISCALL);

    ctx->RegisterObjectMethod(className.c_str(), "void setName( const string &in _name )", asMETHOD(T, setName), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "string getName()", asMETHOD(T, getName), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setPadding( float U, float D, float L, float R)", asMETHOD(T, setPadding), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setPaddingU( float paddingSize )", asMETHOD(T, setPaddingU), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setPaddingD( float paddingSize )", asMETHOD(T, setPaddingD), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setPaddingL( float paddingSize )", asMETHOD(T, setPaddingL), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setPaddingR( float paddingSize )", asMETHOD(T, setPaddingR), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setDisplacement( vec2 newDisplacement = vec2(0,0) )", asMETHOD(T, setDisplacement), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setDisplacementX( float newDisplacement = 0 )", asMETHOD(T, setDisplacementX), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setDisplacementY( float newDisplacement = 0 )", asMETHOD(T, setDisplacementY), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "vec2 getDisplacement( vec2 newDisplacement = vec2(0,0) )", asMETHOD(T, getDisplacement), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "float getDisplacementX()", asMETHOD(T, getDisplacementX), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "float getDisplacementY()", asMETHOD(T, getDisplacementY), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setDefaultSize( vec2 newDefault )", asMETHOD(T, setDefaultSize), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "vec2 getDefaultSize()", asMETHOD(T, getDefaultSize), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setSize( const vec2 _size )", asMETHOD(T, setSize), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setSizeX( const float x )", asMETHOD(T, setSizeX), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void setSizeY( const float y )", asMETHOD(T, setSizeY), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "vec2 getSize()", asMETHOD(T, getSize), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "float getSizeX()", asMETHOD(T, getSizeX), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "float getSizeY()", asMETHOD(T, getSizeY), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "void sendMessage( IMMessage@ theMessage )", asMETHOD(T, sendMessage), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "IMElement@ findElement( const string &in elementName )", asMETHOD(T, findElement), asCALL_THISCALL);
    ctx->RegisterObjectMethod(className.c_str(), "IMElement@ getParent()", asMETHOD(T, getParent), asCALL_THISCALL);

    ctx->RegisterObjectMethod(className.c_str(), "void setMouseOver(bool hovered)", asMETHOD(T, setScriptMouseOver), asCALL_THISCALL);

    std::string opCastStr = className + "@ opCast()";

    // Expose the inheritance
    ctx->RegisterObjectMethod("IMElement", opCastStr.c_str(), asFUNCTION((refCast<IMElement,T>)), asCALL_CDECL_OBJLAST);
    ctx->RegisterObjectMethod(className.c_str(), "IMElement@ opImplCast()", asFUNCTION((refCast<T,IMElement>)), asCALL_CDECL_OBJLAST);

}

static int context_instance_id_counter = 1;
static std::map<int,IMUIContext*> context_instance;

extern "C" {

    static int AS_CreateIMUIContext() {
        int id = context_instance_id_counter++;
        IMUIContext* instance = new IMUIContext();
        context_instance[id] = instance;
        return id;
    }

    static IMUIContext* AS_GetIMUIContext(int id) {
        std::map<int,IMUIContext*>::iterator it = context_instance.find(id);
        if( it != context_instance.end() ) {
            return it->second;
        } else {
            return NULL;
        }
    }

    static void AS_DisposeIMUIContext(int id) {
        std::map<int,IMUIContext*>::iterator it = context_instance.find(id);
        if( it != context_instance.end() ) {
            delete it->second;
            context_instance.erase(it);
        }
    }
    static void AS_Context_queueImage( IMUIContext* t, IMUIImage* img ) {
        t->queueImage(*img);
    }

    static void AS_Context_queueText( IMUIContext* t, IMUIText* txt ) {
        t->queueText(*txt);
    }

    static IMGUI* AS_CreateIMGUI() {
        return new IMGUI();
    }
}

void AttachIMUI( ASContext *ctx ) {
    ctx->RegisterEnum("UIState");
    ctx->RegisterEnumValue("UIState", "kNothing", IMUIContext::kNothing);
    ctx->RegisterEnumValue("UIState", "kHot", IMUIContext::kHot);
    ctx->RegisterEnumValue("UIState", "kActive", IMUIContext::kActive);

    ctx->RegisterEnum("UIMouseState");
    ctx->RegisterEnumValue("UIMouseState", "kMouseUp", IMUIContext::kMouseUp);
    ctx->RegisterEnumValue("UIMouseState", "kMouseDown", IMUIContext::kMouseDown);
    ctx->RegisterEnumValue("UIMouseState", "kMouseStillUp", IMUIContext::kMouseStillUp);
    ctx->RegisterEnumValue("UIMouseState", "kMouseStillDown", IMUIContext::kMouseStillDown);

    ctx->RegisterEnum("TextFlags");
    ctx->RegisterEnumValue("TextFlags", "kTextShadow", TextAtlasRenderer::kTextShadow );

    /**
     * IMUIImage
     **/

    ctx->RegisterObjectType("IMUIImage", sizeof(IMUIImage), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK );

    // from IMUIRenderable

    ctx->RegisterObjectMethod("IMUIImage", "void setPosition( vec3 newPos )", asMETHOD(IMUIImage, setPosition), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMUIImage", "void setColor( vec4 newColor )", asMETHOD(IMUIImage, setColor), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMUIImage", "void setRotation( float newRotation )", asMETHOD(IMUIImage, setRotation), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMUIImage", "void setClipping( vec2 offset, vec2 size )", asMETHOD(IMUIImage, setClipping), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMUIImage", "void disableClipping()", asMETHOD(IMUIImage, disableClipping), asCALL_THISCALL);

    // from IMUIImage
    ctx->RegisterObjectBehaviour("IMUIImage", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(IMUIImageDefaultConstructor),  asCALL_CDECL_OBJFIRST );

    ctx->RegisterObjectBehaviour("IMUIImage", asBEHAVE_CONSTRUCT, "void f( string filename )", asFUNCTION(IMUIImageFileNameConstructor),  asCALL_CDECL_OBJFIRST );

    ctx->RegisterObjectBehaviour("IMUIImage", asBEHAVE_CONSTRUCT, "void f( const IMUIImage &in other )", asFUNCTION(IMUIImageCopyConstructor),  asCALL_CDECL_OBJFIRST );

    ctx->RegisterObjectMethod("IMUIImage", "IMUIImage& opAssign(const IMUIImage &in other)", asFUNCTION(IMUIImageAssign), asCALL_CDECL_OBJFIRST);

    ctx->RegisterObjectBehaviour("IMUIImage", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(IMUIImageDestructor),  asCALL_CDECL_OBJFIRST );

    ctx->RegisterObjectMethod("IMUIImage", "bool loadImage( string filename )", asMETHOD(IMUIImage, loadImage), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMUIImage", "bool isValid() const", asMETHOD(IMUIImage, isValid), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMUIImage", "float getTextureWidth() const", asMETHOD(IMUIImage, getTextureWidth), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMUIImage", "float getTextureHeight() const", asMETHOD(IMUIImage, getTextureHeight), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMUIImage", "void setRenderOffset( vec2 offset, vec2 size )", asMETHOD(IMUIImage, setRenderOffset), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMUIImage", "void setRenderSize( vec2 size )", asMETHOD(IMUIImage, setRenderSize), asCALL_THISCALL);

    /**
     * IMUIText
     **/

    ctx->RegisterObjectType("IMUIText", sizeof(IMUIText), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK );

    // from IMUIRenderable

    ctx->RegisterObjectMethod("IMUIText", "void setPosition( vec3 newPos )", asMETHOD(IMUIText, setPosition), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMUIText", "void setColor( vec4 newColor )", asMETHOD(IMUIText, setColor), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMUIText", "void setRotation( float newRotation )", asMETHOD(IMUIText, setRotation), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMUIText", "void setClipping( vec2 offset, vec2 size )", asMETHOD(IMUIText, setClipping), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMUIText", "void disableClipping()", asMETHOD(IMUIText, disableClipping), asCALL_THISCALL);

    // from IMUIText
    ctx->RegisterObjectBehaviour("IMUIText", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(IMUITextDefaultConstructor),  asCALL_CDECL_OBJFIRST );

    ctx->RegisterObjectBehaviour("IMUIText", asBEHAVE_CONSTRUCT, "void f( const IMUIText &in other )", asFUNCTION(IMUITextCopyConstructor),  asCALL_CDECL_OBJFIRST );

    ctx->RegisterObjectMethod("IMUIText", "IMUIText& opAssign(const IMUIText &in other)", asFUNCTION(IMUITextAssign), asCALL_CDECL_OBJFIRST);

    ctx->RegisterObjectBehaviour("IMUIText", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(IMUITextDestructor),  asCALL_CDECL_OBJFIRST );

    ctx->RegisterObjectMethod("IMUIText", "vec2 getDimensions() const", asMETHOD(IMUIText, getDimensions), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMUIText", "vec2 getBoundingBoxDimensions() const", asMETHOD(IMUIText, getBoundingBoxDimensions), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMUIText", "void setText( const string &in _text )", asMETHOD(IMUIText, setText), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMUIText", "void setRenderFlags( int newFlags )", asMETHOD(IMUIText, setRenderFlags), asCALL_THISCALL);

    /**
     * IMUIContext
     **/

    //ctx->RegisterObjectType("IMUIContext", sizeof(IMUIContext), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK );
    ctx->RegisterObjectType("IMUIContext", 0, asOBJ_REF | asOBJ_NOCOUNT );

    ctx->RegisterGlobalFunction(
        "int CreateIMUIContext()",
        asFUNCTION(AS_CreateIMUIContext),
        asCALL_CDECL
    );

    ctx->RegisterGlobalFunction(
        "IMUIContext@ GetIMUIContext(int id)",
        asFUNCTION(AS_GetIMUIContext),
        asCALL_CDECL
    );

    ctx->RegisterGlobalFunction(
        "void DisposeIMUIContext(int id)",
        asFUNCTION(AS_DisposeIMUIContext),
        asCALL_CDECL
    );

    ctx->RegisterObjectMethod(
        "IMUIContext",
        "void UpdateControls()",
        asMETHOD(IMUIContext, UpdateControls),
        asCALL_THISCALL);

    ctx->RegisterObjectMethod(
        "IMUIContext",
        "bool DoButton(int, vec2, vec2, UIState &out)",
        asMETHOD(IMUIContext, DoButton),
        asCALL_THISCALL);

    ctx->RegisterObjectMethod(
        "IMUIContext",
        "void Init()",
        asMETHOD(IMUIContext, Init),
        asCALL_THISCALL);

    ctx->RegisterObjectMethod(
          "IMUIContext",
          "vec2 getMousePosition()",
          asMETHOD(IMUIContext, getMousePosition),
          asCALL_THISCALL);

    ctx->RegisterObjectMethod(
          "IMUIContext",
          "UIMouseState getLeftMouseState()",
          asMETHOD(IMUIContext, getLeftMouseState),
          asCALL_THISCALL);

    ctx->RegisterObjectMethod(
          "IMUIContext",
          "void queueImage( IMUIImage &in newImage  )",
          asMETHOD(IMUIContext, queueImage),
          asCALL_THISCALL);

    ctx->RegisterObjectMethod(
          "IMUIContext",
          "void queueText( IMUIText &in newText )",
          asMETHOD(IMUIContext, queueText),
          asCALL_THISCALL);

    ctx->RegisterObjectMethod(
          "IMUIContext",
          "IMUIText makeText( string &in fontName, int size, int fontFlags, int renderFlags = 0 )",
          asMETHOD(IMUIContext, makeText),
          asCALL_THISCALL);

    ctx->RegisterObjectMethod(
          "IMUIContext",
          "void render()",
          asMETHOD(IMUIContext, render),
          asCALL_THISCALL);

    ctx->RegisterObjectMethod(
          "IMUIContext",
          "void clearTextAtlases()",
          asMETHOD(IMUIContext, clearTextAtlases),
          asCALL_THISCALL);

    /*******
     *
     * IMGUI elements
     *
     */

    /*******
     *
     * Constants
     *
     */

    ctx->RegisterGlobalProperty("const float UNDEFINEDSIZE", &ASUNDEFINEDSIZE);
	ctx->RegisterGlobalProperty("const int UNDEFINEDSIZEI", &ASUNDEFINEDSIZEI);

    ctx->RegisterEnum("DividerOrientation");
    ctx->RegisterEnumValue("DividerOrientation", "DOVertical", DOVertical);
    ctx->RegisterEnumValue("DividerOrientation", "DOHorizontal", DOHorizontal);

    ctx->RegisterEnum("ContainerAlignment");
    ctx->RegisterEnumValue("ContainerAlignment", "CATop", CATop);
    ctx->RegisterEnumValue("ContainerAlignment", "CALeft", CALeft);
    ctx->RegisterEnumValue("ContainerAlignment", "CACenter", CACenter);
    ctx->RegisterEnumValue("ContainerAlignment", "CARight", CARight);
    ctx->RegisterEnumValue("ContainerAlignment", "CABottom", CABottom);

    ctx->RegisterEnum("ExpansionPolicy");
    ctx->RegisterEnumValue("ExpansionPolicy", "ContainerExpansionStatic", ContainerExpansionStatic);
    ctx->RegisterEnumValue("ExpansionPolicy", "ContainerExpansionExpand", ContainerExpansionExpand);
    ctx->RegisterEnumValue("ExpansionPolicy", "ContainerExpansionInheritMax", ContainerExpansionInheritMax);

    /*******
     *
     * Messages
     *
     */

    ctx->RegisterObjectType("IMMessage", 0, asOBJ_REF );

    ctx->RegisterObjectBehaviour("IMMessage", asBEHAVE_ADDREF, "void f()", asMETHOD(IMMessage,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMMessage", asBEHAVE_RELEASE, "void f()", asMETHOD(IMMessage,Release), asCALL_THISCALL);

    ctx->RegisterObjectProperty("IMMessage",
                                "string name",
                                asOFFSET(IMMessage, name));


    ctx->RegisterObjectBehaviour("IMMessage", asBEHAVE_FACTORY, "IMMessage@ f( const string &in name)", asFUNCTION(IMMessage::ASFactory), asCALL_CDECL);

    ctx->RegisterObjectBehaviour("IMMessage", asBEHAVE_FACTORY, "IMMessage@ f( const string &in name, int param )", asFUNCTION(IMMessage::ASFactory_int), asCALL_CDECL);

    ctx->RegisterObjectBehaviour("IMMessage", asBEHAVE_FACTORY, "IMMessage@ f( const string &in name, float param )", asFUNCTION(IMMessage::ASFactory_float), asCALL_CDECL);

    ctx->RegisterObjectBehaviour("IMMessage", asBEHAVE_FACTORY, "IMMessage@ f( const string &in name, const string &in param )", asFUNCTION(IMMessage::ASFactory_string), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMMessage", "int numInts()", asMETHOD(IMMessage, numInts), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMMessage", "int numFloats()", asMETHOD(IMMessage, numFloats), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMMessage", "int numStrings()", asMETHOD(IMMessage, numStrings), asCALL_THISCALL);


    ctx->RegisterObjectMethod("IMMessage", "void addInt( int param )", asMETHOD(IMMessage, addInt), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMMessage", "void addFloat( float param )", asMETHOD(IMMessage, addFloat), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMMessage", "void addString( const string &in param )", asMETHOD(IMMessage, addString), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMMessage", "int getInt( int index ) ", asMETHOD(IMMessage, getInt), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMMessage", "float getFloat( int index ) ", asMETHOD(IMMessage, getFloat), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMMessage", "string getString( int index ) ", asMETHOD(IMMessage, getString), asCALL_THISCALL);


    /*******
     *
     * Controller items "CItem"
     *
     */
    ctx->RegisterObjectType("CItem", 0, asOBJ_REF );
    ctx->RegisterObjectBehaviour("CItem", asBEHAVE_FACTORY, "CItem@ f()", asFUNCTION(ControllerItem::ASFactory), asCALL_CDECL);
    ctx->RegisterObjectBehaviour("CItem", asBEHAVE_ADDREF, "void f()", asMETHOD(ControllerItem,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("CItem", asBEHAVE_RELEASE, "void f()", asMETHOD(ControllerItem,Release), asCALL_THISCALL);

    ctx->RegisterObjectProperty("CItem", "bool execute_on_select", asOFFSET(ControllerItem, execute_on_select));
    ctx->RegisterObjectProperty("CItem", "bool skip_show_border", asOFFSET(ControllerItem, skip_show_border));

    ctx->RegisterObjectMethod("CItem", "void setMessage(IMMessage@ message)", asMETHOD(ControllerItem, setMessage), asCALL_THISCALL);
    ctx->RegisterObjectMethod("CItem", "void setMessageOnSelect(IMMessage@ message)", asMETHOD(ControllerItem, setMessageOnSelect), asCALL_THISCALL);
    ctx->RegisterObjectMethod("CItem", "void setMessages(IMMessage@ message, IMMessage@ message_on_select, IMMessage@ message_left, IMMessage@ message_right, IMMessage@ message_up, IMMessage@ message_down)", asMETHOD(ControllerItem, setMessages), asCALL_THISCALL);

    ctx->RegisterObjectMethod("CItem", "IMMessage@ getMessage()", asMETHOD(ControllerItem, getMessage), asCALL_THISCALL);
    ctx->RegisterObjectMethod("CItem", "IMMessage@ getMessageOnSelect()", asMETHOD(ControllerItem, getMessageOnSelect), asCALL_THISCALL);
    ctx->RegisterObjectMethod("CItem", "IMMessage@ getMessageLeft()", asMETHOD(ControllerItem, getMessageLeft), asCALL_THISCALL);
    ctx->RegisterObjectMethod("CItem", "IMMessage@ getMessageRight()", asMETHOD(ControllerItem, getMessageRight), asCALL_THISCALL);
    ctx->RegisterObjectMethod("CItem", "IMMessage@ getMessageUp()", asMETHOD(ControllerItem, getMessageUp), asCALL_THISCALL);
    ctx->RegisterObjectMethod("CItem", "IMMessage@ getMessageDown()", asMETHOD(ControllerItem, getMessageDown), asCALL_THISCALL);

    ctx->RegisterObjectMethod("CItem", "bool isActive()", asMETHOD(ControllerItem, isActive), asCALL_THISCALL);

    /*******
     *
     * GUIState
     *
     */

//    struct GUIState
//    {
//        vec2 mousePosition;
//        IMUIContext::ButtonState leftMouseState;
//    };

    ctx->RegisterObjectType("GUIState", 0, asOBJ_REF | asOBJ_NOCOUNT );
    ctx->RegisterObjectProperty("GUIState",
                                "vec2 mousePosition",
                                asOFFSET(GUIState, mousePosition));
    ctx->RegisterObjectProperty("GUIState",
                                "UIMouseState leftMouseState",
                                asOFFSET(GUIState, leftMouseState));

    /*******
     *
     * Font setup
     *
     */

    ctx->RegisterGlobalFunction("vec4 HexColor(const string &in hex)", asFUNCTION(HexColor), asCALL_CDECL);

    ctx->RegisterObjectType("FontSetup", sizeof(FontSetup), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK );

    ctx->RegisterObjectBehaviour("FontSetup", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(FontSetup_ASconstructor_default),  asCALL_CDECL_OBJFIRST );

    ctx->RegisterObjectBehaviour("FontSetup", asBEHAVE_CONSTRUCT, "void f( const string &in _name, int32 _size, vec4 _color, bool _shadowed = false )", asFUNCTION(FontSetup_ASconstructor_params),  asCALL_CDECL_OBJFIRST );

    ctx->RegisterObjectBehaviour("FontSetup", asBEHAVE_CONSTRUCT, "void f( const FontSetup &in other )", asFUNCTION(FontSetup_AScopy),  asCALL_CDECL_OBJFIRST );

    ctx->RegisterObjectMethod("FontSetup", "FontSetup& opAssign(const FontSetup &in other)", asFUNCTION(FontSetup_ASassign), asCALL_CDECL_OBJFIRST);

    ctx->RegisterObjectBehaviour("FontSetup", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(FontSetup_ASdestructor),  asCALL_CDECL_OBJFIRST );

    ctx->RegisterObjectProperty("FontSetup",
                                "string fontName",
                                asOFFSET(FontSetup, fontName));

    ctx->RegisterObjectProperty("FontSetup",
                                "int size",
                                asOFFSET(FontSetup, size));

    ctx->RegisterObjectProperty("FontSetup",
                                "vec4 color",
                                asOFFSET(FontSetup, color));

    ctx->RegisterObjectProperty("FontSetup",
                                "float rotation",
                                asOFFSET(FontSetup, rotation));

    ctx->RegisterObjectProperty("FontSetup",
                                "bool shadowed",
                                asOFFSET(FontSetup, shadowed));

    /*******
     *
     * SizePolicy
     *
     */
    ctx->RegisterObjectType("SizePolicy", sizeof(SizePolicy), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK );

    ctx->RegisterObjectBehaviour("SizePolicy", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(SizePolicy_ASfactory_noparams),  asCALL_CDECL_OBJFIRST );

    ctx->RegisterObjectBehaviour("SizePolicy", asBEHAVE_CONSTRUCT, "void f( float _defaultSize )", asFUNCTION(SizePolicy_ASfactory_params),  asCALL_CDECL_OBJFIRST );

    ctx->RegisterObjectBehaviour("SizePolicy", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(SizePolicy_ASdestructor),  asCALL_CDECL_OBJFIRST );

    ctx->RegisterObjectBehaviour("SizePolicy", asBEHAVE_CONSTRUCT, "void f( const SizePolicy &in other )", asFUNCTION(SizePolicy_ASCopyConstructor),  asCALL_CDECL_OBJFIRST );

    ctx->RegisterObjectMethod("SizePolicy", "IMUIText& opAssign(const SizePolicy &in other)", asFUNCTION(SizePolicy_ASAssign), asCALL_CDECL_OBJFIRST);

    ctx->RegisterObjectMethod("SizePolicy", "SizePolicy expand( float _maxSize = UNDEFINEDSIZE )", asMETHOD(SizePolicy, expand), asCALL_THISCALL);

    ctx->RegisterObjectMethod("SizePolicy", "SizePolicy inheritMax()", asMETHOD(SizePolicy, inheritMax), asCALL_THISCALL);

    ctx->RegisterObjectMethod("SizePolicy", "SizePolicy staticMax()", asMETHOD(SizePolicy, staticMax), asCALL_THISCALL);

    ctx->RegisterObjectMethod("SizePolicy", "SizePolicy overflowClip( bool shouldClip )", asMETHOD(SizePolicy, overflowClip), asCALL_THISCALL);

    /*******
     *
     * Tweens
     *
     */

    ctx->RegisterEnum("IMTweenType");
    ctx->RegisterEnumValue("IMTweenType", "linearTween", linearTween);
    ctx->RegisterEnumValue("IMTweenType", "inQuadTween", inQuadTween);
    ctx->RegisterEnumValue("IMTweenType", "outQuadTween", outQuadTween);
    ctx->RegisterEnumValue("IMTweenType", "inOutQuadTween", inOutQuadTween);
    ctx->RegisterEnumValue("IMTweenType", "outInQuadTween", outInQuadTween);
    ctx->RegisterEnumValue("IMTweenType", "inCubicTween", inCubicTween);
    ctx->RegisterEnumValue("IMTweenType", "outCubicTween", outCubicTween);
    ctx->RegisterEnumValue("IMTweenType", "inOutCubicTween", inOutCubicTween);
    ctx->RegisterEnumValue("IMTweenType", "outInCubicTween", outInCubicTween);
    ctx->RegisterEnumValue("IMTweenType", "inQuartTween", inQuartTween);
    ctx->RegisterEnumValue("IMTweenType", "outQuartTween", outQuartTween);
    ctx->RegisterEnumValue("IMTweenType", "inOutQuartTween", inOutQuartTween);
    ctx->RegisterEnumValue("IMTweenType", "outInQuartTween", outInQuartTween);
    ctx->RegisterEnumValue("IMTweenType", "inQuintTween", inQuintTween);
    ctx->RegisterEnumValue("IMTweenType", "outQuintTween", outQuintTween);
    ctx->RegisterEnumValue("IMTweenType", "inOutQuintTween", inOutQuintTween);
    ctx->RegisterEnumValue("IMTweenType", "outInQuintTween", outInQuintTween);
    ctx->RegisterEnumValue("IMTweenType", "inSineTween", inSineTween);
    ctx->RegisterEnumValue("IMTweenType", "outSineTween", outSineTween);
    ctx->RegisterEnumValue("IMTweenType", "inOutSineTween", inOutSineTween);
    ctx->RegisterEnumValue("IMTweenType", "outInSineTween", outInSineTween);
    ctx->RegisterEnumValue("IMTweenType", "inExpoTween", inExpoTween);
    ctx->RegisterEnumValue("IMTweenType", "outExpoTween", outExpoTween);
    ctx->RegisterEnumValue("IMTweenType", "inOutExpoTween", inOutExpoTween);
    ctx->RegisterEnumValue("IMTweenType", "outInExpoTween", outInExpoTween);
    ctx->RegisterEnumValue("IMTweenType", "inCircTween", inCircTween);
    ctx->RegisterEnumValue("IMTweenType", "outCircTween", outCircTween);
    ctx->RegisterEnumValue("IMTweenType", "inOutCircTween", inOutCircTween);
    ctx->RegisterEnumValue("IMTweenType", "outInCircTween", outInCircTween);
    ctx->RegisterEnumValue("IMTweenType", "outBounceTween", outBounceTween);
    ctx->RegisterEnumValue("IMTweenType", "inBounceTween", inBounceTween);
    ctx->RegisterEnumValue("IMTweenType", "inOutBounceTween", inOutBounceTween);
    ctx->RegisterEnumValue("IMTweenType", "outInBounceTween", outInBounceTween);

    /**
     * Behavior base clases
     **/

    ctx->RegisterObjectType("IMUpdateBehavior", 0, asOBJ_REF );
    ctx->RegisterObjectBehaviour("IMUpdateBehavior", asBEHAVE_FACTORY, "IMUpdateBehavior@ f()", asFUNCTION(IMUpdateBehavior::ASFactory), asCALL_CDECL);
    ctx->RegisterObjectBehaviour("IMUpdateBehavior", asBEHAVE_ADDREF, "void f()", asMETHOD(IMUpdateBehavior,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMUpdateBehavior", asBEHAVE_RELEASE, "void f()", asMETHOD(IMUpdateBehavior,Release), asCALL_THISCALL);


    ctx->RegisterObjectType("IMMouseOverBehavior", 0, asOBJ_REF );
    ctx->RegisterObjectBehaviour("IMMouseOverBehavior", asBEHAVE_FACTORY, "IMMouseOverBehavior@ f()", asFUNCTION(IMMouseOverBehavior::ASFactory), asCALL_CDECL);
    ctx->RegisterObjectBehaviour("IMMouseOverBehavior", asBEHAVE_ADDREF, "void f()", asMETHOD(IMMouseOverBehavior,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMMouseOverBehavior", asBEHAVE_RELEASE, "void f()", asMETHOD(IMMouseOverBehavior,Release), asCALL_THISCALL);

    ctx->RegisterObjectType("IMMouseClickBehavior", 0, asOBJ_REF );
    ctx->RegisterObjectBehaviour("IMMouseClickBehavior", asBEHAVE_FACTORY, "IMMouseClickBehavior@ f()", asFUNCTION(IMMouseClickBehavior::ASFactory), asCALL_CDECL);
    ctx->RegisterObjectBehaviour("IMMouseClickBehavior", asBEHAVE_ADDREF, "void f()", asMETHOD(IMMouseClickBehavior,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMMouseClickBehavior", asBEHAVE_RELEASE, "void f()", asMETHOD(IMMouseClickBehavior,Release), asCALL_THISCALL);

    /**
     * Behaviors clases
     **/

    //// IMUpdateBehavior

    ctx->RegisterObjectType("IMFadeIn", 0, asOBJ_REF );
    ctx->RegisterObjectBehaviour("IMFadeIn", asBEHAVE_FACTORY, "IMFadeIn@ f(uint64 time, IMTweenType _tweener)", asFUNCTION(IMFadeIn::ASFactory), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMUpdateBehavior", "IMFadeIn@ opCast()", asFUNCTION((refCast<IMUpdateBehavior,IMFadeIn>)), asCALL_CDECL_OBJLAST);
    ctx->RegisterObjectMethod("IMFadeIn", "IMUpdateBehavior@ opImplCast()", asFUNCTION((refCast<IMFadeIn,IMUpdateBehavior>)), asCALL_CDECL_OBJLAST);

    ctx->RegisterObjectBehaviour("IMFadeIn", asBEHAVE_ADDREF, "void f()", asMETHOD(IMFadeIn,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMFadeIn", asBEHAVE_RELEASE, "void f()", asMETHOD(IMFadeIn,Release), asCALL_THISCALL);


    ctx->RegisterObjectType("IMMoveIn", 0, asOBJ_REF );
    ctx->RegisterObjectBehaviour("IMMoveIn", asBEHAVE_FACTORY, "IMMoveIn@ f(uint64 time, vec2 offset, IMTweenType _tweener)", asFUNCTION(IMMoveIn::ASFactory), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMUpdateBehavior", "IMMoveIn@ opCast()", asFUNCTION((refCast<IMUpdateBehavior,IMMoveIn>)), asCALL_CDECL_OBJLAST);
    ctx->RegisterObjectMethod("IMMoveIn", "IMUpdateBehavior@ opImplCast()", asFUNCTION((refCast<IMMoveIn,IMUpdateBehavior>)), asCALL_CDECL_OBJLAST);

    ctx->RegisterObjectBehaviour("IMMoveIn", asBEHAVE_ADDREF, "void f()", asMETHOD(IMMoveIn,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMMoveIn", asBEHAVE_RELEASE, "void f()", asMETHOD(IMMoveIn,Release), asCALL_THISCALL);



    ctx->RegisterObjectType("IMChangeTextFadeOutIn", 0, asOBJ_REF );
    ctx->RegisterObjectBehaviour("IMChangeTextFadeOutIn", asBEHAVE_FACTORY, "IMChangeTextFadeOutIn@ f(uint64 time, const string &in _targetText, IMTweenType _tweenerOut, IMTweenType _tweenerIn)", asFUNCTION(IMChangeTextFadeOutIn::ASFactory), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMUpdateBehavior", "IMChangeTextFadeOutIn@ opCast()", asFUNCTION((refCast<IMUpdateBehavior,IMChangeTextFadeOutIn>)), asCALL_CDECL_OBJLAST);
    ctx->RegisterObjectMethod("IMChangeTextFadeOutIn", "IMUpdateBehavior@ opImplCast()", asFUNCTION((refCast<IMChangeTextFadeOutIn,IMUpdateBehavior>)), asCALL_CDECL_OBJLAST);

    ctx->RegisterObjectBehaviour("IMChangeTextFadeOutIn", asBEHAVE_ADDREF, "void f()", asMETHOD(IMChangeTextFadeOutIn,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMChangeTextFadeOutIn", asBEHAVE_RELEASE, "void f()", asMETHOD(IMChangeTextFadeOutIn,Release), asCALL_THISCALL);



    ctx->RegisterObjectType("IMChangeImageFadeOutIn", 0, asOBJ_REF );
    ctx->RegisterObjectBehaviour("IMChangeImageFadeOutIn", asBEHAVE_FACTORY, "IMChangeImageFadeOutIn@ f(uint64 time, const string &in _targetImage, IMTweenType _tweenerOut, IMTweenType _tweenerIn)", asFUNCTION(IMChangeImageFadeOutIn::ASFactory), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMUpdateBehavior", "IMChangeImageFadeOutIn@ opCast()", asFUNCTION((refCast<IMUpdateBehavior,IMChangeImageFadeOutIn>)), asCALL_CDECL_OBJLAST);
    ctx->RegisterObjectMethod("IMChangeImageFadeOutIn", "IMUpdateBehavior@ opImplCast()", asFUNCTION((refCast<IMChangeImageFadeOutIn,IMUpdateBehavior>)), asCALL_CDECL_OBJLAST);

    ctx->RegisterObjectBehaviour("IMChangeImageFadeOutIn", asBEHAVE_ADDREF, "void f()", asMETHOD(IMChangeImageFadeOutIn,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMChangeImageFadeOutIn", asBEHAVE_RELEASE, "void f()", asMETHOD(IMChangeImageFadeOutIn,Release), asCALL_THISCALL);



    ctx->RegisterObjectType("IMPulseAlpha", 0, asOBJ_REF );
    ctx->RegisterObjectBehaviour("IMPulseAlpha", asBEHAVE_FACTORY, "IMPulseAlpha@ f(float lower, float upper, float _speed)", asFUNCTION(IMPulseAlpha::ASFactory), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMUpdateBehavior", "IMPulseAlpha@ opCast()", asFUNCTION((refCast<IMUpdateBehavior,IMPulseAlpha>)), asCALL_CDECL_OBJLAST);
    ctx->RegisterObjectMethod("IMPulseAlpha", "IMUpdateBehavior@ opImplCast()", asFUNCTION((refCast<IMPulseAlpha,IMUpdateBehavior>)), asCALL_CDECL_OBJLAST);

    ctx->RegisterObjectBehaviour("IMPulseAlpha", asBEHAVE_ADDREF, "void f()", asMETHOD(IMPulseAlpha,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMPulseAlpha", asBEHAVE_RELEASE, "void f()", asMETHOD(IMPulseAlpha,Release), asCALL_THISCALL);


    ctx->RegisterObjectType("IMPulseBorderAlpha", 0, asOBJ_REF );
    ctx->RegisterObjectBehaviour("IMPulseBorderAlpha", asBEHAVE_FACTORY, "IMPulseBorderAlpha@ f(float lower, float upper, float _speed)", asFUNCTION(IMPulseBorderAlpha::ASFactory), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMUpdateBehavior", "IMPulseBorderAlpha@ opCast()", asFUNCTION((refCast<IMUpdateBehavior,IMPulseBorderAlpha>)), asCALL_CDECL_OBJLAST);
    ctx->RegisterObjectMethod("IMPulseBorderAlpha", "IMUpdateBehavior@ opImplCast()", asFUNCTION((refCast<IMPulseBorderAlpha,IMUpdateBehavior>)), asCALL_CDECL_OBJLAST);

    ctx->RegisterObjectBehaviour("IMPulseBorderAlpha", asBEHAVE_ADDREF, "void f()", asMETHOD(IMPulseAlpha,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMPulseBorderAlpha", asBEHAVE_RELEASE, "void f()", asMETHOD(IMPulseAlpha,Release), asCALL_THISCALL);



    //// IMMouseOverBehavior
	ctx->RegisterObjectType("IMMouseOverScale", 0, asOBJ_REF );
    ctx->RegisterObjectBehaviour("IMMouseOverScale", asBEHAVE_FACTORY, "IMMouseOverScale@ f(uint64 time, float offset, IMTweenType _tweener)", asFUNCTION(IMMouseOverScale::ASFactory), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMMouseOverBehavior", "IMMouseOverScale@ opCast()", asFUNCTION((refCast<IMMouseOverBehavior,IMMouseOverScale>)), asCALL_CDECL_OBJLAST);
    ctx->RegisterObjectMethod("IMMouseOverScale", "IMMouseOverBehavior@ opImplCast()", asFUNCTION((refCast<IMMouseOverScale,IMMouseOverBehavior>)), asCALL_CDECL_OBJLAST);

    ctx->RegisterObjectBehaviour("IMMouseOverScale", asBEHAVE_ADDREF, "void f()", asMETHOD(IMMouseOverScale,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMMouseOverScale", asBEHAVE_RELEASE, "void f()", asMETHOD(IMMouseOverScale,Release), asCALL_THISCALL);



    ctx->RegisterObjectType("IMMouseOverMove", 0, asOBJ_REF );
    ctx->RegisterObjectBehaviour("IMMouseOverMove", asBEHAVE_FACTORY, "IMMouseOverMove@ f(uint64 time, vec2 offset, IMTweenType _tweener)", asFUNCTION(IMMouseOverMove::ASFactory), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMMouseOverBehavior", "IMMouseOverMove@ opCast()", asFUNCTION((refCast<IMMouseOverBehavior,IMMouseOverMove>)), asCALL_CDECL_OBJLAST);
    ctx->RegisterObjectMethod("IMMouseOverMove", "IMMouseOverBehavior@ opImplCast()", asFUNCTION((refCast<IMMouseOverMove,IMMouseOverBehavior>)), asCALL_CDECL_OBJLAST);

    ctx->RegisterObjectBehaviour("IMMouseOverMove", asBEHAVE_ADDREF, "void f()", asMETHOD(IMMouseOverMove,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMMouseOverMove", asBEHAVE_RELEASE, "void f()", asMETHOD(IMMouseOverMove,Release), asCALL_THISCALL);



    ctx->RegisterObjectType("IMMouseOverShowBorder", 0, asOBJ_REF );
    ctx->RegisterObjectBehaviour("IMMouseOverShowBorder", asBEHAVE_FACTORY, "IMMouseOverShowBorder@ f()", asFUNCTION(IMMouseOverShowBorder::ASFactory), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMMouseOverBehavior", "IMMouseOverShowBorder@ opCast()", asFUNCTION((refCast<IMMouseOverBehavior,IMMouseOverShowBorder>)), asCALL_CDECL_OBJLAST);
    ctx->RegisterObjectMethod("IMMouseOverShowBorder", "IMMouseOverBehavior@ opImplCast()", asFUNCTION((refCast<IMMouseOverShowBorder,IMMouseOverBehavior>)), asCALL_CDECL_OBJLAST);

    ctx->RegisterObjectBehaviour("IMMouseOverShowBorder", asBEHAVE_ADDREF, "void f()", asMETHOD(IMMouseOverShowBorder,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMMouseOverShowBorder", asBEHAVE_RELEASE, "void f()", asMETHOD(IMMouseOverShowBorder,Release), asCALL_THISCALL);



    ctx->RegisterObjectType("IMMouseOverPulseColor", 0, asOBJ_REF );
    ctx->RegisterObjectBehaviour("IMMouseOverPulseColor", asBEHAVE_FACTORY, "IMMouseOverPulseColor@ f(vec4 first, vec4 second, float _speed)", asFUNCTION(IMMouseOverPulseColor::ASFactory), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMMouseOverBehavior", "IMMouseOverPulseColor@ opCast()", asFUNCTION((refCast<IMMouseOverBehavior,IMMouseOverPulseColor>)), asCALL_CDECL_OBJLAST);
    ctx->RegisterObjectMethod("IMMouseOverPulseColor", "IMMouseOverBehavior@ opImplCast()", asFUNCTION((refCast<IMMouseOverPulseColor,IMMouseOverBehavior>)), asCALL_CDECL_OBJLAST);

    ctx->RegisterObjectBehaviour("IMMouseOverPulseColor", asBEHAVE_ADDREF, "void f()", asMETHOD(IMMouseOverPulseColor,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMMouseOverPulseColor", asBEHAVE_RELEASE, "void f()", asMETHOD(IMMouseOverPulseColor,Release), asCALL_THISCALL);


    ctx->RegisterObjectType("IMMouseOverPulseBorder", 0, asOBJ_REF );
    ctx->RegisterObjectBehaviour("IMMouseOverPulseBorder", asBEHAVE_FACTORY, "IMMouseOverPulseBorder@ f(vec4 first, vec4 second, float _speed)", asFUNCTION(IMMouseOverPulseBorder::ASFactory), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMMouseOverBehavior", "IMMouseOverPulseBorder@ opCast()", asFUNCTION((refCast<IMMouseOverBehavior,IMMouseOverPulseBorder>)), asCALL_CDECL_OBJLAST);
    ctx->RegisterObjectMethod("IMMouseOverPulseBorder", "IMMouseOverBehavior@ opImplCast()", asFUNCTION((refCast<IMMouseOverPulseBorder,IMMouseOverBehavior>)), asCALL_CDECL_OBJLAST);

    ctx->RegisterObjectBehaviour("IMMouseOverPulseBorder", asBEHAVE_ADDREF, "void f()", asMETHOD(IMMouseOverPulseBorder,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMMouseOverPulseBorder", asBEHAVE_RELEASE, "void f()", asMETHOD(IMMouseOverPulseBorder,Release), asCALL_THISCALL);



    ctx->RegisterObjectType("IMMouseOverPulseBorderAlpha", 0, asOBJ_REF );
    ctx->RegisterObjectBehaviour("IMMouseOverPulseBorderAlpha", asBEHAVE_FACTORY, "IMMouseOverPulseBorderAlpha@ f(vec4 first, vec4 second, float _speed)", asFUNCTION(IMMouseOverPulseBorderAlpha::ASFactory), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMMouseOverBehavior", "IMMouseOverPulseBorderAlpha@ opCast()", asFUNCTION((refCast<IMMouseOverBehavior,IMMouseOverPulseBorderAlpha>)), asCALL_CDECL_OBJLAST);
    ctx->RegisterObjectMethod("IMMouseOverPulseBorderAlpha", "IMMouseOverBehavior@ opImplCast()", asFUNCTION((refCast<IMMouseOverPulseBorderAlpha,IMMouseOverBehavior>)), asCALL_CDECL_OBJLAST);

    ctx->RegisterObjectBehaviour("IMMouseOverPulseBorderAlpha", asBEHAVE_ADDREF, "void f()", asMETHOD(IMMouseOverPulseBorderAlpha,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMMouseOverPulseBorderAlpha", asBEHAVE_RELEASE, "void f()", asMETHOD(IMMouseOverPulseBorderAlpha,Release), asCALL_THISCALL);



    ctx->RegisterObjectType("IMFixedMessageOnMouseOver", 0, asOBJ_REF );
    ctx->RegisterObjectBehaviour("IMFixedMessageOnMouseOver", asBEHAVE_FACTORY, "IMFixedMessageOnMouseOver@ f(IMMessage@ _enterMessage, IMMessage@ _overMessage, IMMessage@ _leaveMessage)", asFUNCTION(IMFixedMessageOnMouseOver::ASFactory), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMMouseOverBehavior", "IMFixedMessageOnMouseOver@ opCast()", asFUNCTION((refCast<IMMouseOverBehavior,IMMouseOverShowBorder>)), asCALL_CDECL_OBJLAST);
    ctx->RegisterObjectMethod("IMFixedMessageOnMouseOver", "IMMouseOverBehavior@ opImplCast()", asFUNCTION((refCast<IMFixedMessageOnMouseOver,IMMouseOverBehavior>)), asCALL_CDECL_OBJLAST);

    ctx->RegisterObjectBehaviour("IMFixedMessageOnMouseOver", asBEHAVE_ADDREF, "void f()", asMETHOD(IMFixedMessageOnMouseOver,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMFixedMessageOnMouseOver", asBEHAVE_RELEASE, "void f()", asMETHOD(IMFixedMessageOnMouseOver,Release), asCALL_THISCALL);


    ctx->RegisterObjectType("IMMouseOverFadeIn", 0, asOBJ_REF );
    ctx->RegisterObjectBehaviour("IMMouseOverFadeIn", asBEHAVE_FACTORY, "IMMouseOverFadeIn@ f(uint64 time, IMTweenType _tweener, float targetAlpha = 1.0f)", asFUNCTION(IMMouseOverFadeIn::ASFactory), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMMouseOverBehavior", "IMMouseOverFadeIn@ opCast()", asFUNCTION((refCast<IMMouseOverBehavior,IMMouseOverFadeIn>)), asCALL_CDECL_OBJLAST);
    ctx->RegisterObjectMethod("IMMouseOverFadeIn", "IMMouseOverBehavior@ opImplCast()", asFUNCTION((refCast<IMMouseOverFadeIn,IMMouseOverBehavior>)), asCALL_CDECL_OBJLAST);

    ctx->RegisterObjectBehaviour("IMMouseOverFadeIn", asBEHAVE_ADDREF, "void f()", asMETHOD(IMMouseOverFadeIn,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMMouseOverFadeIn", asBEHAVE_RELEASE, "void f()", asMETHOD(IMMouseOverFadeIn,Release), asCALL_THISCALL);


    ctx->RegisterObjectType("IMMouseOverSound", 0, asOBJ_REF);
    ctx->RegisterObjectBehaviour("IMMouseOverSound", asBEHAVE_FACTORY, "IMMouseOverSound@ f(const string &in audioFile)", asFUNCTION(IMMouseOverSound::ASFactory), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMMouseOverBehavior", "IMMouseOverSound@ opCast()", asFUNCTION((refCast<IMMouseOverBehavior,IMMouseOverSound>)), asCALL_CDECL_OBJLAST);
    ctx->RegisterObjectMethod("IMMouseOverSound", "IMMouseOverBehavior@ opImplCast()", asFUNCTION((refCast<IMMouseOverSound,IMMouseOverBehavior>)), asCALL_CDECL_OBJLAST);

    ctx->RegisterObjectBehaviour("IMMouseOverSound", asBEHAVE_ADDREF, "void f()", asMETHOD(IMMouseOverSound,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMMouseOverSound", asBEHAVE_RELEASE, "void f()", asMETHOD(IMMouseOverSound,Release), asCALL_THISCALL);


    //// IMMouseClickBehavior

    ctx->RegisterObjectType("IMFixedMessageOnClick", 0, asOBJ_REF );
    ctx->RegisterObjectBehaviour("IMFixedMessageOnClick", asBEHAVE_FACTORY, "IMFixedMessageOnClick@ f(IMMessage@ _enterMessage, IMMessage@ _overMessage, IMMessage@ _leaveMessage)", asFUNCTION(IMFixedMessageOnClick::ASFactory), asCALL_CDECL);

    ctx->RegisterObjectBehaviour("IMFixedMessageOnClick", asBEHAVE_FACTORY, "IMFixedMessageOnClick@ f(const string &in messageName)", asFUNCTION(IMFixedMessageOnClick::ASFactory), asCALL_CDECL);

    ctx->RegisterObjectBehaviour("IMFixedMessageOnClick", asBEHAVE_FACTORY, "IMFixedMessageOnClick@ f(const string &in messageName, int param)", asFUNCTION(IMFixedMessageOnClick::ASFactory_int), asCALL_CDECL);

    ctx->RegisterObjectBehaviour("IMFixedMessageOnClick", asBEHAVE_FACTORY, "IMFixedMessageOnClick@ f(const string &in messageName, const string &in param)", asFUNCTION(IMFixedMessageOnClick::ASFactory_string), asCALL_CDECL);

    ctx->RegisterObjectBehaviour("IMFixedMessageOnClick", asBEHAVE_FACTORY, "IMFixedMessageOnClick@ f(const string &in messageName, float param)", asFUNCTION(IMFixedMessageOnClick::ASFactory_float), asCALL_CDECL);

    ctx->RegisterObjectBehaviour("IMFixedMessageOnClick", asBEHAVE_FACTORY, "IMFixedMessageOnClick@ f(IMMessage@ message)", asFUNCTION(IMFixedMessageOnClick::ASFactoryMessage), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMMouseClickBehavior", "IMFixedMessageOnClick@ opCast()", asFUNCTION((refCast<IMMouseClickBehavior,IMFixedMessageOnClick>)), asCALL_CDECL_OBJLAST);
    ctx->RegisterObjectMethod("IMFixedMessageOnClick", "IMMouseClickBehavior@ opImplCast()", asFUNCTION((refCast<IMFixedMessageOnClick,IMMouseClickBehavior>)), asCALL_CDECL_OBJLAST);

    ctx->RegisterObjectBehaviour("IMFixedMessageOnClick", asBEHAVE_ADDREF, "void f()", asMETHOD(IMFixedMessageOnClick,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMFixedMessageOnClick", asBEHAVE_RELEASE, "void f()", asMETHOD(IMFixedMessageOnClick,Release), asCALL_THISCALL);


    ctx->RegisterObjectType("IMSoundOnClick", 0, asOBJ_REF);
    ctx->RegisterObjectBehaviour("IMSoundOnClick", asBEHAVE_FACTORY, "IMSoundOnClick@ f(const string &in audioFile)", asFUNCTION(IMSoundOnClick::ASFactory), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMMouseClickBehavior", "IMSoundOnClick@ opCast()", asFUNCTION((refCast<IMMouseClickBehavior,IMSoundOnClick>)), asCALL_CDECL_OBJLAST);
    ctx->RegisterObjectMethod("IMSoundOnClick", "IMMouseClickBehavior@ opImplCast()", asFUNCTION((refCast<IMSoundOnClick,IMMouseClickBehavior>)), asCALL_CDECL_OBJLAST);

    ctx->RegisterObjectBehaviour("IMSoundOnClick", asBEHAVE_ADDREF, "void f()", asMETHOD(IMSoundOnClick,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMSoundOnClick", asBEHAVE_RELEASE, "void f()", asMETHOD(IMSoundOnClick,Release), asCALL_THISCALL);

    ctx->RegisterFuncdef("void OnClickCallback()");

    ctx->RegisterObjectType("IMFuncOnClick", 0, asOBJ_REF);
    ctx->RegisterObjectBehaviour("IMFuncOnClick", asBEHAVE_FACTORY, "IMFuncOnClick@ f(OnClickCallback@ func)", asFUNCTION(IMFuncOnClick::ASFactory), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMMouseClickBehavior", "IMFuncOnClick@ opCast()", asFUNCTION((refCast<IMMouseClickBehavior,IMFuncOnClick>)), asCALL_CDECL_OBJLAST);
    ctx->RegisterObjectMethod("IMFuncOnClick", "IMMouseClickBehavior@ opImplCast()", asFUNCTION((refCast<IMFuncOnClick,IMMouseClickBehavior>)), asCALL_CDECL_OBJLAST);

    ctx->RegisterObjectBehaviour("IMFuncOnClick", asBEHAVE_ADDREF, "void f()", asMETHOD(IMFuncOnClick,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMFuncOnClick", asBEHAVE_RELEASE, "void f()", asMETHOD(IMFuncOnClick,Release), asCALL_THISCALL);


    /*******
     *
     * GUI elements
     *
     */

    registerASIMElement<IMElement>( ctx, "IMElement" );


    registerASIMElement<IMSpacer>( ctx, "IMSpacer" );

    ctx->RegisterObjectBehaviour("IMSpacer", asBEHAVE_FACTORY, "IMSpacer@ f( DividerOrientation _orientation, float size)", asFUNCTION(IMSpacer::ASFactory_static), asCALL_CDECL);

    ctx->RegisterObjectBehaviour("IMSpacer", asBEHAVE_FACTORY, "IMSpacer@ f(DividerOrientation _orientation )", asFUNCTION(IMSpacer::ASFactory_dynamic), asCALL_CDECL);

    registerASIMElement<IMContainer>( ctx, "IMContainer" );

    ctx->RegisterObjectBehaviour("IMContainer", asBEHAVE_FACTORY, "IMContainer@ f(const string &in name, SizePolicy sizeX = SizePolicy(), SizePolicy sizeY  = SizePolicy())", asFUNCTION(IMContainer::ASFactory_named), asCALL_CDECL);

    ctx->RegisterObjectBehaviour("IMContainer", asBEHAVE_FACTORY, "IMContainer@ f( SizePolicy sizeX = SizePolicy(), SizePolicy sizeY  = SizePolicy())", asFUNCTION(IMContainer::ASFactory_unnamed), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMContainer", "void setSizePolicy( SizePolicy sizeX, SizePolicy sizeY )", asMETHOD(IMContainer, setSizePolicy), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMContainer", "void clear()", asMETHOD(IMContainer, clear), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMContainer", "void addFloatingElement( IMElement@ element, const string &in name, vec2 position, int z = -1 )", asMETHOD(IMContainer, addFloatingElement), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMContainer", "void setElement( IMElement@ element )", asMETHOD(IMContainer, setElement), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMContainer", "void setAlignment( ContainerAlignment xAlignment, ContainerAlignment yAlignment )", asMETHOD(IMContainer, setAlignment), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMContainer", "IMElement@ removeElement( const string &in name )", asMETHOD(IMContainer, removeElement), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMContainer", "void moveElement( const string &in name, vec2 newPos )", asMETHOD(IMContainer, moveElement), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMContainer", "void moveElementRelative( const string &in name, vec2 posChange )", asMETHOD(IMContainer, moveElementRelative), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMContainer", "vec2 getElementPosition( const string &in name )", asMETHOD(IMContainer, getElementPosition), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMContainer", "void setBackgroundImage( const string &in fileName, vec4 color )", asMETHOD(IMContainer, setBackgroundImage), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMContainer", "IMElement@ getContents( )", asMETHOD(IMContainer, getContents), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMContainer", "array<IMElement@>@ getFloatingContents( )", asFUNCTION(AS_GetFloatingContents), asCALL_CDECL_OBJFIRST);

    registerASIMElement<IMDivider>( ctx, "IMDivider" );

    ctx->RegisterObjectBehaviour("IMDivider", asBEHAVE_FACTORY, "IMDivider@ f(const string &in name, DividerOrientation _orientation = DOVertical)", asFUNCTION(IMDivider::ASFactory_named), asCALL_CDECL);

    ctx->RegisterObjectBehaviour("IMDivider", asBEHAVE_FACTORY, "IMDivider@ f( DividerOrientation _orientation = DOVertical)", asFUNCTION(IMDivider::ASFactory_unnamed), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMDivider", "void setAlignment( ContainerAlignment xAlignment, ContainerAlignment yAlignment, bool reposition = true )", asMETHOD(IMDivider, setAlignment), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMDivider", "void clear()", asMETHOD(IMDivider, clear), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMDivider", "IMSpacer@ appendSpacer( float _size )", asMETHOD(IMDivider, appendSpacer), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMDivider", "IMSpacer@ appendDynamicSpacer()", asMETHOD(IMDivider, appendDynamicSpacer), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMDivider", "uint getContainerCount()", asMETHOD(IMDivider, getContainerCount), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMDivider", "IMContainer@ getContainerAt( uint i )", asMETHOD(IMDivider, getContainerAt), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMDivider", "IMContainer@ getContainerOf( const string &in _name )", asMETHOD(IMDivider, getContainerOf), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMDivider", "array<IMContainer@>@ getContainers()", asFUNCTION(AS_GetContainers), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("IMDivider", "IMContainer@ append( IMElement@ newElement, float containerSize = UNDEFINEDSIZE )", asMETHOD(IMDivider, append), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMDivider", "DividerOrientation getOrientation()", asMETHOD(IMDivider, getOrientation), asCALL_THISCALL);

    registerASIMElement<IMDivider>( ctx, "IMTextSelectionList" );

    ctx->RegisterObjectBehaviour("IMTextSelectionList", asBEHAVE_FACTORY, "IMTextSelectionList@ f(const string &in _name, FontSetup _fontSetup, float _betweenSpace, IMMouseOverBehavior@ _mouseOver = null )", asFUNCTION(IMTextSelectionList::ASFactory), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMTextSelectionList", "void setAlignment( ContainerAlignment xAlignment, ContainerAlignment yAlignment )", asMETHOD(IMTextSelectionList, setAlignment), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMTextSelectionList", "void setItemUpdateBehaviour( IMUpdateBehavior@ behavior )", asMETHOD(IMTextSelectionList, setItemUpdateBehaviour), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMTextSelectionList", "void addEntry( const string &in name, const string &in text, const string &in message )", asMETHOD(IMTextSelectionList, addEntry), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMTextSelectionList", "void addEntry( const string &in name, const string &in text, const string &in message, const string &in param )", asMETHOD(IMTextSelectionList, addEntryParam), asCALL_THISCALL);

    registerASIMElement<IMImage>( ctx, "IMImage" );

    ctx->RegisterObjectBehaviour("IMImage", asBEHAVE_FACTORY, "IMImage@ f(const string &in name, DividerOrientation _orientation = DOVertical)", asFUNCTION(IMImage::ASFactory), asCALL_CDECL);

    ctx->RegisterObjectMethod("IMImage", "void setSkipAspectFitting(bool val)", asMETHOD(IMImage, setSkipAspectFitting), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMImage", "void setCenter(bool val)", asMETHOD(IMImage, setCenter), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMImage", "void setImageFile( const string &in _fileName )", asMETHOD(IMImage, setImageFile), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMImage", "void scaleToSizeX( float newSize )", asMETHOD(IMImage, scaleToSizeX), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMImage", "void scaleToSizeY( float newSize )", asMETHOD(IMImage, scaleToSizeY), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMImage", "void setRotation( float _rotation )", asMETHOD(IMImage, setRotation), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMImage", "float getRotation()", asMETHOD(IMImage, getRotation), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMImage", "void setImageOffset( vec2 offset, vec2 size )", asMETHOD(IMImage, setImageOffset), asCALL_THISCALL);

    registerASIMElement<IMText>( ctx, "IMText" );

    ctx->RegisterObjectBehaviour("IMText", asBEHAVE_FACTORY, "IMText@ f(const string &in name)", asFUNCTION(IMText::ASFactory_named), asCALL_CDECL);

    ctx->RegisterObjectBehaviour("IMText", asBEHAVE_FACTORY, "IMText@ f(const string &in _text, const string &in _fontName, int _fontSize, vec4 _color = vec4(1.0f))", asFUNCTION(IMText::ASFactory_unnamed), asCALL_CDECL);

    ctx->RegisterObjectBehaviour("IMText", asBEHAVE_FACTORY, "IMText@ f(const string &in _text, FontSetup _fontSetup)", asFUNCTION(IMText::ASFactory_fontsetup), asCALL_CDECL);


    ctx->RegisterObjectMethod("IMText", "void setFont( FontSetup _fontSetup )", asMETHOD(IMText, setFont), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMText", "void setFontByName( const string &in _fontName, int _fontSize )", asMETHOD(IMText, setFontByName), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMText", "void setText( const string &in _text )", asMETHOD(IMText, setText), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMText", "string getText()", asMETHOD(IMText, getText), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMText", "void setShadowed( bool shouldShadow = true )", asMETHOD(IMText, setShadowed), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMText", "void setRotation( float _rotation )", asMETHOD(IMText, setRotation), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMText", "float getRotation()", asMETHOD(IMText, getRotation), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMText", "void updateEngineTextObject()", asMETHOD(IMText, updateEngineTextObject), asCALL_THISCALL);

    /*******
     *
     * Screen metrics
     *
     */
    ctx->RegisterObjectType("ScreenMetrics", 0, asOBJ_REF | asOBJ_NOHANDLE);

    ctx->RegisterObjectProperty("ScreenMetrics",
                                "vec2 GUISpace",
                                asOFFSET(ScreenMetrics, GUISpace));

    ctx->RegisterObjectProperty("ScreenMetrics",
                                "vec2 screenSize",
                                asOFFSET(ScreenMetrics, screenSize));

    ctx->RegisterObjectProperty("ScreenMetrics",
                                "float GUItoScreenXScale",
                                asOFFSET(ScreenMetrics, GUItoScreenXScale));

    ctx->RegisterObjectProperty("ScreenMetrics",
                                "float GUItoScreenYScale",
                                asOFFSET(ScreenMetrics, GUItoScreenYScale));

    ctx->RegisterObjectMethod("ScreenMetrics","vec2 getMetrics()",asMETHOD(ScreenMetrics, getMetrics), asCALL_THISCALL);
    ctx->RegisterObjectMethod("ScreenMetrics","bool checkMetrics( vec2 &in metrics )",asMETHOD(ScreenMetrics, checkMetrics), asCALL_THISCALL);
    ctx->RegisterObjectMethod("ScreenMetrics","void computeFactors()",asMETHOD(ScreenMetrics, computeFactors), asCALL_THISCALL);
    ctx->RegisterObjectMethod("ScreenMetrics","vec2 GUIToScreen( const vec2 pos )",asMETHOD(ScreenMetrics, GUIToScreen), asCALL_THISCALL);
    ctx->RegisterObjectMethod("ScreenMetrics","float getScreenWidth()",asMETHOD(ScreenMetrics, getScreenWidth), asCALL_THISCALL);
    ctx->RegisterObjectMethod("ScreenMetrics","float getScreenHeight()",asMETHOD(ScreenMetrics, getScreenHeight), asCALL_THISCALL);

    ctx->RegisterGlobalProperty("ScreenMetrics screenMetrics", &screenMetrics);


    /*******************************************************************************************/
    /**
     * @brief  Main GUI class
     *
     */

    ctx->RegisterObjectType("IMGUI", 0, asOBJ_REF );

    /* We don't want implicitly created IMGUI instances, because it will result in unexpected behaviour for users.
    ctx->RegisterObjectBehaviour("IMGUI",
                                 asBEHAVE_FACTORY,
                                 "IMGUI@ f()",
                                 asFUNCTION(IMGUI::ASFactory),
                                 asCALL_CDECL);
    */


    ctx->RegisterObjectBehaviour("IMGUI", asBEHAVE_ADDREF, "void f()", asMETHOD(IMGUI,AddRef), asCALL_THISCALL);
    ctx->RegisterObjectBehaviour("IMGUI", asBEHAVE_RELEASE, "void f()", asMETHOD(IMGUI,Release), asCALL_THISCALL);

    ctx->RegisterObjectProperty("IMGUI",
                                "GUIState guistate",
                                asOFFSET(IMGUI, guistate));

    ctx->RegisterObjectMethod("IMGUI", "void setBackgroundLayers( uint numLayers )", asMETHOD(IMGUI, setBackgroundLayers), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMGUI", "void setForegroundLayers( uint numLayers )", asMETHOD(IMGUI, setForegroundLayers), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMGUI", "void setHeaderHeight( float _headerSize )", asMETHOD(IMGUI, setHeaderHeight), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMGUI", "void setFooterHeight( float _footerSize )", asMETHOD(IMGUI, setFooterHeight), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMGUI", "void setHeaderPanels( float first = 0, float second = 0, float third = 0 )", asMETHOD(IMGUI, setHeaderPanels), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMGUI", "void setMainPanels( float first = 0, float second = 0, float third = 0 )", asMETHOD(IMGUI, setMainPanels), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMGUI", "void setFooterPanels( float first = 0, float second = 0, float third = 0 )", asMETHOD(IMGUI, setFooterPanels), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMGUI", "void setup()", asMETHOD(IMGUI, setup), asCALL_THISCALL);
//    ctx->RegisterObjectMethod("IMGUI", "void derivePanelOffsets()", asMETHOD(IMGUI, derivePanelOffsets), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMGUI", "void clear()", asMETHOD(IMGUI, clear), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMGUI", "void setGuides( bool setting )", asMETHOD(IMGUI, setGuides), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMGUI", "void reportError( const string &in newError )", asMETHOD(IMGUI, reportError), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMGUI", "void receiveMessage( IMMessage@ message )", asMETHOD(IMGUI, receiveMessage), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMGUI", "uint getMessageQueueSize()", asMETHOD(IMGUI, getMessageQueueSize), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMGUI", "IMMessage@ getNextMessage()", asMETHOD(IMGUI, getNextMessage), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMGUI", "IMContainer@ getMain( uint panel = 0 )", asMETHOD(IMGUI, getMain), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMGUI", "IMContainer@ getFooter( uint panel = 0 )", asMETHOD(IMGUI, getFooter), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMGUI", "IMContainer@ getHeader( uint panel = 0 )", asMETHOD(IMGUI, getHeader), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMGUI", "IMContainer@ getBackgroundLayer( uint layerNum = 0 )", asMETHOD(IMGUI, getBackgroundLayer), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMGUI", "IMContainer@ getForegroundLayer( uint layerNum = 0 )", asMETHOD(IMGUI, getForegroundLayer), asCALL_THISCALL);

    ctx->RegisterObjectMethod("IMGUI", "void onRelayout()", asMETHOD(IMGUI, onRelayout), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMGUI", "void update()", asMETHOD(IMGUI, update), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMGUI", "void doScreenResize()", asMETHOD(IMGUI, doScreenResize), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMGUI", "void render()", asMETHOD(IMGUI, render), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMGUI", "IMElement@ findElement( const string &in elementName )", asMETHOD(IMGUI, findElement), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMGUI", "string getUniqueName( const string &in type = \"Unkowntype\")", asMETHOD(IMGUI, getUniqueName), asCALL_THISCALL);
    ctx->RegisterObjectMethod("IMGUI", "void drawBox( vec2 boxPos, vec2 boxSize, vec4 boxColor, int zOrder, bool shouldClip = false, vec2 currentClipPos = vec2(UNDEFINEDSIZE, UNDEFINEDSIZE), vec2 currentClipSize = vec2(UNDEFINEDSIZE, UNDEFINEDSIZE) )", asMETHOD(IMGUI, drawBox), asCALL_THISCALL);


    ctx->RegisterGlobalFunction("IMGUI@ CreateIMGUI()", asFUNCTION(AS_CreateIMGUI), asCALL_CDECL );

    ctx->DocsCloseBrace();
}
