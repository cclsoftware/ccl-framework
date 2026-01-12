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
// Filename    : ccl/gui/graphics/3d/shader/d3d/d3dshader.h
// Description : Direct 3D Shader Macros
//
//************************************************************************************************

#ifndef _ccl_d3dshader_h
#define _ccl_d3dshader_h

#include "ccl/public/gui/graphics/3d/shaderconstants3d.h"

#include "core/public/corebasicmacros.h"

#define CCL_REGISTER(TYPE,INDEX) register (LAZYCAT (TYPE, INDEX))

#endif // _ccl_d3dshader_h
