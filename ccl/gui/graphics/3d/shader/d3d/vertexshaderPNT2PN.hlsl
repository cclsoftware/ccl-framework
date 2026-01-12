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
// Filename    : ccl/gui/graphics/3d/shader/d3d/vertexshaderPNT2PN.hlsl
// Description : 3D Shader Implementation
//
//************************************************************************************************

#include "transform.hlsl"

struct VertexShaderInput
{
	float3 positionMS : POSITION; // the positions in model space (MS)
	float3 normalMS : NORMAL; // the normals in model space (MS)
	float2 texCoord : TEXCOORD; // texture coordinates
};

struct VertexShaderOutput
{
	float4 positionSS : SV_POSITION; // the positions in screen space (SS)
	float4 normalWS : NORMAL; // the normals in world space (WS)
	float4 positionWS: POSITION; // the positions in world space (WS)
	float4 cameraPositionWS : POSITION1; // camera position in world space (WS)
};

VertexShaderOutput main (VertexShaderInput input)
{
	float4x4 modelViewProjectionMatrix = mul (modelViewMatrix, projectionMatrix);

	VertexShaderOutput output;
	output.positionSS = mul (float4 (input.positionMS, 1.0), modelViewProjectionMatrix);
	output.normalWS = mul (float4 (input.normalMS, 0.0), normalMatrix);
	output.positionWS = mul (float4 (input.positionMS, 1.0), modelMatrix);
	output.cameraPositionWS = cameraPosition;
	return output;
}
