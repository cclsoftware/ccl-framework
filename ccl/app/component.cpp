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
// Filename    : ccl/app/component.cpp
// Description : Component class
//
//************************************************************************************************

#define DEBUG_LOG 0
#define DEBUG_TERMINATE (0 && DEBUG_LOG)

#include "ccl/app/component.h"

#include "ccl/base/message.h"
#include "ccl/base/boxedtypes.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/text/stringbuilder.h"

#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/plugins/versionnumber.h"
#include "ccl/public/plugins/iobjecttable.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/ithememanager.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/cclversion.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Application module check
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace CCL 
{
	static bool inMainAppModule = false;

	bool System::IsInMainAppModule ()
	{
		return inMainAppModule || System::IsInMainModule ();
	}

	void System::SetInMainAppModule (bool state)
	{
		inMainAppModule = state;
	}

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization/Termination
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (RootComponent, kAppLevel)
{
	auto& root = RootComponent::instance ();
	tresult result = root.initialize (root.asUnknown ());
	return result == kResultTrue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_TERM_LEVEL (RootComponent, kAppLevel)
{
	auto& root = RootComponent::instance ();
	ASSERT (root.canTerminate () != 0)

	#if DEBUG_TERMINATE
	CCL_PRINT ("\n\n")
	CCL_PRINT ("**************************************************\n");
	CCL_PRINT ("*** T E R M I N A T I N G  C O M P O N E N T S ***\n");
	CCL_PRINT ("**************************************************\n");
	#endif

	root.terminate ();

	#if DEBUG_TERMINATE
	CCL_PRINT ("**************************************************\n");
	CCL_PRINT ("**************************************************\n");
	CCL_PRINT ("\n\n");
	#endif

	root.signal (Message (kDestroyed)); // give observers a chance to unlink before childs are removed
	root.removeAll ();
}

//************************************************************************************************
// Component
//************************************************************************************************

DEFINE_CLASS (Component, ObjectNode)
DEFINE_CLASS_NAMESPACE (Component, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

Component::Component (StringRef name, StringRef title)
: ObjectNode (name),
  title (title),
  context (nullptr),
  flags (0)
{
	paramList.setController (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Component::~Component ()
{
	ASSERT (signalSlots.isEmpty ())

	ASSERT (context == nullptr)
	if(context)
		context->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Component::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (IComponent)
	QUERY_INTERFACE (IExtensible)
	QUERY_INTERFACE (IController)
	QUERY_INTERFACE (IParamObserver)
	QUERY_INTERFACE (IViewFactory)
	QUERY_INTERFACE (ICommandHandler)
	QUERY_INTERFACE (IContextMenuHandler)
	return ObjectNode::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITheme* Component::getTheme () const
{
	auto& root = RootComponent::instance ();
	return root.getTheme ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef Component::getTitle () const
{
	if(!title.isEmpty ())
		return title;
	return getName ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Component::setTitle (StringRef _title)
{
	title = _title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Component::hasTitle () const
{
	return title.isEmpty () == false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* Component::getContext () const
{
	return context;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Component::addComponent (Component* c)
{
	return addChild (c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Component::addIComponent (IComponent* ic)
{
	Component* c = unknown_cast<Component> (ic);
	ASSERT (c) // component must be implemented in current module!
	if(!c)
	{
		safe_release (ic);
		return false;
	}

	return addComponent (c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Component::signalHasChild (Component* c)
{
	signalHasChild (c->getName ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Component::signalHasChild (StringRef name)
{
	MutableCString prop (String ("hasChild[") << name << CCLSTR ("]"));
	signalPropertyChanged (prop);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* Component::resolveToOriginalContext (IUnknown* context)
{
	if(UnknownPtr<IComponentAlias> alias = context)
		if(isEqualUnknown (alias->getPlugInUnknown (), asUnknown ()))
			context = alias->getHostContext ();
	return context;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Component::initialize (IUnknown* _context)
{
	ASSERT (context == nullptr)
	take_shared<IUnknown> (context, _context);

	tresult result = kResultOk;
	if(countChildren () > 0)
	{
		for(int i = 0; i < getChildren ().count (); i++)
		{
			Component* c = (Component*)getChildren ().at (i);
			result = c->initialize (_context);
			if(result != kResultOk)
			{
				for(int j = i - 1; j >= 0; j--)
				{
					c = (Component*)getChildren ().at (j);
					c->terminate ();
				}

				take_shared<IUnknown> (context, nullptr);
				break;
			}
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Component::terminate ()
{
	#if DEBUG_TERMINATE
	IObjectNode* g = getParent ();
	int level = 0;
	for(; g; g = g->getParent (), level++);

	for(int i = 0; i < level; i++)
		CCL_PRINT ("+")

	CCL_PRINTF ("- (%d) Terminating ", level)
	CCL_PRINT (myClass ().getPersistentName ())
	CCL_PRINT (" \"")
	CCL_PRINT (getName ())
	CCL_PRINTLN ("\" ***")
	#endif

	if(countChildren () > 0)
		ArrayForEachReverse (getChildren (), Component, c)
			c->terminate ();
		EndFor

	take_shared<IUnknown> (context, nullptr);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Component::canTerminate () const
{
	if(countChildren () > 0)
		ArrayForEach (getChildren (), Component, c)
			if(!c->canTerminate ())
				return false;
		EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API Component::getExtension (StringID id)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Component::addObject (StringID name, IUnknown* object)
{
	objects.append (ObjectEntry (name, object));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Component::addObject (StringID name, Object* object)
{
	objects.append (ObjectEntry (name, ccl_as_unknown (object)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API Component::getObject (StringID name, UIDRef classID)
{
	ListForEach (objects, ObjectEntry, e)
		if(e.name == name)
			return e.object;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Component::paramChanged (IParameter* param)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Component::paramEdit (IParameter* param, tbool begin)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API Component::createView (StringID name, VariantRef data, const Rect& bounds)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Component::checkCommandCategory (CStringRef category) const
{
	return countChildren () > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Component::interpretCommand (const CommandMsg& msg)
{
	CCL_PRINTF ("Component::interpretCommand %s%s [%s]\n", msg.checkOnly () ? "(checkOnly)" : "", CCL_INDENT, myClass ().getPersistentName ());
	CCL_ADD_INDENT (2)
	if(countChildren () > 0)
		ArrayForEach (getChildren (), Component, c)
			if(c->checkCommandCategory (msg.category))
			{
				if(c->interpretCommand (msg))
					return true;
			}
		EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Component::appendContextMenu (IContextMenu& contextMenu)
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Component::load (const Storage& storage)
{
	// Note: Subclass needs to call loadIdentity() + loadChildren() if appropriate.

	if(paramList.count () > 0)
		paramList.load (storage);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Component::save (const Storage& storage) const
{
	// Note: Subclass needs to call saveIdentity() + saveChildren() if appropriate.

	if(paramList.count () > 0)
		paramList.save (storage);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Component::loadIdentity (const Storage& storage)
{
	if(storage.getArchive ())
	{
		StringID saveType = storage.getSaveType ();
		if(saveType == Archive::kSaveTypeCopy)
			return true;

		if(saveType == Archive::kSaveTypeUndo)
			if(isValidObjectUID ())
				return true;
	}

	Attributes& a = storage.getAttributes ();

	String name;
	if(a.get (name, "name"))
		setName (name);

	Boxed::UID uid;
	if(a.get (uid, "uniqueID"))
		setObjectUID (uid);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Component::saveIdentity (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();

	if(!getName ().isEmpty ())
		a.set ("name", getName ());

	if(isValidObjectUID ())
	{
		Boxed::UID uid (getObjectUID ());
		a.set ("uniqueID", uid, true);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Component::saveChild (const Storage& storage, const Component& child) const
{
	AutoPtr<Attributes> a2 = NEW Attributes;
	bool result = child.save (Storage (*a2, storage));
	ASSERT (result == true)
	if(result && !a2->isEmpty ())
	{
		MutableCString name (child.getName ());
		ASSERT (name.isEmpty () == false)
		CCL_PRINTF ("Component: Saving child %s\n", name.str ())
		storage.getAttributes ().set (name, a2, Attributes::kShare);
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Component::loadChild (const Storage& storage, Component& child)
{
	bool result = false;
	MutableCString name (child.getName ());
	ASSERT (name.isEmpty () == false)
	Attributes* a2 = storage.getAttributes ().getAttributes (name);
	if(a2)
	{
		CCL_PRINTF ("Component: Loading child %s\n", name.str ())
		result = child.load (Storage (*a2, storage));
		ASSERT (result == true)
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Component::saveChildren (const Storage& storage, bool onlyStorable) const
{
	if(countChildren () > 0)
		ArrayForEach (getChildren (), Component, c)
			if(onlyStorable && !c->isStorable ())
				continue;

			AutoPtr<Attributes> a2 = NEW Attributes;
			bool result = c->save (Storage (*a2, storage));
			ASSERT (result == true)
			if(result && !a2->isEmpty ())
			{
				MutableCString name (c->getName ());
				ASSERT (name.isEmpty () == false)
				CCL_PRINTF ("Component: Saving child %s\n", name.str ())
				storage.getAttributes ().set (name, a2, Attributes::kShare);
			}
		EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Component::loadChildren (const Storage& storage, bool loadEmpty, bool onlyStorable)
{
	if(countChildren () > 0)
	{
		Attributes emptyAttributes;
		ArrayForEach (getChildren (), Component, c)
			if(onlyStorable && !c->isStorable ())
				continue;

			MutableCString name (c->getName ());
			ASSERT (name.isEmpty () == false)
			Attributes* childAttributes = storage.getAttributes ().getAttributes (name);
			if(childAttributes == nullptr && loadEmpty)
				childAttributes = &emptyAttributes;
			if(childAttributes)
			{
				CCL_PRINTF ("Component: Loading child %s\n", name.str ())
				bool result = c->load (Storage (*childAttributes, storage));
				ASSERT (result == true)
			}
		EndFor
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Component::signalPropertyChanged (StringID propertyID, bool deferred)
{
	if(deferred)
		deferSignal (NEW Message (kPropertyChanged, String (propertyID)));
	else
		signal (Message (kPropertyChanged, String (propertyID)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Component::propertyChanged (StringID propertyID)
{
	signalPropertyChanged (propertyID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (Component)
	DEFINE_PROPERTY_TYPE ("name", ITypeInfo::kString | ITypeInfo::kReadOnly)
	DEFINE_PROPERTY_TYPE ("title", ITypeInfo::kString | ITypeInfo::kReadOnly)
	DEFINE_PROPERTY_CLASS_ ("parent", "Component", ITypeInfo::kReadOnly)
END_PROPERTY_NAMES (Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Component::getProperty (Variant& var, MemberID propertyId) const
{
	auto resolveChild = [&] (StringID childName)
	{
		IObjectNode* child = lookupChild (String (childName));
		if(!child)
		{
			int index = 0;
			if(childName.getIntValue (index))
				child = getChild (index);
		}
		return child;
	};

	MutableCString arrayKey;
	if(propertyId == "name")
	{
		var = getName ();
		return true;
	}
	else if(propertyId == "title")
	{
		var = getTitle ();
		return true;
	}
	else if(propertyId == "parent")
	{
		var = getParent ();
		return true;
	}
	else if(propertyId == "self")
	{
		var = ccl_as_unknown (*this);
		return true;
	}
	else if(propertyId.getBetween (arrayKey, "children[", "]"))
	{
		var = resolveChild (arrayKey);
		return true;
	}
	else if(propertyId.getBetween (arrayKey, "hasChild[", "]"))
	{
		var = resolveChild (arrayKey) != nullptr;
		return true;
	}
	else if(propertyId.startsWith ("numChildren"))
	{
		var = countChildren ();
		return true;
	}
	else if(propertyId.getBetween (arrayKey, "hasParam[", "]"))
	{
		var = findParameter (arrayKey) != nullptr;
		return true;
	}
	else if(propertyId.getBetween (arrayKey, "hasProperty[", "]"))
	{
		Variant unused;
		var = getProperty (unused, arrayKey) != 0;
		return true;
	}
	else if(propertyId.getBetween (arrayKey, "paramValue[", "]"))
	{
		IParameter* param = findParameter (arrayKey);
		if(param)
			var = param->getValue ();
		else
			var = 0;
		return true;
	}
	else if(propertyId.getBetween (arrayKey, "paramEnabled[", "]"))
	{
		IParameter* param = findParameter (arrayKey);
		if(param)
			var = param->isEnabled () != 0;
		else
			var = false;
		return true;
	}
	else if(propertyId.getBetween (arrayKey, "paramMin[", "]"))
	{
		IParameter* param = findParameter (arrayKey);
		if(param)
			var = param->getMin ();
		else
			var = 0;
		return true;
	}
	else if(propertyId.getBetween (arrayKey, "paramMax[", "]"))
	{
		IParameter* param = findParameter (arrayKey);
		if(param)
			var = param->getMax ();
		else
			var = 0;
		return true;
	}
	else if(propertyId.getBetween (arrayKey, "paramEditable[", "]"))
	{
		IParameter* param = findParameter (arrayKey);
		if(param)
			var = param->isEnabled () != 0 && param->isReadOnly () == 0;
		else
			var = false;
		return true;
	}
	else if(propertyId.getBetween (arrayKey, "hasObject[", "]"))
	{
		var = const_cast<Component&> (*this).getObject (arrayKey, kNullUID) != nullptr;
		return true;
	}
	else
		return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Component::toString (String& string, int flags) const
{
	string = getTitle ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (Component)
	DEFINE_METHOD_ARGR ("findParameter", "name: string", "Parameter")
	DEFINE_METHOD_ARGR ("interpretCommand", "category: string, name: string, checkOnly: bool = false, invoker: Object = null", "bool")
END_METHOD_NAMES (Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Component::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "findParameter")
	{
		IParameter* p = findParameter (MutableCString (msg[0].asString ()));
		returnValue.takeShared (p);
		return true;
	}
	else if(msg == "interpretCommand")
	{
		MutableCString commandCategory (msg[0].asString ());
		MutableCString commandName (msg[1].asString ());
		bool checkOnly = msg.getArgCount () > 2 ? msg[2].asBool () : false;
		IUnknown* invoker = msg.getArgCount () > 3 ? msg[3].asUnknown () : nullptr;
		returnValue = interpretCommand (CommandMsg (commandCategory, commandName, invoker, checkOnly ? CommandMsg::kCheckOnly : 0));
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// TranslationVariables
//************************************************************************************************

DEFINE_STRINGID_ (TranslationVariables::kAppName, "APPNAME")
DEFINE_STRINGID_ (TranslationVariables::kAppCompany, "APPCOMPANY")
DEFINE_STRINGID_ (TranslationVariables::kAppVersion, "APPVERSION")
DEFINE_STRINGID_ (TranslationVariables::kCopyrightYear, "COPYYEAR")
DEFINE_STRINGID_ (TranslationVariables::kFrameworkName, "CCLNAME")
DEFINE_STRINGID_ (TranslationVariables::kFrameworkAuthor, "CCLAUTHOR")

//////////////////////////////////////////////////////////////////////////////////////////////////

void TranslationVariables::setBuiltinVariables (Attributes& variables)
{
	auto& root = RootComponent::instance ();
	if(!root.getApplicationTitle ().isEmpty ())
		variables.set (TranslationVariables::kAppName, root.getApplicationTitle ());
	if(!root.getCompanyName ().isEmpty ())
		variables.set (TranslationVariables::kAppCompany, root.getCompanyName ());
	if(!root.getApplicationVersion ().isEmpty ())
		variables.set (TranslationVariables::kAppVersion, VersionNumber ().scan (root.getApplicationVersion ()).print (VersionNumber::kShort));
	variables.set (TranslationVariables::kCopyrightYear, CCL_COPYRIGHT_YEAR);
	variables.set (TranslationVariables::kFrameworkName, CCL_PRODUCT_NAME);
	variables.set (TranslationVariables::kFrameworkAuthor, CCL_AUTHOR_NAME);
}

//************************************************************************************************
// RootComponent
//************************************************************************************************

RootComponent& RootComponent::instance ()
{
	static RootComponent Root;
	return Root;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (RootComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

RootComponent::RootComponent ()
: Component (CCLSTR ("root")),
  quitRequested (false),
  restartRequested (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

RootComponent::~RootComponent ()
{
	ASSERT (getTheme () == nullptr)
	removeAll (); // remove while vtable valid!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef RootComponent::getGeneratorName () const
{
	static String generator;
	if(generator.isEmpty ())
	{
		ASSERT (System::IsInMainThread ())
		generator = getApplicationTitle ();
		String version = getApplicationVersion ();
		ASSERT (!version.isEmpty ())
		generator << "/" << version;
	}
	return generator;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef RootComponent::getCreatorName () const
{
	static String creator;
	if(creator.isEmpty ())
	{
		ASSERT (System::IsInMainThread ())
		System::GetSystem ().getUserName (creator);
	}
	return creator;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IRootComponent* RootComponent::getHostRootComponent () const
{
	return UnknownPtr<IRootComponent> (System::GetObjectTable ().getObjectByName (IObjectTable::kHostApp));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RootComponent::getHostAppDescription (Description& description) const
{
	IRootComponent* appRoot = getHostRootComponent ();
	ASSERT (appRoot)
	if(!appRoot)
		return false;

	appRoot->getDescription (description);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String RootComponent::getHostApplicationID () const
{
	Description description;
	getHostAppDescription (description);
	return description.appID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String RootComponent::getHostApplicationTitle () const
{
	Description description;
	getHostAppDescription (description);
	return description.appTitle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url& RootComponent::makeUrl (Url& url, StringRef objectPath) const
{
	static const String objectProtocol = CCLSTR ("object");
	url.setProtocol (objectProtocol);
	ASSERT (getApplicationID ().isEmpty () == false)
	url.setHostName (String (getApplicationID ()));
	url.setPath (objectPath);
	return url;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RootComponent::loadStrings (IAttributeList* variables)
{
	ITranslationTable* stringTable = nullptr;
	System::GetLocaleManager ().loadModuleStrings (stringTable, System::GetCurrentModuleRef (), getApplicationID (), variables);
	ASSERT (stringTable != nullptr)
	LocalString::setTable (stringTable);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RootComponent::unloadStrings ()
{
	System::GetLocaleManager ().unloadStrings (getStringTable ());
	LocalString::tableDestroyed ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITranslationTable* RootComponent::getStringTable ()
{
	return LocalString::getTable ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RootComponent::loadTheme (UrlRef defaultPath, const IUrl* searchPath1, const IUrl* searchPath2)
{
	ASSERT (getTheme () == nullptr)

	// register additional search locations
	if(searchPath1)
		System::GetThemeManager ().addSearchLocation (*searchPath1);
	if(searchPath2)
		System::GetThemeManager ().addSearchLocation (*searchPath2);

	Url url;

	// 1) default path (development)
	if(!defaultPath.isEmpty ())
	{
		if(System::GetFileSystem ().fileExists (defaultPath))
			url = defaultPath;
	}

	Url imagePath;
	AutoPtr<IExecutableImage> image = System::GetExecutableLoader ().createImage (System::GetCurrentModuleRef ());
	image->getPath (imagePath);

	const FileType& themeFileType = System::GetThemeManager ().getThemeFileType ();

	// 2) skin folder next to executable
	if(url.isEmpty ())
	{
		String name;
		Url localPath (imagePath);
		localPath.getName (name, false);
		localPath.ascend ();
		localPath.descend (name << "." << themeFileType.getExtension (), IUrl::kFolder);
		if(System::GetFileSystem ().fileExists (localPath))
			url = localPath;
	}

	// 3) skin file next to executable
	if(url.isEmpty ())
	{
		Url localPath (imagePath);
		localPath.setFileType (themeFileType, true);
		if(System::GetFileSystem ().fileExists (localPath))
			url = localPath;
	}

	// 4) compiled-in skin resource
	if(url.isEmpty ())
	{
		ResourceUrl resourcePath (CCLSTR ("default"));
		resourcePath.setFileType (themeFileType, true);
		url = resourcePath;
	}

	ITheme* theme = nullptr;
	ASSERT (getApplicationID ().isEmpty () == false)
	tresult result = System::GetThemeManager ().loadTheme (theme, url, getApplicationID (), getStringTable (), System::GetCurrentModuleRef ());
	ViewBox::setModuleTheme (theme); // assign even if load failed to allow proper unload later
	if(result == kResultOk)
		return true;

	#if DEBUG
	UrlFullString fullPath (url);
	MutableCString str (fullPath);
	CCL_WARN ("RootComponent::loadTheme() failed:\n%s\n", str.str ())
	#endif
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RootComponent::unloadTheme ()
{
	ITheme* theme = getTheme ();
	if(theme)
	{
		ViewBox::setModuleTheme (nullptr);
		System::GetThemeManager ().unloadTheme (theme);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITheme* RootComponent::getTheme () const
{
	return ViewBox::getModuleTheme ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RootComponent::isQuitting () const
{
	return System::GetGUI ().isQuitting () != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API RootComponent::getDescription (Description& description) const
{
	description = Description ();
	description.appID = String (getApplicationID ());
	description.appTitle = getApplicationTitle ();
	description.appVersion = getApplicationVersion ();
	description.appVendor = getCompanyName ();
}
