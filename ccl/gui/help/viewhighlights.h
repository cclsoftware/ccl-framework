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
// Filename    : ccl/gui/help/viewhighlights.h
// Description : View Highlights
//
//************************************************************************************************

#ifndef _ccl_viewhighlights_h
#define _ccl_viewhighlights_h

#include "ccl/public/gui/framework/idleclient.h"
#include "ccl/base/collections/objectlist.h"
#include "ccl/public/gui/framework/iwindow.h"

namespace CCL {

class View;
class Window;

//************************************************************************************************
// ViewHighlights
/** Manages highlighting views using sprite overlays. */
//************************************************************************************************

class ViewHighlights: public Object,
					  public IdleClient,
					  public IWindowEventHandler
{
public:
	ViewHighlights ();
	~ViewHighlights ();

	void addView (View* view, bool exclusive);
	void removeView (View* view);
	void removeAll ();

	void modifyHighlights (bool begin);

	CLASS_INTERFACE2 (ITimerTask, IWindowEventHandler, Object)

private:
	ObjectList windowItems;
	bool isModifying;

	static constexpr int64 kRefreshRate = 50;

	enum ShapeType { kRoundRect, kCircle };

	class WindowItem;
	class ViewItem;
	class Style;

	WindowItem* getWindowItem (Window* window, bool create);
	void removeWindowItem (WindowItem* windowItem);

	static bool handlesWindow (Window& window);
	bool hasAnyHighlights () const;
	void updateSprites (bool deferred);

	void onWindowAdded (Window* window);

	// IWindowEventHandler
	tbool CCL_API onWindowEvent (WindowEvent& windowEvent) override;

	// IdleClient
	void onIdleTimer () override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
};

} // namespace CCL

#endif // _ccl_viewhighlights_h
