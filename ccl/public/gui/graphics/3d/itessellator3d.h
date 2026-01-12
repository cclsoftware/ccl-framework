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
// Filename    : ccl/public/gui/graphics/3d/itessellator3d.h
// Description : 3D tesselators for geometric primitives
//
//************************************************************************************************

#ifndef _ccl_itesselator3d_h
#define _ccl_itesselator3d_h

#include "ccl/public/gui/graphics/3d/igeometrysource3d.h"

namespace CCL {

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	/** Cube Tessellator [ICubeTessellator3D] */
	DEFINE_CID (CubeTessellator3D, 0xabd9356a, 0x7c95, 0x4cd8, 0x8f, 0xb1, 0x42, 0xd8, 0x4b, 0xf4, 0xce, 0xf3)

	/** Grid Tessellator [IGridTessellator3D] */
	DEFINE_CID (GridTessellator3D, 0x51bfe9c6, 0x5485, 0x4c09, 0x94, 0x87, 0xb4, 0x64, 0x7b, 0x4f, 0xa6, 0x6d)

	/** UV Sphere Tessellator [IUVSphereTessellator3D] */
	DEFINE_CID (UVSphereTessellator3D, 0xebd3bd6c, 0xa986, 0x4372, 0xa2, 0x36, 0xb1, 0xd5, 0x39, 0x8c, 0x68, 0xc3)
}

//************************************************************************************************
// ITessellator
/** Base interface for tesselators
	\ingroup gui_graphics3d */
//************************************************************************************************

interface ITessellator3D: IGeometrySource3D
{
	enum Flags
	{
		kGenerateNormals = 1<<0,
		kGenerateInverseNormals = 1<<1,
		kGenerateTextureCoordinates = 1<<2,
		kWindingOrderCW = 1<<3,
		kWindingOrderCCW = 0
	};

	virtual tresult CCL_API generate (uint32 flags) = 0;

	DECLARE_IID (ITessellator3D)
};

DEFINE_IID (ITessellator3D, 0x717d2083, 0x963c, 0x45da, 0xa0, 0x8a, 0x7e, 0xcc, 0x19, 0x13, 0xca, 0xa)

//************************************************************************************************
// ICubeTessellator
/** Tessellator interface for cubes
	\ingroup gui_graphics3d */
//************************************************************************************************

interface ICubeTessellator3D: ITessellator3D
{
	DECLARE_IID (ICubeTessellator3D)
};

DEFINE_IID (ICubeTessellator3D, 0x4755bf22, 0xb4c0, 0x4fa2, 0xb5, 0x60, 0x39, 0xd6, 0xf3, 0x9, 0x66, 0xdb)

//************************************************************************************************
// IGridTessellator
/** Tessellator interface for 2D grids on the XZ-plane
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IGridTessellator3D: ITessellator3D
{
	virtual tresult CCL_API setGridSize (uint32 width, uint32 height) = 0;

	virtual tresult CCL_API setCellSize (float width, float height) = 0;

	DECLARE_IID (IGridTessellator3D)
};

DEFINE_IID (IGridTessellator3D, 0xd4d6887f, 0x3b91, 0x490f, 0x8b, 0xd6, 0x28, 0xe5, 0x6a, 0x6d, 0x9f, 0x95)

//************************************************************************************************
// IUVSphereTessellator
/** Tessellator interface for UV spheres
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IUVSphereTessellator3D: ITessellator3D
{
	virtual tresult CCL_API setRadius (float radius) = 0;

	virtual tresult CCL_API setNumberOfParallels (uint32 count) = 0;

	virtual tresult CCL_API setNumberOfMeridians (uint32 count) = 0;

	DECLARE_IID (IUVSphereTessellator3D)
};

DEFINE_IID (IUVSphereTessellator3D, 0x7ae4c7c7, 0x9699, 0x42c7, 0xa7, 0xaf, 0x44, 0xab, 0x22, 0xf4, 0xcf, 0x6f)

} // namespace CCL

#endif // _ccl_itesselator3d_h
