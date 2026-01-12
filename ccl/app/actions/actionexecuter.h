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
// Filename    : ccl/app/actions/actionexecuter.h
// Description : Action Executer
//
//************************************************************************************************

#ifndef _ccl_actionexecuter_h
#define _ccl_actionexecuter_h

#include "ccl/app/actions/action.h"
#include "ccl/app/actions/iactioncontext.h"

#include "ccl/public/app/iactionjournal.h"

namespace CCL {

class ActionJournal;

//************************************************************************************************
// ActionExecuter
//************************************************************************************************

class ActionExecuter: public Object,
					  public IActionExecuter
{
public:
	DECLARE_CLASS (ActionExecuter, Object)
	DECLARE_METHOD_NAMES (ActionExecuter)

	ActionExecuter (IActionContext* context = nullptr);
	~ActionExecuter ();

	static ActionExecuter* createInstance (IActionContext* context, UIDRef cid);

	IActionContext* getActionContext () const;
	virtual void setActionContext (IActionContext* context);
	operator IActionContext* () const { return getActionContext (); }

	virtual bool execute (Action* action);

	template <typename Lambda>
	bool executeAdHoc (const Lambda& perform); ///< execute the given lambda as part of an action sequence (only once, no undo)

	bool isMultiplePending () const;
	bool isPerformingAction () const;
	bool isInAction () const; ///< isPerformingAction or isMultiplePending
	virtual Action* beginMultiple (StringRef description, StringRef details = nullptr);
	virtual bool endMultiple (bool cancel = false);
	Action* beginMultiAction (Action* multiAction);
	void setMultipleDescription (StringRef description, StringRef details = nullptr); ///< for a pending multi action

	void preventMerge (); ///< prevent merge in current multi action

	void takeFlags (const ActionExecuter& from);

	/** @see ActionJournal::ExecutionFlags */
	PROPERTY_BOOL (executeImmediately, ExecuteImmediately)
	PROPERTY_BOOL (suppressSideEffects, SuppressSideEffects) 
	PROPERTY_BOOL (directEditMode, DirectEditMode)

	struct JournalDisabler;

	// IActionExecuter
	tbool CCL_API beginMultiAction (StringRef description, StringRef details = nullptr) override;
	tbool CCL_API endMultiAction (tbool cancel = false) override;
	void CCL_API setExecuteActionImmediately (tbool state) override;
	tbool CCL_API isExecuteActionImmediately () const override;
	void CCL_API setJournalEnabled (tbool enabled) override;	
	tbool CCL_API isJournalEnabled () const override;
	
	CLASS_INTERFACE (IActionExecuter, Object)

protected:
	IActionContext* actionContext;

	class MergeBlocker;

	ActionJournal* getJournal () const;
	int getExecutionFlags () const;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// ActionExecuter::JournalDisabler
//************************************************************************************************

struct ActionExecuter::JournalDisabler
{
	ActionExecuter& executor;
	bool wasEnabled;

	JournalDisabler (ActionExecuter& executor, bool disable = true)
	: executor (executor),
	  wasEnabled (executor.isJournalEnabled () != 0)
	{ if(disable) executor.setJournalEnabled (false); }

	~JournalDisabler ()
	{ executor.setJournalEnabled (wasEnabled); }
};

//************************************************************************************************
// ActionCatcher
/** Catches all actions executed via the ActionJournal in a separate MultiAction. */
//************************************************************************************************

class ActionCatcher: private ActionExecuter
{
public:
	DECLARE_CLASS (ActionCatcher, ActionExecuter)

	ActionCatcher (IActionContext* context = nullptr);
	~ActionCatcher ();

	/// execute collected actions now, showing a progress dialog
	void executeWithProgress (StringRef text);

	/// detach the pending multiaction, e.g. for using it as sub action of another action
	Action* detachAction ();

private:
	Action* pendingAction;
};

//************************************************************************************************
// OpenEndActionHandler
/** Supports long-term actions with indeterminate end. 
    Creates multi action in private journal and pushes this action to target journal when finished.
	The target journal must be provided via the passed ActionExecuter. */
//************************************************************************************************

class OpenEndActionHandler: public Object,
						    public IActionContext
{
public:
	DECLARE_CLASS (OpenEndActionHandler, Object)

	OpenEndActionHandler ();
	~OpenEndActionHandler ();

	bool begin (ActionExecuter& targetExecutor, StringRef description, StringRef details = nullptr);
	void end (ActionExecuter& targetExecutor, bool cancel);

	// IActionContext
	ActionJournal* getActionJournal () const override;

	CLASS_INTERFACES (Object)
private:
	ActionJournal* actionJournal;
	SharedPtr<Action> action;
	SharedPtr<IActionContext> targetActionContext;
};

//************************************************************************************************
// ImmediateMultiActionScope
/** Collects actions in a beginMultiple / endMultiple block,
	forces "immediate" execution of the resulting MultiAction when done. */
//************************************************************************************************

struct ImmediateMultiActionScope
{
	IActionContext* context;

	ImmediateMultiActionScope (IActionContext* context);
	~ImmediateMultiActionScope ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// ActionExecuter inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename Lambda>
bool ActionExecuter::executeAdHoc (const Lambda& perform)
{ return execute (NEW AdHocAction<Lambda> (perform)); }

} // namespace CCL

#endif // _ccl_actionexecuter_h
