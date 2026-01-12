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
// Filename    : ccl/gui/controls/commandbar/commandbarview.cpp
// Description : Command Bar View
//
//************************************************************************************************

#include "ccl/gui/controls/commandbar/commandbarview.h"

#include "ccl/gui/controls/editbox.h"
#include "ccl/gui/views/sprite.h"
#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/views/viewanimation.h"
#include "ccl/gui/skin/form.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/dialogs/alert.h"
#include "ccl/gui/dialogs/dialogbuilder.h"
#include "ccl/gui/dialogs/commandselector.h"
#include "ccl/gui/dialogs/fileselector.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/popup/extendedmenu.h"
#include "ccl/gui/popup/contextmenu.h"
#include "ccl/gui/touch/touchinput.h"
#include "ccl/gui/system/dragndrop.h"
#include "ccl/gui/controls/button.h"
#include "ccl/gui/commands.h"

#include "ccl/public/gui/icontroller.h"
#include "ccl/public/gui/iviewfactory.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/paramlist.h"
#include "ccl/public/gui/icontextmenu.h"
#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/skinxmldefs.h"
#include "ccl/public/gui/framework/abstractdraghandler.h"

#include "ccl/base/storage/attributes.h"
#include "ccl/base/message.h"

#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/base/irecognizer.h"
#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("CommandBar")
	XSTRING (Assign, "Assign")
	XSTRING (Command, "Assign Command")
	XSTRING (RemoveButton, "Remove Button")
	XSTRING (RemoveGroup, "Remove Group")
	XSTRING (RemovePage, "Remove Page")
	XSTRING (RemoveMenu, "Remove Submenu")
	XSTRING (RemoveMenuItem, "Remove Menu Item")
	XSTRING (RemoveSeparator, "Remove Separator")
	XSTRING (NewButton, "New Button")
	XSTRING (NewGroup, "New Group")
	XSTRING (NewPage, "New Page")
	XSTRING (NewMenuButton, "New Menu Button")
	XSTRING (NewSubMenu, "New Submenu")
	XSTRING (NewMenuItem, "New Menu Item")
	XSTRING (NewSeparator, "New Separator")
	XSTRING (DefaultLayout, "Default")
	XSTRING (Horizontal, "Horizontal")
	XSTRING (Vertical, "Vertical")
	XSTRING (InitialGroupTitle, "Group")
	XSTRING (InitialPageTitle, "Page")
	XSTRING (InitialMenuTitle, "Menu")
	XSTRING (InitialMenuItemTitle, "Menu Item")
	XSTRING (Separator, "Separator")
	XSTRING (Icon, "Icon")
	XSTRING (SelectImage, "Select Image")
	XSTRING (RemoveImage, "Remove Image")
	XSTRING (MoveTo, "Move to")
	XSTRING (SelectPage, "Go to")
	XSTRING (SafetyQuestion, "Do you want to remove \"%(1)\"?")
	XSTRING (Unnamed, "<Unnamed>")
	XSTRING (EditMenu, "Edit Menu") 
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum CommandBarTags
	{
		kCommand = 100,
		kTitle,
		kColor,
		kTab,
		kLayout,
		kPageMenu
	};
}

//************************************************************************************************
// CommandBarView::MenuEditor
//************************************************************************************************

class CommandBarView::MenuEditor: public Object,
								  public AbstractController,
								  public ItemViewObserver<AbstractItemModel>,
								  public IItemDragVerifier,
								  public IParamObserver
{
public:
	MenuEditor (CommandBarView* commandBarView, CommandBar::GroupItem* menuContent);
	~MenuEditor ();

	void runDialog ();
	View* createView ();

	// AbstractController
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;

	// AbstractItemModel
	void CCL_API viewAttached (IItemView* itemView) override;
	tbool CCL_API getRootItem (ItemIndex& index) override;
	tbool CCL_API getSubItems (IUnknownList& items, ItemIndexRef index) override;
	tbool CCL_API getItemTitle (String& title, ItemIndexRef index) override;
	IImage* CCL_API getItemIcon (ItemIndexRef index) override;
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override;
	tbool CCL_API canExpandItem (ItemIndexRef index) override;
	tbool CCL_API canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView = nullptr) override;
	tbool CCL_API insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session) override;
	tbool CCL_API openItem (ItemIndexRef index, int column, const EditInfo& info) override;
	tbool CCL_API appendItemMenu (IContextMenu& contextMenu, ItemIndexRef index, const IItemSelection& selection) override;
	tbool CCL_API canRemoveItem (ItemIndexRef index) override;
	tbool CCL_API removeItem (ItemIndexRef index) override;

	// IItemDragVerifier
	tbool CCL_API verifyTargetItem (ItemIndex& item, int& relation) override;

	// IParamObserver
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API paramEdit (IParameter* param, tbool begin) override;

	CLASS_INTERFACE3 (IController, IItemModel, IItemDragVerifier, Object)

private:
	CommandBarView* commandBarView;
	SharedPtr<CommandBar::GroupItem> menuContent;
	AutoPtr<IImage> menuIcon;
	AutoPtr<IImage> itemIcon;

	enum { kItemName = 100 };

	template<class ItemClass = CommandBar::Item>
	static ItemClass* resolve (ItemIndexRef index);
	void setCommandBarView (CommandBarView* view);

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
};

//************************************************************************************************
// CommandBarView::AddItemContext
//************************************************************************************************

struct CommandBarView::AddItemContext: public Unknown
{
	AddItemContext (CommandBar::Item* parentItem, PointRef where)
	: parentItem (parentItem), where (where)
	{
		if(CommandBar::ButtonItem* button = ccl_cast<CommandBar::ButtonItem> (parentItem))
			if(button->getMenuContent ())
				parentItem = button->getMenuContent ();
	}

	CommandBar::Item* parentItem;
	Point where;
};

//************************************************************************************************
// CommandBarView::MenuEditor
//************************************************************************************************

CommandBarView::MenuEditor::MenuEditor (CommandBarView* commandBarView, CommandBar::GroupItem* menuContent)
: commandBarView (nullptr),
  menuContent (menuContent)
{
	setCommandBarView (commandBarView);

	menuIcon.share (FrameworkTheme::instance ().getImage (ThemeNames::kItemViewFolderIcon));
	//itemIcon.share (FrameworkTheme::instance ().getImage (...)); - do we need this?
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandBarView::MenuEditor::~MenuEditor ()
{
	setCommandBarView (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandBarView::MenuEditor::setCommandBarView (CommandBarView* view)
{
	if(view != commandBarView)
	{
		if(commandBarView)
		{
			commandBarView->removeObserver (this);
			if(commandBarView->getModel ())
				commandBarView->getModel ()->removeObserver (this);
		}

		commandBarView = view;

		if(commandBarView)
		{
			if(commandBarView->getModel ())
				commandBarView->getModel ()->addObserver (this);
			commandBarView->addObserver (this);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandBarView::MenuEditor::runDialog ()
{
	if(View* view = createView ())
	{
		DialogBuilder builder;
		builder.setTheme (view->getTheme ());
		Promise p (builder.runDialogAsync (view));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* CommandBarView::MenuEditor::createView ()
{
	Theme& theme = FrameworkTheme::instance ();
	return unknown_cast<View> (theme.createView ("CCL/MenuEditor", asUnknown ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class ItemClass>
ItemClass* CommandBarView::MenuEditor::resolve (ItemIndexRef index)
{
	return unknown_cast<ItemClass> (index.getObject ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandBarView::MenuEditor::viewAttached (IItemView* itemView)
{
	ItemViewObserver<AbstractItemModel>::viewAttached (itemView);

	UnknownPtr<ITreeView> treeView (getItemView ());
	if(treeView)
	{
		ITreeItem* rootItem = treeView->getRootItem ();
		treeView->expandItem (rootItem, true, ITreeView::kExpandChilds); // (kTreeViewExpandAll does not work as expected here)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API CommandBarView::MenuEditor::getObject (StringID name, UIDRef classID)
{
	if(name == "menuTree")
		return asUnknown ();

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarView::MenuEditor::getRootItem (ItemIndex& index)
{
	index = ItemIndex (ccl_as_unknown (menuContent));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarView::MenuEditor::getSubItems (IUnknownList& items, ItemIndexRef index)
{
	if(CommandBar::GroupItem* menuGroup = resolve<CommandBar::GroupItem> (index))
		for(int i = 0, num = menuGroup->countChilds (); i < num; i++)
			items.add (ccl_as_unknown (menuGroup->getChild (i)), true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarView::MenuEditor::getItemTitle (String& title, ItemIndexRef index)
{
	if(CommandBar::Item* item = resolve (index))
	{
		title = item->getTitle ();
		if(title.isEmpty ())
			title = XSTR (InitialMenuItemTitle);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API CommandBarView::MenuEditor::getItemIcon (ItemIndexRef index)
{
	CommandBar::Item* item = resolve (index);

	if(ccl_cast<CommandBar::MenuItem> (item))
		return itemIcon;
	else if(ccl_cast<CommandBar::MenuGroupItem> (item))
		return menuIcon;

	return nullptr; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarView::MenuEditor::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	if(resolve<CommandBar::MenuSeparatorItem> (index))
	{
		Coord y = info.rect.getCenter ().y;
		info.graphics.drawLine (Point (info.rect.left, y), Point (info.rect.right - 4, y), Pen (Color (info.style.adaptiveColor).setAlphaF (0.5f)));
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarView::MenuEditor::canExpandItem (ItemIndexRef index)
{
	return resolve<CommandBar::GroupItem> (index) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarView::MenuEditor::canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView)
{
    if(CommandBar::Item* item = unknown_cast<CommandBar::Item> (data.getFirst ()))
	{
		UnknownPtr<IItemView> itemView (targetView);
		if(itemView && itemView->getModel () == this)
		{
			UnknownPtr<IItemDragTarget> dragTarget (targetView);
			if(dragTarget)
			{
				int flags = IItemView::kCanDragBetweenItems|IItemView::kCanDragOnItem|IItemView::kDropInsertsData;
				AutoPtr<IDragHandler> dragHandler (dragTarget->createDragHandler (flags, this));
				session->setDragHandler (dragHandler);
				session->setResult (IDragSession::kDropMove);
				return true;
			}
		}
    }
	return false;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarView::MenuEditor::insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session)
{
	if(CommandBar::Item* movedItem = unknown_cast<CommandBar::Item> (data.getFirst ()))
	{
		ItemIndex targetIndex;
		int relation = 0;
	
		UnknownPtr<IItemViewDragHandler> itemViewHandler (session ? session->getDragHandler () : nullptr);
		if(itemViewHandler && itemViewHandler->getTarget (targetIndex, relation))
		{
			CommandBar::Item* targetItem = resolve (targetIndex);
			if(!targetItem && relation == IItemViewDragHandler::kAfterItem)
				targetItem = menuContent->getChild (menuContent->countChilds () - 1); // after last in root menu

			if(targetItem)
			{
				CommandBarModel* model = commandBarView->getModel ();
				CommandBar::InsertContext insertContext;

				if(relation == IItemViewDragHandler::kOnItem)
				{
					if(targetItem == movedItem) // not into self
						return true;

					insertContext.parent = ccl_cast<CommandBar::MenuGroupItem> (targetItem);
				}
				else
				{
					insertContext.parent = ccl_cast<CommandBar::MenuGroupItem> (model->findParentItem (targetItem));
					if(insertContext.parent)
						insertContext.index = insertContext.parent->getIndex (targetItem);
				}
				
				ASSERT (insertContext.parent)
				if(insertContext.parent)
				{
					// shift index if we move a view upwards in the same parent
					int oldIndex = insertContext.parent->getIndex (movedItem);
					if(oldIndex >= 0 && oldIndex < insertContext.index)
						insertContext.index--;

					if(relation == IItemViewDragHandler::kAfterItem)
						insertContext.index++;

					if(insertContext.index >= insertContext.parent->countChilds ())
						insertContext.index = -1;

					movedItem->retain ();
					model->removeItem (movedItem);
					model->addItem (movedItem, insertContext);
				}
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarView::MenuEditor::verifyTargetItem (ItemIndex& item, int& relation)
{
	// kOnItem (-> "into") is only allowed on submenus
	if(relation == IItemViewDragHandler::kOnItem && !resolve<CommandBar::MenuGroupItem> (item))
		relation = IItemViewDragHandler::kBeforeOrAfterItem;
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarView::MenuEditor::canRemoveItem (ItemIndexRef index)
{
	return resolve (index) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarView::MenuEditor::removeItem (ItemIndexRef index)
{
	if(CommandBar::Item* item = resolve (index))
		return commandBarView->onRemoveItem (CommandMsg (), Variant (item->asUnknown ()));

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarView::MenuEditor::openItem (ItemIndexRef index, int column, const EditInfo& info)
{
	CommandBar::Item* item = resolve (index);
	if(item && !ccl_cast<CommandBar::MenuSeparatorItem> (item))
	{
		if(IItemView* itemView = getItemView ())
		{
			// determine depth in tree (for inset)
			int depth = -1;
			ITreeItem* treeItem = index.getTreeItem ();
			while(treeItem = treeItem->getParentItem ())
				depth++;

			AutoPtr<VisualStyle> editStyle (NEW VisualStyle);
			editStyle->copyFrom (info.view->getVisualStyle ());
			editStyle->setOptions (StyleID::kTextAlign, Alignment::kLeftCenter);

			Rect rect (info.rect);
			rect.left += editStyle->getMetric<Coord> ("itemInset", 0) * depth;
			EditBox* editBox = NEW EditBox (rect);
			StyleModifier (*editBox).setCommonStyle (Styles::kTransparent);
			editBox->setVisualStyle (editStyle);			

			IParameter* nameParam = editBox->getParameter ();
			nameParam->setName (MutableCString (item->getID ()));
			nameParam->fromString (item->getTitle ());
			nameParam->connect (this, kItemName);

			itemView->setEditControl (editBox);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarView::MenuEditor::appendItemMenu (IContextMenu& contextMenu, ItemIndexRef index, const IItemSelection& selection) 
{
	CommandBar::Item* item = resolve (index);
	if(!item)
		item = menuContent; // empty space: root menu
	
	bool isMenu = ccl_cast<CommandBar::MenuGroupItem> (item);
	bool isMenuItem = !isMenu && ccl_cast<CommandBar::MenuItem> (item);
	if(isMenuItem)
	{
		commandBarView->appendAssignMenu (contextMenu, item);
		contextMenu.addSeparatorItem ();

		UnknownPtr<IObserver> ctrler (commandBarView->getController ());
		if(ctrler)
		{
			Message msg (CommandBarView::kExtendButtonMenu, &contextMenu, item->asUnknown ());
			ctrler->notify (this, msg);

			contextMenu.addSeparatorItem ();
		}
	}

	AutoPtr<CommandBarView::AddItemContext> addItemContext (NEW CommandBarView::AddItemContext (item, Point ()));
	UnknownPtr<IMenu> menu (&contextMenu);

	auto addCommandItem = [&] (StringRef title, StringID category, StringID name, ICommandHandler* handler, IImage* icon = nullptr)
	{
		if(IMenuItem* item = menu->addCommandItem (title, category, name, handler))
			item->setItemAttribute (IMenuItem::kItemIcon, Variant (icon));
	};

	addCommandItem (XSTR (NewMenuItem), "Command", "New Menu Item", CommandDelegate<CommandBarView>::make (commandBarView, &CommandBarView::onAddItem, addItemContext)/*, itemIcon*/);
	addCommandItem (XSTR (NewSubMenu), "Command", "New Menu", CommandDelegate<CommandBarView>::make (commandBarView, &CommandBarView::onAddItem, addItemContext)/*, menuIcon*/);
	addCommandItem (XSTR (NewSeparator), "Command", "New Menu Separator", CommandDelegate<CommandBarView>::make (commandBarView, &CommandBarView::onAddItem, addItemContext));
	if(item != menuContent)
	{
		contextMenu.addSeparatorItem ();

		String removeTitle;
		if(isMenu)
			removeTitle = XSTR (RemoveMenu);
		else if(isMenuItem)
			removeTitle = XSTR (RemoveMenuItem);
		else
			removeTitle = XSTR (RemoveSeparator);
		contextMenu.addCommandItem (removeTitle, "Command", "Remove", CommandDelegate<CommandBarView>::make (commandBarView, &CommandBarView::onRemoveItem, ccl_as_unknown (item)));
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarView::MenuEditor::paramChanged (IParameter* param)
{
	if(param->getTag () == kItemName)
	{
		// item name edit box finished:
		if(CommandBar::Item* item = unknown_cast<CommandBar::Item> (commandBarView->getModel ()->getItemByID (String (param->getName ()))))
			commandBarView->getModel ()->setItemProperty (item, "title", param->getValue ());
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandBarView::MenuEditor::paramEdit (IParameter* param, tbool begin)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandBarView::MenuEditor::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged && commandBarView && isEqualUnknown (subject, ccl_as_unknown (commandBarView->getModel ())))
	{
		UnknownPtr<ITreeView> treeView (getItemView ());
		if(treeView)
		{
			ItemIndex focusIndex;
			SharedPtr<CommandBar::Item> focusItem = getItemView ()->getFocusItem (focusIndex) ? resolve (focusIndex) : nullptr;

			ITreeItem* rootItem = treeView->getRootItem ();
			treeView->refreshItem (rootItem);
			treeView->expandItem (rootItem, true, ITreeView::kExpandChilds);

			if(focusItem)
				getItemView ()->setFocusItem (ItemIndex (ccl_as_unknown (focusItem)));
		}
	}
	else if(subject == commandBarView)
	{
		if(msg == kDestroyed)
			setCommandBarView (nullptr);
		else if(msg == "willRemoveModel")
		{
			if(commandBarView->getModel ())
				commandBarView->getModel ()->removeObserver (this);
		}
	}
}

//************************************************************************************************
// CommandBarView::TargetList
//************************************************************************************************

class CommandBarView::TargetList
{
public:
	TargetList (CommandBarView& commandBarView);

	CommandBar::CommandTarget* findTarget (String name) const;
	bool isEmpty () const;
	Iterator* newIterator () const;

private:
	ObjectArray targets;

	void collect (CommandBarView& commandBarView);
};

//************************************************************************************************
// CommandBarView::ContextMenuDelegate
//************************************************************************************************

class CommandBarView::ContextMenuDelegate: public View
{
public:
	ContextMenuDelegate (RectRef size)
	:  View (size)
	{}

	bool onContextMenu (const ContextMenuEvent& event) override
	{
		if(CommandBarView* commandBarView = getParent<CommandBarView> ())
		{
			ContextMenuEvent e2 (event);
			clientToWindow (e2.where);
			commandBarView->windowToClient (e2.where);

			return commandBarView->onContextMenu (e2);
		}
		return View::onContextMenu (event);
	}
};

//************************************************************************************************
// CommandBarView::Builder
//************************************************************************************************

class CommandBarView::Builder: public Object,
							   public AbstractController,
							   public ICommandHandler,
							   public IParamObserver,
							   public IViewFactory
{
public:
	Builder (CommandBarView* commandBarView);

	PROPERTY_MUTABLE_CSTRING (itemFormName, ItemFormName)

	IView* createView (CommandBar::Item& item);

	// IController
	IParameter* CCL_API findParameter (StringID name) const override;

	// ICommandHandler
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

	// IParamObserver
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API paramEdit (IParameter* param, tbool begin) override;

	// IViewFactory
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

	// Object
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACES (Object)

private:
	CommandBarView* commandBarView;
	CommandBar::Item* currentItem;
	int currentChildIndex;
	ParamList paramList;

	String getItemTitle (CommandBar::Item& item) const;
	CommandBar::Item* findItem (IParameter& param) const;
	void buildMenu (IMenu& menu, CommandBar::MenuGroupItem& groupItem);
};

//************************************************************************************************
// CommandBarView::TargetList
//************************************************************************************************

CommandBarView::TargetList::TargetList (CommandBarView& commandBarView)
{
	targets.objectCleanup (true);
	collect (commandBarView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandBarView::TargetList::collect (CommandBarView& commandBarView)
{
	UnknownPtr<IController> controller (commandBarView.getController ());
	if(controller)
	{
		UnknownPtr<IArrayObject> commandTargets = controller->getObject ("commandTargets", ccl_iid<IArrayObject> ());
		if(commandTargets)
		{
			int numTargets = commandTargets->getArrayLength ();
			for(int i = 0; i < numTargets; i++)
			{
				Variant var;
				if(commandTargets->getArrayElement (var, i))
				{
					UnknownPtr<IObject> object (var.asUnknown ());
					if(object)
					{
						CommandBar::CommandTarget* target = NEW CommandBar::CommandTarget;
						target->fromProperties (*object);
						targets.add (target);
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandBar::CommandTarget* CommandBarView::TargetList::findTarget (String name) const
{
	ArrayForEachFast (targets, CommandBar::CommandTarget, t)
		if(t->getName () == name)
			return t;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarView::TargetList::isEmpty () const
{
	return targets.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* CommandBarView::TargetList::newIterator () const
{
	return targets.newIterator ();
}

//************************************************************************************************
// CommandBarView::MoveItemDragHandler
//************************************************************************************************

class CommandBarView::MoveItemDragHandler: public Unknown,
										   public AbstractDragHandler
{
public:
	MoveItemDragHandler (CommandBarView* commandBarView, CommandBar::Item* item)
	: commandBarView (commandBarView),
	  item (item)
	{}

	// IDragHandler
	tbool CCL_API dragEnter (const DragEvent& event) override
	{
		// position sprite
		Rect rect;
		Color color = commandBarView->getTheme ().getThemeColor (ThemeElements::kAlphaCursorColor);
		color = commandBarView->getVisualStyle ().getColor ("dragspritecolor", color);
		AutoPtr<SolidDrawable> drawable = NEW SolidDrawable (color);
		if(NativeGraphicsEngine::instance ().hasGraphicsLayers ())
			positionSprite = NEW SublayerSprite (commandBarView, drawable, rect);
		else
		{
			drawable->takeOpacity ();
			positionSprite = NEW FloatingSprite (commandBarView, drawable, rect);
		}

		#if 0
		// sprite for dragged button
		if(View* view = commandBarView->findViewForItem (*item))
		{
			AutoPtr<IImage> image (ViewScreenCapture ().takeScreenshot (view));
			if(image)
			{
				AutoPtr<IDrawable> drawable = NEW ImageDrawable (image, .8f);

				Point p;
				view->clientToWindow (p);
				view->windowToClient (p);
				Rect size (p.x, p.y, view->getSize ().getSize ());

				setSprite (Sprite::createSprite (commandBarView, drawable, size));
			}
		}
		#endif
		return dragOver (event);
	}

	tbool CCL_API dragOver (const DragEvent& event) override
	{
		CommandBar::InsertContext context;

		if(CommandBar::Item* mouseItem = commandBarView->findItem (event.where))
			if(commandBarView->findInsertContext (context, item, mouseItem, event.where) && context.parent)
			{
				// position sprite
				if(View* parentView = commandBarView->findViewForItem (*context.parent))
				{
					int layout = commandBarView->getContainerLayoutDirection (*context.parent);
					bool isTable = layout == (Styles::kVertical|Styles::kHorizontal);
					bool isVertical = layout == Styles::kVertical;
					bool isAppend = context.index < 0;

					// find reference item
					CommandBar::Item* referenceItem = isAppend
						? context.parent->getChild (context.parent->countChilds () - 1)
						: context.parent->getChild (context.index);

					View* referenceView = referenceItem ? commandBarView->findViewForItem (*referenceItem) : parentView;
					if(referenceView)
					{
						Point p, parentLoc;
						referenceView->clientToWindow (p);
						parentView->clientToWindow (parentLoc);

						Rect rect (parentView->getSize ());
						if(isTable)
						{
							if(!isAppend)
								rect = referenceView->getSize (); // full cell rect
							else
							{
								// todo: append after last item
								cleanup (); //  hide sprite for now
								return true;
							}
						}
						else if(isVertical)
						{
							p.x = parentLoc.x;
							rect.setHeight (2);
						}
						else
						{
							p.y = parentLoc.y;
							rect.setWidth (2);
						}
						commandBarView->windowToClient (p);

						// append (after last): right edge
						if(isAppend && referenceView != parentView)
						{
							if(isVertical)
								p.y += referenceView->getHeight ();
							else
								p.x += referenceView->getWidth ();
						}

						rect.moveTo (p);
						positionSprite->move (rect);
						if(!positionSprite->isVisible ())
							positionSprite->show ();
					}
				}
			}
		return AbstractDragHandler::dragOver (event);
	}

	tbool CCL_API drop (const DragEvent& event) override
	{
		cleanup ();
		return true;
	}

	tbool CCL_API dragLeave (const DragEvent& event) override
	{
		cleanup ();
		return true;
	}

	tbool CCL_API afterDrop (const DragEvent& event) override
	{
		CommandBar::InsertContext context;
		if(CommandBar::Item* mouseItem = commandBarView->findItem (event.where))
		{
			if(commandBarView->findInsertContext (context, item, mouseItem, event.where) && context.parent)
			{
				if(item->getType() != "Page") // pages cannot be dragged
				{
					if(CommandBarModel* model = commandBarView->getModel ())
					{
						bool isTable = commandBarView->getContainerLayoutDirection (*context.parent) == (Styles::kVertical|Styles::kHorizontal);
						bool isAppend = context.index < 0;
						if(isTable && isAppend)
							return true; // not implemented for table yet

						// shift index if we move a view upwards in the same parent
						int oldIndex = context.parent->getIndex (item);
						if(oldIndex >= 0 && oldIndex < context.index && !isTable)
							context.index--;

						if(context.index >= context.parent->countChilds ())
							context.index = -1;

						item->retain ();
						model->removeItem (item);
						model->addItem (item, context);
					}
				}
			}
		}

		return true;
	}

	virtual void cleanup ()
	{
		if(positionSprite)
			positionSprite->hide ();
	}

	CLASS_INTERFACE (IDragHandler, Unknown)

protected:
	CommandBarView* commandBarView;
	SharedPtr<CommandBar::Item> item;
	AutoPtr<Sprite> positionSprite;
};

//************************************************************************************************
// CommandBarView::Builder
//************************************************************************************************

CommandBarView::Builder::Builder (CommandBarView* commandBarView)
: commandBarView (commandBarView),
  itemFormName ("CommandBarItem"),
  currentItem (nullptr),
  currentChildIndex (0)
{
	paramList.setController (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CommandBarView::Builder::queryInterface (CCL::UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (IController)
	QUERY_INTERFACE (ICommandHandler)
	QUERY_INTERFACE (IParamObserver) 
	QUERY_INTERFACE (IViewFactory) 
	return Object::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String CommandBarView::Builder::getItemTitle (CommandBar::Item& item) const
{
	return item.getTitle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarView::Builder::getProperty (Variant& var, MemberID propertyId) const
{
	if(currentItem)
	{
		if(currentItem->getProperty (var, propertyId))
			return true;

		MutableCString arrayKey;
		if(propertyId == "layout")
		{
			// access layout property from parent
			if(CommandBarModel* model = commandBarView->getModel ())
				if(CommandBar::Item* parent = model->findParentItem (currentItem))
					return parent->getProperty (var, propertyId);
		}
		else if(propertyId == "orientation")
		{
			if(commandBarView->canSwitchOrientation ())
				var = commandBarView->isVerticalOrientation () ? "vertical" : "horizontal";
			else
				var = String::kEmpty;
			return true;
		}
		else if(propertyId == "itemIndex")
		{
			var = currentChildIndex;
			return true;
		}
		else if(propertyId.getBetween (arrayKey, "child[", "]")) 
		{
			CommandBar::Item* child = currentItem->getChild (arrayKey.scanInt (-1));
			var = ccl_as_unknown (child);
			return true;
		}
		else if(propertyId == "parent")
		{
			CommandBar::Item* parent = nullptr;
			if(CommandBarModel* model = commandBarView->getModel ())
				parent = model->findParentItem (currentItem);

			var = ccl_as_unknown (parent);
			return true;
		}
		else if(propertyId == "selectedPageIndex")
		{
			var = commandBarView->getSelectedPageIndex ();
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API CommandBarView::Builder::findParameter (StringID name) const
{
	IParameter* p = nullptr;
	if(currentItem)
	{
		// internal name: itemId.parameterName
		MutableCString id (currentItem->getID ());
		id += '.';
		id += name;
		
		p = paramList.lookup (id);
		if(!p)
		{
			ParamList& mutableParamList = const_cast<ParamList&> (paramList);

			if(name == "command")
			{
				if(CommandBar::ButtonItem* button = ccl_cast<CommandBar::ButtonItem> (currentItem))
				{ 
					if(button->isExternalTarget ())
					{
						// parameter of controller
						UnknownPtr<IController> controller (commandBarView->getController ());
						if(controller)
							p = controller->findParameter (button->getCommandName ());

						if(!p && button->getControlType () == CommandBar::ButtonItem::kMenu && button->getCommandName () == "pageMenu")
							p = mutableParamList.addMenu (button->getCommandName ());
					}
					else
					{
						if(button->getControlType () == CommandBar::ButtonItem::kMenu)
							p = mutableParamList.addMenu (id);
						else
							p = mutableParamList.addCommand (button->getCommandCategory (), button->getCommandName (), id, Tag::kCommand);
					}
				}
			}
			else if(name == "title")
			{
				p = mutableParamList.addString (id, Tag::kTitle);
				p->setValue (getItemTitle (*currentItem));
			}
			else if(name == "color")
			{
				p = mutableParamList.addColor (id, Tag::kColor);
				if(UnknownPtr<IColorParam> c = p)
					c->setColor (Color::fromInt (currentItem->getColor ()));
			}
			else if(name == "tab")
				p = mutableParamList.addInteger (0, currentItem->countChilds () - 1, id, Tag::kTab);
			else if(name == "layout")
			{
				if(CommandBar::GroupItem* group = ccl_cast<CommandBar::GroupItem> (currentItem))
				{
					p = mutableParamList.addList (id, Tag::kLayout);
					UnknownPtr<IListParameter> layout (p);
					layout->appendValue ("default", XSTR (DefaultLayout));
					layout->selectValue (group->getLayout ());
				}
			}
			else if(name == "orientation")
			{
				p = mutableParamList.addList (id, Tag::kLayout);
				UnknownPtr<IListParameter> layout (p);
				layout->appendValue ("horizontal", XSTR (Horizontal));
				layout->appendValue ("vertical", XSTR (Vertical));
				p->setValue (commandBarView->isVerticalOrientation () ? 1 : 0);
			}
			else if(name == "selectedPageTitle")
				p = mutableParamList.add (return_shared (commandBarView->selectedPageTitle));
		}
	}
	return p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandBar::Item* CommandBarView::Builder::findItem (IParameter& param) const
{
	CommandBarModel* model = commandBarView->getModel ();
	if(!model)
		return nullptr;

	MutableCString id (param.getName ());
	int dotIndex = id.lastIndex ('.');
	ASSERT (dotIndex >= 0)
	if(dotIndex >= 0)
		id.truncate (dotIndex);

	return model->findItem (String (id));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarView::Builder::checkCommandCategory (CStringRef category) const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarView::Builder::interpretCommand (const CommandMsg& msg)
{
	struct DeferredCommand: public Object
	{
		static bool perform (ICommandHandler* commandHandler, const CommandMsg& msg, CommandBar::Item* item, bool tryGlobal)
		{
			if(msg.checkOnly ())
				return performInternal (commandHandler, msg, item, tryGlobal);
			else
				NEW DeferredCommand (commandHandler, msg, item, tryGlobal);
			return true;
		}

		static bool performInternal (ICommandHandler* commandHandler, const CommandMsg& msg, CommandBar::Item* item, bool tryGlobal)
		{
			CommandMsg msg2 (msg);
			if(item)
				msg2.invoker = ccl_as_unknown (item);

			if(commandHandler->interpretCommand (msg2))
				return true;
				
			if(tryGlobal)
			{
				// if handler doesn't interpret the command, fallback to global CommandTable (for buttons assigned to commands)
				return CommandTable::instance ().performCommand (msg);
			}
			return false;
		}

		DeferredCommand (ICommandHandler* commandHandler, const CommandMsg& msg, CommandBar::Item* item, bool tryGlobal)
		: commandHandler (commandHandler),
		  item (item),
		  commandMsg (msg),
		  cmdCategory (msg.category),
		  cmdName (msg.name),
		  tryGlobal (tryGlobal)
		{
			commandMsg.category = cmdCategory;
			commandMsg.name = cmdName;

			if(auto menuItem = unknown_cast<MenuItem> (commandMsg.invoker))
				invoker.share (commandMsg.invoker);

			(NEW Message ("perform"))->post (this);
		}

		void CCL_API notify (ISubject* subject, MessageRef msg) override
		{
			if(msg == "perform")
			{
				ASSERT (!commandMsg.checkOnly ())
				performInternal (commandHandler, commandMsg, item, tryGlobal);
				delete this;
			}
		}

	private:
		SharedPtr<ICommandHandler> commandHandler; ///< keep handler alive, CommandBarView might go away during command execution (view commands)
		SharedPtr<CommandBar::Item> item;
		CommandMsg commandMsg;
		MutableCString cmdCategory;
		MutableCString cmdName;
		AutoPtr<IUnknown> invoker;
		bool tryGlobal;
	};

	// delegate to a command handler of view controller
	if(ICommandHandler* commandHandler = commandBarView->getCommandHandler ())
	{
		// resolve invoker: command param to command bar item
		CommandBar::Item* item = nullptr;
		bool tryGlobal = false;

		if(UnknownPtr<IParameter> param = msg.invoker)
		{
			item = findItem (*param);
			tryGlobal = isEqualUnknown (param->getController (), asUnknown ());
		}
		else if(MenuItem* menuItem = unknown_cast<MenuItem> (msg.invoker))
		{
			if(item = unknown_cast<CommandBar::Item> (menuItem->getItemData ()))
				tryGlobal = true;
		}

		// execute deferred
		return DeferredCommand::perform (commandHandler, msg, item, tryGlobal);			
	}
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarView::Builder::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kTitle:
		if(CommandBar::Item* item = findItem (*param))
		{
			String title;
			param->toString (title);
			if(CommandBarModel* model = commandBarView->getModel ())
				model->setItemProperty (item, "title", title);
		}
		return true;

	case Tag::kLayout:
		if(CommandBar::GroupItem* group = ccl_cast<CommandBar::GroupItem> (findItem (*param)))
		{
			UnknownPtr<IListParameter> layout (param); 
			if(layout)
			{
				String layoutName (layout->getSelectedValue ().asString ());
				if(layoutName == "Default")
					layoutName.empty ();

				if(CommandBarModel* model = commandBarView->getModel ())
					model->setItemProperty (group, "layout", layoutName);
			}
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandBarView::Builder::paramEdit (IParameter* param, tbool begin)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandBarView::Builder::buildMenu (IMenu& menu, CommandBar::MenuGroupItem& groupItem)
{
	menu.setMenuAttribute (IMenu::kMenuData, groupItem.asUnknown ());

	UnknownPtr<IController> controller (commandBarView->getController ());

	for(int i = 0, num = groupItem.countChilds (); i < num; i++)
	{
		CommandBar::Item* item = groupItem.getChild (i);
		if(CommandBar::ButtonItem* button = ccl_cast<CommandBar::ButtonItem> (item))
		{
			String title (button->getTitle ());
			if(title.isEmpty ())
				title = " ";

			MutableCString cmdCategory (button->getCommandCategory ());
			MutableCString cmdName (button->getCommandName ());
			if(button->isExternalTarget () && controller)
			{
				// parameter of controller
				if(UnknownPtr<ICommandParameter> param = controller->findParameter (button->getCommandName ()))
				{
					cmdCategory = param->getCommandCategory ();
					cmdName = param->getCommandName ();
				}
			}		

			IMenuItem* menuItem = menu.addCommandItem (title, cmdCategory, cmdName, this);
			menuItem->setItemAttribute (IMenuItem::kItemData, button->asUnknown ());

			#if 0
			if(IImage* icon = button->getIcon ())
				menuItem->setItemAttribute (IMenuItem::kItemIcon, icon);
			#endif
		}
		else if(CommandBar::MenuGroupItem* subGroup = ccl_cast<CommandBar::MenuGroupItem> (item))
		{
			IMenu* subMenu = menu.createMenu ();
			subMenu->setMenuAttribute (IMenu::kMenuTitle, subGroup->getTitle ());
			buildMenu (*subMenu, *subGroup);

			menu.addMenu (subMenu);
		}
		else if(ccl_cast<CommandBar::MenuSeparatorItem> (item))
			menu.addSeparatorItem ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandBarView::Builder::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IParameter::kExtendMenu)
	{
		UnknownPtr<IParameter> param (subject);
		UnknownPtr<IMenu> menu (msg.getArg (0));
		if(menu && param)
		{
			if(param->getName () == "pageMenu")
				commandBarView->buildPagesMenu (*menu, CommandDelegate<CommandBarView>::make (commandBarView, &CommandBarView::onSelectPage, 0), false);
			else
			{
				// build menu from sub-items
				if(CommandBar::ButtonItem* buttonItem = ccl_cast<CommandBar::ButtonItem> (findItem (*param)))
					if(CommandBar::MenuGroupItem* menuContent = ccl_cast<CommandBar::MenuGroupItem> (buttonItem->getMenuContent ()))
						buildMenu (*menu, *menuContent);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API CommandBarView::Builder::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name.contains ("@child") && currentItem)
	{
		int index = -1;
		sscanf (name, "@child[%d]", &index);
		if(index >= 0)
		{
			CommandBar::Item* child = currentItem->getChild (index);
			ASSERT (child != nullptr)
			if(child)
			{
				ScopedVar<int> scope (currentChildIndex, index);
				return createView (*child);
			}
		}
	}
	else if(name == "ContextMenuDelegate")
	{
		return NEW ContextMenuDelegate (bounds);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CommandBarView::Builder::createView (CommandBar::Item& item)
{
	ScopedVar<CommandBar::Item*> scope (currentItem, &item);

	#if DEBUG_LOG
	MutableCString s (CCL_INDENT); s += "createView "; s += MutableCString (item.getType ());
	s += ": "; s += MutableCString (item.getTitle ());
	CCL_LOGSCOPE (s)
	#endif

	Attributes arguments;
	arguments.set ("title", getItemTitle (item));
	
	CommandBar::ButtonItem* button = ccl_cast<CommandBar::ButtonItem> (&item);
	if(button)
	{
		AutoPtr<IImage> icon;
		icon.share (button->getIcon ());
		if(icon == nullptr)
		{
			if(CommandBar::CommandTarget* target = button->getTarget ())
				icon.share (target->getIcon ());
		}
		
		if(commandBarView && commandBarView->isAttached ()) // need visual style for correct icon limits
			commandBarView->resizeIcon (icon);

		if(icon)
			arguments.set ("icon", icon, Attributes::kShare);
	}

	IView* view = nullptr;
	if(commandBarView)
	{
		view = commandBarView->getTheme ().createView (itemFormName, asUnknown (), &arguments);
		if(Form* form = unknown_cast<Form> (view))
			form->setTitle (item.getID ()); // using form title (invisible) to map view to item

		// scaling: only for buttons & custom items
		if(button || ccl_cast<CommandBar::CustomItem> (&item))
		{
			float scaleX = commandBarView->getScaleFactorX ();
			float scaleY = commandBarView->getScaleFactorY ();

			if(scaleX != 1.f || scaleY != 1.f)
			{
				Rect rect (view->getSize ());
				rect.setWidth ((Coord)(rect.getWidth () * scaleX));
				rect.setHeight ((Coord)(rect.getHeight () * scaleY));
				view->setSize (rect);
			}
		}
	}
	return view;
}

//************************************************************************************************
// CommandBarView
//************************************************************************************************

DEFINE_CLASS (CommandBarView, View)
DEFINE_CLASS_UID (CommandBarView, 0x9b1dd365, 0x5d22, 0x48b0, 0x86, 0x92, 0xdb, 0x98, 0x69, 0xd5, 0xd5, 0x71)

DEFINE_STRINGID_MEMBER_ (CommandBarView, kExtendButtonMenu, "ExtendButtonMenu")
DEFINE_STRINGID_MEMBER_ (CommandBarView, kExtendAssignMenu, "ExtendAssignMenu")

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandBarView::CommandBarView (RectRef size)
: View (size),
  model (nullptr),
  itemFormName ("CommandBarItem"),
  contextMenuFormName ("CommandBarContextMenu"),
  selectedPageTitle (ccl_new<IParameter> (ClassID::StringParam)),
  scaleFactorX (1.f),
  scaleFactorY (1.f),
  hasContextMenuPopup (false)
{
	selectedPageTitle->setName ("selectedPageTitle");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandBarView::~CommandBarView ()
{
	setModel (nullptr);
	selectedPageTitle->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandBarView::setModel (CommandBarModel* _model)
{
	if(model && _model != model)
		signal (Message ("willRemoveModel"));

	share_and_observe (this, model, _model);

	UnknownPtr<IObject> controllerObj (controller);
	if(controllerObj && model)
	{
		// controller can suggest scale factors to avoid visible rescaling when layout is complete
		Variant var;
		if(controllerObj->getProperty (var, "CommandBarViewScaleX") && var.asFloat () > 0)
			setScaleFactorX (var.asFloat ());
		if(controllerObj->getProperty (var, "CommandBarViewScaleY") && var.asFloat () > 0)
			setScaleFactorY (var.asFloat ());
	}

	makeViews ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandBarModel* CommandBarView::getModel () const
{
	return model;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API CommandBarView::getController () const
{ 
	return controller; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarView::setController (IUnknown* c)
{ 
	controller = c;

	UnknownPtr<IController> iController (controller);
	if(iController)
	{
		commandHandler = iController->getObject ("commandHandler", ccl_iid<ICommandHandler> ());
		contextMenuHandler = iController->getObject ("contextMenuHandler", ccl_iid<IContextMenuHandler> ());
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ICommandHandler* CommandBarView::getCommandHandler ()
{
	return commandHandler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandBarView::collectTargets ()
{
	struct TargetsCollector: public CommandBar::ItemTraverser,
							 private TargetList
	{
		TargetsCollector (CommandBarView& view) : TargetList (view) {}

		bool visit (CommandBar::Item* item) override
		{
			if(CommandBar::ButtonItem* button = ccl_cast<CommandBar::ButtonItem> (item))
			{
				CommandBar::CommandTarget* target = nullptr;
				if(button->isExternalTarget ())
					target = findTarget (String (button->getCommandName ()));
				button->setTarget (target);
			}
			return true;
		}
	};

	if(model)
		TargetsCollector (*this).traverse (&model->getRootItem ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandBarView::onViewsChanged ()
{
	// don't checkFitSize here, will be done finally in makeViews
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandBarView::makeViews ()
{
	removeAll ();
	collectTargets ();

	CommandBar::PageItem* page = getSelectedPage ();
	selectedPageTitle->setValue (page ? page->getTitle () : nullptr);

	if(CommandBar::Item* rootItem = model ? &model->getRootItem () : nullptr)
	{
		AutoPtr<Builder> builder (NEW Builder (this));
		builder->setItemFormName (itemFormName);
		if(View* view = unknown_cast<View> (builder->createView (*rootItem)))
			addView (view);
	}
	checkFitSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarView::canSwitchOrientation () const
{
	return getStyle ().isHorizontal () && getStyle ().isVertical ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarView::isVerticalOrientation () const
{
	// layout property of root, if allowed
	if(canSwitchOrientation ())
	{
	#if 1
		// provided by controller
		Variant var;
		UnknownPtr<IObject> controllerObj (getController ());
		return controllerObj && controllerObj->getProperty (var, "verticalOrientation") && var.asBool ();
	#else
		// old approach: property of root item
		if(CommandBarModel* model = getModel ())
		{
			Variant var;
			return model->getRootItem ().getProperty (var, "layout") && var.asString () == "vertical";
		}
	#endif
	}
	return getStyle ().isVertical ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandBarView::setVerticalOrientation (bool state)
{
#if 1
	UnknownPtr<IObject> controllerObj (getController ());
	if(controllerObj)
		controllerObj->setProperty ("verticalOrientation", state);

	makeViews ();
#else
	model->setItemProperty (&model->getRootItem (), "layout", vertical ? "vertical" : String::kEmpty);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarView::canEditItem (const CommandBar::Item& item) const
{
	if(item.isReadOnly ())
		return false;

	CommandBar::PageItem* page = getSelectedPage ();
	return !page || !page->isReadOnly ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandBar::Item* CommandBarView::findItem (View* view) const
{
	do
	{
		if(Form* form = ccl_cast<Form> (view))
			if(!form->getTitle ().isEmpty ())
				if(CommandBar::Item* item = model->findItem (form->getTitle ()))
					return item;

		view = view->getParent ();
	} while(view && view != this);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandBar::Item* CommandBarView::findItem (PointRef where) const
{
	View* view = findView (where, true);
	return view ? findItem (view) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* CommandBarView::findViewForItem (CommandBar::Item& item) const
{
	struct ItemRecognizer: public Recognizer
	{
		String id;

		ItemRecognizer (CommandBar::Item& item)
		: id (item.getID ()) {}

		tbool CCL_API recognize (IUnknown* object) const override
		{
			View* v = unknown_cast<View> (object);
			return v && v->getTitle () == id;
		}
	};
	
	ItemRecognizer r (item);
	return findView (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarView::findInsertContext (CommandBar::InsertContext& context, CommandBar::Item* newItem, CommandBar::Item* mouseItem, PointRef where) const
{
	auto findIndexInParent = [&] (CommandBar::Item& parent, PointRef where)
	{
		// determine index in parent item
		if(parent.countChilds () > 0)
		{
			Point clicked (where);
			clientToWindow (clicked);

			int layout = getContainerLayoutDirection (parent);
			bool isTable = layout == (Styles::kVertical|Styles::kHorizontal);
			bool isVertical = layout == Styles::kVertical;

			int i = 0;
			int lastVisibleIndex = -1;

			ForEach (parent, CommandBar::Item, child)
				if(View* childView = findViewForItem (*child))
				{
					Point childPos;
					childView->clientToWindow (childPos);

					bool found = false;
					if(isTable)
					{
						// for table layout, use full cell rect to insert before that cell
						Rect r (childView->getSize ());
						r.moveTo (childPos);
						found = r.pointInside (clicked);
					}
					else
						found = (isVertical
							? clicked.y < childPos.y + Coord (childView->getHeight () * .6)
							: clicked.x < childPos.x + Coord (childView->getWidth () * .6));

					if(found)
						return i;

					lastVisibleIndex = i;
				}
				i++;
			EndFor

			if(ccl_cast<CommandBar::RootItem> (&parent))
				return lastVisibleIndex; // (must ignore invisible pages)
		}
		return -1;
	};

	if(model && mouseItem)
	{
		context.parent = mouseItem;

		// search upwards for accepting parent
		while(!context.parent->acceptsChild (newItem))
		{
			if((context.parent = model->findParentItem (context.parent)) == nullptr)
			{
				// when no parent item accepted, try again downwards (deep) from mouseItem (find nearest item on each level, ignoring gaps between groups)
				context.parent = mouseItem;
				context.index = -1;
				while(!context.parent->acceptsChild (newItem))
				{
					int childIndex = findIndexInParent (*context.parent, where);
					if(childIndex < 0)
						childIndex = context.parent->countChilds () - 1;

					CommandBar::Item* child = context.parent->getChild (childIndex);
					if(child)
						context.parent = child;
					else
					{
						context.parent = mouseItem;
						break;
					}
				}
				break;
			}
		}

		// determine index in parent item
		if(context.parent && context.index == -1)
		{
			if(ccl_cast<CommandBar::MenuGroupItem> (context.parent))
				context.index = context.parent->getChildIndex (mouseItem);
			else
				context.index = findIndexInParent (*context.parent, where);
		}

		CCL_PRINTF ("findInsertContext: %s, %d\n", MutableCString (context.parent->getTitle ()).str (), context.index)

		// let model adjust according to nesting rules
		return model->adjustInsertContext (newItem, context);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CommandBarView::getContainerLayoutDirection (CommandBar::Item& item) const
{
	if(CommandBar::Item* child = item.getChild (0))
	{
		// search upwards for a parent with horizontal or vertical style
		if(View* view = findViewForItem (*child))
			while(view = view->getParent ())
			{
				// special detection for table
				Variant layout;
				static_cast<IObject*> (view)->getProperty (layout, ATTR_LAYOUTCLASS);
				MutableCString layoutClass (layout.asString ());
				if(layoutClass == LAYOUTCLASS_TABLE)
					return Styles::kHorizontal|Styles::kVertical;

				int flags = view->getStyle ().common & (Styles::kHorizontal|Styles::kVertical);
				if(flags)
					return flags;
			}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String CommandBarView::getDefaultTitle (CommandBar::ButtonItem& item)
{
	if(item.isExternalTarget ())
	{
		TargetList targets (*this);
		if(CommandBar::CommandTarget* target = targets.findTarget (String (item.getCommandName ())))
		{
			String title (target->getTitle ());
			int slashIndex = title.index ("/");
			if(slashIndex > 0)
				title.remove (0, slashIndex + 1);
			return title;
		}
	}
	else
	{
		if(!item.getCommandName ().isEmpty ())
			if(ICommand* command = CommandTable::instance ().findCommand (item.getCommandCategory (), item.getCommandName ()))
			{
				CommandDescription description;
				command->getDescription (description);
				return description.displayName;
			}
	}
	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandBarView::resizeIcon (AutoPtr<IImage>& image)
{
	if(image)
	{
		const IVisualStyle& vs = getVisualStyle ();
		Point maxIconSize (vs.getMetric<int> ("maxIconWidth", 22), vs.getMetric<int> ("maxIconHeight", 22));
		bool fit = image->getWidth () <= maxIconSize.x && image->getHeight () <= maxIconSize.y;
		if(!fit)
		{
			Rect srcRect (0, 0, image->getWidth (), image->getHeight ());
			Rect maxRect (0, 0, maxIconSize.x, maxIconSize.y);
			Rect dstRect (srcRect);
			dstRect.fitProportionally (maxRect);

			AutoPtr<Bitmap> bitmap = NEW Bitmap (dstRect.getWidth (), dstRect.getHeight (), Bitmap::kRGBAlpha);
			{
				AutoPtr<IGraphics> graphics = NEW BitmapGraphicsDevice (bitmap);
				graphics->drawImage (image, srcRect, dstRect);
			}
			AutoPtr<Bitmap> bitmap2x = NEW Bitmap (dstRect.getWidth (), dstRect.getHeight (), Bitmap::kRGBAlpha, 2.f);
			{
				AutoPtr<IGraphics> graphics = NEW BitmapGraphicsDevice (bitmap2x);
				graphics->drawImage (image, srcRect, dstRect);
			}
			image = NEW MultiResolutionBitmap (bitmap->getNativeBitmap (), bitmap2x->getNativeBitmap ());
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandBarView::attached (View* parent)
{
	SuperClass::attached (parent);

	// resize icons in existing buttons, now that we can access our visual style that contains the limits
	struct IconResizer: public CommandBar::ItemTraverser,
						public Recognizer
	{
		IconResizer (CommandBarView& view)
		: commandBarView (view) {}

		bool visit (CommandBar::Item* item) override
		{
			CommandBar::ButtonItem* buttonItem = ccl_cast<CommandBar::ButtonItem> (item);
			if(buttonItem && buttonItem->getIcon ())
				if(View* itemForm = commandBarView.findViewForItem (*item))
					if(Button* button = ccl_cast<Button> (itemForm->findView (*this))) // -> recognize
					{
						if(button->getIcon ())
						{
							AutoPtr<IImage> icon;
							icon.share (buttonItem->getIcon ());
		
							commandBarView.resizeIcon (icon);

							//float scale = commandBarView.getWindow ()->getContentScaleFactor ();
							button->setIcon (icon);
						}
					}

			return true;
		}

		tbool CCL_API recognize (IUnknown* object) const override
		{
			return unknown_cast<Button> (object) != nullptr;
		}

	private:
		CommandBarView& commandBarView;
	};

	if(model)
		IconResizer (*this).traverse (&model->getRootItem ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarView::runCommandSelector (CommandBar::ButtonItem& item)
{
	AutoPtr<CommandSelector> selector = NEW CommandSelector ();

	CommandDescription description;
	description.category = item.getCommandCategory ();
	description.name = item.getCommandName ();

	if(selector->run (description) == kResultOk)
	{
		// keep a title edited by user, only override if the old title was empty or the default target title
		if(item.getTitle ().isEmpty () || item.getTitle () == getDefaultTitle (item))
			model->setItemProperty (&item, "title", description.displayName);

		model->setItemProperty (&item, "commandCategory", String (description.category));
		model->setItemProperty (&item, "commandName", String (description.name));
		model->setItemProperty (&item, "type", CommandBar::ButtonItem::kButton);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarView::onSetOrientation (CmdArgs args, VariantRef data)
{
	UnknownPtr<IMenuItem> menuItem (args.invoker);
	if(menuItem)
	{
		if(args.checkOnly ())
		{
			menuItem->setItemAttribute (IMenuItem::kItemChecked, isVerticalOrientation ());
		}
		else if(model)
		{
			Variant var;
			bool vertical = menuItem->getItemAttribute (var, IMenuItem::kItemChecked) && var.asBool () == false;
			setVerticalOrientation (vertical);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarView::onAddItem (CmdArgs args, VariantRef data)
{
	if(!args.checkOnly () && model)
	{
		AddItemContext* clickContext = static_cast<AddItemContext*> (data.asUnknown ());
		CommandBar::Item* parentItem = clickContext ? clickContext->parentItem : nullptr;
		if(parentItem)
		{
			CommandBar::InsertContext insertContext;
			AutoPtr<CommandBar::Item> newItem;

			bool isButton = args.name.contains ("Button");
			bool isMenuButton = args.name == ("New Menu Button");
			bool isPage = args.name.contains ("Page");

			if(!isMenuButton && args.name.startsWith ("New Menu"))
			{
				bool isSubMenu = args.name == "New Menu";
				if(isSubMenu)
				{
					newItem = NEW CommandBar::MenuGroupItem;
					newItem->setTitle (XSTR (InitialMenuTitle));
				}
				else if(args.name == "New Menu Separator")
					newItem = NEW CommandBar::MenuSeparatorItem;
				else
					newItem = NEW CommandBar::MenuItem;
			}
			else if(isButton)
			{
				CommandBar::ButtonItem* button = NEW CommandBar::ButtonItem;;
				newItem = button;
				if(isMenuButton)
				{
					AutoPtr<CommandBar::MenuGroupItem> menuContent (NEW CommandBar::MenuGroupItem);

					CommandBar::Item* dummyItem = NEW CommandBar::MenuItem;
					dummyItem->setID (UIDString::generate ());
					menuContent->addChild (dummyItem); // one initial unassigned menu item

					button->setControlType (CommandBar::ButtonItem::kMenu);
					button->setMenuContent (menuContent);
				}
			}
			else if(isPage)
			{
				newItem = NEW CommandBar::PageItem;
				String title = XSTR (InitialPageTitle);
				int childrenCount = model->getRootItem ().countChilds ();
				newItem->setTitle (title.appendASCII (" ").appendIntValue (childrenCount));
				insertContext.index = childrenCount;
				insertContext.parent = &model->getRootItem ();
				model->addItem (newItem.detach (), insertContext);

				if(CommandBar::PageItem* newPage = ccl_cast<CommandBar::PageItem> (model->getRootItem ().getChild (insertContext.index)))
					selectPage (newPage);
			}
			else
			{
				newItem = NEW CommandBar::GroupItem;
				newItem->setTitle (XSTR (InitialGroupTitle));
			}

			if(!isPage)
			{
				if(findInsertContext (insertContext, newItem, parentItem, clickContext->where))
					model->addItem (newItem.detach (), insertContext);
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarView::onRemoveItem (CmdArgs args, VariantRef data)
{
	if(!args.checkOnly () && model)
	{
		if(CommandBar::Item* item = unknown_cast<CommandBar::Item> (data.asUnknown ()))
		{
			String title (item->getTitle ());
			if(title.isEmpty ())
			{
				if(ccl_cast<CommandBar::MenuItem> (item))
					title = XSTR (InitialMenuItemTitle);
				else if(ccl_cast<CommandBar::MenuSeparatorItem> (item))
					title = XSTR (Separator);
				else
					title = XSTR (Unnamed);
			}

			if(Alert::ask (String ().appendFormat (XSTR (SafetyQuestion), title)) == Alert::kYes)
			{
				CommandBar::PageItem* oldPage = ccl_cast<CommandBar::PageItem> (item);
				int pageCount = model->countPages ();
				if(oldPage && pageCount > 1) // if the item is a page item and there is at least one other page
				{
					int pageindex = 0;
					CommandBar::Item* firstPage = model->getPage (pageindex);

					if(firstPage == oldPage)
						firstPage = model->getPage (++pageindex);

					if(!firstPage)
						return false;

					CommandBar::PageItem* newPage = ccl_cast<CommandBar::PageItem> (firstPage);
					if(!newPage)
						return false;

					selectPage (newPage);
				}
				model->removeItem (item);
				if(oldPage && pageCount == 1)
				{
					CommandBar::InsertContext insertContext;
					AutoPtr<CommandBar::Item> newItem = NEW CommandBar::PageItem;
					String title = XSTR (InitialPageTitle);
					newItem->setTitle (title.appendASCII (" 1"));
					insertContext.index = 1; // behind setup group
					CommandBar::Item* root = ccl_cast<CommandBar::Item> (&model->getRootItem ());
					if(findInsertContext (insertContext, newItem, root, Point(0, 0)))
						model->addItem (newItem.detach (), insertContext);

					selectPage (0);
				}
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarView::onMoveToGroup (CmdArgs args, VariantRef data)
{
	if(!args.checkOnly () && model)
	{
		String targetID (args.name);
		CommandBar::Item* moveItem = unknown_cast<CommandBar::Item> (data.asUnknown ());
		CommandBar::Item* targetItem = model->findItem (targetID);

		if(moveItem && targetItem)
		{
			moveItem->retain ();
			model->removeItem (moveItem);
			model->addItem (moveItem, targetItem);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarView::onMoveGroupToPage (CmdArgs args, VariantRef data)
{
	if(!args.checkOnly () && model)
	{
		if(CommandBar::Item* item = unknown_cast<CommandBar::Item> (data.asUnknown ()))
		{
			int pageIndex = getPageIndexFromArgs (args);
			if(pageIndex >= 0)
			{
				CommandBar::Item* itemCopy = ccl_cast<CommandBar::Item> (item->clone ());
				CommandBar::PageItem* parentItem = ccl_cast<CommandBar::PageItem> (model->findParentItem (item));
				CommandBar::PageItem* targetPage = ccl_cast<CommandBar::PageItem> (model->getPage (pageIndex));
				if(!parentItem || !targetPage)
					return false;

				model->removeItem (item);
				targetPage->addChild (itemCopy);
				#if 0 // (was considered a bug)
				selectPage (pageIndex);
				#endif
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarView::onSelectPage (CmdArgs args, VariantRef data)
{
	if(!args.checkOnly () && model)
	{
		int pageIndex = getPageIndexFromArgs (args);
		if(pageIndex >= 0)
			selectPage (pageIndex);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarView::onSelectImage (CmdArgs args, VariantRef data)
{
	if(!args.checkOnly () && model)
	{
		if(CommandBar::ButtonItem* item = unknown_cast<CommandBar::ButtonItem> (data.asUnknown ()))
		{
			AutoPtr<IFileSelector> selector = NativeFileSelector::create ();

			ForEach (Image::getHandlerList (), ImageHandler, handler)
				for(int i = 0; i < handler->getNumFileTypes (); i++)
					if(const FileType* format = handler->getFileType (i))
						selector->addFilter (*format);
			EndFor

			if(selector->run (IFileSelector::kOpenFile))
			{
				AutoPtr<IImage> image = Image::loadImage (*selector->getPath ());
				resizeIcon (image);
				model->setItemProperty (item, "icon", Variant (image, true));
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarView::onRemoveImage (CmdArgs args, VariantRef data)
{
	CommandBar::ButtonItem* item = unknown_cast<CommandBar::ButtonItem> (data.asUnknown ());
	if(item && item->getIcon ())
	{
		if(!args.checkOnly () && model)
			model->setItemProperty (item, "icon", 0);

		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarView::onAssignCommand (CmdArgs args, VariantRef data)
{
	if(!args.checkOnly () && model)
	{
		if(CommandBar::ButtonItem* item = unknown_cast<CommandBar::ButtonItem> (data.asUnknown ()))
			runCommandSelector (*item);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarView::onAssignTarget (CmdArgs args, VariantRef data)
{
	if(!args.checkOnly () && model)
	{
		UnknownPtr<IController> controller (this->controller);
		CommandBar::ButtonItem* item = unknown_cast<CommandBar::ButtonItem> (data.asUnknown ());
		if(controller && item)
		{
			if(IParameter* param = controller->findParameter (args.name))
			{
				int controlType = CommandBar::ButtonItem::kButton;
				if(UnknownPtr<IMenuExtension> (param).isValid ())
					controlType = CommandBar::ButtonItem::kMenu;

				// keep a title edited by user, only override if the old title was empty or the default target title
				if(item->getTitle ().isEmpty () || item->getTitle () == getDefaultTitle (*item))
					if(MenuItem* menuItem = unknown_cast<MenuItem> (args.invoker))
						model->setItemProperty (item, "title", menuItem->getTitle ());

				MutableCString commandCategory;
				UnknownPtr<ICommandParameter> commandParam (param);
				if(commandParam)
					commandCategory = commandParam->getCommandCategory ();

				model->setItemProperty (item, "commandCategory", String (commandCategory));
				model->setItemProperty (item, "commandName", String (args.name));
				model->setItemProperty (item, "type", controlType);
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandBarView::appendAssignMenu (IContextMenu& contextMenu, CommandBar::Item* item)
{
	UnknownPtr<IMenu> popupMenu (&contextMenu);
	if(popupMenu)
	{
		TargetList targets (*this);
		if(!targets.isEmpty ())
		{
			IMenu* targetsMenu = popupMenu->createMenu ();
			targetsMenu->setMenuAttribute (IMenu::kMenuTitle, XSTR (Assign));

			targetsMenu->addCommandItem (CommandWithTitle ("Command", "Assign", XSTR (Command)), CommandDelegate<CommandBarView>::make (this, &CommandBarView::onAssignCommand, ccl_as_unknown (item)), true);
			targetsMenu->addSeparatorItem ();

			UnknownPtr<IObserver> ctrler (controller);
			if(ctrler)
			{
				Message msg (kExtendAssignMenu, targetsMenu, item->asUnknown ());
				ctrler->notify (this, msg);
			}

			struct SubMenuEntry
			{
				SharedPtr<IMenu> subMenu;
				String menuPath;
			};
			Vector<SubMenuEntry> subMenus;

			ForEach (targets, CommandBar::CommandTarget, target)
				if(!target->getName ().isEmpty ())
				{
					IMenu* menu = targetsMenu;
					IMenu* parentMenu = targetsMenu;
					String title (target->getTitle ());
					String menuPath;
					String category (target->getCategory ());
					ForEachStringToken (category, "/", token)
						if(token.isEmpty ())
							continue;
						menuPath << "/" << token;
						
						// check if submenu already created
						menu = nullptr;
						SubMenuEntry* menuEntry = subMenus.findIf ([&] (const SubMenuEntry& entry) { return entry.menuPath == menuPath; });
						if(menuEntry)
							menu = parentMenu = menuEntry->subMenu;
							
						if(!menu)
						{
							IMenu* subMenu = popupMenu->createMenu ();
							subMenu->setMenuAttribute (IMenu::kMenuTitle, token);
							parentMenu->addMenu (subMenu);
							subMenus.add ({subMenu, menuPath});
							menu = parentMenu = subMenu;
						}
					EndFor

					IMenuItem* menuItem = menu->addCommandItem (title, "Command", MutableCString (target->getName ()), CommandDelegate<CommandBarView>::make (this, &CommandBarView::onAssignTarget, ccl_as_unknown (item)));
					if(menuItem && target->getIcon ())
						menuItem->setItemAttribute (IMenuItem::kItemIcon, Variant (target->getIcon ()));

					CommandBar::ButtonItem* buttonItem = ccl_cast<CommandBar::ButtonItem> (item);
					if(menuItem && buttonItem && buttonItem->getCommandName () == MutableCString (target->getName ()))
						menuItem->setItemAttribute (IMenuItem::kItemChecked, true);
				}
			EndFor

			popupMenu->addMenu (targetsMenu);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandBarView::appendIconMenu (IContextMenu& contextMenu, CommandBar::Item* item)
{
	UnknownPtr<IMenu> popupMenu (&contextMenu);
	if(popupMenu)
	{
		IMenu* iconMenu = popupMenu->createMenu ();
		iconMenu->setMenuAttribute (IMenu::kMenuTitle, XSTR (Icon));
		iconMenu->addCommandItem (String () << XSTR (SelectImage) << IMenu::strFollowIndicator, "Command", "Select Image", CommandDelegate<CommandBarView>::make (this, &CommandBarView::onSelectImage, ccl_as_unknown (item)));
		iconMenu->addCommandItem (XSTR (RemoveImage), "Command", "Remove Image", CommandDelegate<CommandBarView>::make (this, &CommandBarView::onRemoveImage, ccl_as_unknown (item)));
		popupMenu->addMenu (iconMenu);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarView::onMouseDown (const MouseEvent& event)
{
	if(event.keys.isSet (KeyState::kLButton))
	{
		if(event.keys.getModifiers () == 0 && wantsContextMenu ())
		{
			// find item, but ignore disabled views
			CommandBar::Item* item = nullptr;
			ObjectList allViews;
			findAllViews (allViews, event.where, true);
			ListForEachObjectReverse (allViews, View, view)
				if(view->isEnabled ())
					if(item = findItem (view))
						break;
			EndFor

			bool openContextMenu = item ? item->isLeftClickContextMenu () : false;
			if(!openContextMenu)
				if(ccl_strict_cast<CommandBar::GroupItem> (item) || ccl_strict_cast<CommandBar::SetupGroupItem> (item))
					openContextMenu = true;
			
			if(openContextMenu)
			{
				// if a popup (e.g. context menu) is still open, close it first and defer the mouseDown handling (prevent 2 popups at the same time)
				AutoPtr<ICommandHandler> mouseDownHandler = makeCommandDelegate ([&, event] (const CommandMsg& msg, VariantRef data)
				{
					if(!msg.checkOnly ())
					{
						getWindow ()->setMouseHandler (nullptr); // mousehandler would prevent opening popup
						onMouseDown (event);
					}
					return true;
				}, nullptr).detach ();

				if(Desktop.closePopupAndDeferCommand (mouseDownHandler, CommandMsg ()))
					return true;

				if(!hasContextMenuPopup && !Desktop.isPopupActive ())
				{
					if(View* view = findViewForItem (*item))
					{
						if(IWindow* window = getWindow ())
						{
							// open at left bottom of visible group rect (group might be clipped)
							Rect visibleRect;
							view->getVisibleClient (visibleRect);
							Coord visibleLeft = visibleRect.left;
							visibleRect.offset (view->getSize ().getLeftTop ());

							Point p (visibleLeft, visibleRect.getHeight () - 1);
							view->clientToWindow (p);
							Point viewOffset (getSize ().getLeftTop ());
							clientToWindow (viewOffset);
							if(CommandBar::Item* alignedItem = findItem (p - viewOffset))
							{
								if(view == findViewForItem (*alignedItem))
								{
									ScopedVar<bool> guard (hasContextMenuPopup, true);
									window->popupContextMenu (p);
									return true;
								}
							}
						}
					}
				}
			}
		}
		else if(event.keys.isSet (KeyState::kCommand))
		{
			CommandBar::Item* item = findItem (event.where);
			if(item && item != &model->getRootItem () && canEditItem (*item))
			{
				View* mouseView = findView (event.where, true);
				int mouseState;
				if(mouseView)
				{
					mouseState = getMouseState ();
					mouseView->setMouseState (View::kMouseDown);
					mouseView->redraw ();
				}
				
				if(detectDrag (event))
				{
					if(mouseView)
						mouseView->setMouseState (mouseState);
					
					SharedPtr<IUnknown> holder (this->asUnknown ()); // view might get removed during drag & drop
					
					AutoPtr<DragSession> session (DragSession::create (this->asUnknown ()));
					session->setSource (this->asUnknown ());
					session->getItems ().add (item->asUnknown (), true);
					session->drag ();
					return true;
				}
			}
		}
	}
	return SuperClass::onMouseDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* CommandBarView::createDragHandler (const DragEvent& event)
{
	if(event.session.getSource () == this->asUnknown ())
	{
		auto item = unknown_cast<CommandBar::Item> (event.session.getItems ().getFirst ());
		if(item && canEditItem (*item))
		{
			if(event.session.getResult () == DragSession::kDropNone)
				event.session.setResult (DragSession::kDropMove);

			return NEW MoveItemDragHandler (this, item);
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CommandBarView::getSelectedPageIndex () const
{
	// provided by controller
	Variant var;
	UnknownPtr<IObject> controllerObj (getController ());
	return controllerObj && model && controllerObj->getProperty (var, "selectedPageIndex") ? ccl_bound (var.asInt (), 0, model->countPages () - 1) : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandBar::PageItem* CommandBarView::getSelectedPage () const
{
	return model ? model->getPage (getSelectedPageIndex ()) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandBarView::selectPage (int index)
{
	UnknownPtr<IObject> controllerObj (getController ());
	if(controllerObj)
		controllerObj->setProperty ("selectedPageIndex", index);

	makeViews ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandBarView::selectPage (CommandBar::PageItem* page)
{
	int index = model->getPageIndex (page);
	ccl_lower_limit (index, 0);
	selectPage (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandBarView::buildPagesSubMenu (UnknownPtr<IMenu> popupMenu, ICommandHandler* handler, String title)
{
	if(popupMenu)
	{
		AutoPtr<IMenu> pagesMenu (popupMenu->createMenu ());
		if(buildPagesMenu (*pagesMenu, handler, true))
		{
			pagesMenu->setMenuAttribute (IMenu::kMenuTitle, title);
			popupMenu->addMenu (pagesMenu);
			pagesMenu->retain ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarView::buildPagesMenu (IMenu& menu, ICommandHandler* handler, bool ignoreSelected)
{
	bool result = false;
	int pageCount = model->countPages ();
	if(pageCount > 1)
	{
		CommandBar::PageItem* selectedPage = getSelectedPage ();
		CommandBar::PageItem* ignorePage = ignoreSelected ? selectedPage : nullptr;

		for(int i = 0; i < pageCount; i++)
		{
			CommandBar::PageItem* page = model->getPage (i);
			if(page && page != ignorePage)
			{
				MutableCString args ("Page[");
				args.appendInteger (i).append ("]");
				IMenuItem* item  = menu.addCommandItem (page->getTitle (), "Command", args, handler);
				if(page == selectedPage)
					item->setItemAttribute (IMenuItem::kItemChecked, true);

				result = true;
			}
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CommandBarView::getPageIndexFromArgs (CmdArgs args)
{
	String argName (args.name);
	int prefixLength = argName.index ("[") + 1;
	int closeIndex = argName.index ("]");
	if(closeIndex > 0)
	{
		Core::int64 pageIndex;
		argName.subString (prefixLength, closeIndex - prefixLength).getIntValue (pageIndex);
		return static_cast<int> (pageIndex);
	}

	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandBarView::buildMoveToGroupMenu (IMenu& menu, CommandBar::Item* item)
{
	IMenu* moveMenu = menu.createMenu ();
	moveMenu->setMenuAttribute (IMenu::kMenuTitle, XSTR (MoveTo));
	menu.addMenu (moveMenu);

	CommandBar::Item* currentParent = model->findParentItem (item);

	int pageCount = model->countPages ();
	for(int i = 0; i < pageCount; i++)
	{
		if(CommandBar::PageItem* page = model->getPage (i))
		{
			AutoPtr<IMenu> pageSubMenu (moveMenu->createMenu ());
			pageSubMenu->setMenuAttribute (IMenu::kMenuTitle, page->getTitle ());

			AutoPtr<Iterator> groupsIter (page->newIterator ());
			if(groupsIter)
				for(auto g : *groupsIter)
				{
					CommandBar::GroupItem* group = ccl_cast<CommandBar::GroupItem> (g);
					ASSERT (group)
					if(group != currentParent)
						pageSubMenu->addCommandItem (group->getTitle (), "Move To Group", MutableCString (group->getID ()), CommandDelegate<CommandBarView>::make (this, &CommandBarView::onMoveToGroup, item->asUnknown ()));
				}

			if(pageSubMenu->countItems () > 0)
				moveMenu->addMenu (pageSubMenu.detach ());
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarView::wantsContextMenu () const
{
	return getVisualStyle ().getMetric<bool> ("contextMenu", true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarView::onContextMenu (const ContextMenuEvent& event)
{
	CommandBar::Item* item = findItem (event.where);
	if(item)
	{
		bool canEdit = canEditItem (*item);
		event.contextMenu.setFocusItem (item->asUnknown ());

		if(!wantsContextMenu ())
			return false;

		CommandBar::SetupGroupItem* mainGroup = ccl_cast<CommandBar::SetupGroupItem> (model->getRootItem ().getChild (0));

		CommandBar::ButtonItem* button = ccl_cast<CommandBar::ButtonItem> (item);
		bool isButton = button != nullptr;
		bool isPage = ccl_cast<CommandBar::PageItem> (item) != nullptr;
		bool isGroup = ccl_cast<CommandBar::GroupItem> (item) != nullptr;
		bool isRoot = item == &model->getRootItem ();
		bool isMainGroup = item == mainGroup;
		bool isPartOfMainGroup = model->findParentItem (item) == mainGroup;

		bool pageMenu = isPage || isMainGroup || isPartOfMainGroup;

		CommandBar::PageItem* selectedPage = getSelectedPage ();

		if(canEdit)
		{
			UnknownPtr<IExtendedMenu> extendedMenu (&event.contextMenu);
			if(extendedMenu && (pageMenu || isGroup || isButton) && !isRoot)
			{
				AutoPtr<Builder> builder (NEW Builder (this));
				builder->setItemFormName (getContextMenuFormName ());
			
				AutoPtr<IView> view;
				if(selectedPage && pageMenu)
					view = builder->createView (*selectedPage);
				else
					view = builder->createView (*item);

				if(view)
					extendedMenu->addViewItem (view);
			}

			if(isButton && !isPartOfMainGroup)
			{
				if(CommandBar::MenuGroupItem* menuContent = ccl_cast<CommandBar::MenuGroupItem> (button->getMenuContent ()))
				{
					event.contextMenu.addCommandItem (String () << XSTR (EditMenu) << Menu::strFollowIndicator, "Command", "Edit Menu",
						makeCommandDelegate ([=] (const CommandMsg& msg, VariantRef data)
						{
							if(!msg.checkOnly ())
							{
								// popup another context menu at the same location that embeds the menu editor
								AutoPtr<Builder> builder (NEW Builder (this));
								builder->setItemFormName (getContextMenuFormName ());

								AutoPtr<MenuEditor> editor (NEW MenuEditor (this, menuContent));

								AutoPtr<ContextMenu> contextMenu = NEW ContextPopupMenu;
								UnknownPtr<IExtendedMenu> extendedMenu (contextMenu->asUnknown ());
								if(extendedMenu)
								{
									if(AutoPtr<IView> view = builder->createView (*item))
										extendedMenu->addViewItem (view);

									if(AutoPtr<View> view = editor->createView ())
										extendedMenu->addViewItem (view);
								}
								contextMenu->popup (event.where, this);
							}
							return true;
						}, button->asUnknown ()));
				}
				else
				{
					appendAssignMenu (event.contextMenu, item);
					appendIconMenu (event.contextMenu, item);
				}
				event.contextMenu.addSeparatorItem ();
			}
		}

		if(!isButton && canSwitchOrientation ())
		{
			event.contextMenu.addSeparatorItem ();
			event.contextMenu.addCommandItem (XSTR (Vertical), "Command", "Orientation", CommandDelegate<CommandBarView>::make (this, &CommandBarView::onSetOrientation, 0));
			event.contextMenu.addSeparatorItem ();
		}

		if(canEdit)
		{
			AutoPtr<AddItemContext> addItemContext (NEW AddItemContext (item, event.where));
			if(pageMenu)
			{
				this->buildPagesSubMenu (&event.contextMenu, CommandDelegate<CommandBarView>::make (this, &CommandBarView::onSelectPage, 0), XSTR (SelectPage));
				event.contextMenu.addSeparatorItem ();
				if(selectedPage)
				{
					AutoPtr<AddItemContext> pageAddItemContext (NEW AddItemContext (selectedPage, event.where));
					event.contextMenu.addCommandItem (XSTR (NewGroup), "Command", "New Group", CommandDelegate<CommandBarView>::make (this, &CommandBarView::onAddItem, pageAddItemContext));
					event.contextMenu.addSeparatorItem ();
				}
				event.contextMenu.addCommandItem (XSTR (NewPage), "Command", "New Page", CommandDelegate<CommandBarView>::make (this, &CommandBarView::onAddItem, addItemContext));
				if(selectedPage)
					event.contextMenu.addCommandItem (XSTR (RemovePage), "Command", "Remove", CommandDelegate<CommandBarView>::make (this, &CommandBarView::onRemoveItem, selectedPage->asUnknown ()));
			}
			else if(isRoot && selectedPage)
			{
				AutoPtr<AddItemContext> pageAddItemContext (NEW AddItemContext (selectedPage, event.where));
				event.contextMenu.addCommandItem (XSTR (NewGroup), "Command", "New Group", CommandDelegate<CommandBarView>::make (this, &CommandBarView::onAddItem, pageAddItemContext));
			}
			else if(isGroup)
			{
				buildPagesSubMenu (&event.contextMenu, CommandDelegate<CommandBarView>::make (this, &CommandBarView::onMoveGroupToPage, item->asUnknown ()), XSTR (MoveTo));
				event.contextMenu.addSeparatorItem ();
				event.contextMenu.addCommandItem (XSTR (NewButton), "Command", "New Button", CommandDelegate<CommandBarView>::make (this, &CommandBarView::onAddItem, addItemContext));
				event.contextMenu.addCommandItem (XSTR (NewMenuButton), "Command", "New Menu Button", CommandDelegate<CommandBarView>::make (this, &CommandBarView::onAddItem, addItemContext));
				event.contextMenu.addCommandItem (XSTR (NewGroup), "Command", "New Group", CommandDelegate<CommandBarView>::make (this, &CommandBarView::onAddItem, addItemContext));
				event.contextMenu.addSeparatorItem ();
				event.contextMenu.addCommandItem (XSTR (RemoveGroup), "Command", "Remove", CommandDelegate<CommandBarView>::make (this, &CommandBarView::onRemoveItem, item->asUnknown ()));
			}
			else if(isButton)
			{
				UnknownPtr<IObserver> ctrler (controller);
				if(ctrler)
				{
					Message msg (kExtendButtonMenu, &event.contextMenu, item->asUnknown ());
					ctrler->notify (this, msg);
				}

				UnknownPtr<IMenu> menu (&event.contextMenu);
				buildMoveToGroupMenu (*menu, item);
				event.contextMenu.addSeparatorItem ();

				event.contextMenu.addCommandItem (XSTR (NewButton), "Command", "New Button", CommandDelegate<CommandBarView>::make (this, &CommandBarView::onAddItem, addItemContext));
				event.contextMenu.addCommandItem (XSTR (NewMenuButton), "Command", "New Menu Button", CommandDelegate<CommandBarView>::make (this, &CommandBarView::onAddItem, addItemContext));
				event.contextMenu.addSeparatorItem ();
				event.contextMenu.addCommandItem (XSTR (RemoveButton), "Command", "Remove", CommandDelegate<CommandBarView>::make (this, &CommandBarView::onRemoveItem, item->asUnknown ()));
			}
		}

		if(contextMenuHandler && !isRoot)
		{
			if(unknown_cast<CommandBar::Item> (event.contextMenu.getFocusItem ()))
				return contextMenuHandler->appendContextMenu (event.contextMenu) == kResultOk;
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandBarView::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged && subject == model)
	{
		makeViews ();

		// pass on to our controller
		UnknownPtr<IObserver> ctrler (controller);
		if(ctrler)
			ctrler->notify (subject, msg);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarView::setProperty (MemberID propertyId, const Variant& var)
{
	auto setScaleFactorProperty = [&] (float& scaleFactor)
	{
		float factor = var.asFloat ();
		if(factor != scaleFactor)
		{
			scaleFactor = factor;
			makeViews ();
		}
	};

	if(propertyId == "scaleX")
	{
		setScaleFactorProperty (scaleFactorX);
		return true;
	}
	if(propertyId == "scaleY")
	{
		setScaleFactorProperty (scaleFactorY);
		return true;
	}
	return Object::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (CommandBarView)
	DEFINE_METHOD_NAME ("dragItem")
END_METHOD_NAMES (CommandBarView)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandBarView::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "dragItem")
	{
		if(msg.getArgCount () > 0 && model)
		{
			// arg[0] might be a view (inside the item form) that triggered this call
			View* view = unknown_cast<View> (msg[0]);
			CommandBar::Item* item = view ? findItem (view) : nullptr;
			if(item && item != &model->getRootItem ())
			{
				SharedPtr<IUnknown> holder (asUnknown ()); // view might get removed during drag & drop

				int inputDevice = IDragSession::kTouchInput;
				if(Window* window = getWindow ())
					inputDevice = window->getTouchInputState ().isInGestureEvent () ? IDragSession::kTouchInput : IDragSession::kMouseInput;

				AutoPtr<DragSession> session (DragSession::create (asUnknown ()));
				session->setInputDevice (inputDevice);
				session->setSource (asUnknown ());
				session->getItems ().add (item->asUnknown (), true);
				Promise (session->dragAsync ());
				return true;
			}
		}
		return true;
	}
	
	return SuperClass::invokeMethod (returnValue, msg);
}
