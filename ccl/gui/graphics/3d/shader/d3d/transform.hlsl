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
// Filename    : ccl/gui/graphics/3d/shader/d3d/transform.hlsl
// Description : 3D Shader Implementation
//
//************************************************************************************************

#include "d3dshader.h"

cbuffer transformBuffer : CCL_REGISTER (b, CCL_3D_SHADER_TRANSFORM_PARAMETERS)
{
	// projection matrix
	float4x4 projectionMatrix;

	// transforms positions from model space to world space
	float4x4 modelMatrix;

	// product of view matrix and model matrix
	float4x4 modelViewMatrix;

	// transforms the normals from model space to world space
	// it's the inverse transpose of the top-left 3x3 part of the model matrix
	float4x4 normalMatrix;
	
	// camera position in world space
	float4 cameraPosition;
}
