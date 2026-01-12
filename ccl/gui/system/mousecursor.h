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
// Filename    : ccl/gui/system/mousecursor.h
// Description : Mouse Cursor
//
//************************************************************************************************

#ifndef _ccl_mousecursor_h
#define _ccl_mousecursor_h

#include "ccl/gui/graphics/imaging/image.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/gui/framework/imousecursor.h"

namespace CCL {

class MouseCursorFactory;

//************************************************************************************************
// MouseCursor
//************************************************************************************************

class MouseCursor: public Object,
				   public IMouseCursor
{
public:
	DECLARE_CLASS (MouseCursor, Object)

	static void setFactory (MouseCursorFactory* factory); ///< platform-specific!
	static MouseCursor* createCursor (int themeCursorId); ///< @see ThemeElements::Cursors
	static MouseCursor* createCursor (Image& image, PointRef hotspot = Point ()); ///< create cursor from image

	virtual void makeCurrent ();
	
	PROPERTY_MUTABLE_CSTRING (name, Name)	///< cursor name
	PROPERTY_SHARED_AUTO (Image, originalImage, OriginalImage)	///< original image from createCursor()

	// IMouseCursor
	IImage* CCL_API createImage () const override;

	CLASS_INTERFACE (IMouseCursor, Object)

protected:
	static MouseCursorFactory* factory;
	bool ownCursor;

	MouseCursor (bool ownCursor = false);
};

//************************************************************************************************
// MouseCursorFactory
//************************************************************************************************

class MouseCursorFactory
{
public:
	virtual MouseCursor* createCursor (int themeCursorId) = 0;
	virtual MouseCursor* createCursor (Image& image, PointRef hotspot) = 0;
};

} // namespace CCL

#endif // _ccl_mousecursor_h
