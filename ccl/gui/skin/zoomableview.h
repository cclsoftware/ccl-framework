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
// Filename    : ccl/gui/skin/zoomableview.h
// Description : ZoomableView class
//
//************************************************************************************************

#ifndef _ccl_zoomableview_h
#define _ccl_zoomableview_h

#include "ccl/gui/views/view.h"

#include "ccl/base/storage/attributes.h"

namespace CCL {

//************************************************************************************************
// ZoomableView
//************************************************************************************************

class ZoomableView: public View
{
public:
	DECLARE_CLASS (ZoomableView, View)

	ZoomableView (RectRef size = Rect (), StyleRef style = 0, StringRef title = nullptr);

	PROPERTY_MUTABLE_CSTRING (formName, FormName)
	PROPERTY_SHARED_AUTO (IUnknown, formController, FormController)

	void setSupportedZoomfactors (const Vector<float>& factors);
	Attributes& getFormArguments () { return formArguments; }

	// View
	void attached (View* parent) override;
	void onSize (const Point& delta) override;

protected:
	Attributes formArguments;
	Vector<float> supportedZoomfactors;
	Point originalSize;

	void init ();
	float determineZoomFactor () const;
	View* createContentView (float zoom);
	void layoutContentView (View& content);
};

} // namespace CCL

#endif // _ccl_zoomableview_h
