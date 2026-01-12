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
// Filename    : ccl/public/gui/graphics/3d/shaderconstants3d.h
// Description : 3D shader constants
//
//************************************************************************************************

#ifndef _ccl_shaderconstants3d_h
#define _ccl_shaderconstants3d_h

//////////////////////////////////////////////////////////////////////////////////////////////////
// Parameter Buffer Indices
//////////////////////////////////////////////////////////////////////////////////////////////////

#define CCL_3D_SHADER_TRANSFORM_PARAMETERS 0
#define CCL_3D_SHADER_MATERIAL_PARAMETERS 1
#define CCL_3D_SHADER_LIGHT_PARAMETERS 2

#define CCL_3D_LAST_SHADER_PARAMETER_INDEX CCL_3D_SHADER_LIGHT_PARAMETERS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Texture Indices
//////////////////////////////////////////////////////////////////////////////////////////////////

#define CCL_3D_DIFFUSE_TEXTURE 0

//////////////////////////////////////////////////////////////////////////////////////////////////
// Other Constants
//////////////////////////////////////////////////////////////////////////////////////////////////

#define CCL_3D_SHADER_MAX_POINTLIGHT_COUNT 5

#define CCL_3D_SHADER_AMBIENTLIGHT_BIT (1 << 0)
#define CCL_3D_SHADER_DIRECTIONALLIGHT_BIT (1 << 1)
#define CCL_3D_SHADER_POINTLIGHT_BIT(index) (1 << (2 + index))

#endif // _ccl_shaderconstants3d_h
