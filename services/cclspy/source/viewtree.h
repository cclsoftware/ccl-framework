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
// Filename    : viewtree.h
// Description : 
//
//************************************************************************************************

#ifndef _viewtree_h
#define _viewtree_h

#include "viewsprite.h"

#include "ccl/base/collections/objectarray.h"
#include "ccl/base/objectnode.h"

#include "ccl/public/text/cclstring.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/base/irecognizer.h"
#include "ccl/public/gui/framework/iitemmodel.h"
#include "ccl/public/gui/icontroller.h"
#include "ccl/public/gui/icommandhandler.h"

namespace CCL {
interface IView;
interface IWindow; }

namespace Spy {

class ViewProperty;
class ViewClass;
class ViewTreeBrowser;

//************************************************************************************************
// ViewItem
//************************************************************************************************

class ViewItem: public CCL::Object
{
public:
	DECLARE_CLASS_ABSTRACT (ViewItem, Object)

	ViewItem (ViewTreeBrowser& browser, CCL::IView* view = nullptr);
	~ViewItem ();

	PROPERTY_STRING (info, Info);

	CCL::IView* getView ();
	ViewClass& getViewClass ();

	ViewSprite* getSprite ();
	void toggleSprite ();

	// Object
	void CCL_API notify (ISubject* subject, CCL::MessageRef msg) override;

private:
	ViewTreeBrowser& browser;
	CCL::IView* view;
	ISubject* viewSubject;
	CCL::AutoPtr<ViewSprite> sprite;
};

//************************************************************************************************
// ViewTreeItemModel
//************************************************************************************************

class ViewTreeItemModel: public CCL::Object,
						 public CCL::ItemViewObserver<CCL::AbstractItemModel>
{
public:
	ViewTreeItemModel (ViewTreeBrowser& browser);

	// IItemModel
	CCL::tbool CCL_API createColumnHeaders (CCL::IColumnHeaderList& list) override;
	CCL::tbool CCL_API getRootItem (CCL::ItemIndex& index) override;
	CCL::tbool CCL_API getSubItems (CCL::IUnknownList& items, CCL::ItemIndexRef index) override;
	CCL::tbool CCL_API canExpandItem (CCL::ItemIndexRef index) override;
	CCL::tbool CCL_API getItemTitle (CCL::String& title, CCL::ItemIndexRef index) override;
	CCL::tbool CCL_API getUniqueItemName (CCL::MutableCString& name, CCL::ItemIndexRef index) override;
	CCL::IImage* CCL_API getItemIcon (CCL::ItemIndexRef index) override;
	CCL::tbool CCL_API drawIconOverlay (CCL::ItemIndexRef index, const DrawInfo& info) override;
	CCL::tbool CCL_API drawCell (CCL::ItemIndexRef index, int column, const DrawInfo& info) override;
	CCL::tbool CCL_API editCell (CCL::ItemIndexRef index, int column, const EditInfo& info) override;
	CCL::tbool CCL_API openItem (CCL::ItemIndexRef index, int column, const EditInfo& info) override;
	CCL::tbool CCL_API onItemFocused (CCL::ItemIndexRef index) override;
	CCL::tbool CCL_API appendItemMenu (CCL::IContextMenu& menu, CCL::ItemIndexRef item, const CCL::IItemSelection& selection) override;
	CCL::tbool CCL_API interpretCommand (const CCL::CommandMsg& msg, CCL::ItemIndexRef item, const CCL::IItemSelection& selection) override;

	bool onSaveImage (CCL::CmdArgs args, CCL::VariantRef data);
	bool onShowPlatformInformation (CCL::CmdArgs args, CCL::VariantRef data);

	CLASS_INTERFACE (IItemModel, Object)

protected:
	ViewTreeBrowser& browser;

	CCL::IView* getView (CCL::ItemIndexRef index);
};

//************************************************************************************************
// ViewTreeBrowser
//************************************************************************************************

class ViewTreeBrowser: public CCL::ObjectNode,
					   public CCL::AbstractController
{
public:
	ViewTreeBrowser ();

	DECLARE_CLASS (ViewTreeBrowser, ObjectNode)

	void browseView (CCL::IView* view);
	void setRootWindow (CCL::IWindow* window);
	void setRootView (CCL::IView* view);

	ViewItem* getRootItem ();
	CCL::IView* getRootView ();
	CCL::IView* findView (const ViewItem& viewItem);
	CCL::IView* findView (const ViewItem& viewItem, CCL::IView& startView);

	PROPERTY_AUTO_POINTER (CCL::IColumnHeaderList, columns, Columns)
	ViewProperty* getColumnProperty (int column);

	ViewSprite* showSprite (CCL::IView* view);
	void hideSprite (CCL::IView* view);
	ViewSprite* getSprite (CCL::IView* view);

	void onViewDestroyed (ViewItem& viewItem);

	// IController
	IUnknown* CCL_API getObject (CCL::StringID name, CCL::UIDRef classID) override;

	CLASS_INTERFACE (IController, ObjectNode)

private:
	CCL::AutoPtr<ViewTreeItemModel> viewTreeItemModel;
	CCL::AutoPtr<ViewItem> rootItem;
	CCL::IView* rootView;	// todo: observe for kdestroyed, if not a window (handler->onClose)
	CCL::IWindow* window;
	CCL::ObjectArray columnProperties;
	CCL::ObjectArray sprites;

	bool mustRebuildTree (CCL::IView* newView, CCL::IView* focusView);
	void makeViewPath (CCL::MutableCString& path, const CCL::IView& view);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline CCL::IView* ViewItem::getView ()				{ return view; }
inline ViewSprite* ViewItem::getSprite ()			{ return sprite; }

inline ViewItem* ViewTreeBrowser::getRootItem ()	{ return rootItem; }
inline CCL::IView* ViewTreeBrowser::getRootView ()	{ return rootView; }
inline ViewProperty* ViewTreeBrowser::getColumnProperty (int column) { return (ViewProperty*)columnProperties.at (column); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Spy

#endif // _viewtree_h
