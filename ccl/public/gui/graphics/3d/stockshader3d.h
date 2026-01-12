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
// Filename    : ccl/public/gui/graphics/3d/stockshader3d.h
// Description : 3D stock shaders
//
//************************************************************************************************

#ifndef _ccl_stockshader3d_h
#define _ccl_stockshader3d_h

#include "ccl/public/text/cstring.h"

#include "ccl/public/gui/graphics/3d/shaderconstants3d.h"

namespace CCL {

//************************************************************************************************
// 3D Stock shader names
//************************************************************************************************

namespace StockShaders
{
	DEFINE_STRINGID (kVertexShaderPN, "shaders/vertexshaderPN")					///< vertex shader using position and normals
	DEFINE_STRINGID (kVertexShaderPNT, "shaders/vertexshaderPNT")				///< vertex shader using positon, normals and texture coordinates
	DEFINE_STRINGID (kConvertingVertexShader, "shaders/vertexshaderPNT2PN")		///< vertex shader using positon, normals and texture coordinates, but passing only position annd normals to the pixel shader
	DEFINE_STRINGID (kBillboardVertexShader, "shaders/billboardvertexshader")	///< vertex shader used to render billboard sprites
	DEFINE_STRINGID (kSolidColorMaterialShader, "shaders/solidcolormaterial")	///< solid color pixel shader
	DEFINE_STRINGID (kTextureMaterialShader, "shaders/texturematerial")			///< textured pixel shader

	const int kMaxPointLightCount = CCL_3D_SHADER_MAX_POINTLIGHT_COUNT;
}

//************************************************************************************************
// 3D Shader parameter buffer indices
//************************************************************************************************

DEFINE_ENUM (ParameterBufferIndex3D)
{
	kTransformParameters = CCL_3D_SHADER_TRANSFORM_PARAMETERS,
	kMaterialParameters = CCL_3D_SHADER_MATERIAL_PARAMETERS,
	kLightParameters = CCL_3D_SHADER_LIGHT_PARAMETERS,
	
	kLastShaderParameterIndex = CCL_3D_LAST_SHADER_PARAMETER_INDEX
};

//************************************************************************************************
// 3D Shader texture indices
//************************************************************************************************

DEFINE_ENUM (TextureIndex3D)
{
	kDiffuseTexture = CCL_3D_DIFFUSE_TEXTURE
};

//************************************************************************************************
// 3D Shader parameter names
//************************************************************************************************

namespace ParamName3D
{
	DEFINE_STRINGID (kProjectionMatrix, "projectionMatrix") ///< [Transform3D]
	DEFINE_STRINGID (kModelMatrix, "modelMatrix") ///< [Transform3D]
	DEFINE_STRINGID (kModelViewMatrix, "modelViewMatrix") ///< [Transform3D]
	DEFINE_STRINGID (kNormalMatrix, "normalMatrix") ///< [Transform3D]
	DEFINE_STRINGID (kCameraPosition, "cameraPosition") ///< [PointF4D]

	DEFINE_STRINGID (kAmbientLight, "ambientLight") ///< [struct]
	DEFINE_STRINGID (kDirectionalLight, "directionalLight") ///< [struct]
	DEFINE_STRINGID (kPointLight, "pointLight") ///< [struct]

	DEFINE_STRINGID (kLightColor, "color") ///< [ColorF]
	DEFINE_STRINGID (kLightDirection, "direction") ///< [PointF4D]
	DEFINE_STRINGID (kLightPosition, "position") ///< [PointF4D]
	
	DEFINE_STRINGID (kPointLightConstantTerm, "constantTerm") ///< [float]
	DEFINE_STRINGID (kPointLightLinearFactor, "linearFactor") ///< [float]
	DEFINE_STRINGID (kPointLightQuadraticFactor, "quadraticFactor") ///< [float]

	DEFINE_STRINGID (kLightMask, "lightMask") ///< [int]
	DEFINE_STRINGID (kMaterialColor, "materialColor") ///< [ColorF]
	DEFINE_STRINGID (kShininess, "shininess") ///< [float]
	DEFINE_STRINGID (kOpacity, "opacity") ///< [float]
}

} // namespace CCL

#endif // _ccl_stockshader3d_h
