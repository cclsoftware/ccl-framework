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
// Filename    : ccl/gui/popup/menupopupselector.h
// Description : Menu Popup Selector
//
//************************************************************************************************

#ifndef _ccl_menupopupselector_h
#define _ccl_menupopupselector_h

#include "ccl/gui/popup/itemviewpopup.h"

namespace CCL {

class Menu;
class MenuItem;

//************************************************************************************************
// MenuPopupSelector
/** Displays a menu as a TreeView in a PopupSelector. */
//************************************************************************************************

class MenuPopupSelector: public TreeViewPopup
{
public:
	typedef TreeViewPopup SuperClass;

	MenuPopupSelector (Menu* menu, bool shouldSelectCheckedItems = false);
	~MenuPopupSelector ();

	// IItemModel
	tbool CCL_API getRootItem (ItemIndex& index) override;
	tbool CCL_API canExpandItem (ItemIndexRef index) override;
	tbool CCL_API canSelectItem (ItemIndexRef index) override;
	StringID CCL_API getItemBackground (ItemIndexRef index) override;
	tbool CCL_API getSubItems (IUnknownList& items, ItemIndexRef index) override;
	tbool CCL_API getItemTitle (String& title, ItemIndexRef index) override;
	tbool CCL_API getItemTooltip (String& tooltip, ItemIndexRef index, int column) override;
	IImage* CCL_API getItemIcon (ItemIndexRef index) override;
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override;

	// TreeViewPopup
	VisualStyle* getVisualStyle (Theme& theme) override;
	ItemControl* createItemControl (RectRef size) override;
	void onItemViewCreated () override;
	void CCL_API attached (IWindow& popupWindow) override;
	Result CCL_API onKeyDown (const KeyEvent& event) override;
	Result CCL_API onMouseDown (const MouseEvent& event, IWindow& popupWindow) override;
	void CCL_API onPopupClosed (Result result) override;
	bool hasPopupResult () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

private:
	bool selectCheckedMenuItems (Menu& menu);
	bool selectMatchingItem (Menu& menu, const MenuItem& item);
	MenuItem* getCurrentMenuItem ();

	Menu* menu;
	bool mustCloseOnSelect;
	bool condensedSeparators;
	bool shouldSelectCheckedItems;
	bool hasCheckedItems;
	SharedPtr<IImage> checkMarkIcon;
	AutoPtr<IImage> noCheckMarkIcon;
};

} // namespace CCL

#endif // _ccl_menupopupselector_h
