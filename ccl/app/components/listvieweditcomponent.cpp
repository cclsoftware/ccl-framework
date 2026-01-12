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
// Filename    : ccl/app/components/listvieweditcomponent.cpp
// Description : List View Edit Component
//
//************************************************************************************************

#include "ccl/app/components/listvieweditcomponent.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (ListViewEditComponent)
	DEFINE_COMMAND_ ("Edit", "Enter Edit Mode", ListViewEditComponent::onEnterEditMode, CommandFlags::kHidden)
END_COMMANDS (ListViewEditComponent)

//************************************************************************************************
// ListViewEditComponent
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ListViewEditComponent, ListEditComponent)
IMPLEMENT_COMMANDS (ListViewEditComponent, ListEditComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

ListViewEditComponent::ListViewEditComponent (ListViewModelBase* listModel)
: listModel (listModel)
{
	listModel->setEditModeParam (editModeParam);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewEditComponent::checkEditItems (bool state)
{
	listModel->visitItems ([state] (ListViewItem& item)
	{
		item.setChecked (state);
		return true;
	});
	
	listModel->invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewEditComponent::onEnterEditMode (CmdArgs args)
{
	if(!args.checkOnly ())
		editModeParam->setValue (true, true);
		
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewEditComponent::performCommand (const CommandMsg& msg)
{
	IItemView* itemView = listModel->getItemView ();
	ViewBox view (itemView);
	UnknownPtr<ICommandHandler> controller (view ? view.getController () : nullptr);
	if(controller)
	{
		itemView->selectAll (false);

		listModel->visitItems ([&] (ListViewItem& item)
		{
			if(item.isChecked ())
			{
				ItemIndex index;
				if(listModel->getIndex (index, &item))
					itemView->selectItem (index, true);
			}
			return true;
		});

		controller->interpretCommand (msg);
	}
}
