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
// Filename    : ccl/app/editing/scaleview.h
// Description : Scale View
//
//************************************************************************************************

#ifndef _ccl_scaleview_h
#define _ccl_scaleview_h

#include "ccl/app/controls/usercontrol.h"

namespace CCL {

class Scale;

//************************************************************************************************
// ScaleView
//************************************************************************************************

class ScaleView: public UserControl
{
public:
	DECLARE_CLASS (ScaleView, UserControl)

	ScaleView (Scale* scale = nullptr, RectRef size = Rect (), StyleRef style = 0, StringRef title = nullptr);
	~ScaleView ();

	void setScale (Scale* scale);
	Scale* getScale () const;

	// UserControl
	void draw (const DrawEvent& event) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseMove (const MouseEvent& event) override;
	bool onMouseLeave (const MouseEvent& event) override;
	bool onMouseWheel (const MouseWheelEvent& event) override;
	IMouseHandler* CCL_API createMouseHandler (const MouseEvent& event) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	Scale* scale;

	virtual bool getHelp (IHelpInfoBuilder& helpInfo, bool canDragZoom = true);
	void updateHelp (const MouseEvent& event);
};

} // namespace CCL

#endif // _ccl_scaleview_h
