//-----------------------------------------------------------------------------
//           Name: texturepack.cpp
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
#include "texturepack.h"

#include <Graphics/textures.h>
#include <Graphics/graphics.h>
#include <Graphics/shaders.h>

#include <Math/vec2math.h>

void DrawRect(const vec2 &rect_min, const vec2 &rect_max){
    glBegin(GL_QUADS);
        glTexCoord2f(0,0);
        glVertex3f(rect_min[0],rect_min[1],0.0f);
        glTexCoord2f(1,0);
        glVertex3f(rect_max[0],rect_min[1],0.0f);
        glTexCoord2f(1,1);
        glVertex3f(rect_max[0],rect_max[1],0.0f);
        glTexCoord2f(0,1);
        glVertex3f(rect_min[0],rect_max[1],0.0f);
    glEnd();
}

TexturePackNode::TexturePackNode():
    filled(false)
{
    children[0] = NULL;
    children[1] = NULL;
}

void TexturePackNode::SetRect(const vec2 _rect_min , const vec2 _rect_max)
{
    rect_min = _rect_min;
    rect_max = _rect_max;
}

TexturePackNode* TexturePackNode::Insert( const TextureAssetRef &texture )
{
    TexturePackNode* new_node;

    //If node has children, try to insert in each child
    if(children[0]){
        new_node = children[0]->Insert(texture);
        if(new_node){
            return new_node;
        } else {
            return children[1]->Insert(texture);
        }
    // If node has no children, try to insert in this node
    } else {
        // If node is full, can't do anything here!
        if(filled){
            return NULL;
        }
        
        // If image is bigger than node, can't fit here!
        const vec2 rect_dimensions = rect_max - rect_min;
        const int texture_width = Textures::Instance()->getWidth(texture->GetTextureRef());
        const int texture_height = Textures::Instance()->getHeight(texture->GetTextureRef());
        if(texture_width > rect_dimensions[0] ||
           texture_height > rect_dimensions[1]){
            return NULL;
        }

        // If image is exactly the size of this node, draw it in!
        if(texture_width == rect_dimensions[0] &&
           texture_height == rect_dimensions[1]){
            Textures::Instance()->bindTexture(texture->GetTextureRef());
            DrawRect(rect_min,rect_max);
            filled = true;
            return this;
        }

        // Image fits inside, but not perfectly.
        // Time to split!
        children[0] = new TexturePackNode();
        children[1] = new TexturePackNode();
            
        int extra_width = (int)rect_dimensions[0] - texture_width;
        int extra_height = (int)rect_dimensions[1] - texture_height;
        if(extra_width > extra_height){
            children[0]->SetRect(rect_min, vec2(rect_min[0]+texture_width, rect_max[1]));
            children[1]->SetRect(vec2(rect_min[0]+texture_width, rect_min[1]), rect_max);
        } else {
            children[0]->SetRect(rect_min, vec2(rect_max[0], rect_min[1]+texture_height));
            children[1]->SetRect(vec2(rect_min[0], rect_min[1]+texture_height), rect_max);
        }

        // Insert in the correctly-sized child node
        return children[0]->Insert(texture);
    }
}

void TexturePackNode::Dispose()
{
    if(children[0]){
        delete children[0];
        children[0] =  NULL;
    }
    if(children[1]){
        delete children[1];
        children[1] =  NULL;
    }
    filled = false;
}

TexturePackNode::~TexturePackNode()
{
    Dispose();
}

const vec2& TexturePackNode::GetMin() const
{
    return rect_min;
}

const vec2& TexturePackNode::GetMax() const
{
    return rect_max;
}

TexturePack::TexturePack() {
}


struct TexturePackParamSort {
    TexturePackParam param;
    int area;
    int orig_id;
};

struct TextureSizeSorter {
    bool operator()(const TexturePackParamSort& a, const TexturePackParamSort& b){
        return a.area > b.area;
    }
};

void TexturePack::Create( int _size, std::vector<TexturePackParam> &textures )
{
    Dispose();

    size = _size;

    int widest = 0;
    int tallest = 0;

    std::vector<TexturePackParamSort> sorted_textures(textures.size());
    for(unsigned i=0; i<sorted_textures.size(); i++){
        sorted_textures[i].param = textures[i];
        sorted_textures[i].orig_id = i;
        Textures::Instance()->EnsureInVRAM(textures[i].texture->GetTextureRef());
        sorted_textures[i].area = Textures::Instance()->getWidth(textures[i].texture->GetTextureRef())*
                                  Textures::Instance()->getHeight(textures[i].texture->GetTextureRef());
        textures[sorted_textures[i].orig_id].inserted = false;
    }
    sort(sorted_textures.begin(), sorted_textures.end(), TextureSizeSorter());
    
    int total_area_remaining = size*size;
    int cutoff = 0;
    for(int i=(int)sorted_textures.size()-1; i>=0; i--){
        total_area_remaining -= sorted_textures[i].area;
        if(total_area_remaining<0){
            cutoff = i+1;
            break;
        } else {
            widest = max(widest, Textures::Instance()->getWidth(textures[sorted_textures[i].orig_id].texture->GetTextureRef()));
            tallest = max(tallest, Textures::Instance()->getHeight(textures[sorted_textures[i].orig_id].texture->GetTextureRef()));
        }
    }

    const int area_used = size*size - total_area_remaining;
    while(area_used < size*size/4 && size >= widest*2 && size >= tallest*2){
        size/=2;
    }

    //pack_texture = Textures::Instance()->makeTextureColor(size,size);
    pack_texture = Textures::Instance()->makeTextureTestPattern(size,size);
    Textures::Instance()->SetTextureName(pack_texture, "Texture Pack");

    GLuint framebuffer;
    Graphics::Instance()->genFramebuffers(&framebuffer, "texture_pack");
    Graphics::Instance()->PushFramebuffer();
    Graphics::Instance()->bindFramebuffer(framebuffer);
    Graphics::Instance()->framebufferColorTexture2D(pack_texture);

    Shaders::Instance()->noProgram();

    GLState gl_state;
    gl_state.blend = false;
    gl_state.cull_face = false;
    gl_state.depth_test = false;
    gl_state.depth_write = false;

    Graphics::Instance()->setGLState(gl_state);
    glColor4f(1.0f,1.0f,1.0f,1.0f);

    Graphics::Instance()->PushViewport();
    Graphics::Instance()->setViewport(0,0,size,size);
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0,size,0,size,-100,100);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    root_node.SetRect(vec2(0.0f,0.0f),vec2((float)size,(float)size));

    TexturePackNode *inserted;
    for(unsigned i=cutoff; i<sorted_textures.size(); i++){
        inserted = root_node.Insert(sorted_textures[i].param.texture);
        if(inserted){
            textures[sorted_textures[i].orig_id].offset[0] = inserted->GetMin()[0]/(float)size;
            textures[sorted_textures[i].orig_id].offset[1] = inserted->GetMin()[1]/(float)size;
            textures[sorted_textures[i].orig_id].offset[2] = Textures::Instance()->getWidth(textures[sorted_textures[i].orig_id].texture->GetTextureRef())/(float)size;
            textures[sorted_textures[i].orig_id].offset[3] = Textures::Instance()->getHeight(textures[sorted_textures[i].orig_id].texture->GetTextureRef())/(float)size;
            textures[sorted_textures[i].orig_id].inserted = true;
        }
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    Graphics::Instance()->PopViewport();
    Graphics::Instance()->PopFramebuffer();
    Graphics::Instance()->deleteFramebuffer(&framebuffer);

    //Textures::Instance()->SetDebugTextureAssetRef(pack_texture);
    Textures::Instance()->DeleteUnusedTextures();
}

TexturePack::~TexturePack() {
    Dispose();
}

void TexturePack::Dispose()
{
    root_node.Dispose();
}
