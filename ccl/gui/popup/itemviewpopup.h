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
// Filename    : ccl/gui/popup/itemviewpopup.h
// Description : ItemView Popup Selector
//
//************************************************************************************************

#ifndef _ccl_itemviewpopup_h
#define _ccl_itemviewpopup_h

#include "ccl/base/object.h"

#include "ccl/public/gui/framework/popupselectorclient.h"
#include "ccl/public/gui/framework/iitemmodel.h"
#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/framework/styleflags.h"

namespace CCL {

class Theme;
class ItemControl;
class VisualStyle;

//************************************************************************************************
// ItemViewPopup
/** Base class for using a PopupSelector with an ItemView. */
//************************************************************************************************

class ItemViewPopup: public Object,
					 public AbstractItemModel,
					 public PopupSelectorClient
{
public:
	ItemViewPopup ();
	~ItemViewPopup ();

	virtual void setVisualStyle (VisualStyle* visualStyle);

	// PopupSelectorClient
	IView* CCL_API createPopupView (SizeLimit& limits) override;
	void CCL_API attached (IWindow& popupWindow) override;
	Result CCL_API onMouseUp (const MouseEvent& event, IWindow& popupWindow) override;
	void CCL_API onPopupClosed (Result result) override;

	CLASS_INTERFACE2 (IItemModel, IPopupSelectorClient, Object)

protected:
	bool isInsideItemView (const MouseEvent& event, Point* clientPos = nullptr) const;
	virtual IItemModel* getItemModel ();
	virtual VisualStyle* getVisualStyle (Theme& theme);
	virtual ItemControl* createItemControl (RectRef size) = 0;
	virtual void onItemViewCreated ();

	StyleFlags itemViewStyle;
	StyleFlags scrollStyle;
	VisualStyle* visualStyle;
	ObservedPtr<IItemView> itemView;
	ItemIndex initialMouseItem;
};

//************************************************************************************************
// TreeViewPopup
//************************************************************************************************

class TreeViewPopup: public ItemViewPopup
{
public:
	// ItemViewPopup
	ItemControl* createItemControl (RectRef size) override;
	void onItemViewCreated () override;
};

//************************************************************************************************
// ListViewPopup
//************************************************************************************************

class ListViewPopup: public ItemViewPopup
{
public:
	ListViewPopup ();

	// ItemViewPopup
	ItemControl* createItemControl (RectRef size) override;
	void onItemViewCreated () override;

protected:
	Styles::ListViewType listViewType;
};

} // namespace CCL

#endif // _ccl_itemviewpopup_h
