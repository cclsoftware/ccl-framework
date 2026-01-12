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
// Filename    : ccl/gui/graphics/mutableregion.h
// Description : Mutable Region
//
//************************************************************************************************

#ifndef _ccl_mutableregion_h
#define _ccl_mutableregion_h

#include "ccl/base/object.h"

#include "ccl/public/gui/graphics/updatergn.h"

#include "ccl/public/collections/vector.h"

#include "core/public/gui/corerectlist.h"

namespace CCL {

//************************************************************************************************
// MutableRegion
/** Internally stores a maximum of 5 rectangles, combines intersecting rectangles. */
//************************************************************************************************

class MutableRegion: public Object,
					 public IMutableRegion
{
public:
	DECLARE_CLASS (MutableRegion, Object)
		
	const ConstVector<Rect>& getRects () const { return rects.getRects (); }
	
	// IMutableRegion
	void CCL_API addRect (RectRef rect) override;
	tbool CCL_API rectVisible (RectRef rect) const override;
	void CCL_API setEmpty () override;
	Rect CCL_API getBoundingBox () const override;
	
	CLASS_INTERFACE (IMutableRegion, Object)
		
protected:
	Core::RectList<5> rects;
};

//************************************************************************************************
// SelectionRegion
/** Keeps an unlimited number of individual rectangles that can be retrieved as they were added. */
//************************************************************************************************

class SelectionRegion: public Object,
					   public IMutableRegion
{
public:
	DECLARE_CLASS (SelectionRegion, Object)
		
	const ConstVector<Rect>& getRects () const { return rects; }

	// IMutableRegion
	void CCL_API addRect (RectRef rect) override;
	tbool CCL_API rectVisible (RectRef rect) const override;
	void CCL_API setEmpty () override;
	Rect CCL_API getBoundingBox () const override;

	CLASS_INTERFACE (IMutableRegion, Object)

protected:
	Vector<Rect> rects;
};

} // namespace CCL

#endif // _ccl_mutableregion_h
