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
// Filename    : ccl/gui/controls/commandbar/commandbarview.h
// Description : Command Bar View
//
//************************************************************************************************

#ifndef _ccl_commandbarview_h
#define _ccl_commandbarview_h

#include "ccl/gui/views/view.h"

#include "ccl/gui/controls/commandbar/commandbarmodel.h"
#include "ccl/public/gui/icommandhandler.h"
#include "ccl/public/gui/icontextmenu.h"
#include "ccl/public/gui/framework/imenu.h"

namespace CCL {

//************************************************************************************************
// CommandBarView
/** A specialized view that manages a user customizable arrangement of controls.*/
//************************************************************************************************

class CommandBarView: public View
{
public:
	DECLARE_CLASS (CommandBarView, View)
	DECLARE_METHOD_NAMES (CommandBarView)

	CommandBarView (RectRef size = Rect ());
	~CommandBarView ();

	PROPERTY_MUTABLE_CSTRING (itemFormName, ItemFormName)
	PROPERTY_MUTABLE_CSTRING (contextMenuFormName, ContextMenuFormName)
	PROPERTY_VARIABLE (float, scaleFactorX, ScaleFactorX)
	PROPERTY_VARIABLE (float, scaleFactorY, ScaleFactorY)

	void setModel (CommandBarModel* model);
	CommandBarModel* getModel () const;

	ICommandHandler* getCommandHandler ();

	// View
	IUnknown* CCL_API getController () const override;
	tbool CCL_API setController (IUnknown* controller) override;
	void attached (View* parent) override;

	DECLARE_STRINGID_MEMBER (kExtendButtonMenu)
	DECLARE_STRINGID_MEMBER (kExtendAssignMenu)

protected:
	CommandBarModel* model;
	SharedPtr<IUnknown> controller;
	UnknownPtr<ICommandHandler> commandHandler;
	UnknownPtr<IContextMenuHandler> contextMenuHandler;
	IParameter* selectedPageTitle;
	bool hasContextMenuPopup;

	class Builder;
	class TargetList;
	class MoveItemDragHandler;
	class ContextMenuDelegate;
	class MenuEditor;
	friend class Builder;
	friend class MoveItemDragHandler;
	struct AddItemContext;

	void resizeIcon (AutoPtr<IImage>& image);

	void collectTargets ();
	void makeViews ();
	bool canSwitchOrientation () const;
	bool isVerticalOrientation () const;
	void setVerticalOrientation (bool state);
	bool canEditItem (const CommandBar::Item& item) const;
	CommandBar::Item* findItem (View* view) const;
	CommandBar::Item* findItem (PointRef where) const;
	View* findViewForItem (CommandBar::Item& item) const;
	bool findInsertContext (CommandBar::InsertContext& context, CommandBar::Item* newItem, CommandBar::Item* mouseItem, PointRef where) const;
	int getContainerLayoutDirection (CommandBar::Item& item) const; ///< kHorizontal, kVertical or both for table
	String getDefaultTitle (CommandBar::ButtonItem& item);
	bool runCommandSelector (CommandBar::ButtonItem& item);
	void appendAssignMenu (IContextMenu& contextMenu, CommandBar::Item* item);
	void appendIconMenu (IContextMenu& contextMenu, CommandBar::Item* item);
	bool wantsContextMenu () const;

	int getSelectedPageIndex () const;
	CommandBar::PageItem* getSelectedPage () const;
	void selectPage (int index);
	void selectPage (CommandBar::PageItem* page);

	void buildMoveToGroupMenu (IMenu& menu, CommandBar::Item* item);
	void buildPagesSubMenu (UnknownPtr<IMenu> popupMenu, ICommandHandler* handler, String menuTitle);
	bool buildPagesMenu (IMenu& menu, ICommandHandler* handler, bool ignoreSelected);
	int getPageIndexFromArgs (CmdArgs args);

	bool onSetOrientation (CmdArgs args, VariantRef data);
	bool onAssignCommand (CmdArgs args, VariantRef data);
	bool onAssignTarget (CmdArgs args, VariantRef data);
	bool onSelectImage (CmdArgs args, VariantRef data);
	bool onRemoveImage (CmdArgs args, VariantRef data);
	bool onAddItem (CmdArgs args, VariantRef data);
	bool onRemoveItem (CmdArgs args, VariantRef data);
	bool onMoveToGroup (CmdArgs args, VariantRef data);
	bool onMoveGroupToPage (CmdArgs args, VariantRef data);
	bool onSelectPage (CmdArgs args, VariantRef data);

	// View
	void onViewsChanged () override;
	bool onMouseDown (const MouseEvent& event) override;
	IDragHandler* createDragHandler (const DragEvent& event) override;
	bool onContextMenu (const ContextMenuEvent& event) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

} // namespace CCL

#endif // _ccl_commandbar_h
