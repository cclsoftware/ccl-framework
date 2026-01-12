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
// Filename    : ccl/gui/graphics/3d/shader/metal/billboardvertexshader.metal
// Description : 3D Shader Implementation
//
//************************************************************************************************

#include "transform.h"

using namespace metal;

struct VertexData
{
	float3 positionMS [[attribute(0)]];
	float2 texCoord [[attribute(1)]];
};

struct VertexShaderOutput
{
	float4 positionSS [[position]];
	float4 normalWS;
	float4 positionWS;
	float4 cameraPositionWS;
	float2 texCoord;
};

VertexShaderOutput vertex function (VertexData v [[stage_in]], constant TransformBuffer& constants [[buffer (CCL_3D_SHADER_TRANSFORM_PARAMETERS)]])
{
	float4x4 billboardMatrix = constants.modelViewMatrix;
	float scaleX = length (float3 (billboardMatrix[0][0], billboardMatrix[1][0], billboardMatrix[2][0]));
	float scaleY = length (float3 (billboardMatrix[0][1], billboardMatrix[1][1], billboardMatrix[2][1]));
	billboardMatrix[0][0] = scaleX;
	billboardMatrix[0][1] = 0.0;
	billboardMatrix[0][2] = 0.0;

	billboardMatrix[1][0] = 0.0;
	billboardMatrix[1][1] = scaleY;
	billboardMatrix[1][2] = 0.0;

	billboardMatrix[2][0] = 0.0;
	billboardMatrix[2][1] = 0.0;
	billboardMatrix[2][2] = 1.0;
	
	float4x4 modelViewProjectionMatrix = billboardMatrix * constants.projectionMatrix;

	VertexShaderOutput output;
	output.positionSS = float4 (v.positionMS, 1.0) * modelViewProjectionMatrix;
	output.normalWS = float4 (0.0, 0.0, 1.0, 0.0);
	output.positionWS = float4 (v.positionMS, 1.0) * constants.modelMatrix;
	output.cameraPositionWS = constants.cameraPosition;
	output.texCoord = v.texCoord;
	return output;
}
