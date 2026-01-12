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
// Filename    : ccl/gui/graphics/3d/shader/glsl/lighting.frag
// Description : 3D Shader Implementation
//
//************************************************************************************************

#include "glslshader.h"

precision mediump float;

struct AmbientLight
{
	vec4 color;
};

struct DirectionalLight
{
	vec4 direction;
	vec4 color;
};

struct PointLight
{
	vec4 position;
	
	vec4 color;
	
	float constantTerm;
	float linearFactor;
	float quadraticFactor;
};

#if VULKAN_SHADER
layout (binding = CCL_3D_SHADER_LIGHT_PARAMETERS) uniform LightBuffer
#else
uniform struct LightBuffer
#endif
{
	// Lights
	AmbientLight ambientLight;
	DirectionalLight directionalLight;
	PointLight pointLight[CCL_3D_SHADER_MAX_POINTLIGHT_COUNT];
} lightBuffer;

vec3 getBlinnPhongLightColor (in vec3 color, in vec3 lightDirection, in vec3 viewDirection, in vec3 normal, in float shininess)
{
	vec3 diffuseColor = color * max (0.f, dot (normalize (lightDirection), normalize (normal)));
	
	float specularStrength = min (0.5f, shininess);
	vec3 halfwayDirection = normalize (lightDirection + viewDirection);
	float specular = pow (max (dot (halfwayDirection, normalize (normal)), 0.0), shininess);
	vec3 specularColor = specularStrength * specular * color;  
	
	return diffuseColor + specularColor;
}

vec3 getLightColor (in vec4 position, in vec4 normal, in vec4 cameraPosition, in float shininess, in int lightMask)
{	
	vec3 color = vec3 (1.f, 1.f, 1.f);
	
	vec4 viewDirection = cameraPosition - position;
	
	// ambient term
	if((lightMask & CCL_3D_SHADER_AMBIENTLIGHT_BIT) != 0)
		color = lightBuffer.ambientLight.color.rgb;

	// directional light
	if((lightMask & CCL_3D_SHADER_DIRECTIONALLIGHT_BIT) != 0)
		color += getBlinnPhongLightColor (lightBuffer.directionalLight.color.rgb, -lightBuffer.directionalLight.direction.xyz, viewDirection.xyz, normal.xyz, shininess);

	// point lights
	for(int i = 0; i < CCL_3D_SHADER_MAX_POINTLIGHT_COUNT; i++)
	{
		if((lightMask & CCL_3D_SHADER_POINTLIGHT_BIT (i)) != 0 && lightBuffer.pointLight[i].constantTerm + lightBuffer.pointLight[i].linearFactor + lightBuffer.pointLight[i].quadraticFactor > 0.0f)
		{
			vec4 lightDirection = lightBuffer.pointLight[i].position - position;
			vec3 pointLightColor = getBlinnPhongLightColor (lightBuffer.pointLight[i].color.rgb, lightDirection.xyz, viewDirection.xyz, normal.xyz, shininess);
			
			float distance = length (lightDirection.xyz);
			float attenuation = 1.f / (lightBuffer.pointLight[i].constantTerm + lightBuffer.pointLight[i].linearFactor * distance + lightBuffer.pointLight[i].quadraticFactor * distance * distance);
			
			color += pointLightColor * attenuation;
		}
	}
	
	return color;
}
