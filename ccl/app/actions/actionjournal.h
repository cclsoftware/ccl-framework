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
// Filename    : ccl/app/actions/actionjournal.h
// Description : Action Journal
//
//************************************************************************************************

#ifndef _ccl_actionjournal_h
#define _ccl_actionjournal_h

#include "ccl/base/storage/configuration.h"

#include "ccl/base/collections/objectstack.h"

#include "ccl/public/app/iactionjournal.h"

namespace CCL {

class Action;
class LogBuffer;
interface IProgressProvider;

//************************************************************************************************
// ActionJournal
/** Action journal is keeping track of undoable actions. */
//************************************************************************************************

class ActionJournal: public Object,
					 public IActionJournal
{
public:
	DECLARE_CLASS (ActionJournal, Object)

	ActionJournal ();
	~ActionJournal ();
	
	DECLARE_STRINGID_MEMBER (kExecuted)
	DECLARE_STRINGID_MEMBER (kUndone)
	DECLARE_STRINGID_MEMBER (kRedone)
	DECLARE_STRINGID_MEMBER (kWillRedo)
	DECLARE_STRINGID_MEMBER (kRemovedAll)
	DECLARE_STRINGID_MEMBER (kUndoReduced)
	DECLARE_STRINGID_MEMBER (kMerged)
	DECLARE_STRINGID_MEMBER (kSquashed)

	PROPERTY_BOOL (enabled, Enabled)
	PROPERTY_BOOL (undoRedoSuspended, UndoRedoSuspended)
	PROPERTY_POINTER (IProgressProvider, progressProvider, ProgressProvider) ///< not set by default, can be set to getStandardProgressProvider or custom provider

	enum ExecutionFlags 
	{ 
		kExecuteImmediatly = 1<<0,         ///< the action is executed immediatly, even if a multi-action was started. otherwise endMultiple will trigger the actual execution
		kExecuteWithoutSideEffects = 1<<1, ///< suppress side effect handling. side effects are also not performed, if the journal is disabled.
		kExecuteDirectEdit = 1<<2          ///< in combination with kExecuteImmediatly, the journal will try to merge the action with the recent action of a running multi-action
	};

	bool execute (Action* action, int executionFlags = 0); ///< see ExecutionFlags

	bool canUndo () const;
	bool canRedo () const;
	bool undo ();
	bool redo ();

	Action* peekUndo () const;
	Action* peekRedo () const;
	Iterator* newUndoIterator () const;
	Iterator* newRedoIterator () const;

	tbool CCL_API isPerformingAction () const override;
	bool isExecutingAction () const;
	bool isRestorePending () const;
	Action* addRestorePoint ();
	bool rollback (Action* restorePoint);

	void beginTransaction (StringRef title);
	void endTransaction ();

	void removeAll ();

	bool isDirty () const;			///< true if Actions were executed
	bool isModified () const;		///< true if modified after last save
	void setSavedNow ();			///< set 'saved' timestamp to now
	int64 CCL_API getLastEditTime () const override;	///< [IActionJournal] get timestamp of last edit

	bool isMultiplePending () const;
	Action* peekMultiple ();
	Action* beginMultiple (StringRef description = nullptr, StringRef details = nullptr);
	Action* beginMultiple (Action* multiAction);
	bool endMultiple (bool cancel = false, int executionFlags = 0); ///< see ExecutionFlags

	const LogBuffer& getLogBuffer () const;

	CLASS_INTERFACE (IActionJournal, Object)

	static String& getUndoString (String& string, ActionJournal* journal = nullptr);
	static String& getRedoString (String& string, ActionJournal* journal = nullptr);
	static IProgressProvider* getStandardProgressProvider ();

protected:
	static const Configuration::IntValue undoStackLimit;
	class ProgressProvider;
	class Transaction;

	ObjectStack undoStack;
	ObjectStack redoStack;
	ObjectStack multiStack;
	int undoCount;
	int64 lastEditTime;
	int64 lastSaveTime;
	LogBuffer& logBuffer;
	bool signalSuspended;
	Action* executingAction;
	Action* sideEffectAction;
	bool inUndoRedo;
	Transaction* activeTransaction;

	void signalAction (StringID messageId, Action* action);
	void logAction (const char* prefix, const Action* action);

	// IActionJournal
	tbool CCL_API canUndoLastEdit () const override { return canUndo (); }
	tbool CCL_API canRedoLastEdit () const override { return canRedo (); }
	tbool CCL_API undoLastEdit () override { return undo (); }
	tbool CCL_API redoLastEdit () override { return redo (); }
};

//************************************************************************************************
// ActionJournalDisabler
//************************************************************************************************

struct ActionJournalDisabler
{
	ActionJournal* journal;
	bool wasEnabled;

	ActionJournalDisabler (ActionJournal* journal, bool disable = true)
	: journal (journal),
	  wasEnabled (journal ? journal->isEnabled () : false)
	{ if(journal && disable) journal->setEnabled (false); }

	~ActionJournalDisabler ()
	{ if(journal) journal->setEnabled (wasEnabled); }
};

} // namespace CCL

#endif // _ccl_actionjournal_h
