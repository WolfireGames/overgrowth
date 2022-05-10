//-----------------------------------------------------------------------------
//           Name: animationeffect.cpp
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

#include "animationeffect.h"

#include <Internal/timer.h>
#include <Internal/common.h>
#include <Internal/filesystem.h>

#include <Graphics/textures.h>
#include <Graphics/animationeffectsystem.h>

#include <XML/xml_helper.h>
#include <Logging/logdata.h>
#include <Main/engine.h>
#include <Asset/AssetLoader/fallbackassetloader.h>

#include <tinyxml.h>

#include <cstdio>
#include <string>

using std::string;

int AnimationEffect::Load(const string& path, uint32_t load_flags) {
    sub_error = 0;
    clear();
    video_path = path.substr(0, path.size() - 4) + ".ogv";

    // I guess this is some kind of fallback onto pure .ogv file if it exists, rather than xml loaded?
    if (!FileExists(video_path.c_str(), kDataPaths | kModPaths)) {
        video_path.clear();

        TiXmlDocument doc;
        if (LoadXMLRetryable(doc, path, "Animation Effect")) {
            TiXmlHandle hDoc(&doc);
            TiXmlHandle hRoot = hDoc.FirstChildElement();
            TiXmlElement* root = hRoot.ToElement();

            int num_frames;
            if (root->QueryIntAttribute("frames", &num_frames) != TIXML_SUCCESS) {
                num_frames = 0;
            }
            if (root->QueryIntAttribute("framerate", &frame_rate) != TIXML_SUCCESS) {
                frame_rate = 30;
            }

            int num_digits = (int)(logf((float)num_frames) / logf(10)) + 1;
            const int FORMAT_BUF_SIZE = 256;
            char format[FORMAT_BUF_SIZE];
            string new_path = path.substr(0, path.size() - 4);
            FormatString(format, FORMAT_BUF_SIZE, "%s%%0%dd.tga", new_path.c_str(), num_digits);

            const int TEX_PATH_BUF_SIZE = 256;
            char tex_path[TEX_PATH_BUF_SIZE];
            frames.resize(num_frames);
            for (int i = 0; i < num_frames; ++i) {
                FormatString(tex_path, TEX_PATH_BUF_SIZE, format, i);
                frames[i] = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>(tex_path, PX_SRGB, 0x0);
            }
        } else {
            return kLoadErrorMissingFile;
        }
    }

    return kLoadOk;
}

const char* AnimationEffect::GetLoadErrorString() {
    return "";
}

void AnimationEffect::Unload() {
}

void AnimationEffect::Reload() {
    Load(path_, 0x0);
}

void AnimationEffect::ReportLoad() {
}

void AnimationEffect::clear() {
    frames.clear();
}

AnimationEffect::~AnimationEffect() {
    clear();
}

AnimationEffect::AnimationEffect(AssetManager* owner, uint32_t asset_id) : Asset(owner, asset_id), sub_error(0) {
}

AssetLoaderBase* AnimationEffect::NewLoader() {
    return new FallbackAssetLoader<AnimationEffect>();
}

void AnimationEffectReader::Update(float timestep) {
    if (!use_theora) {
        time += timestep;
        int frame = (int)(time * ae_ref->frame_rate);
        if (frame >= (int)ae_ref->frames.size()) {
            ae_ref.clear();
        }
    }
}

bool AnimationEffectReader::valid() {
    return ae_ref.valid();
}

void AnimationEffectReader::AttachTo(const AnimationEffectRef& _ae_ref) {
    ae_ref = _ae_ref;
    time = 0.0f;
    // if (clip) {
    //     Dispose();
    // }
    use_theora = !ae_ref->video_path.empty();
}

TextureRef& AnimationEffectReader::GetTextureAssetRef() {
    // int frame = (int)(time * ae_ref->frame_rate)%((int)ae_ref->frames.size());
    // return ae_ref->frames[frame];

    // if (!clip && use_theora) {
    //     clip_mem = new TheoraMemoryFileDataSource(ae_ref->video_path);
    //     TheoraVideoManager* mgr = Engine::Instance()->GetAnimationEffectSystem()->mgr;
    //     clip = mgr->createVideoClip(clip_mem, TH_BGRA);
    //     clip->setAutoRestart(0);
    //     clip_tex = Textures::Instance()->makeTextureColor(
    //         clip->getWidth(), clip->getHeight(), GL_RGBA, GL_RGBA, 0.0f, 0.0f, 0.0f, 0.0f, false);
    //     Textures::Instance()->SetTextureName(clip_tex, "Animation Effect Clip");
    // }

    // TheoraVideoFrame* f = clip->getNextFrame();
    // if (f) {
    //     Textures::Instance()->bindTexture(clip_tex);
    //     glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, clip->getWidth(), f->getHeight(), GL_BGRA, GL_UNSIGNED_BYTE, f->getBuffer());
    //     clip->popFrame();
    // }
    return clip_tex;
}

AnimationEffectReader::~AnimationEffectReader() {
    Dispose();
}

AnimationEffectReader::AnimationEffectReader() : use_theora(false) {
}

AnimationEffectReader::AnimationEffectReader(const AnimationEffectReader& other) {
    time = other.time;
    ae_ref = other.ae_ref;
    use_theora = other.use_theora;
}

void AnimationEffectReader::Dispose() {
    if (use_theora) {
        // Engine::Instance()->GetAnimationEffectSystem()->mgr->destroyVideoClip(clip);
        // if (clip_mem) {
        //     delete clip_mem;
        //     clip_mem = NULL;
        // }
    }
}

bool AnimationEffectReader::Done() {
    // if (clip) {
    //     return clip->isDone();
    // }
    return false;
}
