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
// Filename    : ccl/app/actions/actionexecuter.cpp
// Description : Action Executer
//
//************************************************************************************************

#include "ccl/app/actions/actionexecuter.h"

#include "ccl/app/actions/action.h"
#include "ccl/app/actions/actionjournal.h"
#include "ccl/app/actions/iactioncontext.h"

#include "ccl/base/kernel.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/base/iprogress.h"
#include "ccl/public/gui/framework/iprogressdialog.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// ActionExecuter::MergeBlocker
/** Used as first sub action of a MultiAction to prevent trying to merge actions.
	E.g. when the first	merge in tryMergeSubActions would succeed, but subsequent actions can't be merged. */
//************************************************************************************************

class ActionExecuter::MergeBlocker: public Action
{
protected:
	// Action
	bool execute () override { return false; }
};

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (IActionContext, 0x63ffd68e, 0xf46d, 0x43e6, 0xbe, 0x26, 0x14, 0x57, 0x33, 0x4a, 0x23, 0x77)

//************************************************************************************************
// MultiActionScope
//************************************************************************************************

MultiActionScope::MultiActionScope (IActionContext* context, StringRef description)
: context (context)
{
	if(context && context->getActionJournal ())
		context->getActionJournal ()->beginMultiple (description);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MultiActionScope::MultiActionScope (IActionContext* context, StringRef description, StringRef details)
: context (context)
{
	if(context && context->getActionJournal ())
		context->getActionJournal ()->beginMultiple (description, details);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MultiActionScope::MultiActionScope (IActionContext* context, Action* multiAction)
: context (context)
{
	ASSERT (multiAction)
	if(!multiAction)
	{
		this->context = nullptr;
		return;
	}

	if(context && context->getActionJournal ())
		context->getActionJournal ()->beginMultiple (multiAction);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MultiActionScope::~MultiActionScope ()
{
	if(context && context->getActionJournal ())
		context->getActionJournal ()->endMultiple ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiActionScope::cancel ()
{
	if(context && context->getActionJournal ())
	{
		context->getActionJournal ()->endMultiple (true);
		context = nullptr;
	}
}

//************************************************************************************************
// ImmediateMultiActionScope
//************************************************************************************************

ImmediateMultiActionScope::ImmediateMultiActionScope (IActionContext* context)
: context (context)
{
	if(context && context->getActionJournal ())
	{
		ASSERT (context->getActionJournal ()->isMultiplePending ()) // should be used inside another multi action scope
		context->getActionJournal ()->beginMultiple (String::kEmpty);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ImmediateMultiActionScope::~ImmediateMultiActionScope ()
{
	if(context && context->getActionJournal ())
		context->getActionJournal ()->endMultiple (false, ActionJournal::kExecuteImmediatly); // immediate
}

//************************************************************************************************
// ActionExecuter
//************************************************************************************************

ActionExecuter* ActionExecuter::createInstance (IActionContext* context, UIDRef cid)
{
	ASSERT (cid.isValid ())
	AutoPtr<Object> object = Kernel::instance ().getClassRegistry ().createObject (cid);
	if(object)
	{
		ActionExecuter* executer = ccl_cast<ActionExecuter> (object);
		if(executer)
		{
			executer->setActionContext (context);
			executer->retain ();
			return executer;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (ActionExecuter, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ActionExecuter::ActionExecuter (IActionContext* _context)
: actionContext (nullptr),
  executeImmediately (false),
  suppressSideEffects (false),
  directEditMode (false)
{
	if(_context)
		setActionContext (_context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ActionExecuter::~ActionExecuter ()
{
	setActionContext (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IActionContext* ActionExecuter::getActionContext () const
{
	return actionContext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActionExecuter::setActionContext (IActionContext* _context)
{
	take_shared (actionContext, _context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ActionJournal* ActionExecuter::getJournal () const
{
	return actionContext ? actionContext->getActionJournal () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ActionExecuter::isMultiplePending () const
{
	ActionJournal* journal = getJournal ();
	if(journal)
		return journal->isMultiplePending ();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ActionExecuter::isPerformingAction () const
{
	ActionJournal* journal = getJournal ();
	if(journal)
		return journal->isPerformingAction () != 0;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ActionExecuter::isInAction () const
{
	ActionJournal* journal = getJournal ();
	if(journal)
		return journal->isMultiplePending () || journal->isPerformingAction () != 0;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActionExecuter::setMultipleDescription (StringRef description, StringRef details)
{
	ActionJournal* journal = getJournal ();
	Action* multiAction = journal ? journal->peekMultiple () : nullptr;
	if(multiAction)
	{
		multiAction->setDescription (description);

		if(!details.isEmpty ())
			multiAction->setDetailedDescription (details);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ActionExecuter::getExecutionFlags () const
{
	int flags = 0;
	if(executeImmediately)
		flags |= ActionJournal::kExecuteImmediatly;
	if(suppressSideEffects)
		flags |= ActionJournal::kExecuteWithoutSideEffects;
	if(directEditMode)
		flags |= ActionJournal::kExecuteDirectEdit;
	return flags;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ActionExecuter::execute (Action* action)
{
	ASSERT (action != nullptr)
	if(!action)
		return false;

	ActionJournal* journal = getJournal ();
	if(journal)
	{
		return journal->execute (action, getExecutionFlags ());
	}

	AutoPtr<Action> releaser (action);
	return action->executeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Action* ActionExecuter::beginMultiple (StringRef description, StringRef details)
{
	ActionJournal* journal = getJournal ();
	ASSERT (journal != nullptr)
	if(journal)
		return journal->beginMultiple (description, details);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ActionExecuter::endMultiple (bool cancel)
{
	ActionJournal* journal = getJournal ();
	ASSERT (journal != nullptr)
	if(journal)
		return journal->endMultiple (cancel, getExecutionFlags ());
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Action* ActionExecuter::beginMultiAction (Action* multiAction)
{
	ActionJournal* journal = getJournal ();
	ASSERT (journal != nullptr)
	if(journal)
		return journal->beginMultiple (multiAction);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ActionExecuter::beginMultiAction (StringRef description, StringRef details)
{
	return beginMultiple (description, details) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ActionExecuter::endMultiAction (tbool cancel)
{
	return endMultiple (cancel);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ActionExecuter::setExecuteActionImmediately (tbool state)
{
	setExecuteImmediately (state != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ActionExecuter::isExecuteActionImmediately () const
{
	return isExecuteImmediately ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ActionExecuter::setJournalEnabled (tbool enabled)
{
	ActionJournal* journal = getJournal ();
	ASSERT (journal != nullptr)
	if(journal)
		journal->setEnabled (enabled != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ActionExecuter::isJournalEnabled () const
{
	ActionJournal* journal = getJournal ();
	ASSERT (journal != nullptr)
	return journal ? journal->isEnabled () : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActionExecuter::preventMerge ()
{
	ActionJournal* journal = getJournal ();
	ASSERT (journal != nullptr)
	if(journal)
	{
		Action* multiAction = journal->peekMultiple ();
		ASSERT (multiAction)
		if(multiAction)
		{
			multiAction->preventMerge ();

			ASSERT (!multiAction->hasSubActions ())
			execute (NEW MergeBlocker);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActionExecuter::takeFlags (const ActionExecuter& from)
{
	setExecuteImmediately (from.isExecuteImmediately ());
	setSuppressSideEffects (from.isSuppressSideEffects ());
	setDirectEditMode (from.isDirectEditMode ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ActionExecuter::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "executeImmediately")
	{
		var = isExecuteImmediately ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ActionExecuter::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "executeImmediately")
	{
		setExecuteImmediately (var.asBool ());
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ActionExecuter)
	DEFINE_METHOD_NAME ("beginMultiple")
	DEFINE_METHOD_NAME ("endMultiple")
	DEFINE_METHOD_NAME ("setJournalEnabled")
	DEFINE_METHOD_NAME ("isJournalEnabled")
END_METHOD_NAMES (ActionExecuter)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ActionExecuter::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "beginMultiple")
	{
		String description (msg[0].asString ());
		beginMultiple (description);
		return true;
	}
	else if(msg == "endMultiple")
	{
		bool cancel = false;
		if(msg.getArgCount () > 0)
			cancel = msg[0].asBool ();
		endMultiple (cancel);
		return true;
	}
	else if(msg == "setJournalEnabled")
	{
		bool enabled = true;
		if(msg.getArgCount () > 0)
			enabled = msg[0].asBool ();
		setJournalEnabled (enabled);
		return true;
	}
	else if (msg == "isJournalEnabled")
	{
		returnValue = isJournalEnabled ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// ActionCatcher
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ActionCatcher, ActionExecuter)

//////////////////////////////////////////////////////////////////////////////////////////////////

ActionCatcher::ActionCatcher (IActionContext* context)
: ActionExecuter (context)
{
	pendingAction = return_shared (beginMultiple (nullptr));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ActionCatcher::~ActionCatcher ()
{
	if(pendingAction)
	{
		endMultiple ();
		pendingAction->release ();
		pendingAction = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Action* ActionCatcher::detachAction ()
{
    if(!pendingAction)
        return nullptr;

    pendingAction->executeAll ();

    // transfer all subActions to a new MultiAction (prevent them from being undone in the cancel step below)
    AutoPtr<Action> multiAction (NEW MultiAction (pendingAction->getDescription ()));
    ForEach (*pendingAction, Action, subAction)
        subAction->retain ();
        multiAction->addAction (subAction);
    EndFor
    pendingAction->removeSubActions ();

    // cancel (remove pending from journal)
    endMultiple (true);

    ASSERT (pendingAction->getRetainCount () == 1)
    pendingAction->release ();
    pendingAction = nullptr;

    multiAction->setExecuted (true);
    return multiAction.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActionCatcher::executeWithProgress (StringRef text)
{
	if(!pendingAction)
		return;

	ActionJournal* journal = getJournal ();
	ASSERT (journal)
	if(!journal)
		return;

	ProgressNotifyScope progressScope (journal->getProgressProvider (), text, false);

	pendingAction->executeAll (progressScope);
	pendingAction->setExecuted (true);

	endMultiple ();
	pendingAction->release ();
	pendingAction = nullptr;
}

//************************************************************************************************
// OpenEndActionHandler
//************************************************************************************************

DEFINE_CLASS_HIDDEN (OpenEndActionHandler, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

OpenEndActionHandler::OpenEndActionHandler ()
: action (nullptr),
  actionJournal (NEW ActionJournal)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

OpenEndActionHandler::~OpenEndActionHandler ()
{
	safe_release (actionJournal);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ActionJournal* OpenEndActionHandler::getActionJournal () const
{
	return const_cast<ActionJournal*> (actionJournal);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API OpenEndActionHandler::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (IActionContext)

	ASSERT (targetActionContext)

	if(targetActionContext)
		return targetActionContext->queryInterface (iid, ptr);

	return Object::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenEndActionHandler::begin (ActionExecuter& targetExecutor, StringRef description, StringRef details)
{
	targetActionContext = targetExecutor.getActionContext ();
	ASSERT (targetActionContext)

	targetExecutor.setActionContext (this);

	action = targetExecutor.beginMultiple (description, details);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenEndActionHandler::end (ActionExecuter& targetExecutor, bool cancel)
{
	if(action)
	{
		bool success = targetExecutor.endMultiple (cancel);

		if(targetActionContext && cancel == false && success && action->isExecuted ())
			if(ActionJournal* targetActionJournal = targetActionContext->getActionJournal ())
			{
				action->retain ();
				targetActionJournal->execute (action);
			}

		action = nullptr;
	}

	targetExecutor.setActionContext (targetActionContext);
}

