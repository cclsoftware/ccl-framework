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
// Filename    : ccl/app/utilities/batchoperation.cpp
// Description : Batch Operation
//
//************************************************************************************************

#include "ccl/app/utilities/batchoperation.h"
#include "ccl/app/utilities/multiprogress.h"

#include "ccl/app/controls/listviewmodel.h"

#include "ccl/public/gui/framework/iprogressdialog.h"
#include "ccl/public/system/cclerror.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/iclipboard.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileOperations")
	XSTRING (FileXofY, "File %(1) of %(2)")
END_XSTRINGS

BEGIN_XSTRINGS ("Edit")
	XSTRING (Copy,	"Copy")
END_XSTRINGS

//************************************************************************************************
// BatchOperationComponent::TaskListModel
//************************************************************************************************

class BatchOperationComponent::TaskListModel: public ListViewModel
{
public:
	TaskListModel ();

	// ListViewModel
	void onItemChecked (ListViewItem* item) override;
	tbool CCL_API appendItemMenu (IContextMenu& menu, ItemIndexRef item, const IItemSelection& selection) override;
	tbool CCL_API interpretCommand (const CommandMsg& msg, ItemIndexRef item, const IItemSelection& selection) override;

	typedef ListViewModel SuperClass;
};

//************************************************************************************************
// BatchOperationComponent::TaskListItem
//************************************************************************************************

class BatchOperationComponent::TaskListItem: public ListViewItem
{
	PROPERTY_SHARED_AUTO (BatchOperation::Task, task, Task)
};

//************************************************************************************************
// BatchOperation::RunningState
//************************************************************************************************

struct BatchOperation::RunningState: public Unknown,
									 public IObserver
{
	class TotalOperation: public AsyncOperation
	{
	public:
		TotalOperation (RunningState& runningState)
		: runningState (runningState)
		{}

		// AsyncOperation
		void CCL_API cancel () override
		{
			AsyncOperation::cancel ();
			if(runningState.currentTask)
				runningState.currentTask->abort ();
		}

	private:
		RunningState& runningState;
	};

	ErrorContextGuard errorContext;
	SharedPtr<IProgressNotify> progress;
	AutoPtr<MultiProgress> multiProgress;
	MultiProgress::Step* taskProgressStep;
	ProgressNotifyScope* progressScope;
	AutoPtr<Iterator> taskIter;
	SharedPtr<Task> currentTask;
	SharedPtr<TotalOperation> totalOperation;
	AutoPtr<IAsyncOperation> taskOperation;

	RunningState (IProgressNotify* progress)
	: progress (progress),
	  multiProgress (NEW MultiProgress (progress)),
	  taskProgressStep (nullptr),
	  progressScope (nullptr)
	{
		if(progress)
			ISubject::addObserver (progress, this);
	}

	~RunningState ()
	{
		if(progress)
			ISubject::removeObserver (progress, this);

		delete progressScope;
	}

	Task* nextTask ()
	{
		while(Task* task = (Task*)taskIter->next ())
		{
			if(totalOperation && totalOperation->getState () >= IAsyncInfo::kCompleted)
				return nullptr;

			if(progress->isCanceled ())
				return nullptr;

			if(!task->isDisabled ())
				return task;
		}
		return nullptr;
	}

	// IObserver
	void CCL_API notify (ISubject* subject, MessageRef msg) override
	{
		if(msg == IProgressDialog::kCancelButtonHit)
		{
			if(currentTask)
				currentTask->abort ();
		}
	}

	CLASS_INTERFACE (IObserver, Unknown)
};

//************************************************************************************************
// BatchOperation
//************************************************************************************************

BatchOperation::BatchOperation ()
: totalResult (kResultOk),
  cancelEnabled (true),
  modalProgress (false),
  state (nullptr)
{
	tasks.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BatchOperation::~BatchOperation ()
{
	safe_release (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BatchOperation::addTask (Task* task)
{
	tasks.add (task);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* BatchOperation::newIterator () const
{
	return tasks.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BatchOperation::prepare ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* BatchOperation::prepareAsync ()
{
	if(prepare ())
	{
		return AsyncOperation::createCompleted ();
	}
	else
	{
		return nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BatchOperation::run (StringRef progressTitle, IWindow* parentWindow)
{
	AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
	progress->setTitle (progressTitle);

	if(parentWindow != nullptr)
		UnknownPtr<IProgressDialog> (progress)->setParentWindow (parentWindow);

	return run (progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* BatchOperation::runAsync (StringRef progressTitle, IWindow* parentWindow)
{
	UID progressClass (ClassID::ProgressDialog);
	#if CCL_PLATFORM_DESKTOP // modal progress dialog works on desktop platforms only!
	if(isModalProgress ())
		progressClass = ClassID::ModalProgressDialog;
	#endif

	AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (progressClass);
	ASSERT (progress != nullptr)
	progress->setTitle (progressTitle);
	if(parentWindow != nullptr)
		UnknownPtr<IProgressDialog> (progress)->setParentWindow (parentWindow);

	return runAsync (progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BatchOperation::beginBatch (IProgressNotify* progress)
{
	ASSERT (progress)

	if(!progress || tasks.isEmpty ())
		return false;

	int numTasks = tasks.count ();

	// determine if we need 1 or 2 progress bars
	int progressLevels = 1;
	if(numTasks > 1)
	{
		ForEach (tasks, Task, task)
			if(task->hasProgressInfo ())
			{
				progressLevels = 2;
				break;
			}
		EndFor
	}

	UnknownPtr<IProgressDialog> dialog (progress);
	if(dialog)
		dialog->constrainLevels (progressLevels, progressLevels);

	if(!isCancelEnabled ())
		progress->setCancelEnabled (false);

	totalResult = kResultOk;

	state->multiProgress->setNumSteps (numTasks);
	state->multiProgress->setStepCountPattern (stepCountPattern.isEmpty () ? XSTR (FileXofY) : stepCountPattern);

	state->progressScope = NEW ProgressNotifyScope (progress);

	state->taskIter = tasks.newIterator ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BatchOperation::endBatch ()
{
	postRun (state->progress, state->progress->isCanceled () != 0);

	delete state->progressScope;
	state->progressScope = nullptr;

	if(state->progress->isCanceled () || (state->totalOperation && state->totalOperation->getState () == IAsyncInfo::kCanceled))
	{
		totalResult = kResultAborted;

		ForEach (tasks, Task, task)
			task->onCanceled ();
		EndFor

		onCanceled ();
	}
	else
	{
		ForEach (tasks, Task, task)
			task->onFinished ();
		EndFor

		bool allSucceeded = (totalResult == kResultOk);
		onFinished (allSucceeded);
	}

	if(state->totalOperation)
	{
		int opState = (totalResult == kResultOk) ? IAsyncInfo::kCompleted : (totalResult == kResultAborted) ? IAsyncInfo::kCanceled : IAsyncInfo::kFailed;
		state->totalOperation->setResult (Variant (static_cast<IContainer*> (this), true));
		state->totalOperation->setState (opState);
	}

	UnknownPtr<IModalProgressDialog> modalProgress (state->progress);
	if(modalProgress)
		modalProgress->close ();

	// remove constraints
	UnknownPtr<IProgressDialog> dialog (state->progress);
	if(dialog)
		dialog->constrainLevels (1, -1);

	safe_release (state);
	return totalResult == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BatchOperation::beginTask (Task* task)
{
	ASSERT (!state->currentTask)
	System::GetErrorHandler ().beginContext ();

	if(!task->prepare ())
	{
		System::GetErrorHandler ().endContext ();
		state->currentTask = nullptr;
		return false;
	}

	state->currentTask = task;

	ASSERT (!state->taskProgressStep)
	state->taskProgressStep = NEW MultiProgress::Step (*state->multiProgress);
	(*state->taskProgressStep)->setProgressText (task->getProgressText ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BatchOperation::endTask (Task* task, bool success)
{
	delete state->taskProgressStep;
	state->taskProgressStep = nullptr;

	task->setState (success ? Task::kSucceeded : Task::kFailed);

	if(!success)
	{
		totalResult = kResultFailed;

		// try to get error information for the task
		if(task->getErrorText ().isEmpty ())
			if(IErrorContext* context = System::GetErrorHandler ().peekContext ())
				if(context->getEventCount () >= 1)
					task->setErrorText (context->getEvent (0).message);
	}

	if(state->currentTask)
		System::GetErrorHandler ().endContext ();
	state->currentTask = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BatchOperation::run (IProgressNotify* progress)
{
	ASSERT (state == nullptr)
	state = NEW RunningState (progress);

	if(prepare () && beginBatch (progress))
	{
		while(Task* task = state->nextTask ())
		{
			bool success = false;
			if(beginTask (task))
				success = task->perform (*state->taskProgressStep);

			endTask (task, success);
		}
		return endBatch ();
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* BatchOperation::runAsync (IProgressNotify* progress)
{
	ASSERT (state == nullptr)
	state = NEW RunningState (progress);

	ASSERT (!state->totalOperation)
	state->totalOperation = NEW RunningState::TotalOperation (*state);
	IAsyncOperation* totalOperation = state->totalOperation;

	SharedPtr<BatchOperation> This (this);

	Promise p (prepareAsync ());
	p.then ([This, progress] (IAsyncOperation& operation)
	{
		if(operation.getState () == IAsyncInfo::kCompleted)
		{
			if(This->beginBatch (progress))
			{
				This->state->totalOperation->setState (AsyncOperation::kStarted);

				if(!This->startNextTaskAsync ())
					This->endBatch ();
				else if(This->state != nullptr)
				{
					UnknownPtr<IModalProgressDialog> modalProgress (progress);
					if(modalProgress.isValid ())
						modalProgress->run ();
				}
			}
		}
		else if(operation.getState () == IAsyncInfo::kCanceled)
		{
			// canceled
		}
	});

	return totalOperation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BatchOperation::startNextTaskAsync ()
{
	while(Task* task = state->nextTask ())
	{
		if(beginTask (task))
		{
			ASSERT (!state->taskOperation)
			state->taskOperation = task->performAsync ();
			if(state->taskOperation)
			{
				(*state->taskProgressStep)->updateAnimated ();
				state->taskOperation->setProgressHandler (*state->taskProgressStep);
				state->taskOperation->setCompletionHandler (this);
				return true;
			}
		}
		endTask (task, false);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API BatchOperation::onCompletion (IAsyncOperation& operation)
{
	// task operation completed
	ASSERT (state)
	ASSERT (state->currentTask)
	ASSERT (state->taskOperation == &operation)

	if(state->currentTask)
	{
		if(state->taskOperation)
			state->taskOperation->setProgressHandler (nullptr);

		bool result = operation.getState () == IAsyncInfo::kCompleted;
		endTask (state->currentTask, result);

		state->taskOperation.release ();
	}

	if(!startNextTaskAsync ())
		endBatch ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BatchOperation::cancel ()
{
	if(state)
		if(UnknownPtr<IProgressDialog> dialog = (IUnknown*)state->progress)
			dialog->tryCancel ();
}

//************************************************************************************************
// AsyncBatchTask
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (AsyncBatchTask, BatchOperation::Task)

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant AsyncBatchTask::runModal (IAsyncCall* call, StringRef title, StringRef text, IWindow* parentWindow)
{
	AutoPtr<BatchOperation> op = NEW BatchOperation;
	op->setModalProgress (true);
	AutoPtr<AsyncBatchTask> task = NEW AsyncBatchTask (call);
	task->setProgressText (text);
	op->addTask (return_shared<AsyncBatchTask> (task));
	Promise p (op->runAsync (title, parentWindow));
	Variant result = task->getResult ();
	result.share ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AsyncBatchTask::AsyncBatchTask (IAsyncCall* asyncCall)
: asyncCall (asyncCall)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

String AsyncBatchTask::getProgressText ()
{
	return progressText;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* AsyncBatchTask::performAsync ()
{
	Promise p = asyncCall->call ();
	pendingOperation = p; // keep first operation alive!
	return return_shared<IAsyncOperation> (p.then (this, &AsyncBatchTask::onCompletion));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncBatchTask::onCompletion (IAsyncOperation& operation)
{
	result = operation.getResult ();
	result.share ();
	AsyncOperation::deferDestruction (pendingOperation.detach ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncBatchTask::abort ()
{
	if(pendingOperation)
		pendingOperation->cancel ();
}

//************************************************************************************************
// BatchOperationComponent::TaskListModel
//************************************************************************************************

BatchOperationComponent::TaskListModel::TaskListModel ()
{
	getColumns ().addColumn (18, nullptr, kCheckBoxID);
	getColumns ().addColumn (200, nullptr, kTitleID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BatchOperationComponent::TaskListModel::onItemChecked (ListViewItem* item)
{
	for(int i = 0, num = countFlatItems (); i < num; i++)
		if(TaskListItem* item = (TaskListItem*)resolve (i))
			item->getTask ()->setState (item->isChecked () ? BatchOperation::Task::kWaiting : BatchOperation::Task::kDisabled);

	SuperClass::onItemChecked (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BatchOperationComponent::TaskListModel::appendItemMenu (IContextMenu& menu, ItemIndexRef item, const IItemSelection& selection)
{
	menu.addCommandItem (XSTR (Copy), "Edit", "Copy", nullptr);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BatchOperationComponent::TaskListModel::interpretCommand (const CommandMsg& msg, ItemIndexRef item, const IItemSelection& selection)
{
	if(msg.category == "Edit" && msg.name == "Copy")
	{
		if(!msg.checkOnly ())
		{
			// copy titles of all tasks
			String text;
			for(int i = 0, num = countFlatItems (); i < num; i++)
				if(TaskListItem* item = (TaskListItem*)resolve (i))
					text << item->getTitle () << "\n";

			System::GetClipboard ().setText (text);
		}
		return true;
	}
	return false;
}

//************************************************************************************************
// BatchOperationComponent
//************************************************************************************************

BatchOperationComponent::BatchOperationComponent (BatchOperation& batchOperation)
: batchOperation (batchOperation),
  taskListModel (nullptr)
{
	paramList.addString ("headerText");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BatchOperationComponent::~BatchOperationComponent ()
{
	if(taskListModel)
		taskListModel->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BatchOperationComponent::TaskListModel* BatchOperationComponent::createListModel () const
{
	TaskListModel* model = NEW TaskListModel;

	ForEach (batchOperation, BatchOperation::Task, task)
		TaskListItem* item = NEW TaskListItem;
		item->setTitle (task->getTitle ().isEmpty () ? UrlDisplayString (task->getSourcePath (), Url::kStringDisplayPath) : task->getTitle ());
		item->setTask (task);
		item->setChecked (!task->isDisabled ());
		model->addItem (item);
	EndFor

	return model;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API BatchOperationComponent::getObject (StringID name, UIDRef classID)
{
	if(name == "taskList")
	{
		if(!taskListModel)
			taskListModel = createListModel ();
		return ccl_as_unknown (taskListModel);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BatchOperationComponent::runListDialog (StringID formName, StringRef headerText)
{
	if(IParameter* header = paramList.lookup ("headerText"))
		header->setValue (headerText);


	if(ITheme* theme = getTheme ())
		if(IView* dialogView = theme->createView (formName.isEmpty () ? "CCL/TaskListDialog" : formName, this->asUnknown ()))
			return DialogBox ()->runDialog (dialogView) == DialogResult::kOkay;

	// fall back to alert if form not found
	return Alert::ask (headerText, Alert::kYesNo) == Alert::kYes;
}

//************************************************************************************************
// BatchOperation::Task
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (BatchOperation::Task, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

String BatchOperation::Task::getProgressText ()
{ return String::kEmpty; }

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BatchOperation::Task::prepare ()
{ return true; }

//////////////////////////////////////////////////////////////////////////////////////////////////

void BatchOperation::Task::abort ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BatchOperation::Task::onFinished ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BatchOperation::Task::onCanceled ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BatchOperation::Task::perform (IProgressNotify* progress)
{
	CCL_NOT_IMPL ("perform() or performAsync() must be overwritten!\n")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* BatchOperation::Task::performAsync ()
{
	SharedPtr<AsyncOperation> operation = NEW AsyncOperation;

	if(perform (nullptr))
		operation->setState (IAsyncInfo::kCompleted);
	else
		operation->setState (IAsyncInfo::kFailed);

	return operation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String BatchOperation::Task::buildTextFromFileName (StringRef pattern, UrlRef path)
{
	String fileName;
	path.getName (fileName);

	String text;
	Variant args[] = { fileName };
	text.appendFormat (pattern, args, 1);
	return text;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String BatchOperation::Task::buildTextFromSourceFileName (StringRef pattern)
{
	return buildTextFromFileName (pattern, sourcePath);
}

//************************************************************************************************
// DummyBatchTask
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (DummyBatchTask, BatchOperation::Task)

//************************************************************************************************
// BatchTaskFilePromise
//************************************************************************************************

BatchTaskFilePromise::BatchTaskFilePromise (BatchOperation::Task* task)
: task (task)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BatchTaskFilePromise::getFileName (String& fileName) const
{
	task->getSourcePath ().getName (fileName, false);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BatchTaskFilePromise::getFileType (FileType& fileType) const
{
	fileType = task->getDestPath ().getFileType ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BatchTaskFilePromise::createFile (UrlRef destPath, IProgressNotify* progress)
{
	task->setDestPath (destPath);
	if(task->prepare ())
		if(task->perform (progress))
			return kResultOk;

	return kResultFailed;
}
