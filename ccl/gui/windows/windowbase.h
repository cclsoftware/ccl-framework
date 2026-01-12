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
// Filename    : ccl/gui/windows/windowbase.h
// Description : Window Base
//
//************************************************************************************************

#ifndef _ccl_windowbase_h
#define _ccl_windowbase_h

#include "ccl/gui/views/view.h"

#include "ccl/public/gui/framework/iwindow.h"

namespace CCL {

//************************************************************************************************
// WindowBase
//************************************************************************************************

class WindowBase: public View,
				  public IWindowBase
{
public:
	DECLARE_CLASS (WindowBase, View)

	WindowBase (const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);
	~WindowBase ();

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Nesting
	//////////////////////////////////////////////////////////////////////////////////////////////

	bool addChild (WindowBase* view);
	bool removeChild (WindowBase* view);
	WindowBase* getChildWindow (int index) const;
	Iterator* getChildWindows () const;
	WindowBase* getParentWindow () const;
	WindowBase* getFirstActivatableChild () const;
	WindowBase* getActiveChild () const; ///< note: for an inactive parent WindowBase, the returned child is inactive
	WindowBase* getDeepestActiveWindow ();

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Activation
	//////////////////////////////////////////////////////////////////////////////////////////////

	tbool CCL_API isActive () const override; // (IWindowBase)
	void CCL_API activate () override;		 // (IWindowBase)
	virtual bool canActivate () const;

	void setLastFocusView (View* view);
	IView* getLastFocusView () const;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// View
	//////////////////////////////////////////////////////////////////////////////////////////////

	void attached (View* parent) override;
	void removed (View* parent) override;
	void onActivate (bool state) override;
	bool onMouseDown (const MouseEvent& event) override;
	bool setHelpIdentifier (StringRef id) override;
	StringRef getHelpIdentifier () const override;
	AccessibilityProvider* getAccessibilityProvider () override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

	CLASS_INTERFACE (IWindowBase, View)

private:
	static bool activateViewTree (View& parentView, bool state);

	ObjectList childs;
	WindowBase* activeChild;
	bool active;
	ObservedPtr<IView> lastFocusView;
	String helpIdentifier;

	void setActiveChild (WindowBase* child);
	bool isActiveDescendant (WindowBase* windowBase) const;
	bool isFocusViewAllowed (View& focusView) const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline WindowBase* WindowBase::getChildWindow (int index) const { return (WindowBase*)childs.at (index); }
inline WindowBase* WindowBase::getActiveChild () const { return activeChild; }
inline Iterator* WindowBase::getChildWindows () const { return childs.newIterator (); }
inline IView* WindowBase::getLastFocusView () const { return lastFocusView; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_windowbase_h
