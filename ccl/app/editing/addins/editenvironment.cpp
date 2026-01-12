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
// Filename    : ccl/app/editing/addins/editenvironment.cpp
// Description : Edit Environment
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/editing/addins/editenvironment.h"
#include "ccl/app/editing/addins/editaddincollection.h"

#include "ccl/app/editing/editor.h"
#include "ccl/app/editing/editmodel.h"
#include "ccl/app/editing/editview.h"
#include "ccl/app/editing/tasks/edittaskcollection.h"

#include "ccl/app/actions/iactioncontext.h"
#include "ccl/app/actions/actionjournal.h"

#include "ccl/public/app/signals.h"

#include "ccl/base/message.h"
#include "ccl/base/boxedtypes.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// EditEnvironment
//************************************************************************************************

EditEnvironment* EditEnvironment::getInstance (EditView* editView)
{
	if(EditorComponent* editor = editView ? unknown_cast<EditorComponent> (editView->getController ()) : nullptr)
		return editor->getEditEnvironment ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditEnvironment* EditEnvironment::getInstance (const Component& component)
{
	auto* rootComponent = unknown_cast<Component> (component.getRoot ());
	return rootComponent ? rootComponent->getComponent<EditEnvironment> (CCLSTR (kComponentName)) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (EditEnvironment, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditEnvironment::EditEnvironment (StringRef name)
: Component (name.isEmpty () ? CCLSTR (kComponentName) : name),
  signalSink (Signals::kEditorRegistry),
  actionContext (nullptr),
  activeEditor (nullptr),
  addIns (nullptr)
{
	signalSink.setObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditEnvironment::~EditEnvironment ()
{
	ASSERT (activeEditor == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditAddInCollection& EditEnvironment::getAddIns ()
{
	if(addIns == nullptr)
		addComponent (addIns = NEW EditAddInCollection);
	return *addIns;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* CCL_API EditEnvironment::findChild (StringRef id) const
{
	if(id == "MainEditor")
		return UnknownPtr<IObjectNode> (ccl_const_cast (this)->getMainEditor ());

	return SuperClass::findChild (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API EditEnvironment::terminate ()
{
	setActiveEditor (nullptr);

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditEnvironment::signalEditEvent (StringID name)
{
	#if DEBUG_LOG
	MutableCString editorName;
	if(activeEditor)
	{
		editorName = activeEditor->getName ();
		editorName.appendFormat (" (%s)", CCL_DEBUG_ID (activeEditor));
	}
	CCL_PRINTF ("Edit Environment[%s] %s (active editor: %s)\n", CCL_DEBUG_ID (this), name.str (), editorName.str ())
	#endif

	signal (Message (name));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditEnvironment::setActiveEditor (EditorComponent* editor)
{
	// TODO: check if activeEditor can be shared!
	if(editor != activeEditor)
	{
		#if DEBUG_LOG
		MutableCString editorName;
		if(editor)
		{
			editorName = editor->getName ();
			editorName.appendFormat (" (%s)", CCL_DEBUG_ID (editor));
		}
		CCL_PRINTF ("EditEnvironment[%s] setActiveEditor: %s\n", CCL_DEBUG_ID (this), editorName.str ())
		#endif

		if(activeEditor)
			signalSlots.unadvise (&activeEditor->getModel ().getSelection ());
		activeEditor = editor;
		if(activeEditor)
			signalSlots.advise (&activeEditor->getModel ().getSelection (), kChanged, this, &EditEnvironment::onSelectionChanged);

		signalEditEvent (kActiveEditorChanged);
		signalEditEvent (kSelectionChanged); // required, implicitly true when active editor changes
		signalEditEvent (kFocusItemChanged);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditEnvironment::onSelectionChanged (MessageRef msg)
{
	signalEditEvent (kSelectionChanged);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API EditEnvironment::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kEditorActivated)
	{
		EditorComponent* editor = EditorRegistry::instance ().getActiveEditor ();

		if(editor && editor->getEditEnvironment () != this)
		{
			CCL_PRINTF ("EditEnvironment[%s]: ignore foreign editor:  %s\n", CCL_DEBUG_ID (this), MutableCString (editor->getName ()).appendFormat (" (%s)", CCL_DEBUG_ID (editor)).str ())
			editor = nullptr;
		}

		setActiveEditor (editor);
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObject* CCL_API EditEnvironment::getMainEditor ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObject* CCL_API EditEnvironment::getActiveEditor ()
{
	return activeEditor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISelection* CCL_API EditEnvironment::getActiveSelection ()
{
	return activeEditor ? &activeEditor->getModel ().getSelection () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API EditEnvironment::getFocusItem ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObject* CCL_API EditEnvironment::getFocusItemPropertyEditor ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API EditEnvironment::getAddInInstance (StringRef name)
{
	return getAddIns ().findChild (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditEnvironment::runTask (UIDRef cid, bool checkOnly, IAttributeList* arguments)
{
	ASSERT (cid.isValid ())
	if(activeEditor)
		ForEach (*activeEditor, Component, c)
			if(EditTaskCollection* taskCollection = ccl_cast<EditTaskCollection> (c))
				if(EditTaskDescription* task = taskCollection->findTask (cid))
				{
					if(checkOnly)
						return taskCollection->canRunTask (*task);
					else
					{
						AutoPtr<Attributes> args;
						if(arguments)
						{
							args = NEW PersistentAttributes; // edit tasks require PersistentAttributes!
							args->copyFrom (*arguments);
						}

						return taskCollection->runTask (*task, args, true);
					}
				}
		EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditEnvironment::canRunEditTask (UIDRef cid)
{
	return runTask (cid, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditEnvironment::runEditTask (UIDRef cid, IAttributeList* arguments)
{
	return runTask (cid, false, arguments);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditEnvironment::runEditTaskWithClassName (StringRef className, IAttributeList* arguments)
{
	const IClassDescription* description = System::GetPlugInManager ().getClassDescription (className);
	ASSERT (description)
	return description ? runEditTask (description->getClassID (), arguments) != 0 : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IActionJournal* CCL_API EditEnvironment::getActionJournal ()
{
	return actionContext ? actionContext->getActionJournal () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (EditEnvironment)
	DEFINE_PROPERTY_NAME ("mainEditor")
	DEFINE_PROPERTY_NAME ("activeEditor")
	DEFINE_PROPERTY_NAME ("actionSelection")
	DEFINE_PROPERTY_NAME ("focusItem")
	DEFINE_PROPERTY_NAME ("focusItemPropertyEditor")
END_PROPERTY_NAMES (EditEnvironment)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditEnvironment::getProperty (Variant& var, MemberID propertyId) const
{
	#define RETURN_PROPERTY(name, Method) \
	if(propertyId == name) { var.takeShared (const_cast<EditEnvironment*> (this)->Method ()); return true; }

	RETURN_PROPERTY ("mainEditor", getMainEditor)
	RETURN_PROPERTY ("activeEditor", getActiveEditor)
	RETURN_PROPERTY ("actionSelection", getActiveEditor)
	RETURN_PROPERTY ("focusItem", getFocusItem)
	RETURN_PROPERTY ("focusItemPropertyEditor", getFocusItemPropertyEditor)

	#undef RETURN_PROPERTY
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (EditEnvironment)
	DEFINE_METHOD_NAME ("runEditTask")
END_METHOD_NAMES (EditEnvironment)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditEnvironment::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "runEditTask")
	{
		UID cid = Boxed::UID::fromVariant (msg[0]);
		UnknownPtr<IAttributeList> arguments = msg.getArgCount () > 1 ? msg[1].asUnknown () : nullptr;
		returnValue = runEditTask (cid, arguments);
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}
