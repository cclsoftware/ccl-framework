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
// Filename    : ccl/gui/layout/dockpanel.h
// Description : Docking Panel
//
//************************************************************************************************

#ifndef _ccl_dockpanel_h
#define _ccl_dockpanel_h

#include "ccl/base/objectnode.h"

#include "ccl/gui/views/imageview.h"
#include "ccl/gui/layout/idockpanel.h"

#include "ccl/public/gui/icontroller.h"
#include "ccl/public/gui/iparamobserver.h"

namespace CCL {

class DockPanelView;
interface IDockPanelItemVisitor;

//************************************************************************************************
// DockPanelItem
//************************************************************************************************

class DockPanelItem: public ObjectNode,
					 public AbstractController,
					 public IParamObserver,
					 public IDockPanelItem
{
public:
	DECLARE_CLASS (DockPanelItem, ObjectNode)

	DockPanelItem (StringRef name = nullptr);
	DockPanelItem (const DockPanelItem&);
	~DockPanelItem ();

	PROPERTY_POINTER (View, view, View)
	PROPERTY_POINTER (IUnknown, controller, Controller)

	PROPERTY_FLAG (state, kVisible, isVisible)
	PROPERTY_FLAG (state, kHidable, isHidable)
	enum { kLastDockPanelItemFlag = 1 };

	IParameter* getVisible () const;
	View* getParentView () const;
	DockPanelItem* getParentItem () const;
	DockPanelItem* findChildItem (IRecognizer& recognizer) const;
	void traverse (IDockPanelItemVisitor& visitor);
	virtual View* createView (Theme& theme);

	void setViewAndState (View* view);
	int getIndex (DockPanelItem* item, bool onlyVisible = true) const;
	void collectItemsFlat (Container& container);
	void reset ();
	virtual void hideAll ();

	// IDockPanelItem
	void CCL_API init (StringRef name, IUnknown* controller, int state, IParameter* titleParam) override;
	void CCL_API setViewFactory (IViewFactory* factory) override;
	void CCL_API show () override;
	void CCL_API hide () override;
	void CCL_API kill () override;
	tbool CCL_API addItem (IDockPanelItem* item) override;
	void CCL_API removeItems () override;
	IDockPanelItem* CCL_API findItem (IUnknown* controller, tbool deep = false) override;

	// IController
	int CCL_API countParameters () const override;
	IParameter* CCL_API getParameterAt (int index) const override;
	IParameter* CCL_API findParameter (StringID name) const override;
	// TODO: getParameterByTag?

	// IParamObserver
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API paramEdit (IParameter* param, tbool begin) override {}

	// ObjectNode
	UIDRef CCL_API getObjectUID () const override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACES (ObjectNode)

protected:
	IViewFactory* viewFactory;
	int state;
	mutable IParameter* visible;
};

//************************************************************************************************
// DockPanelGroup
//************************************************************************************************

class DockPanelGroup: public DockPanelItem
{
public:
	DECLARE_CLASS (DockPanelGroup, DockPanelItem)

	DockPanelGroup (StringRef name = nullptr);

	PROPERTY_OBJECT (StyleFlags, style, Style)

	// DockPanelItem
	View* createView (Theme& theme) override;
};

//************************************************************************************************
// DockPanelRoot
//************************************************************************************************

class DockPanelRoot: public DockPanelGroup
{
public:
	DECLARE_CLASS (DockPanelRoot, DockPanelGroup)

	DockPanelRoot (StringRef name = nullptr);
	DockPanelRoot (const DockPanelRoot&);

	PROPERTY_POINTER (DockPanelView, ownerView, ownerView)
};

//************************************************************************************************
// DockPanelView
//************************************************************************************************

class DockPanelView: public ImageView,
					 public IDockPanelView
{
public:
	DECLARE_CLASS (DockPanelView, ImageView)

	DockPanelView (const Rect& size = Rect (), StyleRef style = StyleFlags ());
	~DockPanelView ();

	// IDockPanelView
	void CCL_API setItems (IDockPanelItem* items) override;
	IDockPanelItem* CCL_API getItems () override;

	// ImageView
	void onChildSized (View* child, const Point& delta) override;

	CLASS_INTERFACE (IDockPanelView, ImageView)

protected:
	DockPanelItem* items;
};

//************************************************************************************************
// IDockPanelItemVisitor
//************************************************************************************************

interface IDockPanelItemVisitor
{
	virtual void visit (DockPanelItem& item) = 0;
};

} // namespace CCL

#endif // _ccl_dockpanel_h
