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
// Filename    : ccl/public/gui/graphics/updatergn.h
// Description : Update Region
//
//************************************************************************************************

#ifndef _ccl_updatergn_h
#define _ccl_updatergn_h

#include "ccl/public/base/iunknown.h"

#include "ccl/public/gui/graphics/rect.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (MutableRegion, 0xB3FD9505, 0x1594, 0x42D2, 0xB0, 0x15, 0xA4, 0x1D, 0xC0, 0x3E, 0x44, 0x9D)
}

//************************************************************************************************
// IUpdateRegion
/** Interface to native update region. 
	\ingroup gui_graphics */
//************************************************************************************************

interface IUpdateRegion: IUnknown
{
	/** Check if any part of the rectangle is within the update region. */
	virtual tbool CCL_API rectVisible (RectRef rect) const = 0;

	/** Get the bounding box of all contained parts. */
	virtual Rect CCL_API getBoundingBox () const = 0;

	DECLARE_IID (IUpdateRegion)
};

//************************************************************************************************
// IMutableRegion
/** Interface to mutable region. 
	\ingroup gui_graphics */
//************************************************************************************************

interface IMutableRegion: IUpdateRegion
{
	/** Add rectangle to region. */
	virtual void CCL_API addRect (RectRef rect) = 0;

	/** Remove all parts of this region. */
	virtual void CCL_API setEmpty () = 0;

	DECLARE_IID (IMutableRegion)
};

//************************************************************************************************
// UpdateRgn
/** Update region. 
	\ingroup gui_graphics */
//************************************************************************************************

struct UpdateRgn
{
	Rect bounds;			///< region boundaries in view coordinates
	Point offset;			///< offset into native region
	IUpdateRegion* region;	///< native update region (can be null)

	/** Construct region. */
	UpdateRgn (RectRef bounds = Rect (), 
			   IUpdateRegion* region = nullptr, 
			   PointRef offset = Point ())
	: bounds (bounds),
	  region (region),
	  offset (offset)
	{}

	/** Construct subpart of other region. */
	UpdateRgn (const UpdateRgn& other, RectRef subPart);

	/** Check if region is empty. */
	bool isEmpty () const;

	/** Check if any part of the rectangle is within the region. */
	bool rectVisible (RectRef rect) const;
};

} // namespace CCL

#endif // _ccl_updatergn_h
