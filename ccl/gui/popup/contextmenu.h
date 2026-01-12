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
// Filename    : ccl/gui/popup/contextmenu.h
// Description : Context Menu
//
//************************************************************************************************

#ifndef _ccl_contextmenu_h
#define _ccl_contextmenu_h

#include "ccl/gui/popup/popupselector.h"

#include "ccl/base/collections/objectarray.h"
#include "ccl/base/signalsource.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/gui/icontextmenu.h"
#include "ccl/public/gui/graphics/point.h"

namespace CCL {

class View;
class Menu;

//************************************************************************************************
// ContextMenu
//************************************************************************************************

class ContextMenu: public Object,
				   public IContextMenu
{
public:
	DECLARE_CLASS_ABSTRACT (ContextMenu, Object)
	DECLARE_PROPERTY_NAMES (ContextMenu)
	DECLARE_METHOD_NAMES (ContextMenu)

	virtual int countItems () const = 0;

	virtual void popup (const Point& where, View* view = nullptr) = 0;

	// IContextMenu
	void CCL_API setContextID (StringID id) override;
	StringID CCL_API getContextID () const override;
	void CCL_API setFocusItem (IUnknown* item) override;
	IUnknown* CCL_API getFocusItem () const override;

	CLASS_INTERFACE (IContextMenu, Object)

protected:
	MutableCString contextID;
	AutoPtr<IUnknown> focusItem;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
};

//************************************************************************************************
// ContextPopupMenu
//************************************************************************************************

class ContextPopupMenu: public ContextMenu
{
public:
	DECLARE_CLASS (ContextPopupMenu, ContextMenu)

	ContextPopupMenu (StringID menuType = nullptr);
	~ContextPopupMenu ();

	Menu* getMenu () const;

	// ContextMenu
	int countItems () const override;
	void popup (const Point& where, View* view = nullptr) override;
	tresult CCL_API addHeaderItem (StringRef title) override;
	tresult CCL_API addCommandItem (StringRef title, CStringRef category, CStringRef name, ICommandHandler* handler) override;
	tresult CCL_API removeCommandItem (CStringRef category, CStringRef name) override;
	tresult CCL_API addSeparatorItem () override;
	tbool CCL_API hasCommandHandler (ICommandHandler* handler) const override;
	tbool CCL_API hasCommandItem (CStringRef category, CStringRef name) const override;
	IContextMenu* CCL_API addSubContextMenu (StringRef title) override;
	tresult CCL_API setInitialSubMenu (StringRef path) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACES (ContextMenu)

protected:
	Menu* menu;
	ObjectArray subContextMenus;
	AutoPtr<PopupSelector> popupSelector;
	CCL::SignalSink controlSink;

	static ContextPopupMenu* nonModalInstance;

	static Menu* createMenu (StringID menuType);

	ContextPopupMenu (Menu* menu);

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

} // namespace CCL

#endif // _ccl_contextmenu_h
