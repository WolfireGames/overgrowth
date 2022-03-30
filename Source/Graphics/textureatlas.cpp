//-----------------------------------------------------------------------------
//           Name: textureatlas.cpp
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
#include "textureatlas.h"

#include <Graphics/graphics.h>
#include <Graphics/shaders.h>

#include <Math/ivec2math.h>
#include <Logging/logdata.h>
#include <Internal/profiler.h>
#include <Utility/stacktrace.h>
#include <Main/engine.h>

#include <algorithm>

TextureAtlas::TextureAtlas( AtlasNodeTree& atlasNodeTree, TextureData::ColorSpace colorSpace )
{
    PROFILER_ENTER(g_profiler_ctx, "Getting dimensions");
    this->dim = atlasNodeTree.GetNodeRoot()->dim;
    LOGI << "Creating a decal texture atlas with dimensions " << dim << std::endl;
    PROFILER_LEAVE(g_profiler_ctx);

    //atlas_texture = Textures::Instance()->makeTextureColor( dim[0], dim[1], GL_RGBA, GL_RGBA, 1,0,0,1, true );
    //atlas_texture = Textures::Instance()->makeTextureTestPattern( 128, 128, GL_RGBA, GL_RGBA, true );
    PROFILER_ENTER(g_profiler_ctx, "Making texture");
    GLenum format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    if (colorSpace == TextureData::sRGB) {
        format = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
    }
    Textures* textures = Textures::Instance();
    atlas_texture = textures->makeTexture(dim[0], dim[1], format, format, true);
    textures->SetTextureName(atlas_texture, "Atlas Texture");
    PROFILER_LEAVE(g_profiler_ctx);
}

TextureAtlasRef TextureAtlas::SetTextureToNode(const std::string& rel_path, AtlasNodeTree::AtlasNodeRef ref )
{
    TextureAssetRef tex = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>( rel_path );

    int reduction_factor = Graphics::Instance()->config_.texture_reduction_factor();
    ivec2 texdim = ivec2(Textures::Instance()->getWidth(tex->GetTextureRef()),Textures::Instance()->getHeight(tex->GetTextureRef())) / reduction_factor;


    TextureAtlasRef ret;

    ret.source = this;
    ret.pos = ref.node->pos;

    //I supect that in fact the node dimensions are used when rendering, so the size has to be the same for
    //textures to render correctly
    if( texdim == ref.node->dim || (texdim[0]<= ref.node->dim[0] && texdim[1] <= ref.node->dim[1]) )
    {
        dirtyNodes.push(std::pair<std::string,AtlasNodeTree::AtlasNode*>(rel_path,ref.node));

        ret.dim = texdim;
    }
    else
    {
        LOGE << "Texture is bigger than node, this prevents us from rendering the texture to the node because the node size is used as the texture size when rendering. Either make the texture bigger, or make the texture atlas have smaller minimum size." << std::endl;
        LOGE << "What will be rendered is unset memory as this node will be allocated but not written to." << std::endl;
        LOGE << "Texture: " << rel_path << std::endl;
        LOGE << "texdim: " << texdim << " ref.node->dim: " << ref.node->dim << std::endl;
        //LOGE << GenerateStacktrace()  << std::endl;

        ret.dim = ref.node->dim;
    }

    vec2 texel_size = vec2(1.0f / this->dim[0], 1.0f / this->dim[1]);

    ret.uv_start = vec2(ret.pos)/vec2(this->dim) + vec2(texel_size[0] * 0.5f, texel_size[1] * 0.5f);
    ret.uv_size = vec2(ret.dim)/vec2(this->dim) - texel_size;

    return ret;
}

void TextureAtlas::DrawToAtlasNode( const std::string& rel_path, const AtlasNodeTree::AtlasNode& node )
{
    TextureAssetRef tex = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>( rel_path );

    if( !rel_path.empty() )
    { 
        Textures::Instance()->subImage( atlas_texture, tex->GetTextureRef(), node.pos[0], node.pos[1] );
    }
    else
    {
        //Texture::Instance()->subImageTestPattern( atlas_texture, node.pos[0], node.pos[1] );
    }
}

void TextureAtlas::Draw()
{
    //Let's load one texture per frame to even out the load a little between frames.
    if( dirtyNodes.empty() == false )
    {
        DrawToAtlasNode( dirtyNodes.front().first, *(dirtyNodes.front().second) ); 
        dirtyNodes.pop();
    }
}
