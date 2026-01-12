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
// Filename    : ccl/gui/views/graphicsport.cpp
// Description : View Graphics Port
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/views/graphicsport.h"
#include "ccl/gui/views/view.h"

#include "ccl/gui/graphics/nativegraphics.h"

using namespace CCL;

//************************************************************************************************
// GraphicsPort
//************************************************************************************************

DEFINE_CLASS_HIDDEN (GraphicsPort, GraphicsDevice)

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsPort::GraphicsPort (View* view)
: view (view),
  device (nullptr)
{
	ASSERT (view != nullptr)

	device = view->getGraphicsDevice (offset);
	ASSERT (device != nullptr)
	setNativeDevice (device->getNativeDevice ());

	// move origin to view
	ASSERT (nativeDevice != nullptr)
	oldOrigin = nativeDevice->getOrigin ();
	if(offset != oldOrigin)
		nativeDevice->setOrigin (offset);

	saveState ();

	// clip to view client area or layer size
	view->getVisibleClientForUpdate (visibleRect);
	#if DEBUG_LOG
	if(visibleRect.isEmpty ())
	{
		CCL_PRINT ("Empty rect for ")
		CCL_PRINTLN (view->getName ())
	}
	#endif
	addClip (visibleRect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsPort::GraphicsPort (const GraphicsPort& port, const Point& _offset)
: view (port.view),
  device (port.device),
  offset (port.offset)
{
	ASSERT (device != nullptr)
	device->retain ();

	ASSERT (device->getNativeDevice () != nullptr)
	setNativeDevice (device->getNativeDevice ());

	oldOrigin = nativeDevice->getOrigin ();

	offset.offset (_offset);
	if(offset != oldOrigin)
		nativeDevice->setOrigin (offset);

	saveState ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsPort::~GraphicsPort ()
{
	restoreState ();

	if(nativeDevice->getOrigin () != oldOrigin)
		nativeDevice->setOrigin (oldOrigin);

	device->release ();
}
