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
// Filename    : ccl/gui/views/focusnavigator.cpp
// Description : Focus navigator
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/views/focusnavigator.h"

#include "ccl/gui/windows/window.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/skin/form.h"
#include "ccl/gui/commands.h"

#include "ccl/public/gui/commanddispatch.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG_LOG
namespace {

void printView (const char* text1, View* view, const char* text2 = 0)
{
	if(text1)
		CCL_PRINT (text1);
	CCL_PRINT (view->myClass ().getPersistentName ());
	if(!view->getTitle ().isEmpty ())
	{
		CCL_PRINT (" \"")
		CCL_PRINT (view->getTitle ());
		CCL_PRINT ("\"")
	}
	if(text2)
		CCL_PRINT (text2);
	CCL_PRINT ("\n")
}

} // namespace
#define PRINT_VIEW(text1,view,text2) printView (text1, view, text2)
#else
#define PRINT_VIEW(text1,view,text2)
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

REGISTER_COMMAND_("Navigation", "Focus Next",		CommandFlags::kGlobal)
REGISTER_COMMAND_("Navigation", "Focus Previous",	CommandFlags::kGlobal)
REGISTER_COMMAND_("Navigation", "Enter",			CommandFlags::kGlobal)
REGISTER_COMMAND_("Navigation", "Cancel",			CommandFlags::kGlobal)

//************************************************************************************************
// FocusNavigator::LateCommandHandler
/** Handles Enter, Cancel. Registered late (e.g. after application Runtime) so these commands can
	be overriden e.g. by application components. */
//************************************************************************************************

class FocusNavigator::LateCommandHandler: public Object,
										  public ICommandHandler
{
public:
	// ICommandHandler
	tbool CCL_API checkCommandCategory (CStringRef category) const override
	{
		return category == "Navigation";
	}

	tbool CCL_API interpretCommand (const CommandMsg& msg) override
	{
		if(msg.category == "Navigation")
		{
			if(msg.name == "Enter")
				return msg.checkOnly () ? true : FocusNavigator::instance ().simulateKeyEvent (VKey::kReturn);
			else if(msg.name == "Cancel")
				return msg.checkOnly () ? true : FocusNavigator::instance ().simulateKeyEvent (VKey::kEscape);
		}
		return false;
	}

	CLASS_INTERFACE (ICommandHandler, Object)
};

//************************************************************************************************
// FocusNavigator
//************************************************************************************************

DEFINE_SINGLETON (FocusNavigator)

CCL_KERNEL_INIT_LEVEL (FocusNavigator, kFrameworkLevelSecond)
{
	CommandTable::instance ().addHandler (&FocusNavigator::instance ());
	return true;
}

CCL_KERNEL_TERM_LEVEL (FocusNavigator, kFrameworkLevelSecond)
{
	CommandTable::instance ().removeHandler (&FocusNavigator::instance ());

	if(ICommandHandler* laterHandler = FocusNavigator::instance ().getLateCommandHandler ())
		CommandTable::instance ().removeHandler (laterHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window* FocusNavigator::getTargetWindow ()
{
	Window* window = Desktop.getTopWindow (kDialogLayer);
	if(!window)
		window = Desktop.getActiveWindow ();

	return window;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FocusNavigator::checkCommandCategory (CStringRef category) const
{
	return category == "Navigation";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FocusNavigator::interpretCommand (const CommandMsg& msg)
{
	if(msg.category == "Navigation")
	{
		if(msg.name == "Focus Next")
		{
			return msg.checkOnly () ? true : navigateFocus (true);
		}
		else if(msg.name == "Focus Previous")
		{
			return msg.checkOnly () ? true : navigateFocus (false);
		}
		else if(!lateCommandHandler)
		{
			// register "late" handler on first navigation command interpretation
			CommandTable::instance ().addHandler (lateCommandHandler = NEW LateCommandHandler);
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FocusNavigator::simulateKeyEvent (VirtualKey vKey)
{
	if(Window* window = getTargetWindow ())
	{
		window->onKeyDown (KeyEvent (KeyEvent::kKeyDown, vKey));
		window->onKeyUp (KeyEvent (KeyEvent::kKeyUp, vKey));
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FocusNavigator::onKeyDown (const KeyEvent& event)
{
	Command* cmd = CommandTable::instance ().lookupCommand (event);
	if(cmd && cmd->getCategory () == "Navigation"
		&& (cmd->getName () == "Focus Next" || cmd->getName () == "Focus Previous"))
		return cmd->interpretSafe (this);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FocusNavigator::navigateFocus (bool forward)
{
	if(Window* window = getTargetWindow ())
	{
		View* startView = window->getFocusView ();
		if(!startView)
		{
			startView = window->getSavedFocusView ();
			if(startView)
			{
				if(!startView->getWindow ())
					startView = nullptr;
				else if(startView->wantsFocus ())
				{
					startView->takeFocus (); // visit the saved focus view again
					return true;
				}
			}
		}
		if(!startView)
			startView = window;

		View* newFocusView = forward ? getNext (startView) : getPrevious (startView);;
		if(newFocusView && newFocusView->takeFocus ())
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FocusNavigator::isFocusable (View* view)
{
	return view->wantsFocus () && view->isEnabled ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* FocusNavigator::findNextDeep (View* parent, View* startView)
{
	ASSERT (startView == nullptr || startView->getParent () == parent)
	if(!parent)
		return nullptr;

	bool skip = startView != nullptr; // skip all up to startView
	ForEachViewFast (*parent, child)
		if(skip)
		{
			if(child == startView)
				skip = false;
			continue;
		}

		// try this view
		if(isFocusable (child))
		{
			PRINT_VIEW ("   found: ", child, "");
			return child;
		}

		// try childs
		if(View* deepChild = getFirst (child))
			return deepChild;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* FocusNavigator::findPreviousDeep (View* parent, View* startView)
{
	ASSERT (startView == nullptr || startView->getParent () == parent)
	if(!parent)
		return nullptr;

	bool skip = startView != nullptr; // skip all up to startView
	ForEachViewFastReverse (*parent, child)
		if(skip)
		{
			if(child == startView)
				skip = false;
			continue;
		}

		// try childs
		if(View* deepChild = getLast (child))
			return deepChild;

		// try this view
		if(isFocusable (child))
		{
			PRINT_VIEW ("   found: ", child, "");
			return child;
		}
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* FocusNavigator::getFirst (View* parent)
{
	PRINT_VIEW ("FocusNavigator::getFirst (", parent, ")");
	if(Form* form = ccl_cast<Form> (parent))
		if(View* firstExplicit = form->findFirstFocusView ())
			return firstExplicit;

	return findNextDeep (parent, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* FocusNavigator::getFirstExplicit (View* parent)
{
	if(Form* form = ccl_cast<Form> (parent))
		if(View* firstExplicit = form->findFirstFocusView ())
			return firstExplicit;

	ForEachViewFast (*parent, child)
		if(View* v = getFirstExplicit (child))
			return v;
	EndFor

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* FocusNavigator::getLast (View* parent)
{
	PRINT_VIEW ("FocusNavigator::getLast (", parent, ")");
	return findPreviousDeep (parent, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* FocusNavigator::getNextSibling (View* view)
{
	View* parent = view->getParent ();
	if(parent)
	{
		// try following siblings
		if(View* v = findNextDeep (parent, view))
			return v;

		// continue with siblings of parent (one level upwards)
		return getNextSibling (parent);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* FocusNavigator::getNext (View* view)
{
	if(view)
	{
		PRINT_VIEW ("FocusNavigator::getNext (", view, ")");

		// try childs
		if(View* deepChild = getFirst (view))
			return deepChild;

		// try following siblings
		if(View* sibling = getNextSibling (view))
			return sibling;

		Window* window = view->getWindow ();
		if(window && window != view)
			return getNext (window);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* FocusNavigator::getPrevious (View* view)
{
	if(view)
	{
		PRINT_VIEW ("FocusNavigator::getPrevious (", view, ")");

		View* parent = view->getParent ();
		if(parent)
		{
			// try preceding siblings
			if(View* v = findPreviousDeep (parent, view))
				return v;

			// try parent
			if(isFocusable (parent))
			{
				PRINT_VIEW ("   found (parent): ", parent, "");
				return parent;
			}

			// up one level (siblings of parent)
			return getPrevious (parent);
		}
		else
			return getLast (view);
	}
	return nullptr;
}
