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
// Filename    : ccl/app/editing/tasks/edittaskhandler.cpp
// Description : Edit Task Handler
//
//************************************************************************************************

#include "ccl/app/editing/tasks/edittaskhandler.h"

#include "ccl/app/component.h"
#include "ccl/app/paramcontainer.h"
#include "ccl/app/actions/action.h"
#include "ccl/app/actions/actionjournal.h"
#include "ccl/app/actions/iactioncontext.h"

#include "ccl/app/presets/objectpreset.h"
#include "ccl/public/app/presetmetainfo.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/settings.h"

#include "ccl/public/storage/ipersistattributes.h"
#include "ccl/public/gui/framework/ithememanager.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/system/ipackagemetainfo.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/plugins/icoderesource.h"
#include "ccl/public/plugins/stubobject.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// EditTaskAction
//************************************************************************************************

class EditTaskAction: public MultiAction
{
public:
	DECLARE_CLASS_ABSTRACT (EditTaskAction, MultiAction)

	EditTaskAction (const EditTaskDescription& description);

	PROPERTY_OBJECT (UID, cid, ClassID)
	PROPERTY_SHARED_AUTO (Attributes, savedValues, SavedValues)

	static EditTaskAction* beginTask (IActionContext* context, const EditTaskDescription& description);
	static bool endTask (IActionContext* context, bool cancel = false);

	// MultiAction
	bool isDragable () const override;
	IUnknown* createIcon () override;
	IUnknown* createDragObject () override;
};

//************************************************************************************************
// EditTaskStub
//************************************************************************************************

class EditTaskStub: public StubObject,
					public IEditTask
{
public:
	DECLARE_STUB_METHODS (IEditTask, EditTaskStub)

	tresult CCL_API prepareEdit (IObject& context) override
	{
		Variant returnValue;
		if(!invokeMethod (returnValue, Message ("prepareEdit", &context)))
			return kResultUnexpected;
		return returnValue.asResult ();
	}

	tresult CCL_API performEdit (IObject& context) override
	{
		Variant returnValue;
		if(!invokeMethod (returnValue, Message ("performEdit", &context)))
			return kResultUnexpected;
		return returnValue.asResult ();
	}
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (EditTaskStub, kFirstRun)
{
	REGISTER_STUB_CLASS (IEditTask, EditTaskStub)
	return true;
}

//************************************************************************************************
// EditTaskCandidate
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (EditTaskCandidate, Object)

//************************************************************************************************
// EditTaskAction
//************************************************************************************************

EditTaskAction* EditTaskAction::beginTask (IActionContext* context, const EditTaskDescription& description)
{
	EditTaskAction* action = nullptr;
	ActionJournal* journal = context ? context->getActionJournal () : nullptr;
	ASSERT (journal != nullptr)
	if(journal)
		journal->beginMultiple (action = NEW EditTaskAction (description));
	return action;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditTaskAction::endTask (IActionContext* context, bool cancel)
{
	ActionJournal* journal = context ? context->getActionJournal () : nullptr;
	ASSERT (journal != nullptr)
	if(journal)
		return journal->endMultiple (cancel);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (EditTaskAction, MultiAction)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTaskAction::EditTaskAction (const EditTaskDescription& description)
: MultiAction (description.getLocalizedName ()),
  cid (description.getClassID ())
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditTaskAction::isDragable () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* EditTaskAction::createIcon ()
{
	if(const IClassDescription* description = System::GetPlugInManager ().getClassDescription (cid))
		return return_shared<IImage> (EditTaskDescription (*description).getIcon ());
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* EditTaskAction::createDragObject ()
{
	if(const IClassDescription* description = System::GetPlugInManager ().getClassDescription (cid))
	{
		if(savedValues) // drag as preset
		{
			ObjectPreset* preset = NEW ObjectPreset (description);
			preset->setData (savedValues->asUnknown ());
			return preset->asUnknown ();
		}
		else
		{
			IClassDescription* result = nullptr;
			description->clone (result);
			return result;
		}
	}
	return nullptr;
}

//************************************************************************************************
// EditTaskHandler::DragInfo
//************************************************************************************************

struct EditTaskHandler::DragInfo
{
	SharedPtr<IClassDescription> description;
	SharedPtr<Attributes> savedValues;

	bool assign (IUnknown* object)
	{
		savedValues = nullptr;
		description = UnknownPtr<IClassDescription> (object);

		if(description == nullptr) // try as preset
		{
			UnknownPtr<IPreset> preset (object);
			if(preset && preset->getMetaInfo ())
			{
				UID cid;
				if(PresetMetaAttributes (*preset->getMetaInfo ()).getClassID (cid))
					if(const IClassDescription* d = System::GetPlugInManager ().getClassDescription (cid))
					{
						d->clone (description);
						savedValues = unknown_cast<Attributes> (preset->getUserData ());
					}
			}
		}

		return description && description->getCategory () == PLUG_CATEGORY_EDITTASK;
	}
};

//************************************************************************************************
// EditTaskHandler
//************************************************************************************************

bool EditTaskHandler::runningTask = false;

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditTaskHandler::canCreateTask (EditTaskDescription& taskDescription, IUnknown* object)
{
	DragInfo info;
	if(info.assign (object))
	{
		taskDescription.assign (*info.description);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTaskHandler* EditTaskHandler::createTask (IUnknown* object)
{
	DragInfo info;
	if(info.assign (object))
	{
		IEditTask* task = ccl_new<IEditTask> (info.description->getClassID ());
		if(task)
		{
			EditTaskHandler* handler = NEW EditTaskHandler (*task, EditTaskDescription (*info.description));
			if(info.savedValues)
			{
				handler->setSilentMode (true);
				handler->setSavedValues (info.savedValues);
			}
			return handler;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTaskHandler* EditTaskHandler::createTask (const EditTaskDescription& description)
{
	if(IEditTask* task = ccl_new<IEditTask> (description.getClassID ()))
		return NEW EditTaskHandler (*task, description);
	else
		return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditTaskHandler::isRunningTask ()
{
	return runningTask;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (EditTaskHandler, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTaskHandler::EditTaskHandler (IEditTask& task, const EditTaskDescription& description)
: task (task),
  parameters (NEW ParamContainer),
  taskDescription (description),
  silentMode (false),
  persistent (true),
  restored (false)
{
	String cidString;
	description.getClassID ().toString (cidString);
	taskSettingsID << PLUG_CATEGORY_EDITTASK << CCLSTR ("/") << cidString;

	// give edit task a chance to listen to parameter changes
	UnknownPtr<IParamObserver> controller (&task);
	parameters->setController (controller);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTaskHandler::~EditTaskHandler ()
{
	ccl_forceGC ();

	parameters->release ();

	ccl_release (&task);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const EditTaskDescription& EditTaskHandler::getDescription () const
{
	return taskDescription;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* EditTaskHandler::createTaskView ()
{
	MutableCString formName, themeId;
	UnknownPtr<IObject> taskObject (&task);
	if(taskObject.isValid ())
	{
		Variant v;
		taskObject->getProperty (v, IEditTask::kFormName);
		formName = v;
		v.clear ();
		taskObject->getProperty (v, IEditTask::kThemeID);
		themeId = v;
	}

	ITheme* theme = nullptr;
	if(!themeId.isEmpty ())
		theme = System::GetThemeManager ().getTheme (themeId);
	else
		theme = RootComponent::instance ().getTheme ();

	ASSERT (!formName.isEmpty ())
	return theme ? theme->createView (formName, &task) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult EditTaskHandler::prepareTask (IObject* editor)
{
	ASSERT (editor != nullptr)
	tresult result = kResultOk;

	restored = false;

	AutoPtr<EditTaskContext> prepareContext = NEW EditTaskContext (this);
	prepareContext->setObject ("parameters", parameters);
	prepareContext->setObject ("editor", editor);

	result = task.prepareEdit (*prepareContext);
	prepareContext->removeAll (); // a naughty script might have kept a reference to the context, remove at least our provided objects (e.g. editor)

	if(result != kResultOk)
		return result;

	restoreValues (); // in case task did not call context.runDialog() or context.restore()

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult EditTaskHandler::performTask (const Container& candidates, IObject* editor)
{
	ASSERT (editor != nullptr)
	tresult result = kResultOk;

	ScopedVar<bool> runningScope (runningTask, true);

	// begin transaction
	bool canceled = false;
	EditTaskCandidate* first = (EditTaskCandidate*)candidates.at (0);
	IActionContext* actionContext = first ? first->getActionContext () : nullptr;
	SharedPtr<EditTaskAction> transaction = EditTaskAction::beginTask (actionContext, taskDescription);

	// run for each candidate
	ForEach (candidates, EditTaskCandidate, candidate)
		AutoPtr<EditTaskContext> candidateContext = NEW EditTaskContext (this);
		candidateContext->setObject ("parameters", parameters);
		candidateContext->setObject ("editor", editor);
		candidate->prepare (*candidateContext);

		result = task.performEdit (*candidateContext);
		candidate->unprepare (*candidateContext);
		candidateContext->removeAll ();

		if(result != kResultOk)
		{
			canceled = true;
			break;
		}
	EndFor

	// finish transaction
	EditTaskAction::endTask (actionContext, canceled);
	if(canceled)
		transaction = nullptr;

	// store task parameters
	if(persistent)
		storeValues (true);
	if(savedValues && transaction)
		transaction->setSavedValues (savedValues);

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult EditTaskHandler::runTask (const Container& candidates, IObject* editor)
{
	tresult result = prepareTask (editor);
	if(result != kResultOk)
		return result;

	return performTask (candidates, editor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult EditTaskHandler::runArgumentDialog ()
{
	AutoPtr<EditTaskContext> prepareContext = NEW EditTaskContext (this);
	prepareContext->setObject ("parameters", parameters);
	prepareContext->set ("isArgumentDialog", true);
	tresult result = task.prepareEdit (*prepareContext);
	if(result != kResultOk)
		return result;

	storeValues (false);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskHandler::storeValues (bool global)
{
	// check if task wants to handle persistence itself
	UnknownPtr<IPersistAttributes> persistTask (&task);
	if(persistTask)
	{
		savedValues = NEW Attributes; // allow to save objects (don't use PersistentAttributes)
		persistTask->storeValues (*savedValues);

		if(global)
			Settings::instance ().getAttributes (taskSettingsID).set ("values", savedValues->clone (), Attributes::kOwns);
	}
	// handle parameters otherwise
	else if(parameters->countParameters () > 0)
	{
		savedValues = NEW PersistentAttributes;
		parameters->storeValues (*savedValues, true);

		if(global)
			parameters->storeSettings (taskSettingsID);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskHandler::restoreValues (bool force)
{
	if(!restored || force)
	{
		restored = true;

		// check if task wants to handle persistence itself
		UnknownPtr<IPersistAttributes> persistTask (&task);
		if(persistTask)
		{
			if(savedValues)
				persistTask->restoreValues (*savedValues);
			else
			{
				if(Attributes* a = Settings::instance ().getAttributes (taskSettingsID).getAttributes ("values"))
					persistTask->restoreValues (*a);
			}
		}
		// handle parameters otherwise
		else if(parameters->countParameters () > 0)
		{
			if(savedValues)
				parameters->restoreValues (*savedValues, true);
			else
				parameters->restoreSettings (taskSettingsID);
		}
	}
}

//************************************************************************************************
// EditTaskArgumentUI
//************************************************************************************************

DEFINE_CLASS (EditTaskArgumentUI, Object)
DEFINE_CLASS_UID (EditTaskArgumentUI, 0x645c45c1, 0x5262, 0x4b77, 0x92, 0xf2, 0xb7, 0x25, 0x9, 0xc7, 0xc0, 0xb0)
DEFINE_CLASS_NAMESPACE (EditTaskArgumentUI, "Host")

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult EditTaskArgumentUI::runDialog (Attributes& arguments, UIDRef cid)
{
	IEditTask* task = ccl_new<IEditTask> (cid);
	if(!task)
		return kResultFalse;

	EditTaskDescription taskDescription;
	const IClassDescription* description = ccl_classof (task);
	ASSERT (description != nullptr)
	if(description)
		taskDescription.assign (*description);

	EditTaskHandler handler (*task, taskDescription);
	handler.setSavedValues (&arguments);
	if(handler.runArgumentDialog () != kResultOk)
		return kResultFalse;

	Attributes* savedValues = handler.getSavedValues ();
	if(savedValues && savedValues != &arguments)
		arguments.copyFrom (*savedValues);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (EditTaskArgumentUI)
	DEFINE_METHOD_ARGR ("runDialog", "arguments: IAttributeList, cid: string", "tresult")
END_METHOD_NAMES (EditTaskArgumentUI)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditTaskArgumentUI::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "runDialog")
	{
		UnknownPtr<IAttributeList> arguments (msg[0].asUnknown ());
		ASSERT (arguments != nullptr)
		PersistentAttributes args;
		if(arguments)
			args.copyFrom (*arguments);

		UID cid;
		cid.fromString (msg[1].asString ());

		tresult result = runDialog (args, cid);
		if((result == kResultOk) && arguments)
			arguments->copyFrom (args);

		returnValue = result;
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// EditTaskContext
//************************************************************************************************

DEFINE_CLASS_HIDDEN (EditTaskContext, Attributes)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditTaskContext::EditTaskContext (EditTaskHandler* handler)
: handler (handler)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskContext::setObject (StringID name, IObject* object)
{
	set (name, object, Attributes::kShare);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditTaskContext::isSilentMode () const
{
	return handler->isSilentMode ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes* EditTaskContext::getArguments () const
{
	return handler->getSavedValues ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditTaskContext::restore (bool force)
{
	handler->restoreValues (force);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (EditTaskContext)
	DEFINE_METHOD_ARGS ("restore", "force: bool = false")
	DEFINE_METHOD_ARGR ("runDialog", "formName: string, themeID: string = null", "tresult")
	DEFINE_METHOD_ARGR ("isSilentMode", "", "bool")
	DEFINE_METHOD_ARGR ("getArguments", "", "Attributes")
END_METHOD_NAMES (EditTaskContext)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditTaskContext::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "isSilentMode")
	{
		returnValue = isSilentMode ();
		return true;
	}
	else if(msg == "restore")
	{
		bool force = msg.getArgCount () >= 1 ? msg[0].asBool () : false;
		restore (force);
		return true;
	}
	else if(msg == "getArguments")
	{
		Attributes* args = getArguments ();
		if(args)
			returnValue = Variant (args->asUnknown (), true);
		return true;
	}
	else if(msg == "runDialog")
	{
		// restore parameters
		handler->restoreValues ();

		// suppress dialog in silent mode
		if(handler->isSilentMode ())
		{
			returnValue = (int)kResultOk;
			return true;
		}

		// Theme ID, can be nil
		ITheme* theme = nullptr;
		if(msg.getArgCount () < 2 || msg[1].isNil ())
			theme = RootComponent::instance ().getTheme ();
		else
			theme = System::GetThemeManager ().getTheme (MutableCString (msg[1].asString ()));

		// Form name, can be nil, in which case we run the standard dialog
		Variant result;
		if(msg.getArgCount () < 1 || msg[0].isNil ())
		{
			// run parameter dialog...
			if(handler->parameters->countParameters () > 0)
			{
				DialogBox dialogBox;

				#if 1
				// TEST: try to find a translation table...
				MutableCString packageID;
				if(UnknownPtr<ICodeResource> codeResource = const_cast<IClassDescription*> (ccl_classof (&handler->task)))
				   if(const IAttributeList* metaInfo = codeResource->getMetaInfo ())
					   packageID = AttributeReadAccessor (*metaInfo).getCString (Meta::kPackageID);

				if(!packageID.isEmpty ())
				{
					ITranslationTable* stringTable = System::GetLocaleManager ().getStrings (packageID);
					dialogBox->setStrings (stringTable);
				}
				#endif

				String name; // used as help identifier
				name << handler->getDescription ().getCategory () << "." << handler->getDescription ().getName ();

				String title (handler->getDescription ().getLocalizedName ());
				if(dialogBox->runWithParameters (name, *handler->parameters, title) == DialogResult::kOkay)
					result = (int)kResultOk;
				else
					result = (int)kResultAborted;
				returnValue = result;
			}
		}
		else
		{
			Variant var;
			IObject* gui = System::GetScriptingManager ().getHost ().getObject ("GUI");
			ASSERT (gui != nullptr)

			Variant button;
			gui->invokeMethod (button, Message ("runDialog", theme, msg[0], &handler->task, Styles::kDialogOkCancel >> 16));
			returnValue = button.asInt () == 1 ? (int)kResultOk : (int)kResultAborted;
		}

		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}
