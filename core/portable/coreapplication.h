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
// Filename    : core/portable/coreapplication.h
// Description : Application class
//
//************************************************************************************************

#ifndef _coreapplication_h
#define _coreapplication_h

#include "core/portable/coretypeinfo.h"
#include "core/public/coreproperty.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// BaseApplication
/** Application base class with support for additional interfaces. 
	Can be used directly for legacy applications with non-standard behavior. 
	\ingroup core_portable */
//************************************************************************************************

class BaseApplication: public TypedObject,
					   public IPropertyHandler
{
public:
	DECLARE_CORE_CLASS ('BApp', BaseApplication, TypedObject)
	
	// IPropertyHandler
	void setProperty (const Property& value) {}
	void getProperty (Property& value) {}
	void release () { ASSERT (0) }
};

//************************************************************************************************
// Application
/** Application base class with defined standard behavior (startup/shutdown/etc.).
	\ingroup core_portable */
//************************************************************************************************

class Application: public BaseApplication
{
public:
	DECLARE_CORE_CLASS ('Appl', Application, BaseApplication)

	Application ();
	~Application ();

	static Application* getInstance ();	
	template <class T> static T* get () { return static_cast<T*> (getInstance ()); }
	
	virtual void startup () {}
	virtual void shutdown () {}
	virtual void idle () {}

private:
	static Application*& instance ()
	{
		static Application* theInstance = nullptr;
		return theInstance;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Application
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Application::Application ()
{
	ASSERT (instance () == 0)
	instance () = this;
}

inline Application::~Application ()
{
	ASSERT (instance () == this)
	instance () = 0;
}

inline Application* Application::getInstance ()
{
	return instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Portable
} // namespace Core

#endif // _coreapplication_h
