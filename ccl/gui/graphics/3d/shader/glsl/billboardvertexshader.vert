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
// Filename    : ccl/gui/graphics/3d/shader/glsl/billboardvertexshader.vert
// Description : 3D Shader Implementation
//
//************************************************************************************************

#include "transform.vert"

layout (location = 0) in vec3 positionMS; // the positions in model space (MS)
layout (location = 1) in vec2 texCoord; // texture coordinates

layout (location = 0) out vec4 positionSS; // the positions in screen space (SS)
layout (location = 1) out vec4 normalWS; // the normals in world space (WS)
layout (location = 2) out vec4 positionWS; // the positions in world space (WS)
layout (location = 3) out vec4 cameraPositionWS; // camera position in world space (WS)
layout (location = 4) out vec2 fragTexCoord; // texture coordinates

void main ()
{
	mat4 modelViewMatrix = transformBuffer.modelViewMatrix;
	float scaleX = length (vec3 (modelViewMatrix[0][0], modelViewMatrix[1][0], modelViewMatrix[2][0]));
	float scaleY = length (vec3 (modelViewMatrix[0][1], modelViewMatrix[1][1], modelViewMatrix[2][1]));
	modelViewMatrix[0][0] = scaleX;
	modelViewMatrix[0][1] = 0.0;
	modelViewMatrix[0][2] = 0.0;
	modelViewMatrix[1][0] = 0.0;
	modelViewMatrix[1][1] = scaleY;
	modelViewMatrix[1][2] = 0.0;
	modelViewMatrix[2][0] = 0.0;
	modelViewMatrix[2][1] = 0.0;
	modelViewMatrix[2][2] = 1.0;

	mat4 modelViewProjectionMatrix = modelViewMatrix * transformBuffer.projectionMatrix;

	positionSS = vec4 (positionMS, 1.0) * modelViewProjectionMatrix;
	normalWS = vec4 (0.0, 0.0, 1.0, 0.0);
	positionWS = vec4 (positionMS, 1.0) * transformBuffer.modelMatrix;
	cameraPositionWS = transformBuffer.cameraPosition;
	fragTexCoord = texCoord;
	
	gl_Position = positionSS;
	#if VULKAN_SHADER
	gl_Position.y = -gl_Position.y;
	#endif
}
