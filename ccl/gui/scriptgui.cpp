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
// Filename    : ccl/gui/scriptgui.cpp
// Description : Scripting GUI
//
//************************************************************************************************

#include "ccl/gui/scriptgui.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/commands.h"
#include "ccl/gui/theme/theme.h"
#include "ccl/gui/theme/thememanager.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/dialogs/alert.h"
#include "ccl/gui/dialogs/dialogbuilder.h"
#include "ccl/gui/dialogs/fileselector.h"
#include "ccl/gui/help/helpmanager.h"
#include "ccl/gui/system/clipboard.h"

#include "ccl/base/kernel.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/configuration.h"

#include "ccl/base/collections/arraybox.h"

#include "ccl/app/params.h"
#include "ccl/app/paramcontainer.h"
 
#include "ccl/public/collections/hashmap.h"
#include "ccl/public/system/ipackagemetainfo.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/plugins/icoderesource.h"
#include "ccl/public/plugins/stubobject.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// ScriptComponentStub
//************************************************************************************************

class ScriptComponentStub: public StubObject, 
						   public IScriptComponent
{
public:
	DECLARE_STUB_METHODS (IScriptComponent, ScriptComponentStub)

	// IScriptComponent
	void CCL_API construct (IUnknown* outerComponent) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("construct", outerComponent));
	}
};

//************************************************************************************************
// FunctionParam
//************************************************************************************************

class FunctionParam: public Parameter
{
public:
	DECLARE_CLASS (FunctionParam, Parameter)

	FunctionParam (StringID name = nullptr)
	: Parameter (name)
	{}
};

DEFINE_CLASS (FunctionParam, Parameter)

//************************************************************************************************
// ControllerStub
//************************************************************************************************

class ControllerStub: public StubObject,
					  public IController
{
public:
	DECLARE_STUB_METHODS (IController, ControllerStub)

	// IController
	int CCL_API countParameters () const override
	{
		AutoPtr<IController> paramList = accessParamList ();
		return paramList ? paramList->countParameters () : 0;
	}

	IParameter* CCL_API getParameterAt (int index) const override
	{
		AutoPtr<IController> paramList = accessParamList ();
		return paramList ? paramList->getParameterAt (index) : nullptr;
	}

	IParameter* CCL_API findParameter (StringID name) const override
	{
		AutoPtr<IController> paramList = accessParamList ();
		return paramList ? paramList->findParameter (name) : nullptr;
	}

	IParameter* CCL_API getParameterByTag (int tag) const override
	{
		AutoPtr<IController> paramList = accessParamList ();
		return paramList ? paramList->getParameterByTag (tag) : nullptr;
	}

	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override
	{
		Variant var;
		getProperty (var, name);

		// This might be a stub instance which needs to be kept alive after this call returns. 
		IUnknown* unk = var.asUnknown ();
		Kernel::instance ().deferDestruction (return_shared (unk));
		return unk;
	}

protected:
	IController* accessParamList () const
	{
		Variant var;
		getProperty (var, "paramList");
		UnknownPtr<IController> paramList (var.asUnknown ());
		return paramList.detach ();
	}
};

//************************************************************************************************
// ScriptParamContainer
//************************************************************************************************

class ScriptParamContainer: public ParamContainer
{
public:
	DECLARE_CLASS (ScriptParamContainer, ParamContainer)
	PROPERTY_SHARED_AUTO (IUnknown, sharedController, SharedController)

	// IObject
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
};

//************************************************************************************************
// ScriptLoaderHook
//************************************************************************************************

class ScriptLoaderHook: public Object,
						public ICodeResourceLoaderHook
{
public:
	// ICodeResourceLoaderHook
	void CCL_API onLoad (ICodeResource& codeResource) override;
	void CCL_API onUnload (ICodeResource& codeResource) override;

	CLASS_INTERFACE (ICodeResourceLoaderHook, Object)

protected:
	PointerHashMap<ITheme*> themeMap;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// Initialization
//************************************************************************************************

CCL_KERNEL_INIT_LEVEL (ScriptGUI, kFrameworkLevelFirst)
{
	REGISTER_STUB_CLASS (IController, ControllerStub)
	REGISTER_STUB_CLASS (IScriptComponent, ScriptComponentStub)
	return true;
}

//************************************************************************************************
// ScriptParamContainer
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (ScriptParamContainer, ParamContainer, "ParamList")
DEFINE_CLASS_UID (ScriptParamContainer, 0xbf3fa199, 0x8290, 0x4a4a, 0x9c, 0xd3, 0x59, 0xad, 0x40, 0x33, 0xb0, 0xe2)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptParamContainer::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "controller")
	{
		UnknownPtr<IParamObserver> controller (var.asUnknown ());
		setController (controller);
		setSharedController (controller); // ensure that stub object keeps alive!
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//************************************************************************************************
// ScriptComponent
//************************************************************************************************

DEFINE_CLASS (ScriptComponent, Object)
DEFINE_CLASS_UID (ScriptComponent, 0xead8461f, 0xd56a, 0x4cc0, 0x87, 0x4d, 0x83, 0x19, 0xb7, 0x3c, 0x30, 0x7a)
DEFINE_IID_ (IScriptComponent, 0x23e05a3c, 0xa606, 0x43e3, 0xa5, 0xdf, 0x63, 0xee, 0xe4, 0xbb, 0xc4, 0xc7)

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptComponent::ScriptComponent (IUnknown* object)
: scriptObject (nullptr)
{
	functions.objectCleanup (true);

	if(object)
		construct (object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptComponent::~ScriptComponent ()
{
	VectorForEach (children, IObjectNode*, child)
		child->release ();
	EndFor

	if(scriptObject)
		scriptObject->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScriptComponent::construct (IUnknown* object)
{
	ASSERT (scriptObject == nullptr) // must be called only once!
	if(scriptObject != nullptr)
		return;

	if(object)
		object->queryInterface (ccl_iid<IObject> (), (void**)&scriptObject);

	// init function
	// Note: Invoking non-existent methods would cause a scripting exception,
	// thus the script object has to implement IScriptComponent for the construct() call.
	UnknownPtr<IScriptComponent> component (scriptObject);
	if(component)
		component->construct (this->asUnknown ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ScriptComponent::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (IObjectNode)
	QUERY_INTERFACE (IController)
	QUERY_INTERFACE (IScriptComponent)
	QUERY_INTERFACE (IParamObserver)
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* CCL_API ScriptComponent::getRoot () const
{
	return const_cast<ScriptComponent*> (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ScriptComponent::countChildren () const
{
	return children.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* CCL_API ScriptComponent::getChild  (int index) const
{
	return children.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* CCL_API ScriptComponent::findChild (StringRef id) const
{
	VectorForEach (children, IObjectNode*, child)
		if(child->getObjectID () == id)
			return child;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API ScriptComponent::getObjectID () const
{
	return CCLSTR ("ScriptComponent");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API ScriptComponent::findParameter (StringID name) const
{
	ScriptComponent* This = const_cast<ScriptComponent*> (this);
	if(name.contains ("(") && name.contains(")"))
	{
		ForEach (functions, FunctionParam, param)
			if(param->getName () == name)
				return param;
		EndFor

		FunctionParam* param = NEW FunctionParam (name);
		This->functions.add (param);
		param->connect (This, functions.index (param));
		return param;
	}
	else if(scriptObject)
	{
		// Note: This simplifies the script code. Instead of implementing IController,
		// parameters can be properties of the script object itself.

		Variant var;
		scriptObject->getProperty (var, name);

		UnknownPtr<IParameter> param (var.asUnknown ());
		if(param && param->getController () == nullptr)
		{
			int tag = (int)name.getHashCode ();
			param->connect (This, tag);
		}

		return param;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptComponent::paramChanged (IParameter* param)
{
	FunctionParam* functionParam = unknown_cast<FunctionParam> (param);
	if(functionParam)
	{
		String methodId = (const char*)functionParam->getName ();
		while(methodId.lastChar () == '(' || methodId.lastChar () == ')' || methodId.lastChar () == ' ')
			methodId.remove (methodId.length () - 1, 1);

		char cmethodId[255];
		methodId.toASCII (cmethodId, 255);

		Variant returnValue;
		if(scriptObject)
			scriptObject->invokeMethod (returnValue, Message (cmethodId));
	}
	else
	{
		// Note: Invoking non-existent methods would cause a scripting exception,
		// thus the script object has to implement IParamObserver for the paramChanged() call.
		UnknownPtr<IParamObserver> controller (scriptObject);
		if(controller)
			controller->paramChanged (param);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScriptComponent::notify (ISubject* subject, MessageRef msg)
{
	// delegate to script object, could be a parameter notification
	UnknownPtr<IObserver> observer (scriptObject);
	if(observer)
		observer->notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptComponent::getProperty (Variant& var, MemberID propertyId) const
{
	// delegate to script object, could be a property needed by the skin
	if(scriptObject)
		if(scriptObject->getProperty (var, propertyId))
			return true;
	
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ScriptComponent)
	DEFINE_METHOD_NAME ("construct")
	DEFINE_METHOD_NAME ("addChild")
END_METHOD_NAMES (ScriptComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptComponent::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "construct")
	{
		construct (msg[0]);
		return true;
	}
	else if(msg == "addChild")
	{
		UnknownPtr<IObjectNode> child (msg[0]);
		if(child)
		{
			child->retain ();
			children.add (child);
		}
		returnValue = child.isValid ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// ScriptGUIHost::ConstantList
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ScriptGUIHost::ConstantList, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_STYLEDEF (ScriptGUIHost::ConstantList::valueNames)
	// Mouse States
	{"kMouseNone",		IView::kMouseNone},
	{"kMouseDown",		IView::kMouseDown},
	{"kMouseOver",		IView::kMouseOver},

	// Key States
	{"kLButton",		KeyState::kLButton},
	{"kMButton",		KeyState::kMButton},
	{"kRButton",		KeyState::kRButton},
	{"kShift",			KeyState::kShift},
	{"kCommand",		KeyState::kCommand},
	{"kOption",			KeyState::kOption},
	{"kControl",		KeyState::kControl},

	// Gestures
	{"kClick",			KeyState::kClick},
	{"kDrag",			KeyState::kDrag},
	{"kDoubleClick",	KeyState::kDoubleClick},
	{"kWheel",			KeyState::kWheel},

	// Dialog Results
	{"kCancel",			DialogResult::kCancel},
	{"kOkay",			DialogResult::kOkay},
	{"kClose",			DialogResult::kClose},
	{"kApply",			DialogResult::kApply},

	// Alert Results
	{"kYes",			Alert::kYes},
	{"kOk",				Alert::kOk},
	{"kRetry",			Alert::kRetry},
	{"kNo",				Alert::kNo},
	{"kAlertCancel",	Alert::kCancel}, // avoid name clash with DialogResult::kCancel
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptGUIHost::ConstantList::getProperty (Variant& var, MemberID propertyId) const
{
	for(int i = 0; i < ARRAY_COUNT (valueNames) && valueNames[i].name; i++)
		if(propertyId == valueNames[i].name)
		{
			var = valueNames[i].value;
			return true;
		}

	CCL_DEBUGGER ("GUI Constant not found!")

	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptGUIHost::ConstantList::getPropertyNames (IPropertyCollector& collector) const
{
	for(int i = 0; i < ARRAY_COUNT (valueNames) && valueNames[i].name; i++)
		collector.addPropertyName (valueNames[i].name);
	return true;
}

//************************************************************************************************
// ScriptGUIHost
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (ScriptGUIHost, Object)
DEFINE_CLASS_NAMESPACE (ScriptGUIHost, NAMESPACE_CCL)
DEFINE_SINGLETON (ScriptGUIHost)

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptGUIHost::ScriptGUIHost ()
: loaderHook (NEW ScriptLoaderHook),
  constantList (NEW ConstantList)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptGUIHost::~ScriptGUIHost ()
{
	loaderHook->release ();
	constantList->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ScriptGUIHost::runDialog (Theme* theme, StringID form, IUnknown* controller, int buttons)
{		
	int result = DialogResult::kCancel;

	DialogBuilder builder;
	builder.setTheme (theme);
	View* view = unknown_cast<View> (theme->createView (form, controller));
	if(view)
		result = builder.runDialog (view, Styles::dialogWindowStyle, buttons, nullptr);

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ICodeResourceLoaderHook* ScriptGUIHost::getHook ()
{
	return loaderHook;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ScriptGUIHost)
	DEFINE_METHOD_NAME ("flushUpdates")
	DEFINE_METHOD_ARGS ("alert", "text")
	DEFINE_METHOD_ARGS ("ask", "text")
	DEFINE_METHOD_ARGR ("runDialog", "theme: Object, formName: string, controller: Object = null, buttons: int = 0", "int") // TODO: replace 'Object' with derived types
	DEFINE_METHOD_ARGR ("runDialogWithParameters", "paramList, title", "int")
	DEFINE_METHOD_NAME ("addIdleTask")
	DEFINE_METHOD_NAME ("removeIdleTask")
	DEFINE_METHOD_NAME ("openUrl")
	DEFINE_METHOD_NAME ("showFile")
	DEFINE_METHOD_NAME ("keyStateToString")
END_METHOD_NAMES (ScriptGUIHost)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptGUIHost::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "flushUpdates")
	{
		bool wait = msg.getArgCount () >= 1 ? msg[0].asBool () : true;
		GUI.flushUpdates (wait);
		return true;
	}
	else if(msg == "alert")
	{
		String text (msg[0].asString ());
		// TODO: alert type!
		Alert::warn (text);
		return true;
	}
	else if(msg == "ask")
	{
		String text (msg[0].asString ());
		// TODO: alert type!
		returnValue = Alert::ask (text);
		return true;
	}
	else if(msg == "runDialog")
	{
		int result = DialogResult::kCancel;
		Theme* theme = nullptr;
		if(msg[0].isString ())
			theme = unknown_cast<Theme> (ThemeManager::instance ().getTheme (MutableCString (msg[0].asString ())));
		else
			theme = unknown_cast<Theme> (msg[0]);				
		MutableCString formName (msg[1].asString ());
		if(theme && !formName.isEmpty ())
		{
			IUnknown* unknown = msg.getArgCount () > 2 ? msg[2].asUnknown () : nullptr;
			AutoPtr<IController> controller;
			controller.share (UnknownPtr<IController> (unknown)); // check if script provides the controller itself
			
			if(!controller) 
				controller = NEW ScriptComponent (unknown); // legacy code, needs cleanup!

			int buttons = 0;
			if(msg.getArgCount () > 3)
				buttons = msg[3].asInt () << 16;

			result = runDialog (theme, formName, controller, buttons);
		}
		returnValue = result;
		return true;
	}
	else if(msg == "runDialogWithParameters")
	{
		int result = DialogResult::kCancel;
		UnknownPtr<IController> paramList (msg[0].asUnknown ());
		String title (msg[1].asString ());
		ASSERT (paramList)
		if(paramList)
			result = DialogBuilder ().runWithParameters (CCLSTR ("ScriptDialog"), *paramList, title);
		returnValue = result;
		return true;
	}
	else if(msg == "addIdleTask")
	{
		UnknownPtr<ITimerTask> task (msg[0]);
		ASSERT (task != nullptr)
		if(task)
			GUI.addIdleTask (task);
		return true;
	}
	else if(msg == "removeIdleTask")
	{
		UnknownPtr<ITimerTask> task (msg[0]);
		ASSERT (task != nullptr)
		if(task)
			GUI.removeIdleTask (task);
		return true;
	}
	else if(msg == "openUrl" || msg == "showFile")
	{
		tresult tr = kResultInvalidArgument;
		UnknownPtr<IUrl> url (msg[0]);
		if(url)
		{
			if(msg == "openUrl")
				tr = System::GetSystemShell ().openUrl (*url);
			else
				tr = System::GetSystemShell ().showFile (*url);
		}
		returnValue = tr;
		return true;
	}
	else if(msg == "keyStateToString")
	{
		String string;
		KeyState (msg[0].asInt ()).toString (string);
		returnValue = Variant (string, true);
		return true;
	}
	else
		return Object::invokeMethod (returnValue, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (ScriptGUIHost)
	DEFINE_PROPERTY_NAME ("Constants")
	DEFINE_PROPERTY_CLASS_ ("Commands", "CommandTable", CCL::ITypeInfo::kReadOnly)
	DEFINE_PROPERTY_NAME ("Themes")
	DEFINE_PROPERTY_NAME ("Desktop")
	DEFINE_PROPERTY_NAME ("Help")
	DEFINE_PROPERTY_NAME ("Configuration")
	DEFINE_PROPERTY_NAME ("Clipboard")
	DEFINE_PROPERTY_NAME ("Alerts")
END_PROPERTY_NAMES (ScriptGUIHost)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptGUIHost::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "Constants")
	{
		var = ccl_as_unknown (constantList);
		return true;
	}
	if(propertyId == "Commands")
	{
		var = ccl_as_unknown (CommandTable::instance ());
		return true;
	}
	else if(propertyId == "Themes")
	{
		var = ccl_as_unknown (ThemeManager::instance ());
		return true;
	}
	else if(propertyId == "Desktop")
	{
		var = ccl_as_unknown (Desktop); // automatic object!
		return true;
	}
	else if(propertyId == "Help")
	{
		var = ccl_as_unknown (HelpManager::instance ());
		return true;
	}
	else if(propertyId == "Configuration")
	{
		var = ccl_as_unknown (Configuration::Registry::instance ());
		return true;
	}
	else if(propertyId == "Clipboard")
	{
		var = ccl_as_unknown (Clipboard::instance ());
		return true;
	}
	else if(propertyId == "Alerts")
	{
		var = ccl_as_unknown (AlertService::instance ());
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptGUIHost::setProperty (MemberID propertyId, const Variant& var)
{
	return false;
}

//************************************************************************************************
// ScriptLoaderHook
//************************************************************************************************

void CCL_API ScriptLoaderHook::onLoad (ICodeResource& codeResource)
{
	if(codeResource.getType () != CodeResourceType::kScript)
		return;
	
	IAttributeList* metaInfo = codeResource.getMetaInfo ();
	if(metaInfo)
	{
		AttributeAccessor accessor (*metaInfo);
		MutableCString packageId = accessor.getCString (Meta::kPackageID);
		String skinFile = accessor.getString ("Package:SkinFile");
		if(!packageId.isEmpty () && !skinFile.isEmpty ())
		{
			// try to find a translation table
			MutableCString sharedTableId = accessor.getCString (Meta::kTranslationSharedTableID);
			MutableCString tableId = sharedTableId.isEmpty () ? packageId : sharedTableId;
			ITranslationTable* stringTable = System::GetLocaleManager ().getStrings (tableId);

			PackageUrl path (String (packageId), skinFile, IUrl::kDetect); // detect type!

			ITheme* theme = nullptr;
			ThemeManager::instance ().loadTheme (theme, path, packageId, stringTable);
			ASSERT (theme != nullptr)
			themeMap.add (&codeResource, theme);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScriptLoaderHook::onUnload (ICodeResource& codeResource)
{
	if(codeResource.getType () != CodeResourceType::kScript)
		return;

	// unload theme
	ITheme* theme = themeMap.lookup (&codeResource);
	if(theme)
	{
		ThemeManager::instance ().unloadTheme (theme);
		themeMap.remove (&codeResource);
	}
}
