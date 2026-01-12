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
// Filename    : viewtree.cpp
// Description : 
//
//************************************************************************************************

#include "viewtree.h"
#include "viewclass.h"
#include "viewproperty.h"

#include "ccl/base/message.h"

#include "ccl/app/utilities/imagefile.h"
#include "ccl/app/utilities/imagebuilder.h"

#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/ifileselector.h"
#include "ccl/public/gui/framework/controlproperties.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/ibitmap.h"
#include "ccl/public/gui/icontextmenu.h"

#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/cclversion.h"

using namespace CCL;
using namespace Spy;

//************************************************************************************************
// ViewItem
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ViewItem, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewItem::ViewItem (ViewTreeBrowser& browser, IView* view)
: browser (browser),
  view (view),
  viewSubject (nullptr)
{
	if(view)
	{
		viewSubject = UnknownPtr<ISubject> (view);
		if(viewSubject)
			viewSubject->addObserver (this);

		sprite.share (browser.getSprite (view));

		// build info string
		info << getViewClass ().getClassName ();

		String name = ViewBox (view).getName ();
		if(!name.isEmpty ())
			info << " name=\"" << name << " \"";
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewItem::~ViewItem ()
{
	if(viewSubject)
		viewSubject->removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewClass& ViewItem::getViewClass ()
{
	return ViewClassRegistry::instance ().getClass (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewItem::toggleSprite ()
{
	if(sprite)
	{
		browser.hideSprite (view);
		sprite.release ();
	}
	else
		sprite.share (browser.showSprite (view));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ViewItem::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kDestroyed)
		browser.onViewDestroyed (*this);
}

//************************************************************************************************
// ViewTreeItemModel
//************************************************************************************************

ViewTreeItemModel::ViewTreeItemModel (ViewTreeBrowser& browser)
: browser (browser)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ViewTreeItemModel::createColumnHeaders (IColumnHeaderList& list)
{
	if(browser.getColumns ())
	{
		list.copyFrom (*browser.getColumns ());
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* ViewTreeItemModel::getView (ItemIndexRef index)
{
	if(ViewItem* viewItem = unknown_cast<ViewItem> (index.getObject ()))
		return viewItem->getView ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ViewTreeItemModel::getRootItem (ItemIndex& index)
{
	if(ViewItem* rootItem = browser.getRootItem ())
	{
		index = rootItem->asUnknown ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ViewTreeItemModel::getSubItems (IUnknownList& items, ItemIndexRef index) 
{ 
	if(IView* view = getView (index))
	{
		AutoPtr<IViewIterator> iter = view->getChildren ().createIterator ();
		if(iter) while(!iter->done ())
			if(IView* childView = iter->next ())
				items.add (ccl_as_unknown (NEW ViewItem (browser, childView)));

		return true;
	}
	return false; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ViewTreeItemModel::canExpandItem (ItemIndexRef index) 
{
	if(ViewItem* viewItem = unknown_cast<ViewItem> (index.getObject ()))
		if(IView* view = viewItem->getView ())
			return !view->getChildren ().isEmpty ();
	return false; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ViewTreeItemModel::getItemTitle (String& title, CCL::ItemIndexRef index)
{
	if(ViewItem* viewItem = unknown_cast<ViewItem> (index.getObject ()))
		title = viewItem->getInfo ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ViewTreeItemModel::getUniqueItemName (MutableCString& name, ItemIndexRef index)
{
	if(ViewItem* viewItem = unknown_cast<ViewItem> (index.getObject ()))
	{
		name.appendFormat ("%x", (void*)viewItem->getView ());
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API ViewTreeItemModel::getItemIcon (ItemIndexRef index) 
{ 
	if(ViewItem* viewItem = unknown_cast<ViewItem> (index.getObject ()))
		return viewItem->getViewClass ().getIcon ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ViewTreeItemModel::drawIconOverlay (ItemIndexRef index, const DrawInfo& info)
{
#if 0
	if(ViewItem* viewItem = unknown_cast<ViewItem> (index.getObject ()))
		if(IView* view = viewItem->getView ())
			if(ViewBox (view).isLayerBackingEnabled ())
			{
				Color color (Colors::kBlack);
				color.setAlphaF (.5f);
				info.graphics.fillRect (info.rect, SolidBrush (color));
			}
	return true;
#else
	return false;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ViewTreeItemModel::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	if(ViewItem* viewItem = unknown_cast<ViewItem> (index.getObject ()))
	{
		if(column == 0)
		{
			String title (viewItem->getInfo ());
			if(!title.isEmpty ())
			{
				Font textFont (info.style.font);
				SolidBrush textBrush (info.style.textBrush);
				if(!viewItem->getView () || !ViewBox (viewItem->getView ()).isAttached ())
					textBrush.setColor (Color (0x66, 0x66, 0x66));
				else if(ViewBox (viewItem->getView ()).isLayerBackingEnabled ())
					textFont.isUnderline (true);
				info.graphics.drawString (info.rect, title, textFont, textBrush, Alignment::kLeft|Alignment::kVCenter);

				if(ViewSprite* sprite = viewItem->getSprite ())
				{
					Rect size (info.rect);
					size.setWidth (Font::getStringWidth (title, info.style.font));

					Color spriteColor (sprite->getBackColor ());
					spriteColor.setAlphaF (.5f);
					info.graphics.fillRect (size, SolidBrush (spriteColor));

					spriteColor.setAlphaF (1.f);
					spriteColor.setIntensity (.5f);
					info.graphics.drawRect (size, Pen (spriteColor));
				}
			}
		}
		else
		{
			if(IView* view = viewItem->getView ())
			{
				if(ViewProperty* property = browser.getColumnProperty (column - 1))
				{
					Variant value;
					if(property->getValue (value, view))
					{
						String string;
						property->toString (string, value);
						info.graphics.drawString (info.rect, string, info.style.font, info.style.textBrush, Alignment::kLeft|Alignment::kVCenter);
					}
				}
			}
		}
		return true;
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ViewTreeItemModel::editCell (ItemIndexRef index, int column, const EditInfo& info)
{
	browser.signal (Message ("ViewItemFocused", getView (index)));

	if(info.editEvent.eventClass == GUIEvent::kMouseEvent
			&& ((MouseEvent&)info.editEvent).keys.isSet (KeyState::kCommand))
		if(ViewItem* viewItem = unknown_cast<ViewItem> (index.getObject ()))
		{
			viewItem->toggleSprite ();
			getItemView ()->invalidateItem (index);
			return true;
		}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ViewTreeItemModel::openItem (ItemIndexRef index, int column, const EditInfo& info)
{
	if(ViewItem* viewItem = unknown_cast<ViewItem> (index.getObject ()))
	{
		viewItem->toggleSprite ();
		getItemView ()->invalidateItem (index);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ViewTreeItemModel::onItemFocused (ItemIndexRef index)
{
	// refresh if empty
	if(ITreeItem* treeItem = index.getTreeItem ())
		if(treeItem->isEmpty ())
		{
			UnknownPtr<ITreeView> treeView (getItemView ());
			if(treeView)
				treeView->refreshItem (treeItem);
		}

	browser.signal (Message ("ViewItemFocused", getView (index)));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ViewTreeItemModel::appendItemMenu (IContextMenu& menu, ItemIndexRef index, const IItemSelection& selection) 
{ 
	if(ViewItem* viewItem = unknown_cast<ViewItem> (index.getObject ()))
	{
		menu.addCommandItem ("Show Documentation", CSTR (CCL_SPY_COMMAND_CATEGORY), CSTR ("Show Documentation"), nullptr);

		if(UnknownPtr<IObject> view = viewItem->getView ())
			if(view->getTypeInfo ().getClassID () == ClassID::ImageView)
			{
				// make explicit handler to work when a modal dialog blocks regular command handling
				menu.addCommandItem ("Save Image...", CSTR (CCL_SPY_COMMAND_CATEGORY), CSTR ("Save Image"), 
									 CommandDelegate<ViewTreeItemModel>::make (this, &ViewTreeItemModel::onSaveImage, viewItem->asUnknown ()));
			}

		if(UnknownPtr<IWindow> window = viewItem->getView ())
		{
			menu.addCommandItem ("Show Platform Information", CSTR (CCL_SPY_COMMAND_CATEGORY), CSTR ("Show Platform Information"), 
									CommandDelegate<ViewTreeItemModel>::make (this, &ViewTreeItemModel::onShowPlatformInformation, viewItem->asUnknown ()));
		}
	}

	return false; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ViewTreeItemModel::interpretCommand (const CommandMsg& msg, ItemIndexRef index, const IItemSelection& selection)
{
	ViewItem* viewItem = nullptr;
	#if 0
	ItemIndex focusItem;
	IItemView* itemView = getItemView ();
	if(itemView && itemView->getFocusItem (focusItem))
		viewItem = unknown_cast<ViewItem> (focusItem.getObject ());
	#else
	viewItem = unknown_cast<ViewItem> (index.getObject ());
	#endif

	if(viewItem && msg.category == CCL_SPY_COMMAND_CATEGORY)
	{
		if(msg.name == "Show Documentation")
		{
			if(!msg.checkOnly ())
			{
				StringID className = viewItem->getViewClass ().getSkinElementName ();
				browser.signal (Message ("Reveal View Documentation", String (className)));
			}
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewTreeItemModel::onSaveImage (CmdArgs args, VariantRef data)
{
	ViewItem* viewItem = unknown_cast<ViewItem> (data.asUnknown ());
	if(viewItem && !args.checkOnly ())
	{
		Variant var;
		if(UnknownPtr<IObject> view = viewItem->getView ())
			view->getProperty (var, kImageViewBackground);

		if(UnknownPtr<IImage> srcImage = var.asUnknown ())
		{
			AutoPtr<IFileSelector> fs = ccl_new<IFileSelector> (ClassID::FileSelector);
			if(const FileType* fileType = ImageFile::getFormatByMimeType (ImageFile::kPNG))
				fs->addFilter (*fileType);

			if(fs->run (IFileSelector::kSaveFile))
			{
				// make real copy to resolve image parts, etc.
				InterfaceList<IImage> toSave;
				UnknownPtr<IMultiResolutionBitmap> multiBitmap (var.asUnknown ());
				if(multiBitmap)
					for(int i = 0; i < multiBitmap->getRepresentationCount (); i++)
					{
						multiBitmap->setCurrentRepresentation (i);
						toSave.append (ImageBuilder::createBitmapCopy (srcImage));
					}
				else
					toSave.append (ImageBuilder::createBitmapCopy (srcImage));

				Url path (*fs->getPath (0));
				if(toSave.count () > 1)
				{
					String baseName;
					path.getName (baseName, false);
					ListForEach (toSave, IImage*, currentImage)
						String fileName (baseName);
						int scaler = ccl_to_int (UnknownPtr<IBitmap> (currentImage)->getContentScaleFactor ());
						if(scaler > 1)
							fileName << "@" << scaler << "x";
						fileName << ".png";

						Url currentPath (path);
						currentPath.ascend ();
						currentPath.descend (fileName);
						ImageFile (ImageFile::kPNG, currentImage).saveToFile (currentPath);
					EndFor
				}
				else
					ImageFile (ImageFile::kPNG, toSave.getFirst ()).saveToFile (path);
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewTreeItemModel::onShowPlatformInformation (CmdArgs args, VariantRef data)
{
	ViewItem* viewItem = unknown_cast<ViewItem> (data.asUnknown ());
	if(viewItem && !args.checkOnly ())
	{
		if(UnknownPtr<IObject> object = viewItem->getView ())
		{
			Variant returnValue;
			object->invokeMethod (returnValue, Message ("showPlatformInformation"));
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

enum
{
	kColumnFlags = IColumnHeaderList::kSizable|IColumnHeaderList::kMoveable,
	kMinW = 5
};

//************************************************************************************************
// ViewTreeBrowser
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ViewTreeBrowser, ObjectNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewTreeBrowser::ViewTreeBrowser ()
: ObjectNode (CCLSTR ("ViewTreeBrowser")),
  columns (ccl_new<IColumnHeaderList> (ClassID::ColumnHeaderList)),
  rootView (nullptr),
  window (nullptr)
{
	viewTreeItemModel = NEW ViewTreeItemModel (*this);
	setRootView (nullptr);

	columns->addColumn (250, CCLSTR ("Tree"), "tree", -1, kColumnFlags);

	ForEach (ViewClassRegistry::instance (), ViewClass, viewClass)
		IterForEach (viewClass->getProperties (), ViewProperty, p)
			//if(!columnProperties.contains (same name))
			columnProperties.add (p);
			
			int flags = kColumnFlags|IColumnHeaderList::kHideable;
			if(p->getName () != "SizeMode")
				flags |= IColumnHeaderList::kHidden;
			columns->addColumn (p->getWidth (), String (p->getName ()), p->getName (), kMinW, flags);
		EndFor
	EndFor

	sprites.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewTreeBrowser::setRootWindow (IWindow* window)
{
	setRootView (UnknownPtr<IView> (window));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewTreeBrowser::setRootView (IView* view)
{
	rootView = view;
	rootItem = NEW ViewItem (*this, view);

	if(!view)
		rootItem->setInfo (CCLSTR ("No View selected"));

	viewTreeItemModel->signal (Message (IItemModel::kNewRootItem));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static bool containsDeep (IView& ancestor, IView& child)
{
	IView* v = &child;
	while(v)
	{
		if(v == &ancestor)
			return true;
		v = v->getParentView ();
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewTreeBrowser::mustRebuildTree (IView* newView, IView* focusView)
{
	// determine an ancestor of focus view 3 levels up:
	// don't rebuild if the new view is a deep child of that view
	if(IView* ancestor = focusView)
	{
		for(int i = 0; i < 3; i++)
			if(IView* parent = ancestor->getParentView ())
				ancestor = parent;
			else
				break;

		if(ancestor && containsDeep (*ancestor, *newView))
			return false;
	}

	// don't rebuild if new view is deep parent of focusView
	if(newView && focusView)
		if(containsDeep (*newView, *focusView) && rootView && containsDeep (*rootView, *newView))
			return false;
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewTreeBrowser::browseView (IView* view)
{
	if(!view)
		return;

	IItemView* itemView = viewTreeItemModel->getItemView ();
	UnknownPtr<ITreeView> treeView (itemView);

	// check if view is already focus item
	IView* focusView = nullptr;
	ItemIndex focusIndex;
	if(itemView && itemView->getFocusItem (focusIndex))
	{
		if(ViewItem* viewItem = unknown_cast<ViewItem> (focusIndex.getObject ()))
			focusView = viewItem->getView ();
		if(focusView == view)
			return;
	}

	IWindow* newWindow = view->getIWindow ();

	// check if a full rebuild is required
	bool mustRebuild = mustRebuildTree (view, focusView);
	if(mustRebuild)
		setRootWindow (newWindow);

	if(treeView)
	{
		MutableCString viewPath;
		makeViewPath (viewPath, *view);

		// find treeItem for view, expand it with all ancestors
		if(ITreeItem* rootItem = treeView->getRootItem ())
		{
			ITreeItem* treeItem = rootItem->findItem (viewPath, true);
			if(!treeItem && !mustRebuild)
			{
				// item not found (may be because of a stuck "wasExpanded" flag): rebuild and try again
				setRootWindow (newWindow);
				if(rootItem = treeView->getRootItem ())
					treeItem = rootItem->findItem (viewPath, true);
			}

			if(treeItem)
			{
				treeView->expandItem (treeItem, true, ITreeView::kExpandParents);
				itemView->setFocusItem (ItemIndex (treeItem), true); // select & make visible
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewTreeBrowser::makeViewPath (MutableCString& path, const IView& view)
{
	if(IView* parent = view.getParentView ())
	{	
		makeViewPath (path, *parent);
		if(!path.isEmpty ())
			path.append ("/");
		path.appendFormat ("%x", &view);
	}
	// "root view" (window) has empty path
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewSprite* ViewTreeBrowser::getSprite (IView* view)
{
	ArrayForEachFast (sprites, ViewSprite, sprite)
		if(sprite->getView () == view)
			return sprite;
	EndFor
	return nullptr;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

ViewSprite* ViewTreeBrowser::showSprite (IView* view)
{
	ViewSprite* sprite = getSprite (view);
	if(!sprite)
	{
		sprite = NEW ViewSprite;
		sprite->setBackColor (Colors::kYellow);
		sprite->setFrameColor (Colors::kYellow);
		sprite->setShowInfo (true);
		sprites.add (sprite);
	}
	sprite->show (view);
	return sprite;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewTreeBrowser::hideSprite (IView* view)
{
	ViewSprite* sprite = getSprite (view);
	if(sprite)
	{
		sprites.remove (sprite);
		sprite->hide ();
		sprite->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ViewTreeBrowser::getObject (StringID name, UIDRef classID)
{
	if(classID == ccl_iid<IItemModel> ())
	{
		if(name == "ViewTree")
		{
			if(viewTreeItemModel)
				return viewTreeItemModel->asUnknown ();
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewTreeBrowser::onViewDestroyed (ViewItem& viewItem)
{
	// find in tree & remove
	IItemView* itemView = viewTreeItemModel->getItemView ();
	UnknownPtr<ITreeView> treeView (itemView);
	if(ITreeItem* rootTreeItem = treeView->getRootItem ())
		if(ITreeItem* treeitem = rootTreeItem->findItem (viewItem.asUnknown (), false))
		{
			IWindow::UpdateCollector uc (ViewBox (treeView).getWindow ());

			if(treeitem == rootTreeItem)
				setRootView (nullptr);
			else
				itemView->removeItem (ItemIndex (treeitem));
		}

	hideSprite (viewItem.getView ());
}
