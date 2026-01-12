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
// Filename    : ccl/app/editing/tasks/edittaskhandler.h
// Description : Edit Task Handler
//
//************************************************************************************************

#ifndef _ccl_edittaskhandler_h
#define _ccl_edittaskhandler_h

#include "ccl/public/app/iedittask.h"

#include "ccl/app/editing/tasks/edittaskdescription.h"

namespace CCL {

class ParamContainer;
class EditTaskHandler;
interface IActionContext;
interface IView;

//************************************************************************************************
// EditTaskContext
/** Edit task context. */
//************************************************************************************************

class EditTaskContext: public Attributes
{
public:
	DECLARE_CLASS (EditTaskContext, Attributes)
	DECLARE_METHOD_NAMES (EditTaskContext)

	EditTaskContext (EditTaskHandler* handler = nullptr);

	void setObject (StringID name, IObject* object);

	bool isSilentMode () const;
	Attributes* getArguments () const;
	void restore (bool force = false);

	const EditTaskDescription& getTaskDescription () const;

protected:
	EditTaskHandler* handler;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// EditTaskCandidate
/** Edit task candidate. */
//************************************************************************************************

class EditTaskCandidate: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (EditTaskCandidate, Object)

	virtual void prepare (EditTaskContext& context) = 0;

	virtual void unprepare (EditTaskContext& context) {}

	virtual IActionContext* getActionContext () = 0;
};

//************************************************************************************************
// EditTaskHandler
/** Helper class to run edit tasks. */
//************************************************************************************************

class EditTaskHandler: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (EditTaskHandler, Object)
	DECLARE_METHOD_NAMES (EditTaskHandler)

	/** Handler takes ownership of task. */
	EditTaskHandler (IEditTask& task, const EditTaskDescription& description);
	~EditTaskHandler ();

	/** Check if task can be created from IUnknown. */
	static bool canCreateTask (EditTaskDescription& description, IUnknown* object);

	/** Create task from IUnknown. */
	static EditTaskHandler* createTask (IUnknown* object);

	/** Create task from description. */
	static EditTaskHandler* createTask (const EditTaskDescription& description);

	/** True inside runTask */
	static bool isRunningTask ();

	PROPERTY_BOOL (silentMode, SilentMode)
	PROPERTY_BOOL (persistent, Persistent)
	PROPERTY_SHARED_AUTO (Attributes, savedValues, SavedValues)
	
	/** Get task description. */
	const EditTaskDescription& getDescription () const;

	/** Prepare task (but not perform). */
	tresult prepareTask (IObject* editor);

	/** Create view for task (must be prepared first). */
	IView* createTaskView ();

	/** Perform task (must be prepared first). */
	tresult performTask (const Container& candidates, IObject* editor);

	/** Run task (prepare + perform). */
	tresult runTask (const Container& candidates, IObject* editor);

	/** Run dialog only (used by EditTaskArgumentUI). */
	tresult runArgumentDialog ();

protected:
	friend class EditTaskContext;
	struct DragInfo;

	IEditTask& task;
	ParamContainer* parameters;
	EditTaskDescription taskDescription;
	String taskSettingsID;
	bool restored;
	static bool runningTask;

	void storeValues (bool global);
	void restoreValues (bool force = false);
};

//************************************************************************************************
// EditTaskArgumentUI
//************************************************************************************************

class EditTaskArgumentUI: public Object
{
public:
	DECLARE_CLASS (EditTaskArgumentUI, Object)
	DECLARE_METHOD_NAMES (EditTaskArgumentUI)

	tresult runDialog (Attributes& arguments, UIDRef cid);

protected:
	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline const EditTaskDescription& EditTaskContext::getTaskDescription () const
{ return handler->getDescription (); }

} // namespace CCL

#endif // _ccl_edittaskhandler_h
