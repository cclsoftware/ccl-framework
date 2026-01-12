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
// Filename    : ccl/public/gui/graphics/3d/vertex3d.h
// Description : Vertex Definitions
//
//************************************************************************************************

#ifndef _ccl_vertex3d_h
#define _ccl_vertex3d_h

#include "ccl/public/gui/graphics/3d/point3d.h"

namespace CCL {

//************************************************************************************************
// DataFormat3D
//************************************************************************************************

DEFINE_ENUM (DataFormat3D)
{
	kUnspecified,
	kR8_Int,
	kR8_UInt,
	kR16_Int,
	kR16_UInt,
	kR32_Int,
	kR32_UInt,
	kR32_Float,
	kR8G8_Int,
	kR8G8_UInt,
	kR16G16_Int,
	kR16G16_UInt,
	kR32G32_Int,
	kR32G32_UInt,
	kR32G32_Float,
	kR32G32B32_Int,
	kR32G32B32_UInt,
	kR32G32B32_Float,
	kR32G32B32A32_Int,
	kR32G32B32A32_UInt,
	kR32G32B32A32_Float,
	kR8G8B8A8_UNORM,
	kB8G8R8A8_UNORM
};

//************************************************************************************************
// VertexElementDescription
//************************************************************************************************

struct VertexElementDescription
{
	CStringPtr semanticName;
	DataFormat3D format;
};

//************************************************************************************************
// VertexP
/** Vertex containing only a position element
\ingroup gui_graphics3d */
//************************************************************************************************

struct VertexP
{
	PointF3D position;

	static constexpr VertexElementDescription kDescription[1]
	{
		{ "POSITION", kR32G32B32_Float }
	};
};

//************************************************************************************************
// VertexPN
/** Vertex containing only a position and a normal vector element
\ingroup gui_graphics3d */
//************************************************************************************************

struct VertexPN
{
	PointF3D position;
	PointF3D normal;

	static constexpr VertexElementDescription kDescription[2]
	{
		{ "POSITION", kR32G32B32_Float },
		{ "NORMAL",   kR32G32B32_Float }
	};
};

//************************************************************************************************
// VertexPT
/** Vertex containing only a position and texture coordinates
\ingroup gui_graphics3d */
//************************************************************************************************

struct VertexPT
{
	PointF3D position;
	PointF textureCoordinate;

	static constexpr VertexElementDescription kDescription[2]
	{
		{ "POSITION", kR32G32B32_Float },
		{ "TEXCOORD", kR32G32_Float }
	};
};

//************************************************************************************************
// VertexPNT
/** Vertex containing a position, a normal vector element and texture coordinates
\ingroup gui_graphics */
//************************************************************************************************

struct VertexPNT
{
	PointF3D position;
	PointF3D normal;
	PointF textureCoordinate;

	static constexpr VertexElementDescription kDescription[3]
	{
		{ "POSITION", kR32G32B32_Float },
		{ "NORMAL",   kR32G32B32_Float },
		{ "TEXCOORD", kR32G32_Float }
	};
};

} // namespace CCL

#endif // _ccl_vertex3d_h
