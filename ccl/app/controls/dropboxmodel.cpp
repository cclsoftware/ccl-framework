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
// Filename    : ccl/app/controls/dropboxmodel.cpp
// Description : DropBox item model
//
//************************************************************************************************

#include "ccl/app/controls/dropboxmodel.h"
#include "ccl/app/component.h"

#include "ccl/base/storage/attributes.h"

#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/iform.h"
#include "ccl/public/gui/framework/idropbox.h"

using namespace CCL;

//************************************************************************************************
// DropBoxModel
//************************************************************************************************

DropBoxModel::DropBoxModel (Component* owner, StringID modelName)
: owner (owner),
  modelName (modelName)
{
	items.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DropBoxModel::removeAll ()
{
	items.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DropBoxModel::addItem (Object* item)
{
	items.add (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DropBoxModel::insertItem (int index, Object* item)
{
	items.insertAt (index, item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DropBoxModel::getSubItems (IUnknownList& subItems, ItemIndexRef index)
{
	for(auto item : items)
		subItems.add (item->asUnknown (), true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API DropBoxModel::createView (StringID name, VariantRef data, const Rect& bounds)
{
	Object* item = unknown_cast<Object> (data);
	if(item && name == MutableCString (modelName).append (IDropBox::kItemSuffix))
	{
		Attributes variables;
		variables.set ("component", owner);

		String title;
		if(item->toString (title))
			variables.set ("itemTitle", title);

		ITheme* theme = RootComponent::instance ().getTheme ();
		IView* view = theme ? theme->createView (getItemFormName (), this->asUnknown (), &variables) : nullptr;
		if(UnknownPtr<IForm> form = view)
			form->setController (item->asUnknown ()); // dropbox uses controller to identify items
		return view;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DropBoxModel::onItemFocused (ItemIndexRef index) 
{
	focusItem = items.at (index.getIndex ());
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API DropBoxModel::createDragSessionData (ItemIndexRef index) 
{
	return return_shared (ccl_as_unknown (items.at (index.getIndex ())));
}
