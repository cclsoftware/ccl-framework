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
// Filename    : ccl/app/components/listvieweditcomponent.h
// Description : List View Edit Component
//
//************************************************************************************************

#ifndef _ccl_listvieweditcomponent_h
#define _ccl_listvieweditcomponent_h

#include "ccl/app/components/listeditcomponent.h"
#include "ccl/app/controls/listviewmodel.h"

#include "ccl/public/gui/commanddispatch.h"

namespace CCL {

//************************************************************************************************
// ListViewEditComponent
//************************************************************************************************
	
class ListViewEditComponent: public ListEditComponent,
							 public CommandDispatcher<ListViewEditComponent>
{
public:
	DECLARE_CLASS_ABSTRACT (ListViewEditComponent, ListEditComponent)

	ListViewEditComponent (ListViewModelBase* listModel);

	bool onEnterEditMode (CmdArgs args);

	DECLARE_COMMANDS (ListViewEditComponent)
	DECLARE_COMMAND_CATEGORY ("Edit", ListEditComponent)
	
protected:
	SharedPtr<ListViewModelBase> listModel;

	// ListEditComponent
	void checkEditItems (bool state) override;
	void performCommand (const CommandMsg& msg) override;
};

} // namespace CCL

#endif // _ccl_listvieweditcomponent_h
