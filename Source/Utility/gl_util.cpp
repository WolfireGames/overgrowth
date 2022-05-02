//-----------------------------------------------------------------------------
//           Name: gl_util.cpp
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
#include "gl_util.h"

const char* GLStringInternalFormat( const GLenum internal_format ) {
	switch( internal_format ) {
	case GL_COMPRESSED_RED:
		return "COMPRESSED_RED RED Generic unorm";
	case GL_COMPRESSED_RG:
		return "COMPRESSED_RG RG Generic unorm";
	case GL_COMPRESSED_RGB:
		return "COMPRESSED_RGB RGB Generic unorm";
	case GL_COMPRESSED_RGBA:
		return "COMPRESSED_RGBA RGBA Generic unorm";
	case GL_COMPRESSED_SRGB:
		return "COMPRESSED_SRGB RGB Generic unorm";
	case GL_COMPRESSED_SRGB_ALPHA:
		return "COMPRESSED_SRGB_ALPHA RGBA Generic unorm";
	case GL_COMPRESSED_RED_RGTC1:
		return "COMPRESSED_RED_RGTC1 RED Specific unorm";
	case GL_COMPRESSED_SIGNED_RED_RGTC1:
		return "COMPRESSED_SIGNED_RED_RGTC1 RED Specific snorm";
	case GL_COMPRESSED_RG_RGTC2:
		return "COMPRESSED_RG_RGTC2 RG Specific unorm";
	case GL_COMPRESSED_SIGNED_RG_RGTC2:
		return "COMPRESSED_SIGNED_RG_RGTC2 RG Specific snorm";
	// case GL_COMPRESSED_RGBA_BPTC_UNORM:
	// 	return "COMPRESSED_RGBA_BPTC_UNORM RGBA Specific unorm";
	// case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
	// 	return "COMPRESSED_SRGB_ALPHA_BPTC_UNORM RGBA Specific unorm";
	// case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
	// 	return "COMPRESSED_RGB_BPTC_SIGNED_FLOAT RGB Specific float";
	// case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
	// 	return "COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT RGB Specific float";
	// case GL_COMPRESSED_RGB8_ETC2:
	// 	return "COMPRESSED_RGB8_ETC2 RGB Specific unorm";
	// case GL_COMPRESSED_SRGB8_ETC2:
	// 	return "COMPRESSED_SRGB8_ETC2 RGB Specific unorm";
	// case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
	// 	return "COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 RGB Specific unorm";
	// case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 :
	// 	return "COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 RGB Specific unorm";
	// case GL_COMPRESSED_RGBA8_ETC2_EAC:
	// 	return "COMPRESSED_RGBA8_ETC2_EAC RGBA Specific unorm";
	// case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
	// 	return "COMPRESSED_SRGB8_ALPHA8_ETC2_EAC RGBA Specific unorm";
	// case GL_COMPRESSED_R11_EAC:
	// 	return "COMPRESSED_R11_EAC RED Specific unorm";
	// case GL_COMPRESSED_SIGNED_R11_EAC:
	// 	return "COMPRESSED_SIGNED_R11_EAC RED Specific snorm";
	// case GL_COMPRESSED_RG11_EAC:
	// 	return "COMPRESSED_RG11_EAC RG Specific unorm";
	// case GL_COMPRESSED_SIGNED_RG11_EAC:
	// 	return "COMPRESSED_SIGNED_RG11_EAC RG Specific snorm";

	case GL_SRGB_ALPHA:
		return "GL_SRGB_ALPHA";
	case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
		return "GL_COMPRESSED_SRGB_S3TC_DXT1_EXT";
	case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
		return "GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT";
	case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
		return "GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT";
	case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
		return "GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT";

	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		return "GL_COMPRESSED_RGBA_S3TC_DXT5_EXT";
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		return "GL_COMPRESSED_RGBA_S3TC_DXT3_EXT";
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		return "GL_COMPRESSED_RGBA_S3TC_DXT1_EXT";
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		return "GL_COMPRESSED_RGB_S3TC_DXT1_EXT";

	default:
		return "UNKNOWN internal format";
	}
}
