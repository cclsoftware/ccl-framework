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
// Filename    : ccl/app/utilities/batchoperation.h
// Description : Batch Operation
//
//************************************************************************************************

#ifndef _ccl_batchoperation_h
#define _ccl_batchoperation_h

#include "ccl/app/component.h"

#include "ccl/base/objectconverter.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/collections/objectlist.h"
#include "ccl/base/asyncoperation.h"

namespace CCL {

interface IWindow;
interface IProgressNotify;

//************************************************************************************************
// BatchOperation
/** Manages a list of tasks. */
//************************************************************************************************

class BatchOperation: public AsyncCompletionHandler,
					  public IContainer
{
public:
	class Task;

	BatchOperation ();
	~BatchOperation ();

	// Tasks
	void addTask (Task* task);
	bool isEmpty () const;
	Iterator* newIterator () const;
	void removeAll ();
	int countTasks () const;
	const ObjectList& getTasks () const;

	/// Run tasks: returns true if all tasks succeeded and not canceled
	bool run (IProgressNotify* progress);
	bool run (StringRef progressTitle, IWindow* parentWindow = nullptr); ///< creates progress dialog
	IAsyncOperation* runAsync (IProgressNotify* progress);
	IAsyncOperation* runAsync (StringRef progressTitle, IWindow* parentWindow = nullptr); ///< creates progress dialog
	void cancel ();

	// Get result
	tresult getResult () const;
	bool wasCanceled () const;

	// Configuration
	PROPERTY_BOOL (cancelEnabled, CancelEnabled)			///< enabled by default
	PROPERTY_BOOL (modalProgress, ModalProgress)			///< request a modal (blocking) progress dialog (default: false); only for asynchronous usage, if the platform supports it
	PROPERTY_STRING (stepCountPattern, StepCountPattern)	///< for progress info, default: "File %(1) of %(2)"

	CLASS_INTERFACE (IContainer, AsyncCompletionHandler)

protected:
	ObjectList tasks;
	tresult totalResult;

	struct RunningState;
	RunningState* state;

	bool beginBatch (IProgressNotify* progress);
	bool endBatch ();
	bool beginTask (Task* task);
	void endTask (Task* task, bool success);

	bool startNextTaskAsync ();

	// IAsyncCompletionHandler
	void CCL_API onCompletion (IAsyncOperation& operation) override;

	// IContainer
	IUnknownIterator* CCL_API createIterator () const override { return newIterator (); }

	virtual bool prepare ();								///< called before progress dialog opens, return false to cancel
	virtual IAsyncOperation* prepareAsync ();
	virtual void onCanceled () {}
	virtual void onFinished (bool allSucceeded) {}
	virtual void postRun (IProgressNotify* progress, bool canceled) {}
};

//************************************************************************************************
// BatchOperation::Task
//************************************************************************************************

class BatchOperation::Task: public CCL::Object
{
public:
	DECLARE_CLASS_ABSTRACT (Task, Object)

	Task (): state (kWaiting), progressInfo (true) {}

	PROPERTY_OBJECT (Url, sourcePath, SourcePath)
	PROPERTY_OBJECT (Url, destPath, DestPath)
	PROPERTY_VARIABLE (int, state, State)
	PROPERTY_STRING (title, Title)							///< task title in "TaskList" dialog
	PROPERTY_STRING (errorText, ErrorText)
	PROPERTY_SHARED_AUTO (Object, userData, UserData)

	enum States { kWaiting, kSucceeded, kFailed, kDisabled };

	bool hasProgressInfo () const;
	void hasProgressInfo (bool state);

	bool succeeded () const;
	bool failed    () const;
	bool isDisabled () const;

	void disable ();										///< task will be skipped, even prepare will not be called
	virtual String getProgressText ();						///< progress text for the whole task
	virtual bool prepare ();								///< called before performing, returns true if it should be performed.
	virtual bool perform (IProgressNotify* progress);		///< perform the actual work
	virtual IAsyncOperation* performAsync ();				///< perform asynchronously
	virtual void abort ();									///< called immediately when user hits cancel button

	virtual void onFinished ();								///< notification after all task are finished.
	virtual void onCanceled ();								///< notification when user has canceled.

protected:
	bool progressInfo;

	// use in derived class:
	String buildTextFromFileName (StringRef pattern, UrlRef path); ///< pattern should have variable %(1) for filename
	String buildTextFromSourceFileName (StringRef pattern);
};

//************************************************************************************************
// AsyncBatchTask
/** Use IAsyncCall as part of batch operation. */
//************************************************************************************************

class AsyncBatchTask: public BatchOperation::Task
{
public:
	DECLARE_CLASS_ABSTRACT (AsyncBatchTask, Task)

	AsyncBatchTask (IAsyncCall* asyncCall);

	static Variant runModal (IAsyncCall* call, StringRef title, StringRef text, IWindow* parentWindow = nullptr); ///< run single IAsyncCall in modal mode

	template <typename T>
	static Variant runModal (const T& lambda, StringRef title, StringRef text, IWindow* parentWindow = nullptr)
	{
		return runModal (static_cast<IAsyncCall*> (AsyncCall::make (lambda)), title, text, parentWindow);
	}

	PROPERTY_OBJECT (Variant, result, Result) ///< stores result of async call
	PROPERTY_STRING (progressText, ProgressText)

	// BatchOperation::Task
	String getProgressText () override;
	IAsyncOperation* performAsync () override;
	void abort () override;

protected:
	SharedPtr<IAsyncCall> asyncCall;
	SharedPtr<IAsyncOperation> pendingOperation;

	void onCompletion (IAsyncOperation& operation);
};

//************************************************************************************************
// DummyBatchTask
//************************************************************************************************

class DummyBatchTask: public BatchOperation::Task
{
public:
	DECLARE_CLASS_ABSTRACT (DummyBatchTask, Task)

	DummyBatchTask (UrlRef url) {setSourcePath (url);}
	bool perform (IProgressNotify* progress) override {return true;}
};

//************************************************************************************************
// BatchTaskFilePromise
/** Wraps a batch task as a FilePromise. */
//************************************************************************************************

class BatchTaskFilePromise: public FilePromise
{
public:
	BatchTaskFilePromise (BatchOperation::Task* task);

	// FilePromise
	tbool CCL_API getFileName (String& fileName) const override;
	tbool CCL_API getFileType (CCL::FileType& fileType) const override;
	tresult CCL_API createFile (UrlRef destPath, CCL::IProgressNotify* progress) override;

private:
	SharedPtr<BatchOperation::Task> task;
};

//************************************************************************************************
// BatchOperationComponent
/** Provides a ListView model "taskList". */
//************************************************************************************************

class BatchOperationComponent: public Component
{
public:
	BatchOperationComponent (BatchOperation& batchOperation);
	~BatchOperationComponent ();

	ParamList& getParams ();

	bool runListDialog (StringID formName = nullptr, StringRef headerText = nullptr);

	// Component
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;

protected:
	class TaskListModel;
	class TaskListItem;
	BatchOperation& batchOperation;
	TaskListModel* taskListModel;

	TaskListModel* createListModel () const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool BatchOperation::isEmpty () const { return tasks.isEmpty (); }
inline const ObjectList& BatchOperation::getTasks () const { return tasks; }
inline void BatchOperation::removeAll () { tasks.removeAll (); }
inline int BatchOperation::countTasks () const { return tasks.count (); }
inline tresult BatchOperation::getResult () const { return totalResult; }
inline bool BatchOperation::wasCanceled () const { return totalResult == kResultAborted; }
inline bool BatchOperation::Task::succeeded () const { return state == kSucceeded; }
inline bool BatchOperation::Task::failed    () const { return state == kFailed; }
inline bool BatchOperation::Task::isDisabled () const { return state == kDisabled; }
inline void BatchOperation::Task::disable () { state = kDisabled; }
inline bool BatchOperation::Task::hasProgressInfo () const { return progressInfo ; }
inline void BatchOperation::Task::hasProgressInfo (bool state) { progressInfo = state; }
inline ParamList& BatchOperationComponent::getParams () { return paramList; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_batchoperation_h
