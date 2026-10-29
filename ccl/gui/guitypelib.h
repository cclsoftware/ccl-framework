//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/gui/guitypelib.h
// Description : Extended GUI Type Library Definitions
//
//************************************************************************************************

#ifndef _ccl_guitypelib_h
#define _ccl_guitypelib_h

#include "ccl/base/typelib.h"

namespace CCL {

//************************************************************************************************
// TypeNames
//** Extended GUI type names (see also iobject.h). */
//************************************************************************************************

namespace TypeNames
{
	const CStringPtr kColor = "color";
	const CStringPtr kImage = "image";
	const CStringPtr kMetric = "metric";
	const CStringPtr kPoint = "point";
	const CStringPtr kPoint3D = "point3d";
	const CStringPtr kRect = "rect";
	const CStringPtr kSize = "size";
	const CStringPtr kRenderAtom = "RenderAtom";
}

} // namespace CCL

//************************************************************************************************
// Custom GUI ADD_TYPELIB_MEMBER macros.
//************************************************************************************************

#define ADD_TYPELIB_MEMBER_ALIGN(name) \
ADD_TYPELIB_MEMBER_WITH_TYPE (name, CCL::ITypeInfo::kInt, CCL::TypeNames::kEnum)

#define ADD_TYPELIB_MEMBER_COLOR(name) \
ADD_TYPELIB_MEMBER_WITH_TYPE (name, CCL::ITypeInfo::kString, CCL::TypeNames::kColor)

#define ADD_TYPELIB_MEMBER_FONT(name) \
ADD_TYPELIB_MEMBER_WITH_TYPE (name, CCL::ITypeInfo::kString, CCL::TypeNames::kString)

#define ADD_TYPELIB_MEMBER_IMAGE(name) \
ADD_TYPELIB_MEMBER_WITH_TYPE (name, CCL::ITypeInfo::kString, CCL::TypeNames::kImage)

#define ADD_TYPELIB_MEMBER_METRIC(name) \
ADD_TYPELIB_MEMBER_WITH_TYPE (name, CCL::ITypeInfo::kFloat, CCL::TypeNames::kMetric)

#define ADD_TYPELIB_MEMBER_OPTIONS(name) \
ADD_TYPELIB_MEMBER_WITH_TYPE (name, CCL::ITypeInfo::kInt, CCL::TypeNames::kEnum)

#define ADD_TYPELIB_MEMBER_RENDERATOM(name) \
ADD_TYPELIB_MEMBER_WITH_TYPE (name, CCL::ITypeInfo::kObject, CCL::TypeNames::kRenderAtom)

#endif // _ccl_guitypelib_h
