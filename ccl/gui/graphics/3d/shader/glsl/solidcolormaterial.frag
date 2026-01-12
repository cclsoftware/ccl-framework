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
// Filename    : ccl/gui/graphics/3d/shader/glsl/solidcolormaterial.frag
// Description : 3D Shader Implementation
//
//************************************************************************************************

#include "lighting.frag"

layout (location = 0) in vec4 positionSS;
layout (location = 1) in vec4 normalWS;
layout (location = 2) in vec4 positionWS;
layout (location = 3) in vec4 cameraPositionWS;

#if VULKAN_SHADER
layout (binding = CCL_3D_SHADER_MATERIAL_PARAMETERS) uniform ConstantBuffer
#else
uniform struct ConstantBuffer
#endif
{
	// Material
	int lightMask;
	vec4 materialColor;
	float shininess;
} cBuffer;

layout (location = 0) out vec4 outColor;

void main ()
{
	vec3 lightColor = getLightColor (positionWS, normalWS, cameraPositionWS, cBuffer.shininess, cBuffer.lightMask);

	outColor = vec4 (lightColor * cBuffer.materialColor.rgb, cBuffer.materialColor.a);
}
