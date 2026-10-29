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
// Filename    : ccl/gui/theme/themerenderer.h
// Description : Theme Renderer
//
//************************************************************************************************

#ifndef _ccl_themerenderer_h
#define _ccl_themerenderer_h

#include "ccl/gui/theme/visualstyle.h"
#include "ccl/gui/theme/visualstyleclass.h"
#include "ccl/gui/theme/legacystyleids.h"

#include "ccl/public/gui/graphics/types.h"
#include "ccl/public/gui/graphics/updatergn.h"

namespace CCL {

class View;
struct WindowUpdateInfo;

//************************************************************************************************
// ThemeRenderer
//************************************************************************************************

class ThemeRenderer: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (ThemeRenderer, Object)

	ThemeRenderer (VisualStyle* _visualStyle, View* _owner)
	: visualStyle (_visualStyle),
	  owner (_owner)
	{}

	View* getOwner () const { return owner; }

	VisualStyle* getVisualStyle () const { return visualStyle; }

	virtual void invalidateStyleConditions () {}

	virtual void setContentScaleFactor (float factor) {}

	virtual void draw (const UpdateRgn& updateRgn) = 0;

	virtual bool needsRedraw (const Point& sizeDelta) { return false; }

	struct UpdateInfo
	{
		WindowUpdateInfo* windowInfo;
		// extended by derived class

		UpdateInfo (WindowUpdateInfo* windowInfo)
		: windowInfo (windowInfo)
		{}
	};

	virtual void update (const UpdateInfo& info);

	virtual int hitTest (PointRef where, Point* offset = nullptr) = 0;

	virtual bool getPartRect (Rect& rect, int partCode) = 0;

protected:
	SharedPtr<VisualStyle> visualStyle;
	View* owner;
};

} // namespace CCL

#endif // _ccl_themerenderer_h
