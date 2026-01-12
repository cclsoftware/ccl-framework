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
// Filename    : ccl/gui/guistubs.cpp
// Description : GUI Stub Classes
//
//************************************************************************************************

#define CCL_BOX_STATIC // use static "boxing"

#include "ccl/public/plugins/stubobject.h"

#include "ccl/public/gui/icommandhandler.h"
#include "ccl/public/gui/icontextmenu.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/iparamobserver.h"
#include "ccl/public/gui/iviewstate.h"
#include "ccl/public/gui/framework/itimer.h"

#include "ccl/base/message.h"
#include "ccl/base/boxedtypes.h"

#include "ccl/public/storage/iattributelist.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// BoxedCommandMsg
//************************************************************************************************

class BoxedCommandMsg: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (BoxedCommandMsg, Object)

	String category;
	String name;
	IUnknown* invoker;
	bool checkOnly;

	typedef Boxed::ValueHelper<BoxedCommandMsg, CommandMsg> Value;

	BoxedCommandMsg (const CommandMsg& msg = CommandMsg ())
	: category (msg.category),
	  name (msg.name),
	  invoker (msg.invoker),
	  checkOnly (msg.checkOnly ())
	{}

	BoxedCommandMsg& operator = (const CommandMsg& msg)
	{
		category = String (msg.category);
		name = String (msg.name);
		invoker = msg.invoker;
		checkOnly = msg.checkOnly ();
		return *this;
	}

	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override
	{
		if(propertyId == "category")
		{
			var = category;
			return true;
		}
		else if(propertyId == "name")
		{
			var = name;
			return true;
		}
		else if(propertyId == "arguments")
		{
			UnknownPtr<IAttributeList> arguments (invoker);
			var.takeShared (arguments);
			return true;
		}
		else if(propertyId == "checkOnly")
		{
			var = checkOnly;
			return true;
		}
		return Object::getProperty (var, propertyId);
	}
};

DEFINE_CLASS_ABSTRACT_HIDDEN (BoxedCommandMsg, Object)

//************************************************************************************************
// CommandHandlerStub
//************************************************************************************************

class CommandHandlerStub: public StubObject,
						  public ICommandHandler
{
public:
	DECLARE_STUB_METHODS (ICommandHandler, CommandHandlerStub)

	// ICommandHandler
	tbool CCL_API checkCommandCategory (CStringRef category) const override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("checkCommandCategory", String (category)));
		return returnValue.asBool ();
	}

	tbool CCL_API interpretCommand (const CommandMsg& commandMsg) override
	{
		Variant returnValue;
		CCL_BOX (BoxedCommandMsg, boxedMessage, commandMsg)
		invokeMethod (returnValue, Message ("interpretCommand", static_cast<IObject*> (boxedMessage)));
		return returnValue.asBool ();
	}
};

//************************************************************************************************
// ContextMenuStub
//************************************************************************************************

class ContextMenuStub: public StubObject,
                       public IContextMenuHandler
{
public:
	DECLARE_STUB_METHODS (IContextMenuHandler, ContextMenuStub)

	tresult CCL_API appendContextMenu (IContextMenu& contextMenu) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("appendContextMenu", &contextMenu));
		ccl_markGC (&contextMenu);
		return returnValue.asResult ();
	}
};

//************************************************************************************************
// ParamObserverStub
//************************************************************************************************

class ParamObserverStub: public StubObject,
						 public IParamObserver
{
public:
	DECLARE_STUB_METHODS (IParamObserver, ParamObserverStub)

	// IParamObserver
	tbool CCL_API paramChanged (IParameter* param) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("paramChanged", param));
		return returnValue.asBool ();
	}

	void CCL_API paramEdit (IParameter* param, tbool begin) override
	{}
};

//************************************************************************************************
// ViewStateHandlerStub
//************************************************************************************************

class ViewStateHandlerStub: public StubObject,
							public IViewStateHandler
{
public:
	DECLARE_STUB_METHODS (IViewStateHandler, ViewStateHandlerStub)

	// IViewStateHandler
	tbool CCL_API saveViewState (StringID viewID, StringID viewName, IAttributeList& attributes, const IViewState* state) const override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("saveViewState", String (viewID), String (viewName), &attributes));
		return returnValue.asBool ();
	}

	tbool CCL_API loadViewState (StringID viewID, StringID viewName, const IAttributeList& attributes, IViewState* state) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("loadViewState", String (viewID), String (viewName), const_cast<IAttributeList*> (&attributes)));
		return returnValue.asBool ();
	}
};

//************************************************************************************************
// TimerTaskStub
//************************************************************************************************

class TimerTaskStub: public StubObject,
					 public ITimerTask
{
public:
	DECLARE_STUB_METHODS (ITimerTask, TimerTaskStub)

	// ITimerTask
	void CCL_API onTimer (ITimer* timer) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("onTimer"));
	}
};

//************************************************************************************************
// GUIStubClasses
//************************************************************************************************

CCL_KERNEL_INIT_LEVEL (GUIStubClasses, kFrameworkLevelFirst)
{
	REGISTER_STUB_CLASS (ICommandHandler, CommandHandlerStub)
	REGISTER_STUB_CLASS (IContextMenuHandler, ContextMenuStub)
	REGISTER_STUB_CLASS (IParamObserver, ParamObserverStub)
	REGISTER_STUB_CLASS (IViewStateHandler, ViewStateHandlerStub)
	REGISTER_STUB_CLASS (ITimerTask, TimerTaskStub)
	return true;
}
