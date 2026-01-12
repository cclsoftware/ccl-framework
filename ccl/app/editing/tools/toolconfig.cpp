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
// Filename    : ccl/app/editing/tools/toolconfig.cpp
// Description : Tool Config
//
//************************************************************************************************

#define DEBUG_LOG 0

#define CCL_BOX_STATIC // use static "boxing"

#include "ccl/app/editing/tools/toolconfig.h"
#include "ccl/app/editing/tools/toolaction.h"

#include "ccl/app/actions/actionexecuter.h"
#include "ccl/app/actions/action.h"

#include "ccl/app/utilities/boxedguitypes.h"
#include "ccl/app/editing/editview.h"
#include "ccl/app/editing/edithandler.h"

#include "ccl/base/kernel.h"
#include "ccl/base/message.h"

#include "ccl/public/base/iarrayobject.h"
#include "ccl/public/plugins/stubobject.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/ipresentable.h"
#include "ccl/public/gui/framework/ihelpmanager.h"
#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/gui/icontextmenu.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (IToolAction, 0x5CE351CA, 0xC884, 0x44D1, 0x80, 0x8B, 0x56, 0xD8, 0xDF, 0x66, 0x3C, 0x87)
DEFINE_IID_ (IToolConfiguration, 0x12804A3F, 0x73EB, 0x409B, 0xAB, 0x48, 0xFC, 0xD7, 0x9C, 0x1D, 0xE9, 0xDF)
DEFINE_IID_ (IToolHelp, 0xb323fc62, 0x8cf1, 0x40c1, 0xbc, 0x6e, 0x83, 0x83, 0x2e, 0xc9, 0x34, 0x63)
DEFINE_IID_ (IToolMode, 0x9DEBB0E8, 0x23F5, 0x4EBB, 0x92, 0x41, 0x04, 0xA8, 0x7B, 0xA2, 0xED, 0x9C)
DEFINE_IID_ (IToolSet, 0x75E80A30, 0xA2EA, 0x44CC, 0xAB, 0x0B, 0x2B, 0x3C, 0xD7, 0x91, 0x4F, 0xB8)
DEFINE_IID_ (IEditHandler, 0x9A7BA5F4, 0xBCB4, 0x4DFE, 0x98, 0x95, 0x35, 0x6B, 0xA5, 0x36, 0x60, 0x54)
DEFINE_IID_ (INativeToolSet, 0x0B9304FA, 0xF92F, 0x41C4, 0xA7, 0x95, 0xA9, 0x2B, 0x3F, 0xA2, 0x4F, 0x40)

namespace CCL {

//************************************************************************************************
// ConfigEditHandler
//************************************************************************************************

class ConfigEditHandler: public EditHandler
{
public:
	DECLARE_CLASS_ABSTRACT (ConfigEditHandler, EditHandler)
	DECLARE_METHOD_NAMES (ConfigEditHandler)

	ConfigEditHandler (IEditHandler& handler, EditView* view)
	: EditHandler (view),
	  handler (&handler),
	  handlerObj (&handler)
	{
		ASSERT (view)
		handlerObj->setProperty ("editor", view->asUnknown ());
		handlerObj->setProperty ("editHandler", this->asUnknown ());

		checkKeys (true);
	}

	~ConfigEditHandler ()
	{
		ccl_forceGC ();
	}

	// EditHandler
	void onBegin () override
	{
		updateProperties ();
		return handler->onBegin ();
	}

	bool onMove (int moveFlags) override
	{
		IWindow::UpdateCollector uc (getEditView ()->getWindow ());

		EditHandler::onMove (moveFlags);

		updateProperties ();
		return handler->onMove (moveFlags);
	}

	void onRelease (bool canceled) override
	{
		EditHandler::onRelease (canceled);

		updateProperties ();
		handler->onRelease (canceled);

		// release circular reference through "editHandler" property
		handler.release ();
		handlerObj.release ();

		if(pendingAction && executer)
		{
			if(canceled)
			{
				// undo previous manipulation
				pendingAction->undoAll ();
				pendingAction.release ();
			}
			else
				executer->execute (pendingAction.detach ());
		}
	}

	void beginDirectManipulation (ActionExecuter& executer, StringRef description)
	{
		// undo previous manipulation
		if(pendingAction)
		{
			pendingAction->undoAll ();
			pendingAction.release ();
		}

		this->executer = &executer;

		// start a simple multiaction
		pendingAction = ActionExecuter (executer.getActionContext ()).beginMultiple (description);

		// executer could create a specialized multiaction
		executer.beginMultiple (nullptr);
	}

	void endDirectManipulation ()
	{
		if(pendingAction && executer)
		{
			// end inner multiaction
			executer->endMultiple (false);

			// execute the pending multiaction
			pendingAction->executeAll ();

			// transfer all subActions to a temporary Action
			AutoPtr<Action> tempAction (NEW MultiAction (pendingAction->getDescription ()));
			ForEach (*pendingAction, Action, subAction)
				subAction->retain ();
				tempAction->addAction (subAction);
			EndFor
			pendingAction->removeSubActions ();

			// cancel (remove pending from journal)
			ActionExecuter (executer->getActionContext ()).endMultiple (true);
			ASSERT (pendingAction->getRetainCount () == 1)

			// restore the pending multi action for undo in next turn or final commit
			pendingAction = tempAction;
			pendingAction->setExecuted (true);
		}
	}

	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override
	{
		if(msg == "beginManipulation")
		{
			ActionExecuter* executer = unknown_cast<ActionExecuter> (msg[0]);
			String description (msg[1].asString ());
			if(executer)
				beginDirectManipulation (*executer, description);
			return true;
		}
		else if(msg == "endManipulation")
		{
			endDirectManipulation ();
			return true;
		}
		else
			return SuperClass::invokeMethod (returnValue, msg);
	}

private:
	SharedPtr<IEditHandler> handler;
	UnknownPtr<IObject> handlerObj;
	SharedPtr<Action> pendingAction;
	SharedPtr<ActionExecuter> executer;

	void updateProperties ()
	{
		CCL_BOX (Boxed::MouseEvent, boxedFirst, first)
		CCL_BOX (Boxed::MouseEvent, boxedPrevious, previous)
		CCL_BOX (Boxed::MouseEvent, boxedCurrent, current)
		handlerObj->setProperty ("first", boxedFirst->asUnknown ());
		handlerObj->setProperty ("previous", boxedPrevious->asUnknown ());
		handlerObj->setProperty ("current", boxedCurrent->asUnknown ());
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (ConfigEditHandler, EditHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ConfigEditHandler)
	DEFINE_METHOD_NAME ("beginManipulation")
	DEFINE_METHOD_NAME ("endManipulation")
END_METHOD_NAMES (ConfigEditHandler)

} // namespace CCL

//************************************************************************************************
// ConfigTool::TouchMouseAction
/** Wrapper that feeds touch input into ConfigTool (as emulated mouse input). */
//************************************************************************************************

class ConfigTool::TouchMouseAction: public ToolAction
{
public:
	TouchMouseAction (ConfigTool* configTool)
	: configTool (configTool) {}

	EditHandler* perform (EditView& editView, const GUIEvent& event, PointRef where) override
	{
		if(auto gestureEvent = event.as<GestureEvent> ())
		{
			if(gestureEvent->getState () == GestureEvent::kBegin)
			{
				IView* view = editView;
				MouseEvent mouseEvent (AbstractTouchMouseHandler::makeMouseEvent (MouseEvent::kMouseDown, *gestureEvent, *view));
				return configTool->mouseDown (editView, mouseEvent);
			}
		}
		return nullptr;
	}
private:
	ConfigTool* configTool;
};

//************************************************************************************************
// ConfigTool
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ConfigTool, EditTool)

//////////////////////////////////////////////////////////////////////////////////////////////////

ConfigTool::ConfigTool (IToolConfiguration* config)
: config (config)
{
	ASSERT (config != nullptr)
	title = config->getTitle ();
	name = config->getName ();
	iconName = config->getIcon ();

	toolHelp = config;

	// create modes
	for(int i = 0, count = config->countModes (); i < count; i++)
	{
		AutoPtr<IToolMode> mode = config->createMode (i);
		ASSERT (mode)
		if(mode)
		{
			EditToolMode* toolMode = NEW EditToolMode;
			toolMode->setTitle (mode->getTitle ());
			toolMode->setName (MutableCString (mode->getName ()));
			toolMode->setIconName (MutableCString (mode->getIcon ()));

			if(IToolConfiguration* modeHandlerConfig = mode->getHandler ())
				toolMode->setHandler (NEW ConfigTool (modeHandlerConfig));
			addMode (toolMode);
		}
	}

	ignoresModeIcons (config->ignoresModeIcons ());

	ASSERT (!title.isEmpty ()) // title has to be translated!
	if(title.isEmpty ())
		title = String (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ConfigTool::setActiveMode (EditToolMode* mode)
{
	SuperClass::setActiveMode (mode);

	Variant var;
	if(mode)
		var = mode->getName ();

	UnknownPtr<IObject> configObj (config);
	configObj->setProperty ("activeMode", var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ConfigTool::mouseEnter (EditView& editView, const MouseEvent& mouseEvent)
{
	mouseMove (editView, mouseEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ConfigTool::mouseMove (EditView& editView, const MouseEvent& mouseEvent)
{
	if(editView.getMouseState ())
		return;

	action = config->findAction (editView, mouseEvent);
	if(action)
	{
		String cursor = action->getCursor (editView, mouseEvent);
		if(!cursor.isEmpty ())
			setMouseCursor (editView.getTheme ().getCursor (MutableCString (cursor)));
		else
			setMouseCursor (editView.getTheme ().getCursor (nullptr));

		wantsCrossCursor (action->wantsCrossCursor (editView, mouseEvent));
		ignoreModifier = action->getIgnoreModifier ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ConfigTool::mouseLeave (EditView& editView, const MouseEvent& mouseEvent)
{
	config->onMouseLeave (editView, mouseEvent);

	setMouseCursor (editView.getTheme ().getCursor (nullptr));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditHandler* ConfigTool::mouseDown (EditView& editView, const MouseEvent& mouseEvent)
{
	mouseMove (editView, mouseEvent);
	if(action)
		return action->onMouseDown (editView, mouseEvent);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* ConfigTool::createTouchHandler (EditView& editView, const TouchEvent& event)
{
	const TouchInfo* touch = event.touches.getTouchInfoByID (event.touchID);
	ASSERT (touch)
	if(touch)
	{
		Point where (touch->where);
		editView.windowToClient (where);

		ToolTouchHandler* handler = NEW ToolTouchHandler (editView);
		handler->getActions ().addAction (NEW TouchMouseAction (this), nullptr, ToolAction::kSingleTap|ToolAction::kDoubleTap|ToolAction::kLongPress);
		handler->prepareGestures ();
		return handler;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ConfigTool::getTooltip ()
{
	if(action)
		return action->getTooltip ();
	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPresentable* ConfigTool::createHelpInfo (EditView& editView, const MouseEvent& mouseEvent)
{
	if(toolHelp)
		return toolHelp->findHelp (editView, mouseEvent);
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void ConfigTool::onAttached (EditView& editView, bool state)
{
	config->onAttached (editView, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ConfigTool::onContextMenu (IContextMenu& contextMenu)
{
	return config->onContextMenu (contextMenu);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ConfigTool::extendModeMenu (IMenu& menu)
{
	return config->extendModeMenu (menu);
}

//************************************************************************************************
// ToolSetStub
//************************************************************************************************

class ToolSetStub: public StubObject,
				   public IToolSet
{
public:
	DECLARE_STUB_METHODS (IToolSet, ToolSetStub)

	int countConfigurations () override
	{
		Variant result;
		getProperty (result, "tools");
		UnknownPtr<IArrayObject> arrayObject (result.asUnknown ());
		return arrayObject ? arrayObject->getArrayLength () : 0;
	}

	IToolConfiguration* createConfiguration (int index) override
	{
		Variant result;
		getProperty (result, "tools");
		UnknownPtr<IArrayObject> arrayObject (result.asUnknown ());
		result.clear ();
		if(arrayObject && arrayObject->getArrayElement (result, index))
		{
			UnknownPtr<IToolConfiguration> config (result.asUnknown ());
			if(config)
				config->retain ();
			return config;
		}
		return nullptr;
	}
};

//************************************************************************************************
// ToolConfigurationStub
//************************************************************************************************

class ToolConfigurationStub: public StubObject,
							 public IToolConfiguration
{
public:
	DECLARE_STUB_METHODS (IToolConfiguration, ToolConfigurationStub)

	String getTitle () override
	{
		Variant result;
		getProperty (result, "title");
		return result.asString ();
	}

	String getName () override
	{
		Variant result;
		getProperty (result, "name");
		return result.asString ();
	}

	String getIcon () override
	{
		Variant result;
		getProperty (result, "icon");
		return result.asString ();
	}

	IToolAction* findAction (EditView& editView, const MouseEvent& mouseEvent) override
	{
		Variant returnValue;
		CCL_BOX (Boxed::MouseEvent, boxedEvent, mouseEvent)
		invokeMethod (returnValue, Message ("findAction", editView.asUnknown (), boxedEvent->asUnknown ()));
		UnknownPtr<IToolAction> action (returnValue);
		if(action)
			action->retain ();
		return action;
	}

	int countModes () override
	{
		Variant result;
		getProperty (result, "modes");
		UnknownPtr<IArrayObject> arrayObject (result.asUnknown ());
		return arrayObject ? arrayObject->getArrayLength () : 0;
	}

	IToolMode* createMode (int index) override
	{
		Variant result;
		getProperty (result, "modes");
		UnknownPtr<IArrayObject> arrayObject (result.asUnknown ());
		result.clear ();
		if(arrayObject && arrayObject->getArrayElement (result, index))
		{
			UnknownPtr<IToolMode> mode (result.asUnknown ());
			if(mode)
				mode->retain ();
			return mode;
		}
		return nullptr;
	}

	bool ignoresModeIcons () override
	{
		Variant result;
		return getProperty (result, "ignoresModeIcons") ? result.asBool () : false;
	}

	void onAttached (EditView& editView, bool state) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("onAttached", editView.asUnknown (), state));
	}

	void onMouseLeave (EditView& editView, const MouseEvent& mouseEvent) override
	{
		Variant returnValue;
		CCL_BOX (Boxed::MouseEvent, boxedEvent, mouseEvent)
		invokeMethod (returnValue, Message ("onMouseLeave", editView.asUnknown (), boxedEvent->asUnknown ()));
	}

	bool onContextMenu (IContextMenu& contextMenu) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("onContextMenu", &contextMenu));
		return returnValue.asBool ();
	}

	bool extendModeMenu (IMenu& menu) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("extendModeMenu", &menu));
		return returnValue.asBool ();
	}
};

//************************************************************************************************
// ToolHelpStub
//************************************************************************************************

class ToolHelpStub: public StubObject,
					public IToolHelp
{
public:
	DECLARE_STUB_METHODS (IToolHelp, ToolHelpStub)

	IPresentable* findHelp (EditView& editView, const MouseEvent& mouseEvent) override
	{
		Variant returnValue;
		CCL_BOX (Boxed::MouseEvent, boxedEvent, mouseEvent)
		invokeMethod (returnValue, Message ("findHelp", editView.asUnknown (), boxedEvent->asUnknown ()));
		UnknownPtr<IPresentable> p (returnValue.asUnknown ());
		return return_shared<IPresentable> (p);
	}
};

//************************************************************************************************
// ToolModeStub
//************************************************************************************************

class ToolModeStub: public StubObject,
					public IToolMode
{
public:
	DECLARE_STUB_METHODS (IToolMode, ToolModeStub)

	String getTitle () override
	{
		Variant result;
		getProperty (result, "title");
		return result.asString ();
	}

	String getName () override
	{
		Variant result;
		getProperty (result, "name");
		return result.asString ();
	}

	String getIcon () override
	{
		Variant result;
		getProperty (result, "icon");
		return result.asString ();
	}

	IToolConfiguration* getHandler () override
	{
		if(!handler)
		{
			Variant result;
			getProperty (result, "handler");
			UnknownPtr<IToolConfiguration> config (result.asUnknown ());
			if(config)
				handler.share (config);
		}
		return handler;
	}

private:
	AutoPtr<IToolConfiguration> handler;
};

//************************************************************************************************
// ToolActionStub
//************************************************************************************************

class ToolActionStub: public StubObject,
					  public IToolAction
{
public:
	DECLARE_STUB_METHODS (IToolAction, ToolActionStub)

	String getCursor (EditView& editView, const MouseEvent& mouseEvent) override
	{
		Variant result;
		getProperty (result, "cursor");

		if(result.isNil ())
		{
			CCL_BOX (Boxed::MouseEvent, boxedEvent, mouseEvent)
			invokeMethod (result, Message ("getCursor", editView.asUnknown (), boxedEvent->asUnknown ()));
		}

		return result.asString ();
	}

	String getTooltip () override
	{
		Variant result;
		getProperty (result, "tooltip");
		return result.asString ();
	}

	bool wantsCrossCursor (EditView& editView, const MouseEvent& mouseEvent) override
	{
		Variant result;
		getProperty (result, "crossCursor");
		return result.asBool ();
	}

	EditHandler* onMouseDown (EditView& editView, const MouseEvent& mouseEvent) override
	{
		// the script might release this object too early
		retain ();

		Variant returnValue;
		CCL_BOX (Boxed::MouseEvent, boxedEvent, mouseEvent)
		invokeMethod (returnValue, Message ("onMouseDown", editView.asUnknown (), boxedEvent->asUnknown ()));
		mouseEvent.doubleClicked = boxedEvent->doubleClicked;

		EditHandler* handler = unknown_cast<EditHandler> (returnValue.asUnknown ());
		if(handler)
			handler->retain ();
		else
		{
			// edit handler implemented in script
			UnknownPtr<IEditHandler> editHandler (returnValue.asUnknown ());
			if(editHandler)
				handler = NEW ConfigEditHandler (*editHandler, &editView);
		}

		release ();
		return handler;
	}
	
	int getIgnoreModifier () override
	{
		Variant result;
		if(getProperty (result, "ignoreModifier"))
			return result.asInt ();
		return 0;
	}

};

//************************************************************************************************
// EditHandlerHookStub
//************************************************************************************************

class EditHandlerHookStub: public StubObject,
						   public IEditHandlerHook
{
public:
	DECLARE_STUB_METHODS (IEditHandlerHook, EditHandlerHookStub)

	String getActionCode (EditView& editView, const MouseEvent& mouseEvent) override
	{
		Variant returnValue;
		CCL_BOX (Boxed::MouseEvent, boxedEvent, mouseEvent)
		invokeMethod (returnValue, Message ("getActionCode", editView.asUnknown (), boxedEvent->asUnknown ()));
		return returnValue.asString ();
	}

	String getCursor (EditView& editView, const MouseEvent& mouseEvent) override
	{
		Variant returnValue;
		CCL_BOX (Boxed::MouseEvent, boxedEvent, mouseEvent)
		invokeMethod (returnValue, Message ("getCursor", editView.asUnknown (), boxedEvent->asUnknown ()));
		return returnValue.asString ();
	}

	bool updateCrossCursor (bool& wantsCrossCursor, EditView& editView, const MouseEvent& mouseEvent) override
	{
		Variant result;
		if(getProperty (result, "crossCursor"))
		{
			wantsCrossCursor = result.asBool ();
			return true;
		}
		return false;
	}

	bool getHelp (IHelpInfoBuilder& helpInfo) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("getHelp", &helpInfo));
		return returnValue.asBool ();
	}

	void performActions (EditView& editView) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("performActions", editView.asUnknown ()));
	}

	void onRelease (EditView& editView, bool canceled) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("onRelease", editView.asUnknown (), canceled));
	}
};

//************************************************************************************************
// EditHandlerStub
//************************************************************************************************

class EditHandlerStub: public StubObject,
					   public IEditHandler
{
public:
	DECLARE_STUB_METHODS (IEditHandler, EditHandlerStub)

	void onBegin () override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("onBegin"));
	}

	bool onMove (int moveFlags) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("onMove", moveFlags));
		return returnValue.asBool ();
	}

	void onRelease (bool canceled) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("onRelease", canceled));

		ccl_forceGC ();
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (ToolConfiguration, kFirstRun)
{
	REGISTER_STUB_CLASS (IToolConfiguration, ToolConfigurationStub)
	REGISTER_STUB_CLASS (IToolMode, ToolModeStub)
	REGISTER_STUB_CLASS (IToolHelp, ToolHelpStub)
	REGISTER_STUB_CLASS (IToolSet, ToolSetStub)
	REGISTER_STUB_CLASS (IToolAction, ToolActionStub)
	REGISTER_STUB_CLASS (IEditHandlerHook, EditHandlerHookStub)
	REGISTER_STUB_CLASS (IEditHandler, EditHandlerStub)
	return true;
}
