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
// Filename    : ccl/public/gui/graphics/3d/igeometrysource3d.h
// Description : 3D Geometry Source Interfaces
//
//************************************************************************************************

#ifndef _ccl_igeometrysource3d_h
#define _ccl_igeometrysource3d_h

#include "ccl/public/base/iunknown.h"

#include "ccl/public/gui/graphics/3d/point3d.h"

namespace CCL {
	
//************************************************************************************************
// IGeometrySource3D
/** 3D geometry source interface.
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IGeometrySource3D: IUnknown
{
	/** Get the number of vertices */
	virtual uint32 CCL_API getVertexCount () const = 0;

	/** Get the number of indices */
	virtual uint32 CCL_API getIndexCount () const = 0;

	/** Get vertices */
	virtual const PointF3D* CCL_API getPositions () const = 0;

	/** Get vertices */
	virtual const PointF3D* CCL_API getNormals () const = 0;

	/** Get vertices */
	virtual const PointF* CCL_API getTextureCoords () const = 0;

	/** Get indices */
	virtual const uint32* CCL_API getIndices () const = 0;

	DECLARE_IID (IGeometrySource3D)
};

DEFINE_IID (IGeometrySource3D, 0x31ca96a6, 0x7e3e, 0x4355, 0xa5, 0x95, 0xb4, 0xb0, 0x37, 0x41, 0x2b, 0xd9)

} // namespace CCL

#endif // _ccl_igeometrysource3d_h
