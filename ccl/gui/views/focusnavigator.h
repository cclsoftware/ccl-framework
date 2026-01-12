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
// Filename    : ccl/gui/views/focusnavigator.h
// Description : Focus navigator
//
//************************************************************************************************

#ifndef _ccl_focusnavigator_h
#define _ccl_focusnavigator_h

#include "ccl/base/singleton.h"
#include "ccl/public/gui/icommandhandler.h"
#include "ccl/public/gui/framework/keycodes.h"

namespace CCL {

class View;
class Window;
struct KeyEvent;

//************************************************************************************************
// FocusNavigator
//************************************************************************************************

class FocusNavigator: public Object,
					  public ICommandHandler,
					  public Singleton<FocusNavigator>
{
public:
	bool onKeyDown (const KeyEvent& event); ///< handle key events (only called by NativeTextControl)

	View* getFirst (View* parent);	///< get the first focusable view inside parent
	View* getLast	(View* parent);	///< get the last focusable view inside parent
	View* getNext     (View* view);	///< get the next focusable view after view
	View* getPrevious (View* view);	///< get the previous focusable view before view

	View* getFirstExplicit (View* parent); ///< only consider forms with a "firstfocus" specified

	PROPERTY_AUTO_POINTER (ICommandHandler, lateCommandHandler, LateCommandHandler)

	// ICommandHandler
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

	CLASS_INTERFACE (ICommandHandler, Object)

private:
	Window* getTargetWindow ();
	bool simulateKeyEvent (VirtualKey vKey);
	bool isFocusable (View* view);
	bool navigateFocus (bool forward);
	View* findNextDeep (View* parent, View* startView);
	View* findPreviousDeep (View* parent, View* startView);
	View* getNextSibling (View* view);

	class LateCommandHandler;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_focusnavigator_h
