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
// Filename    : ccl/app/components/listeditcomponent.h
// Description : List Edit Component
//
//************************************************************************************************

#ifndef _ccl_listeditcomponent_h
#define _ccl_listeditcomponent_h

#include "ccl/app/component.h"

namespace CCL {

//************************************************************************************************
// ListEditComponent
/** Base class for lists with an edit mode. */
//************************************************************************************************

class ListEditComponent: public Component
{
public:
	DECLARE_CLASS (ListEditComponent, Component)

	ListEditComponent ();

	void addEditCommand (StringID paramName, StringID commandCategory, StringID commandName);
	bool isEditMode () const;
	void setEditMode (bool state);
	void enableEditCommands (bool state = true);

	// Component
	tbool CCL_API paramChanged (IParameter* param) override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

protected:
	IParameter* editModeParam;

	virtual void checkEditItems (bool state) {}
	virtual void performCommand (const CommandMsg& msg) {}
};

} // namespace CCL

#endif // _ccl_listeditcomponent_h
