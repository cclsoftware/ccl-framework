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
// Filename    : ccl/app/component.h
// Description : Component class
//
//************************************************************************************************

#ifndef _ccl_component_h
#define _ccl_component_h

#include "ccl/base/objectnode.h"
#include "ccl/base/signalslot.h"

#include "ccl/app/paramcontainer.h"

#include "ccl/public/base/iextensible.h"
#include "ccl/public/plugins/icomponent.h"
#include "ccl/public/gui/iparamobserver.h"
#include "ccl/public/gui/iviewfactory.h"
#include "ccl/public/gui/icommandhandler.h"
#include "ccl/public/gui/icontextmenu.h"
#include "ccl/public/app/irootcomponent.h"

namespace CCL {

class Url;
interface ITheme;
interface IAttributeList;
interface ITranslationTable;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace System
{
	/**	Check if current module is the one using cclapp as host application. */
	extern bool IsInMainAppModule ();	

	/** Overwrite host check in foreign (non-CCL) application. */
	extern void SetInMainAppModule (bool state);
}

//************************************************************************************************
// Component
/**	Base class for application components.
	\ingroup ccl_app */
//************************************************************************************************

class Component: public ObjectNode,
				 public IComponent,
				 public IExtensible,
				 public IController,
				 public IParamObserver,
				 public IViewFactory,
				 public ICommandHandler,
				 public IContextMenuHandler
{
public:
	DECLARE_CLASS (Component, ObjectNode)
	DECLARE_PROPERTY_NAMES (Component)
	DECLARE_METHOD_NAMES (Component)

	Component (StringRef name = nullptr, StringRef title = nullptr);
	~Component ();

	enum Flags
	{
		kStorable = 1<<0,
		kMutable = 1<<1
	};

	PROPERTY_FLAG (flags, kStorable, isStorable) ///< component is stored in load/saveChildren() with onlyStorable == true
	PROPERTY_FLAG (flags, kMutable, isMutable) 

	StringRef getTitle () const;
	void setTitle (StringRef title);
	bool hasTitle () const;

	IUnknown* getContext () const;

	Component* getComponent (StringRef name) const;
	template<class C> C* getComponent (StringRef name) const;
	bool addComponent (Component* c);
	bool addIComponent (IComponent* c);
	void signalHasChild (Component* c);
	Component* lookupComponent (StringRef path) const;
	template<class C> C* lookupComponent (StringRef path) const;

	// IComponent
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
	tbool CCL_API canTerminate () const override;

	// IExtensible
	IUnknown* CCL_API getExtension (StringID id) override;

	// IController
	DECLARE_PARAMETER_LOOKUP (paramList)
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;

	// IParamObserver
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API paramEdit (IParameter* param, tbool begin) override;

	// IViewFactory
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

	// ICommandHandler
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

	// IContextMenuHandler
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu) override;

	/** Helper to access application theme. */
	virtual ITheme* getTheme () const;

	// ObjectNode
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
	bool toString (String& string, int flags) const override;

	CLASS_INTERFACES (ObjectNode)

protected:
	IUnknown* context;
	String title;
	ParamContainer paramList;
	SignalSlotList signalSlots;

	struct ObjectEntry 
	{ 
		MutableCString name;
		IUnknown* object; 
		ObjectEntry (StringID name = nullptr, IUnknown* object = nullptr)
		: name (name), object (object) {} 
	};
	
	LinkedList<ObjectEntry> objects;

	void addObject (StringID name, IUnknown* object);
	void addObject (StringID name, Object* object);

	bool loadIdentity (const Storage& storage);
	bool saveIdentity (const Storage& storage) const;
	bool loadChild (const Storage& storage, Component& child);
	bool saveChild (const Storage& storage, const Component& child) const;
	bool loadChildren (const Storage& storage, bool loadEmpty = false, bool onlyStorable = false);
	bool saveChildren (const Storage& storage, bool onlyStorable = false) const; 

	void signalHasChild (StringRef name);
	void signalPropertyChanged (StringID propertyID, bool deferred = false);
	void propertyChanged (StringID propertyID); // older version of signalPropertyChanged

	// get original context when hosted by foreign module
	IUnknown* resolveToOriginalContext (IUnknown* context);

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;

private:
	int flags;
};

//************************************************************************************************
// TranslationVariables
//************************************************************************************************

namespace TranslationVariables
{
	extern const CString kAppName;
	extern const CString kAppCompany;
	extern const CString kAppVersion;
	extern const CString kCopyrightYear;
	extern const CString kFrameworkName;
	extern const CString kFrameworkAuthor;

	void setBuiltinVariables (Attributes& variables);
}

//************************************************************************************************
// RootComponent
/**	Root of component tree.
	\ingroup ccl_app */
//************************************************************************************************

class RootComponent: public Component,
					 public IRootComponent
{
public:
	DECLARE_CLASS_ABSTRACT (RootComponent, Component)

	RootComponent ();
	~RootComponent ();

	/** Global root component instance for current module. */
	static RootComponent& instance ();
	
	/** Application identifier. */
	PROPERTY_MUTABLE_CSTRING (appID, ApplicationID)

	/** Application package identifier. */
	PROPERTY_MUTABLE_CSTRING (appPackageID, ApplicationPackageID)
	
	/** Company name. */
	PROPERTY_STRING (companyName, CompanyName)

	/** Application title. */
	StringRef getApplicationTitle () const { return getTitle (); }

	/** Application version. */
	PROPERTY_STRING (appVersion, ApplicationVersion)

	IRootComponent* getHostRootComponent () const;
	/** Get description of host application. */
	bool getHostAppDescription (Description& description) const;
	String getHostApplicationID () const;
	String getHostApplicationTitle () const;

	/** Generator name ("Application/Version")*/
	StringRef getGeneratorName () const;

	/** Creator name (current user). */
	StringRef getCreatorName () const;

	/** Make object URL ("object://appID/objectPath"). */
	Url& makeUrl (Url& url, StringRef objectPath) const;

	/** Load module translations. */
	bool loadStrings (IAttributeList* variables = nullptr);

	/** Unload module translations. */
	void unloadStrings ();

	/** Get translation table. */
	ITranslationTable* getStringTable ();

	/** Load module theme. */
	bool loadTheme (UrlRef defaultPath, const IUrl* searchPath1 = nullptr, const IUrl* searchPath2 = nullptr);

	/** Unload module theme. */
	void unloadTheme ();

	/** Check if quit/restart has been requested inside this module. */
	PROPERTY_BOOL (quitRequested, QuitRequested)
	PROPERTY_BOOL (restartRequested, RestartRequested)

	/** Check if application is about to quit. */
	bool isQuitting () const;

	// Component
	ITheme* getTheme () const override;

	CLASS_INTERFACE (IRootComponent, Component)

protected:
	// IRootComponent
	void CCL_API getDescription (Description& description) const override;
};

//************************************************************************************************
// ComponentSingleton
/** Component singleton owned by root component. \ingroup ccl_app */
//************************************************************************************************

template <class T>
class ComponentSingleton
{
public:
	virtual ~ComponentSingleton ()
	{
		if(static_cast<ComponentSingleton*> (theInstance) == this)
			theInstance = nullptr;
	}

	static T& instance ()
	{
		if(!theInstance)
		{
			theInstance = NEW T;
			RootComponent::instance ().addComponent (theInstance);
		}
		return *theInstance;
	}

	static T* peekInstance ()
	{
		return theInstance;
	}

	static Object* __createSingleton () // used by meta class
	{
		return return_shared (&instance ());
	}

protected:
	static T* theInstance;
};

#define DEFINE_COMPONENT_SINGLETON(Class) \
namespace CCL { template<> Class* ComponentSingleton<Class>::theInstance = nullptr; }

//////////////////////////////////////////////////////////////////////////////////////////////////
// Component inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class C>
C* Component::getComponent (StringRef name) const
{ return unknown_cast<C> (findChild (name)); }

inline Component* Component::getComponent (StringRef name) const
{ return getComponent<Component> (name); }

template<class C> 
C* Component::lookupComponent (StringRef path) const
{ return unknown_cast<C> (lookupChild (path)); }

inline Component* Component::lookupComponent (StringRef path) const 
{ return lookupComponent<Component> (path);}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_component_h
