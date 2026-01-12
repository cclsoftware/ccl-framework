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
// Filename    : ccl/app/components/listeditcomponent.cpp
// Description : List Edit Component
//
//************************************************************************************************

#include "ccl/app/components/listeditcomponent.h"

#include "ccl/public/gui/iparameter.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum ListEditComponentTags
	{
		kEditMode = 'Edit',
		kSelectAll = 'SeAl'
	};
}

//************************************************************************************************
// ListEditComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ListEditComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

ListEditComponent::ListEditComponent ()
: Component ("ListEditor")
{
	editModeParam = paramList.addParam (CSTR ("editMode"), Tag::kEditMode);
	paramList.addParam (CSTR ("selectAll"), Tag::kSelectAll);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListEditComponent::addEditCommand (StringID paramName, StringID commandCategory, StringID commandName)
{
	paramList.addCommand (commandCategory, commandName, paramName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListEditComponent::isEditMode () const
{
	return editModeParam->getValue ().asBool ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListEditComponent::setEditMode (bool state)
{
	return editModeParam->setValue (state, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListEditComponent::enableEditCommands (bool state)
{
	paramList.enableCommands (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListEditComponent::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kEditMode:
		// reset edit selection states (when entering / leaving edit mode)
		checkEditItems (false);
		enableEditCommands (param->getValue ());
		paramList.byTag (Tag::kSelectAll)->setValue (false);
		return true;
		
	case Tag::kSelectAll:
		checkEditItems (param->getValue ());
		enableEditCommands (param->getValue ());
		return true;
	}
	return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListEditComponent::interpretCommand (const CommandMsg& msg)
{
	IParameter* param = paramList.byCommand (msg.category, msg.name);
	if(param && isEqualUnknown (param, msg.invoker))
	{
		if(msg.checkOnly ())
			return true;

		// it's one of our edit commands: apply edit selection to list and perform command
		CommandMsg cmd (msg.category, msg.name, this->asUnknown (), msg.flags);
		performCommand (cmd);

		// leave edit mode
		editModeParam->setValue (false);
		return true;
	}
	return SuperClass::interpretCommand (msg);
}
