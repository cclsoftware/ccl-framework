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
// Filename    : ccl/system/plugins/module.h
// Description : Module class
//
//************************************************************************************************

#ifndef _ccl_module_h
#define _ccl_module_h

#include "ccl/base/storage/url.h"

namespace CCL {

interface IExecutableImage;

//************************************************************************************************
// Module
/** Abstract module base class. */
//************************************************************************************************

class Module: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (Module, Object)

	Module (UrlRef path);

	bool load ();
	void unload ();
	bool isLoaded () const;
	const Url& getPath () const;

	/** Module unloader. */
	struct Unloader
	{
		Module& m;
		Unloader (Module& m): m (m) {}
		~Unloader () { m.unload (); }
	};

	// Object
	bool equals (const Object& obj) const override;

protected:
	Url path;

	virtual bool loadInternal () = 0;
	virtual void unloadInternal () = 0;
	virtual bool isLoadedInternal () const = 0;

	virtual bool onLoad ();		///< hook for load
	virtual void onUnload ();	///< hook for unload

	void destruct (); ///< call in dtor of derived class!
};

//************************************************************************************************
// ModuleFilter
//************************************************************************************************

struct ModuleFilter
{
	virtual bool matches (const Module* module) const = 0;
};

//************************************************************************************************
// NativeModule
/** Module class holding an executable image (dynamic library). */
//************************************************************************************************

class NativeModule: public Module
{
public:
	DECLARE_CLASS (NativeModule, Module)

	NativeModule (UrlRef path = Url ());
	~NativeModule ();

	IExecutableImage* getImage ();
	void* getFunctionPointer (CStringPtr name);

protected:
	IExecutableImage* image;

	// Module
	bool loadInternal () override;
	void unloadInternal () override;
	bool isLoadedInternal () const override;
};

} // namespace CCL

#endif // _ccl_module_h
