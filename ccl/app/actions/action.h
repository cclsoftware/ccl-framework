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
// Filename    : ccl/app/actions/action.h
// Description : Undoable Action
//
//************************************************************************************************

#ifndef _ccl_action_h
#define _ccl_action_h

#include "ccl/public/text/cclstring.h"
#include "ccl/public/base/datetime.h"

#include "ccl/base/collections/objectlist.h"
#include "ccl/base/collections/objectstack.h"

namespace CCL {

interface IProgressNotify;

//************************************************************************************************
// Action
/** Base class for undoable action. */
//************************************************************************************************

class Action: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (Action, Object)

	Action (StringRef description = nullptr);

	PROPERTY_STRING (description, Description)
	PROPERTY_OBJECT (DateTime, time, Time)

	StringRef getDetailedDescription ();
	void setDetailedDescription (StringRef details);

	bool executeAll (IProgressNotify* progress = nullptr);
	bool undoAll (IProgressNotify* progress = nullptr);
	bool redoAll (IProgressNotify* progress = nullptr);

	virtual bool isDragable () const;
	virtual IUnknown* createIcon ();
	virtual IUnknown* createDragObject ();

	virtual bool merge (Action* action);
	void preventMerge ();
	virtual bool canMerge () const;
	virtual bool canHaveSideEffects () const;

	virtual void onManipulation (); ///< called during direct manipulation, eg. for adjusting side effects

	PROPERTY_FLAG (flags, 1<<0, isExecuted)				///< supresses execution of this and all subActions. Useful for building a container of direct actions
	PROPERTY_FLAG (flags, 1<<1, isSideEffectsChecked)	///< used by SideEffectRegistry
	void setExecuted (bool state = true);
	void setSideEffectsChecked (bool state = true);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Sub actions
	//////////////////////////////////////////////////////////////////////////////////////////////

	bool hasSubActions () const;
	int countSubActions () const;
	Action* getAction (int index) const;
	Action* getLastAction () const;

	void addAction (Action* action);
	void addActionDuringIteration (Action* action);
	bool addActionAndExecute (Action* action);					///< executes the subAction immediately
	void insertAction (Action* action, int index);
	void insertActionAndExecute (Action* action, int index);	///< executes the subAction immediately
	bool removeAction (Action* action);
	void removeSubActions ();
	void takeDescriptionFromSubAction ();

	const ObjectList& getSubActions () const;
	Iterator* newIterator () const;

protected:
	virtual bool execute () = 0;
	virtual bool undo ();
	virtual bool redo ();

	bool tryMergeSubActions (Action* other, int numActions);

	struct ProgressHandler;
	bool executeAllInternal (ProgressHandler& progressHandler);
	bool undoAllInternal (ProgressHandler& progressHandler);
	bool redoAllInternal (ProgressHandler& progressHandler);

	/// the detailed description can be set diretcly (e.g. from outside), or determined by overriding this method
	virtual bool describeDetails (String& details);

	PROPERTY_FLAG (flags, 1<<2, isMergeDisabled)
	PROPERTY_FLAG (flags, 1<<3, canMergeSubActions) ///< derived class can set this to allow merging of subactions

private:
	String detailedDescription;
	ObjectList subActions;
	int flags;
};

//************************************************************************************************
// MultiAction
/** Action that is only used as container for sub-actions. */
//************************************************************************************************

class MultiAction: public Action
{
public:
	DECLARE_CLASS (MultiAction, Action)

	MultiAction (StringRef description = nullptr);

protected:
	// Action
	bool execute () override { return false; } // (this parent action does nothing itself)
	bool undo () override { return true; }
	bool redo () override { return true; }
};

//************************************************************************************************
// AdHocAction
/** Action that performs code given as a lambda (no undo, never added to the journal). */
//************************************************************************************************

template <typename Lambda>
class AdHocAction: public Action
{
public:
	AdHocAction (const Lambda& perform)
	: perform (perform)
	{}

protected:
	Lambda perform;

	// Action
	bool execute () override { perform (); return false; } // (only once, don't journal)
	bool undo () override { ASSERT (0) return true; }
	bool redo () override { ASSERT (0) return true; }
};

//************************************************************************************************
// RestorePointAction
/** Restore point in undo journal. */
//************************************************************************************************

class RestorePointAction: public Action
{
public:
	DECLARE_CLASS (RestorePointAction, Action)

	RestorePointAction (StringRef description = nullptr);
	~RestorePointAction ();

	PROPERTY_VARIABLE (int64, savedEditTime, SavedEditTime)

	void saveRedo (const ObjectStack& redoStack);
	void restoreRedo (ObjectStack& redoStack);

protected:
	ObjectStack savedRedoStack;

	// Action
	bool execute () override;
};

//************************************************************************************************
// SymmetricAction
/** Class template for actions that perform the same code on execute / undo / redo
	(typically "swapping" a state between the data model and the action).
	Simplifies the implementation to one apply () method, while taking care of the different
	return value rules of execute vs. undo / redo. Also simplifies deriving "direct" action variants. */
//************************************************************************************************

template<class BaseAction>
class SymmetricAction: public BaseAction
{
public:
	using BaseAction::BaseAction;

protected:
	virtual bool apply () = 0;

	// Action
	bool execute () override;
	bool undo () override;
	bool redo () override;
};

//************************************************************************************************
// ActionDescriptions
/** Macros for implementing simple fuctions that return a description in plural or singular.

	namespace ActionDescriptions
	{
		StringRef DoSomething (bool singular = true);
		...
	}
	
	DEFINE_ACTION_DESCRIPTION (DoSomething, DoSomething, DoSomethings)
*/
//************************************************************************************************

#define DEFINE_ACTION_DESCRIPTION(Method, singularStr, pluralStr) \
	StringRef ActionDescriptions::Method (bool singular) { return singular ? XSTR(singularStr) : XSTR(pluralStr); }

#define DEFINE_ACTION_DESCRIPTION_SIMPLE(Method, str) \
	StringRef ActionDescriptions::Method () { return XSTR(str); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void Action::setExecuted (bool state) { isExecuted (state); }
inline void Action::setSideEffectsChecked (bool state) { isSideEffectsChecked (state); }

template<class BaseAction> inline bool SymmetricAction<BaseAction>::execute () { return this->apply (); }
template<class BaseAction> inline bool SymmetricAction<BaseAction>::undo () { this->apply (); return true; }
template<class BaseAction> inline bool SymmetricAction<BaseAction>::redo () { this->apply (); return true; }

} // namespace CCL

#endif // _ccl_action_h
