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
// Filename    : ccl/app/actions/actionjournal.cpp
// Description : Action Journal
//
//************************************************************************************************

#include "ccl/app/actions/actionjournal.h"
#include "ccl/app/actions/action.h"
#include "ccl/app/actions/sideeffect.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/logfile.h"
#include "ccl/base/singleton.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/base/iprogress.h"
#include "ccl/public/gui/framework/iprogressdialog.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Action")
	XSTRING (Undo, "Undo")
	XSTRING (Redo, "Redo")
END_XSTRINGS

//************************************************************************************************
// ActionJournal::ProgressProvider
//************************************************************************************************

class ActionJournal::ProgressProvider: public StaticSingleton<ProgressProvider>,
                                       public IProgressProvider
{
public:
	IMPLEMENT_DUMMY_UNKNOWN (IProgressProvider)
	IProgressNotify* CCL_API createProgressNotify () override;
};

//************************************************************************************************
// ActionJournal::Transaction
//************************************************************************************************

class ActionJournal::Transaction: public Unknown
{
public:
	Transaction (StringRef title, Action* top)
	: title(title), topUndo (top), recursionCounter(0)
	{}
	PROPERTY_STRING (title, Title)
	PROPERTY_POINTER (Action, topUndo, TopUndo)
	PROPERTY_VARIABLE (int, recursionCounter, RecursionCounter)
};

//************************************************************************************************
// ActionJournal
//************************************************************************************************

const Configuration::IntValue ActionJournal::undoStackLimit ("CCL.Actions", "undoStackLimit", -1);

DEFINE_STRINGID_MEMBER_ (ActionJournal, kExecuted, "executed")
DEFINE_STRINGID_MEMBER_ (ActionJournal, kUndone, "undone")
DEFINE_STRINGID_MEMBER_ (ActionJournal, kRedone, "redone")
DEFINE_STRINGID_MEMBER_ (ActionJournal, kWillRedo, "willRedo")
DEFINE_STRINGID_MEMBER_ (ActionJournal, kRemovedAll, "removedAll")
DEFINE_STRINGID_MEMBER_ (ActionJournal, kUndoReduced, "undoReduced")
DEFINE_STRINGID_MEMBER_ (ActionJournal, kMerged, "merged")
DEFINE_STRINGID_MEMBER_ (ActionJournal, kSquashed, "squashed")

//////////////////////////////////////////////////////////////////////////////////////////////////

String& ActionJournal::getUndoString (String& string, ActionJournal* journal)
{
	string = XSTR (Undo);

	Action* action = journal ? (Action*)journal->undoStack.peek () : nullptr;
	if(action && !action->getDescription ().isEmpty ())
	{
		string.append (CCLSTR (" "));
		string.append (action->getDescription ());
	}

	return string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& ActionJournal::getRedoString (String& string, ActionJournal* journal)
{
	string = XSTR (Redo);

	Action* action = journal ? (Action*)journal->redoStack.peek () : nullptr;
	if(action && !action->getDescription ().isEmpty ())
	{
		string.append (CCLSTR (" "));
		string.append (action->getDescription ());
	}

	return string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IProgressProvider* ActionJournal::getStandardProgressProvider ()
{
	return &ProgressProvider::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (ActionJournal, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ActionJournal::ActionJournal ()
: lastEditTime (0),
  lastSaveTime (0),
  enabled (true),
  undoRedoSuspended (false),
  executingAction (nullptr),
  sideEffectAction (nullptr),
  inUndoRedo (false),
  undoCount (0),
  progressProvider (nullptr),
  logBuffer (*NEW LogBuffer),
  signalSuspended (false),
  activeTransaction (nullptr)
{
	undoStack.objectCleanup (true);
	redoStack.objectCleanup (true);

	logBuffer.setTitle (CSTR ("Actions:"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ActionJournal::~ActionJournal ()
{
	ASSERT (multiStack.isEmpty () == true)
	safe_release (activeTransaction);
	delete &logBuffer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActionJournal::signalAction (StringID messageId, Action* action)
{
	if(signalSuspended == false)
		signal (Message (messageId, ccl_as_unknown (action)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const LogBuffer& ActionJournal::getLogBuffer () const
{
	return logBuffer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActionJournal::logAction (const char* prefix, const Action* action)
{
	if(signalSuspended)
		return;

	MutableCString str (prefix);
	str += action->getDescription ();
	str += ": ";
	str += action->myClass ().getPersistentName ();

	// add the classname of the first non-MultiAction
	if(ccl_cast<MultiAction> (action))
		while((action = action->getAction (0)))
			if(!ccl_cast<MultiAction> (action))
			{
				str += ", ";
				str += action->myClass ().getPersistentName ();
				break;
			}

	logBuffer.print (str);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ActionJournal::execute (Action* action, int executionFlags)
{
	ASSERT (action != nullptr)
	if(!action)
		return false;

	bool allowSideEffects = (executionFlags & kExecuteWithoutSideEffects) == 0;
	bool immediately = (executionFlags & kExecuteImmediatly) != 0;
	bool isDirectEdit = (executionFlags & kExecuteDirectEdit) != 0;
	
	ScopedVar<Action*> execScope (executingAction, action);

	if(enabled && allowSideEffects && sideEffectAction == nullptr)
	{
		ScopedVar<Action*> sideEffectScope (sideEffectAction, action);
		SideEffectRegistry::instance ().extendAction (action);
	}

	if(immediately)
	{
		if(!action->executeAll ())
		{
			action->release ();
			return false;
		}
		action->setExecuted (true);
	}

	// collect actions if beginMultiple has been called...
	if(enabled)
	{
		Action* multiAction = (Action*)multiStack.peek ();
		if(multiAction)
		{
			if(immediately && isDirectEdit)
			{
				// try to merge with previous action in multi action...
				if(Action* lastAction = multiAction->getLastAction ())
				{
					if(lastAction->canMerge () && lastAction->merge (action))
					{
						action->release ();
						return true;
					}
				}
			}

			multiAction->addAction (action);
			return true;
		}

		if(execScope.old)
		{
			// when an action is added while another action is executed, add the new action to the one beging executed
			// (can happen when side-effects are created using the action journal)
			execScope.old->addAction (action);
			return true;
		}
	}

	int64 now = System::GetSystemTicks ();

	// try to merge with previous action...
	if(enabled)
		if(Action* lastAction = (Action*)undoStack.peek ())
			if(now - lastEditTime <= 500
				&& lastAction->canMerge ()
				&& lastAction->merge (action))
			{
				logAction ("mrge ", action);
				lastEditTime = now;
				action->release ();
				signalAction (kMerged, lastAction);
				return true;
			}

	ProgressNotifyScope progressScope (isEnabled () ? progressProvider : nullptr, action->getDescription (), false);

	if(!action->executeAll (progressScope))
	{
		logAction ("ignr ", action);
		action->release ();
		return false;
	}

	logAction ("exec ", action);

	if(enabled && inUndoRedo == false)
	{
		undoStack.push (action);
		undoCount++;

		// check limit
		int maxUndoCount = undoStackLimit;
		if(maxUndoCount != -1 && signalSuspended == false) // not while inside restore point
		{
			while(undoCount > maxUndoCount)
			{
				Action* oldest = (Action*)undoStack.removeLast ();
				ASSERT (oldest != nullptr)
				if(oldest)
				{
					signalAction (kUndoReduced, oldest);
					oldest->release ();
				}
				undoCount--;
			}
		}

		redoStack.removeAll ();
		lastEditTime = now;

		DateTime timeStamp;
		System::GetSystem ().getLocalTime (timeStamp);
		action->setTime (timeStamp);

		// if action has still no description, try to get one from subActions
		if(action->getDescription ().isEmpty ())
			action->takeDescriptionFromSubAction ();

		signalAction (kExecuted, action);
	}
	else // do not push to undo stack
		action->release ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ActionJournal::canUndo () const
{
	return undoStack.peek () != nullptr && !isRestorePending () && !isUndoRedoSuspended ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ActionJournal::canRedo () const
{
	return redoStack.peek () != nullptr && !isRestorePending () && !isUndoRedoSuspended ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ActionJournal::undo ()
{
	Action* action = peekUndo ();
	if(!action)
		return false;

	ASSERT (inUndoRedo == false)
	if(inUndoRedo)
		return false;

	ScopedVar<bool> undoScope (inUndoRedo, true);
	logAction ("undo ", action);

	ProgressNotifyScope progressScope (progressProvider, action->getDescription (), false);

	//if(!action->undoAll ())
	//	return false;
	action->undoAll (progressScope); // do not block the whole stack if action returns false!

	undoStack.pop ();
	undoCount--;
	redoStack.push (action);
	lastEditTime = System::GetSystemTicks ();

	signalAction (kUndone, action);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ActionJournal::redo ()
{
	Action* action = peekRedo ();
	if(!action)
		return false;

	ASSERT (inUndoRedo == false)
	if(inUndoRedo)
		return false;

	ScopedVar<bool> redoScope (inUndoRedo, true);
	logAction ("redo ", action);

	signalAction (kWillRedo, action);

	ProgressNotifyScope progressScope (progressProvider, action->getDescription (), false);

	//if(!action->redoAll ())
	//	return false;
	action->redoAll (progressScope); // do not block the whole stack if action returns false!

	redoStack.pop ();
	undoStack.push (action);
	undoCount++;
	lastEditTime = System::GetSystemTicks ();

	signalAction (kRedone, action);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Action* ActionJournal::peekUndo () const
{
	return (Action*)undoStack.peek ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Action* ActionJournal::peekRedo () const
{
	return (Action*)redoStack.peek ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* ActionJournal::newUndoIterator () const
{
	return undoStack.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* ActionJournal::newRedoIterator () const
{
	return redoStack.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ActionJournal::isPerformingAction () const
{
	return executingAction != nullptr || inUndoRedo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ActionJournal::isExecutingAction () const
{
	return executingAction != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ActionJournal::isRestorePending () const
{
	return signalSuspended;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Action* ActionJournal::addRestorePoint ()
{
	ASSERT (enabled == true)
	if(!enabled)
		return nullptr;

	ASSERT (isMultiplePending () == false)
	if(isMultiplePending ())
		return nullptr;

	RestorePointAction* restorePoint = NEW RestorePointAction;
	undoStack.push (restorePoint);

	// save previous redo stack
	restorePoint->saveRedo (redoStack);
	redoStack.removeAll ();
	restorePoint->setSavedEditTime (lastEditTime);

	ASSERT (signalSuspended == false) // recursion?
	signalSuspended = true;
	return restorePoint;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ActionJournal::rollback (Action* _restorePoint)
{
	ASSERT (enabled == true)

	RestorePointAction* restorePoint = ccl_cast<RestorePointAction> (_restorePoint);
	ASSERT (restorePoint != nullptr)
	if(restorePoint == nullptr)
		return false;

	ASSERT (undoStack.contains (restorePoint))

	// seek backwards until restore point
	bool result = false;
	while(undoStack.peek () != restorePoint)
	{
		if(!undo ())
			break;
	}

	// restore previous redo stack
	redoStack.removeAll ();
	restorePoint->restoreRedo (redoStack);
	lastEditTime = restorePoint->getSavedEditTime ();

	ASSERT (undoStack.peek () == restorePoint)
	if(undoStack.peek () == restorePoint)
	{
		undoStack.pop ();
		restorePoint->release ();
		result = true;
	}

	signalSuspended = false;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActionJournal::beginTransaction (StringRef title)
{
	if(enabled)
	{	
		if(activeTransaction == nullptr)
			activeTransaction = NEW Transaction (title, peekUndo ());
		else
			activeTransaction->setRecursionCounter (activeTransaction->getRecursionCounter () + 1);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActionJournal::endTransaction ()
{
	if(activeTransaction)
	{
		// check which actions were added and squash multiple actions into one undo

		if(activeTransaction->getRecursionCounter () <= 0)
		{
			Action* oldTopUndo = activeTransaction->getTopUndo ();
			
			ObjectStack behind;
			bool topUndoFound = false;
			ListForEachObject (undoStack, Action, action) // latest is first
				if(action == oldTopUndo)
				{
					topUndoFound = true;
					break;		
				}
				behind.push (action);
			EndFor
		
			// if the marker action cannot be found, do not squash
			if(topUndoFound == false && oldTopUndo != nullptr) 
				behind.removeAll ();

			if(behind.count () > 1)
			{
				Action* squashAction = NEW MultiAction (activeTransaction->getTitle ());
				squashAction->setTime (static_cast<Action*> (behind.peek ())->getTime ());
				squashAction->setExecuted (true);
				ListForEachObject (behind, Action, action)
					squashAction->addAction (action);
					undoStack.remove (action);		
					undoCount--;
				EndFor		
				undoCount++; // one for the scash 
				undoStack.push (squashAction);
				
				signalAction (kSquashed, squashAction); // signal gui
			}

			safe_release (activeTransaction);
		}
		else
			activeTransaction->setRecursionCounter (activeTransaction->getRecursionCounter () - 1);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActionJournal::removeAll ()
{
	undoStack.removeAll ();
	undoCount = 0;
	redoStack.removeAll ();

	ASSERT (signalSuspended == false)
	signal (Message (kRemovedAll));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ActionJournal::isDirty () const
{
	return !undoStack.isEmpty () || !redoStack.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ActionJournal::isModified () const
{
	if(!lastSaveTime || !lastEditTime)
		return isDirty ();

	return lastEditTime > lastSaveTime;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActionJournal::setSavedNow ()
{
	lastSaveTime = System::GetSystemTicks ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API ActionJournal::getLastEditTime () const
{
	return lastEditTime;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ActionJournal::isMultiplePending () const
{
	return multiStack.peek () != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Action* ActionJournal::peekMultiple ()
{
	return (Action*)multiStack.peek ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Action* ActionJournal::beginMultiple (StringRef description, StringRef details)
{
	CCL_PRINTF ("ActionJournal::beginMultiple (%d) %s\n", multiStack.count (), MutableCString (description).str ())

	Action* action = NEW MultiAction (description);
	action->setDetailedDescription (details);
	multiStack.push (action);
	return action;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Action* ActionJournal::beginMultiple (Action* multiAction)
{
	CCL_PRINTF ("ActionJournal::beginMultiple (%d) %s\n", multiStack.count (), MutableCString (multiAction->getDescription ()).str ())

	multiStack.push (multiAction);
	return multiAction;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ActionJournal::endMultiple (bool cancel, int executionFlags)
{
	Action* action = (Action*)multiStack.pop ();
	ASSERT (action != nullptr)
	if(!action)
		return false;

	CCL_PRINTF ("ActionJournal::endMultiple (%d) %s\n", multiStack.count (), MutableCString (action->getDescription ()).str ())

	if(!action->hasSubActions ())
	{
		action->release ();
		return true;
	}

	if(cancel)
	{
		int count = action->countSubActions ();
		for(int i = count - 1; i >= 0; i--)
		{
			Action* subAction = action->getAction (i);
			if(subAction->isExecuted ())
				subAction->undoAll ();
		}
		action->release ();
		return true;
	}

	// if there is only one action (and not a specialized action class), execute it directly...
	if(action->countSubActions () == 1 && action->isClass (ccl_typeid<MultiAction> ()))
	{
		Action* subAction = action->getAction (0);

		if(!action->canMerge ())
			subAction->preventMerge ();

		// use description from parent action
		StringRef description (action->getDescription ());
		if(!description.isEmpty ())
			subAction->setDescription (description);

		StringRef details (action->getDetailedDescription ());
		if(!details.isEmpty ())
			subAction->setDetailedDescription (details);

		// prevent double execution when only the "topmost" action that was executed is flagged, but only when the subAction was not executed already
		if(subAction->isExecuted () == false)
			subAction->setExecuted (action->isExecuted ());

		action->removeAction (subAction);
		action->release ();

		return execute (subAction, executionFlags);
	}

	return execute (action, executionFlags);
}

//************************************************************************************************
// ActionJournal::ProgressProvider
//************************************************************************************************

IProgressNotify* CCL_API ActionJournal::ProgressProvider::createProgressNotify ()
{
	IProgressNotify* progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
	ASSERT (progress != nullptr)

	UnknownPtr<IProgressDialog> dialog (progress);
	if(dialog.isValid ())
	{
		dialog->constrainLevels (1, 1);
		dialog->setOpenDelay (0.5, true);
	}
	return progress;
}
