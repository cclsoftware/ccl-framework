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
// Filename    : ccl/gui/itemviews/itemviewaccessibility.h
// Description : ItemView Accessibility
//
//************************************************************************************************

#ifndef _ccl_itemviewaccessibility_h
#define _ccl_itemviewaccessibility_h

#include "ccl/gui/views/viewaccessibility.h"
#include "ccl/public/gui/framework/iitemmodel.h"

namespace CCL {

class ItemView;
class ListView;
class TreeView;

//************************************************************************************************
// ItemViewAccessibilityProvider
//************************************************************************************************

class ItemViewAccessibilityProvider: public ViewAccessibilityProvider,
									 public IAccessibilityTableProvider
{
public:
	DECLARE_CLASS_ABSTRACT (ItemViewAccessibilityProvider, ViewAccessibilityProvider)
	
	ItemViewAccessibilityProvider (ItemView& view);
	
	ItemView& getItemView () const;

	virtual void rebuildItemProviders ();
	virtual void getElementName (String& name, ItemIndexRef index, int column) const;
	virtual tresult getElementBounds (Rect& rect, AccessibilityCoordSpace space, ItemIndexRef index, int column) const;
	virtual bool getElementValue (String& value, ItemIndexRef index, int column) const;
	virtual AccessibilityElementRole getElementRole (ItemIndexRef index, int column) const;
	
	// IAccessibilityTableProvider
	int CCL_API countColumns () const override;
	IAccessibilityProvider* CCL_API getColumnHeaderProvider () override;
	IAccessibilityProvider* CCL_API getColumnHeaderItemProvider (IAccessibilityProvider* dataItem) override;
	int CCL_API countRows () const override;
	IAccessibilityProvider* CCL_API getRowHeaderProvider () override;
	IAccessibilityProvider* CCL_API getRowHeaderItemProvider (IAccessibilityProvider* dataItem) override;

	CLASS_INTERFACES (ViewAccessibilityProvider)

protected:
	template<class ItemProvider> void addColumnProviders (ItemIndexRef index);
};

//************************************************************************************************
// ListViewAccessibilityProvider
//************************************************************************************************

class ListViewAccessibilityProvider: public ItemViewAccessibilityProvider
{
public:
	DECLARE_CLASS_ABSTRACT (ListViewAccessibilityProvider, ItemViewAccessibilityProvider)
	
	ListViewAccessibilityProvider (ListView& view);
	
	// ItemViewAccessibilityProvider
	AccessibilityElementRole CCL_API getElementRole () const override;

protected:
	ListView& getListView () const;
};

//************************************************************************************************
// TreeViewAccessibilityProvider
//************************************************************************************************

class TreeViewAccessibilityProvider: public ItemViewAccessibilityProvider
{
public:
	DECLARE_CLASS_ABSTRACT (TreeViewAccessibilityProvider, ItemViewAccessibilityProvider)
	
	TreeViewAccessibilityProvider (TreeView& view);
	
	// ItemViewAccessibilityProvider
	AccessibilityElementRole CCL_API getElementRole () const override;
	AccessibilityElementRole getElementRole (ItemIndexRef index, int column) const override;
	void rebuildItemProviders () override;

protected:
	TreeView& getTreeView () const;
};

} // namespace CCL

#endif // _ccl_itemviewaccessibility_h
