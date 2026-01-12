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
// Filename    : ccl/gui/graphics/3d/shader/metal/transform.h
// Description : 3D Shader Implementation
//
//************************************************************************************************

#include "metalshader.h"

using namespace metal;

struct TransformBuffer
{
	matrix_float4x4 projectionMatrix [[id(0)]];
	matrix_float4x4 modelMatrix [[id(1)]];
	matrix_float4x4 modelViewMatrix [[id(2)]];
	matrix_float4x4 normalMatrix [[id(3)]];
	float4 cameraPosition [[id(4)]];
};
