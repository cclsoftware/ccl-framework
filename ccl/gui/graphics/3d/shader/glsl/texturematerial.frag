#version 310 es

//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/gui/graphics/3d/shader/glsl/texturematerial.frag
// Description : 3D Shader Implementation
//
//************************************************************************************************

#include "lighting.frag"

layout (location = 0) in vec4 positionSS;
layout (location = 1) in vec4 normalWS;
layout (location = 2) in vec4 positionWS;
layout (location = 3) in vec4 cameraPositionWS;
layout (location = 4) in vec2 texCoord;

#if VULKAN_SHADER
layout (binding = CCL_3D_SHADER_MATERIAL_PARAMETERS) uniform ConstantBuffer
#else
uniform struct ConstantBuffer
#endif
{
	// Material
	int lightMask;
	vec4 materialColor;
	float opacity;
	float shininess;
} cBuffer;

#if VULKAN_SHADER
layout (binding = CCL_GLSL_DIFFUSE_TEXTURE) uniform sampler2D textureSampler;
#else
uniform sampler2D textureSampler;
#endif

layout (location = 0) out vec4 outColor;

void main()
{
	vec3 lightColor = getLightColor (positionWS, normalWS, cameraPositionWS, cBuffer.shininess, cBuffer.lightMask);

	vec4 textureColor = texture (textureSampler, texCoord);
	vec4 combinedColor = cBuffer.materialColor * (1.f - textureColor.a) + textureColor * textureColor.a;
	
	outColor = vec4 (lightColor * combinedColor.rgb, cBuffer.opacity * min (textureColor.a + cBuffer.materialColor.a, 1.0));
	//outColor = textureColor;
	//outColor = vec4 (texCoord, 0.0, 1.0);
}
