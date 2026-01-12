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
// Filename    : ccl/gui/graphics/3d/shader/d3d/lighting.hlsl
// Description : 3D Shader Implementation
//
//************************************************************************************************

#include "d3dshader.h"

struct AmbientLight
{
	float4 color;
};

struct DirectionalLight
{
	float4 direction;
	float4 color;
};

struct PointLight
{
	float4 position;
	
	float4 color;
	
	float constantTerm;
	float linearFactor;
	float quadraticFactor;
};

cbuffer lightBuffer : CCL_REGISTER (b, CCL_3D_SHADER_LIGHT_PARAMETERS)
{
	AmbientLight ambientLight;
	DirectionalLight directionalLight;
	PointLight pointLight[CCL_3D_SHADER_MAX_POINTLIGHT_COUNT];
};

float3 getBlinnPhongLightColor (float3 color, float3 lightDirection, float3 viewDirection, float3 normal, float shininess)
{
	float3 diffuseColor = color * max (0, dot (normalize (lightDirection), normalize (normal)));
	
	float specularStrength = min (0.5f, shininess);
	float3 halfwayDirection = normalize (lightDirection + viewDirection);
	float specular = pow (max (dot (halfwayDirection, normalize (normal)), 0.0), shininess);
	float3 specularColor = specularStrength * specular * color;  
	
	return diffuseColor + specularColor;
}

float3 getLightColor (float4 position, float4 normal, float4 cameraPosition, float shininess, int lightMask)
{
	float3 color = float3 (1.f, 1.f, 1.f);
	
	float4 viewDirection = cameraPosition - position;
	
	// ambient term
	if((lightMask & CCL_3D_SHADER_AMBIENTLIGHT_BIT) != 0)
		color = ambientLight.color.rgb;

	// diffuse term (directional light)
	if((lightMask & CCL_3D_SHADER_DIRECTIONALLIGHT_BIT) != 0)
		color += getBlinnPhongLightColor (directionalLight.color.rgb, -directionalLight.direction.xyz, viewDirection.xyz, normal.xyz, shininess);

	// diffuse term (point lights)
	for(int i = 0; i < CCL_3D_SHADER_MAX_POINTLIGHT_COUNT; i++)
	{
		if((lightMask & CCL_3D_SHADER_POINTLIGHT_BIT (i)) != 0 && pointLight[i].constantTerm + pointLight[i].linearFactor + pointLight[i].quadraticFactor > 0.0f)
		{
			float4 lightDirection = pointLight[i].position - position;
			float3 pointLightColor = getBlinnPhongLightColor (pointLight[i].color.rgb, lightDirection.xyz, viewDirection.xyz, normal.xyz, shininess);
			
			float distance = length (lightDirection.xyz);
			float attenuation = 1.f / (pointLight[i].constantTerm + pointLight[i].linearFactor * distance + pointLight[i].quadraticFactor * distance * distance);
			
			color += pointLightColor * attenuation;
		}
	}
	
	return color;
}
