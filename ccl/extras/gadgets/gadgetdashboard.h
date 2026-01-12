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
// Filename    : ccl/extras/gadgets/gadgetdashboard.h
// Description : Gadget Dashboard
//
//************************************************************************************************

#ifndef _ccl_gadgetdashboard_h
#define _ccl_gadgetdashboard_h

#include "ccl/app/component.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/gui/framework/iitemmodel.h"

namespace CCL {
namespace Install {

class GadgetItem;

//************************************************************************************************
// GadgetDashboard
//************************************************************************************************

class GadgetDashboard: public Component,
					   public ItemViewObserver<AbstractItemModel>
{
public:
	DECLARE_CLASS_ABSTRACT (GadgetDashboard, Component)

	GadgetDashboard ();

	PROPERTY_AUTO_POINTER (Component, fillItem, FillItem) ///< additional trailing dropbox item

	void addGadget (GadgetItem* gadget);
	bool setTabCount (int numShared);

	// Component
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

	// IItemModel
	tbool CCL_API getSubItems (IUnknownList& items, ItemIndexRef index) override;

	CLASS_INTERFACE (IItemModel, Component)

protected:
	friend class DashboardView;
	class ViewItem;
	class FillItem;

	const ObjectArray& getViewItems () const;
	ViewItem* getViewItem (int index) const; 

private:
	ObjectArray viewItems;
	int numTabs;	///< number of views in tabs (other views separated below)
	bool initDone;

	enum Tags { kDashboardTab = 100 };
};

} // namespace Install
} // namespace CCL

#endif // _ccl_gadgetdashboard_h
