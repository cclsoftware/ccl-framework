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
// Filename    : ccl/gui/graphics/3d/shader/d3d/billboardvertexshader.hlsl
// Description : 3D Shader Implementation
//
//************************************************************************************************

#include "transform.hlsl"

struct VertexShaderInput
{
	float3 positionMS : POSITION; // the positions in model space (MS)
	float2 texCoord : TEXCOORD; // texture coordinates
};

struct VertexShaderOutput
{
	float4 positionSS : SV_POSITION; // the positions in screen space (SS)
	float4 normalWS : NORMAL; // the normals in world space (WS)
	float4 positionWS: POSITION; // the positions in world space (WS)
	float4 cameraPositionWS : POSITION1; // camera position in world space (WS)
	float2 texCoord : TEXCOORD; // texture coordinates
};

VertexShaderOutput main (VertexShaderInput input)
{
	float4x4 billboardMatrix = modelViewMatrix;
	billboardMatrix[0][0] = length (modelViewMatrix[0]);
	billboardMatrix[0][1] = 0.0;
	billboardMatrix[0][2] = 0.0;
	billboardMatrix[1][0] = 0.0;
	billboardMatrix[1][1] = length (modelViewMatrix[1]);
	billboardMatrix[1][2] = 0.0;
	billboardMatrix[2][0] = 0.0;
	billboardMatrix[2][1] = 0.0;
	billboardMatrix[2][2] = 1.0;

	float4x4 modelViewProjectionMatrix = mul (billboardMatrix, projectionMatrix);

	VertexShaderOutput output;
	output.positionSS = mul (float4 (input.positionMS, 1.0), modelViewProjectionMatrix);
	output.normalWS = float4 (0.0, 0.0, 1.0, 0.0);
	output.positionWS = mul (float4 (input.positionMS, 1.0), modelMatrix);
	output.cameraPositionWS = cameraPosition;
	output.texCoord = input.texCoord;
	return output;
}
