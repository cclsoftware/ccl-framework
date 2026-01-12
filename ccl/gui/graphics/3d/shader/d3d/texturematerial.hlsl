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
// Filename    : ccl/gui/graphics/3d/shader/d3d/texturematerial.hlsl
// Description : 3D Shader Implementation
//
//************************************************************************************************

#include "lighting.hlsl"

struct PixelShaderInput
{
	float4 positionSS : SV_POSITION;
	float4 normalWS : NORMAL;
	float4 positionWS : POSITION;
	float4 cameraPositionWS : POSITION1;
	float2 texCoord : TEXCOORD;
};

cbuffer cBuffer : CCL_REGISTER (b, CCL_3D_SHADER_MATERIAL_PARAMETERS)
{
	// Material
	int lightMask;
	float4 materialColor;
	float shininess;
	float opacity;
};

Texture2D materialTexture : CCL_REGISTER (t, CCL_3D_DIFFUSE_TEXTURE);

SamplerState textureSampler;

float4 main (PixelShaderInput input) : SV_TARGET
{
	float3 lightColor = getLightColor (input.positionWS, input.normalWS, input.cameraPositionWS, shininess, lightMask);

	float4 textureColor = materialTexture.Sample (textureSampler, input.texCoord);
	float4 combinedColor = materialColor * (1.f - textureColor.a) + textureColor * textureColor.a;
	
	return float4 (lightColor * combinedColor.rgb, opacity * min (textureColor.a + materialColor.a, 1.0));
}
