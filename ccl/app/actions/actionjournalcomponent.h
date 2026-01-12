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
// Filename    : ccl/app/actions/actionjournalcomponent.h
// Description : Action Journal Component
//
//************************************************************************************************

#ifndef _ccl_actionjournalcomponent_h
#define _ccl_actionjournalcomponent_h

#include "ccl/app/component.h"

namespace CCL {

class ActionListModel;
class ActionJournal;

//************************************************************************************************
// ActionJournalComponent
/** Component to present state of action journal in a list view. */
//************************************************************************************************

class ActionJournalComponent: public Component
{
public:
	DECLARE_CLASS_ABSTRACT (ActionJournalComponent, Component)

	ActionJournalComponent (ActionJournal& journal, StringRef name = nullptr);
	~ActionJournalComponent ();

	void runDialog ();

	// Component
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;

protected:
	ActionListModel* actionList;
	ActionJournal& journal;
};

} // namespace CCL

#endif // _ccl_actionjournalcomponent_h
