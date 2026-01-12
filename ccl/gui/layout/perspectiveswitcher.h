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
// Filename    : ccl/gui/layout/perspectiveswitcher.h
// Description : Perspective Switcher
//
//************************************************************************************************

#ifndef _ccl_perspectiveswitcher_h
#define _ccl_perspectiveswitcher_h

#include "ccl/gui/popup/itemviewpopup.h"

#include "ccl/base/collections/objectarray.h"

#include "ccl/public/gui/framework/keycodes.h"

namespace CCL {

class Workspace;
class Perspective;
interface IPerspectiveActivator;

//************************************************************************************************
// PerspectiveListModel
/** ItemModel for a ListView that allows selecting available perspectives of a workspace. */
//************************************************************************************************

class PerspectiveListModel: public Object,
							public ItemViewObserver<AbstractItemModel>
{
public:
	PerspectiveListModel (Workspace* workspace);

	void init (Perspective* current, int increment);
	void increment (int increment);

	void activateFocusPerspective ();

	// IItemModel
	int CCL_API countFlatItems () override;
	tbool CCL_API createColumnHeaders (IColumnHeaderList& list) override;
	tbool CCL_API getItemTitle (String& title, ItemIndexRef index) override;
	IImage* CCL_API getItemIcon (ItemIndexRef index) override;
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override;
	tbool CCL_API interpretCommand (const CommandMsg& msg, ItemIndexRef item, const IItemSelection& selection) override;

	CLASS_INTERFACE (IItemModel, Object)

	enum Columns
	{
		kIcon,
		kTitle
	};

private:
	ObjectArray perspectives;

	void addPerspective (Perspective* perspective);
	IPerspectiveActivator* getPerspective (int index);
	int calculateTextWidth ();
	int getFocusIndex ();
	void setFocusIndex (int index);
};

//************************************************************************************************
// PerspectiveSwitcher
//************************************************************************************************

class PerspectiveSwitcher: public ListViewPopup
{
public:
	PerspectiveSwitcher (Workspace* workspace);

	void run (bool next = true);

protected:
	AutoPtr<PerspectiveListModel> perspectiveListModel;
	Workspace* workspace;
	int startIncrement;
	VirtualKey mainKey;
	VirtualKey modifierKey;

	VirtualKey getModifierKeyCode (int modifier);

	// ListViewPopup
	IItemModel* getItemModel () override;
	VisualStyle* getVisualStyle (Theme& theme) override;
	void onItemViewCreated () override;
	Result CCL_API onKeyDown (const KeyEvent& event) override;
	Result CCL_API onKeyUp (const KeyEvent& event) override;
	void CCL_API onPopupClosed (Result result) override;

	typedef ListViewPopup SuperClass;
};

} // namespace CCL

#endif // _ccl_perspectiveswitcher_h
