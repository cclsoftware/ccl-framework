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
// Filename    : ccl/gui/graphics/3d/shader/metal/solidcolormaterial.metal
// Description : 3D Shader Implementation
//
//************************************************************************************************

#include "lighting.h"

using namespace metal;

struct PixelShaderInput
{
	float4 positionSS [[position]];
	float4 normalWS;
	float4 cameraPositionWS;
	float4 positionWS;
};

struct PixelConstants
{
	// Material
	int lightMask [[id(0)]];
	float4 materialColor [[id(1)]];
	float shininess [[id(2)]];
};

fragment float4 function (PixelShaderInput input [[stage_in]], constant PixelConstants& constants [[buffer (CCL_3D_SHADER_MATERIAL_PARAMETERS)]], constant LightConstants& lightConstants [[buffer (CCL_3D_SHADER_LIGHT_PARAMETERS)]])
{
	float3 lightColor = getLightColor (input.positionWS, input.normalWS, input.cameraPositionWS, constants.shininess, constants.lightMask, lightConstants);
	
	return float4 (lightColor * constants.materialColor.xyz, constants.materialColor.a);
}
