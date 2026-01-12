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
// Filename    : ccl/gui/views/graphicsport.h
// Description : View Graphics Port
//
//************************************************************************************************

#ifndef _ccl_graphicsport_h
#define _ccl_graphicsport_h

#include "ccl/gui/graphics/graphicsdevice.h"

namespace CCL {

class View;

//************************************************************************************************
// GraphicsPort
/** Graphics device for painting to a view. */
//************************************************************************************************

class GraphicsPort: public GraphicsDevice
{
public:
	DECLARE_CLASS (GraphicsPort, GraphicsDevice)

	GraphicsPort (View* view = nullptr);
	GraphicsPort (const GraphicsPort& port, const Point& offset = Point ());
	~GraphicsPort ();

	RectRef getVisibleRect () const { return visibleRect; }

protected:
	View* view;
	Rect visibleRect;
	GraphicsDevice* device;
	Point offset;
	Point oldOrigin;
};

} // namespace CCL

#endif // _ccl_graphicsport_h
