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
// Filename    : ccl/app/actions/action.cpp
// Description : Undoable Action
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/actions/action.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/base/iprogress.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

#define LOG_ACTION(exec) \
	CCL_PRINTF ("%s%s::%s (%s) %s\n", CCL_INDENT, myClass ().getPersistentName (), exec, MutableCString (getDescription ()).str (), MutableCString (getDetailedDescription ()).str ())\
	CCL_ADD_INDENT (2)

//************************************************************************************************
// Action::ProgressHandler
//************************************************************************************************

struct Action::ProgressHandler
{
	ProgressHandler (Action& action, IProgressNotify* progress)
	: progress (progress),
	  numActions (0), 
	  i (0)
	{
		if(progress)
			numActions = countActions (action);
	}

	void nextAction (Action* subAction)
	{
		if(progress)
		{
			if(subAction->getDescription ().isEmpty () == false)
				progress->setProgressText (subAction->getDescription ());
			progress->updateProgress (IProgressNotify::State ((double)i++ / numActions));
		}
	}
	
private:
	IProgressNotify* progress;
	int numActions;
	int i;

	int countActions (Action& action) const
	{
		int count = 0;
		ListForEachObject (action.subActions, Action, subAction)
			count += countActions (*subAction);
		EndFor
		return count == 0 ? 1 : count;
	}
};

//************************************************************************************************
// Action
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (Action, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Action::Action (StringRef description)
: description (description),
  flags (0)
{
	subActions.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Action::addAction (Action* action)
{
	subActions.add (action);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Action::addActionDuringIteration (Action* action)
{
	subActions.addDuringIteration (action);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Action::insertAction (Action* action, int index)
{
	subActions.insertAt (index, action);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Action::addActionAndExecute (Action* action)
{
	bool result = action->executeAll ();
	if(result)
	{
		action->setExecuted (true);
		subActions.addDuringIteration (action);
	}
	else
		action->release ();

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Action::insertActionAndExecute (Action* action, int index)
{
	bool result = action->executeAll ();
	if(result)
	{
		action->setExecuted (true);
		subActions.insertAt (index, action);
	}
	else
		action->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Action::removeAction (Action* action)
{
	return subActions.remove (action);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Action::removeSubActions ()
{
	subActions.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Action::hasSubActions () const
{
	return !subActions.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Action::countSubActions () const
{
	return subActions.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Action* Action::getAction (int index) const
{
	return (Action*)subActions.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Action* Action::getLastAction () const
{
	return (Action*)subActions.getLast ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* Action::newIterator () const
{
	return subActions.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ObjectList& Action::getSubActions () const
{
	return subActions;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Action::undo ()
{
	execute ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Action::redo ()
{
	execute ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Action::preventMerge ()
{
	isMergeDisabled (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Action::canMerge () const
{
	if(isMergeDisabled ())
		return false;

	if(hasSubActions ())
	{
		// Normal actions should not be merged if they have sub-actions (like side effects)
		// because overwritten 'merge' implementations usually do not care for sub-actions at all.
		// This can lead to inconsistent undo / redo states if the sub-actions are not mergable
		if(canMergeSubActions () == false)
			return false;

		ForEach (*this, Action, action)
			if(!action->canMerge ())
				return false;
		EndFor	
	}			
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Action::merge (Action* other)
{
	if(hasSubActions () && canMergeSubActions ())
	{	
		// must be of same class
		if(!other->isClass (myClass ()))
			return false;

		// must be the same title in undo stack
		if(description != other->getDescription ())
			return false;

		// must have the same number of subactions >= 1
		int thisCount = countSubActions ();
		int otherCount = other->countSubActions ();
		if(thisCount == 0 || thisCount != otherCount)
			return false;

		if(!tryMergeSubActions (other, thisCount))
			return false;

		CCL_PRINTLN ("SubActions merged.")
		return true; // success :-)	
	}
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Action::canHaveSideEffects () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef Action::getDetailedDescription ()
{
	if(detailedDescription.isEmpty ())
		 describeDetails (detailedDescription);

	return detailedDescription;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Action::setDetailedDescription (StringRef details)
{
	detailedDescription = details;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Action::describeDetails (String& details)
{
	int count = 0;
	String lastDetail;

	ListForEachObject (subActions, Action, action)
		StringRef s (action->getDetailedDescription ());
		if(!s.isEmpty ())
		{
			// simple rule to avoid duplicate occurrences: ignore subsequent same descriptions
			if(s != lastDetail)
			{
				if(count == 4)
				{
					details << ", ...";
					return true;
				}
				if(count != 0)
					details << ", ";
				details << s;
				lastDetail = s;
				count++;
			}
		}
	EndFor
	return count > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Action::takeDescriptionFromSubAction ()
{
	bool isMultiAction = ccl_cast<MultiAction> (this);

	for(auto subAction : iterate_as<Action> (subActions))
	{
		// try deep for a MultiAction
		if(isMultiAction && subAction->getDescription ().isEmpty ())
			subAction->takeDescriptionFromSubAction ();

		if(!subAction->getDescription ().isEmpty ())
		{
			setDescription (subAction->getDescription ());
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Action::isDragable () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* Action::createIcon ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* Action::createDragObject ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Action::onManipulation ()
{
	ListForEachObject (subActions, Action, action)
		action->onManipulation ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Action::executeAll (IProgressNotify* progress)
{
	ProgressHandler progressHandler (*this, progress);
	return executeAllInternal (progressHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Action::undoAll (IProgressNotify* progress)
{
	ProgressHandler progressHandler (*this, progress);
	return undoAllInternal (progressHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Action::redoAll (IProgressNotify* progress)
{
	ProgressHandler progressHandler (*this, progress);
	return redoAllInternal (progressHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Action::executeAllInternal (ProgressHandler& progressHandler)
{
	if(isExecuted ())
		return true;

	LOG_ACTION ("execute")

	bool result = false;
	if(execute ())
		result = true;

	ListForEachObject (subActions, Action, action)
		progressHandler.nextAction (action);

		if(action->executeAllInternal (progressHandler))
			result = true;
		else
		{
			subActions.remove (action);
			action->release ();
		}
	EndFor
	if(result)
		setExecuted ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Action::undoAllInternal (ProgressHandler& progressHandler)
{
	LOG_ACTION ("undo")

	ListForEachObjectReverse (subActions, Action, action)
		progressHandler.nextAction (action);
		action->undoAllInternal (progressHandler);
	EndFor

	undo ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Action::redoAllInternal (ProgressHandler& progressHandler)
{
	redo ();

	ListForEachObject (subActions, Action, action)
		progressHandler.nextAction (action);
		action->redoAllInternal (progressHandler);
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Action::tryMergeSubActions (Action* other, int numActions)
{
	for(int i = 0; i < numActions; i++)
	{
		Action* thisAction = getAction (i);
		Action* otherAction = other->getAction (i);
		ASSERT (thisAction && otherAction)
		if(i == 0) // first merge has to succeed, otherwise we give up
		{
			if(!thisAction->merge (otherAction))
				return false;
		}
		else
		{
			// add as subaction if merge fails
			if(!thisAction->merge (otherAction))
			{
				if(isExecuted ())
					otherAction->executeAll ();
				addAction (return_shared (otherAction));
			}
		}
	}
	return true;
}

//************************************************************************************************
// MultiAction
//************************************************************************************************

DEFINE_CLASS (MultiAction, Action)

//////////////////////////////////////////////////////////////////////////////////////////////////

MultiAction::MultiAction (StringRef description)
: Action (description)
{
	canMergeSubActions (true);
}

//************************************************************************************************
// RestorePointAction
//************************************************************************************************

DEFINE_CLASS (RestorePointAction, Action)

//////////////////////////////////////////////////////////////////////////////////////////////////

RestorePointAction::RestorePointAction (StringRef description)
: Action (description),
  savedEditTime (0)
{
	savedRedoStack.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RestorePointAction::~RestorePointAction ()
{
	ASSERT (savedRedoStack.isEmpty ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RestorePointAction::execute ()
{
	CCL_DEBUGGER ("Restore point must not be executed!")
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RestorePointAction::saveRedo (const ObjectStack& redoStack)
{
	ASSERT (savedRedoStack.isEmpty ())
	savedRedoStack.add (redoStack, Container::kShare);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RestorePointAction::restoreRedo (ObjectStack& redoStack)
{
	redoStack.add (savedRedoStack, Container::kShare);
	savedRedoStack.removeAll ();
}
