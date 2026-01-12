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
// Filename    : ccl/app/actions/actionjournalpreview.h
// Description : Action Journal Preview Helper
//
//************************************************************************************************

#include "ccl/app/actions/action.h"
#include "ccl/app/actions/actionjournal.h"

#include "ccl/public/gui/iparamobserver.h"

#ifndef _actionjournalpreview_h
#define _actionjournalpreview_h

namespace CCL {

//************************************************************************************************
// ActionJournalPreviewHelper
/** Helper for performing temporary actions on parameter preview using an action journal restore point. */
//************************************************************************************************

class ActionJournalPreviewHelper
{
public:
	template<typename Lambda>
	static void perform (ActionJournal& journal, ParamPreviewEvent& e, Lambda applyValue)
	{
		if(e.type == ParamPreviewEvent::kCancel)
			cancel (journal, e);
		else
		{
			changed (journal, e);
			applyValue (e.value);
		}
	}

private:
	static void cancel (ActionJournal& journal, ParamPreviewEvent& e)
	{
		Action* restorePoint = unknown_cast<Action> (e.handlerData.asUnknown ());
		if(restorePoint)
			journal.rollback (restorePoint);
		e.handlerData.clear ();
	}

	static void changed (ActionJournal& journal, ParamPreviewEvent& e)
	{
		Action* restorePoint = unknown_cast<Action> (e.handlerData.asUnknown ());
		if(restorePoint == nullptr)
		{
			SOFT_ASSERT (!journal.isRestorePending (), "Action journal occupied!")
			if(journal.isRestorePending ())
				return;

			restorePoint = journal.addRestorePoint ();
			ASSERT (restorePoint != nullptr)
			e.handlerData.takeShared (ccl_as_unknown (restorePoint));
		}
	}
};

} // namespace CCL

#endif // _actionjournalpreview_h
