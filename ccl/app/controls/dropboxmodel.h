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
// Filename    : ccl/app/controls/dropboxmodel.h
// Description : DropBox item model
//
//************************************************************************************************

#ifndef _ccl_dropboxmodel_h
#define _ccl_dropboxmodel_h

#include "ccl/app/controls/itemviewmodel.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/public/gui/iviewfactory.h"

namespace CCL {

class Component;

//************************************************************************************************
// DropBoxModel
/** Base class for implementing item model to be used with a DropBox. */
//************************************************************************************************

class DropBoxModel: public CCL::ItemModel,
					public CCL::IViewFactory
{
public:
	DropBoxModel (Component* owner, StringID modelName);

	PROPERTY_MUTABLE_CSTRING (modelName, ModelName)
	PROPERTY_MUTABLE_CSTRING (itemFormName, ItemFormName)

	void removeAll ();
	void addItem (Object* item); ///< takes ownership
	void insertItem (int index, Object* item); ///< takes ownership

	Object* getFocusItem () const;

	// IItemModel
	tbool CCL_API getSubItems (CCL::IUnknownList& items, CCL::ItemIndexRef index) override;
	tbool CCL_API onItemFocused (ItemIndexRef index) override;
	IUnknown* CCL_API createDragSessionData (ItemIndexRef index) override;

	// IViewFactory
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

	CLASS_INTERFACE (IViewFactory, ItemModel)

protected:
	Component* owner;
	ObjectList items;
	SharedPtr<Object> focusItem;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Object* DropBoxModel::getFocusItem () const
{ return focusItem; }

} // namespace CCL

#endif // _ccl_dropboxmodel_h
