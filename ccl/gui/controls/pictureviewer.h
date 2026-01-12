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
// Filename    : ccl/gui/controls/pictureviewer.h
// Description : Picture Viewer
//
//************************************************************************************************

#ifndef _ccl_pictureviewer_h
#define _ccl_pictureviewer_h

#include "ccl/gui/views/imageview.h"

namespace CCL {

//************************************************************************************************
// PictureViewer
/** Shows an image and pops up view with a larger version of the image.
Like an imageview, the image is resized to the view area.
On a mouse click, a view pops up that shows the image in it's full size,
but not exceeding a certain limit, which is currently 1024 x 1024 pixels.*/
//************************************************************************************************

class PictureViewer: public ImageView
{
public:
	DECLARE_CLASS (PictureViewer, ImageView)

	PictureViewer (IImage* background = nullptr, const Rect& size = Rect (),
				   StyleRef style = 0, StringRef title = nullptr);

	// ImageView
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseMove  (const MouseEvent& event) override;
	bool onMouseLeave (const MouseEvent& event) override;
	bool onMouseDown (const MouseEvent& event) override;

protected:
	bool hasMagnifier;

	IImage* getLargestFrame () const;
	bool canPopup () const;
	MouseCursor* getMagnifierCursor () const;
	void updateCursor (const Point& where);
	void resetCursor (bool mouseLeaving = false);
	void popupPicture ();
};

} // namespace CCL

#endif // _ccl_pictureviewer_h
