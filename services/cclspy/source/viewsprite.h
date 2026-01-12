//************************************************************************************************
//
// CCL Spy
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
// Filename    : viewsprite.cpp
// Description : View Highlite Sprite
//
//************************************************************************************************

#ifndef _viewsprite_h
#define _viewsprite_h

#include "ccl/base/object.h"

#include "ccl/public/gui/framework/idrawable.h"
#include "ccl/public/gui/framework/itimer.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/isprite.h"

namespace CCL {; }

namespace Spy {

//************************************************************************************************
// ViewSprite
//************************************************************************************************

class ViewSprite: public CCL::Object,
				  public CCL::ITimerTask,
				  public CCL::IWindowEventHandler
{
public:
	ViewSprite ();
	~ViewSprite ();

	PROPERTY_OBJECT (CCL::Color, backColor, BackColor)
	PROPERTY_OBJECT (CCL::Color, frameColor, FrameColor)
	PROPERTY_BOOL (showUntilMouseUp, ShowUntilMouseUp)
	PROPERTY_BOOL (showInfo, ShowInfo)

	void show (CCL::IView* view, CCL::int64 duration = -1);
	void hide ();
	bool isVisible () const;
	CCL::IView* getView ();

	// ITimerTask
	void CCL_API onTimer (CCL::ITimer* timer) override;

	// IWindowEventHandler
	CCL::tbool CCL_API onWindowEvent (CCL::WindowEvent& windowEvent) override;

	CLASS_INTERFACE2 (ITimerTask, IWindowEventHandler, Object)

private:
	CCL::IView* view;
	ISubject* viewSubject;
	CCL::IWindow* window;
	CCL::int64 showUntil;
	CCL::int64 nextUpdate;
	CCL::AutoPtr<CCL::ISprite> sprite;

	enum { kUpdateFreq = 500 };

	void calcSize (CCL::Rect& rect);
	void CCL_API notify (ISubject* subject, CCL::MessageRef msg) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline CCL::IView* ViewSprite::getView () { return view; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Spy

#endif // _viewsprite_h
