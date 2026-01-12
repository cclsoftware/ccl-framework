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
// Filename    : ccl/system/plugins/module.cpp
// Description : Module class
//
//************************************************************************************************

#include "ccl/system/plugins/module.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/system/iexecutable.h"

using namespace CCL;

//************************************************************************************************
// Module
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (Module, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Module::Module (UrlRef _path)
: path (_path)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Module::destruct ()
{
	if(isLoaded ())
		unload ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Url& Module::getPath () const 
{ 
	return path; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Module::equals (const Object& obj) const
{
	const Module* m = ccl_cast<Module> (&obj);
	return m ? m->getPath ().equals (getPath ()) : Object::equals (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Module::load ()
{
	if(isLoaded ())
		return true;

	if(!loadInternal ())
		return false;

	if(!onLoad ())
	{
		unload ();
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Module::unload ()
{
	if(!isLoaded ())
		return;

	onUnload ();

	unloadInternal ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Module::isLoaded () const
{
	return isLoadedInternal ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Module::onLoad ()
{
	return true; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Module::onUnload ()
{}

//************************************************************************************************
// NativeModule
//************************************************************************************************

DEFINE_CLASS (NativeModule, Module)
DEFINE_CLASS_NAMESPACE (NativeModule, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeModule::NativeModule (UrlRef path)
: Module (path),
  image (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeModule::~NativeModule ()
{
	destruct ();
	ASSERT (image == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IExecutableImage* NativeModule::getImage ()
{
	return image;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* NativeModule::getFunctionPointer (CStringPtr name)
{
	ASSERT (image != nullptr)
	return image ? image->getFunctionPointer (name) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NativeModule::loadInternal ()
{
	ASSERT (image == nullptr)
	tresult result = System::GetExecutableLoader ().loadImage (image, getPath ());
	return result == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NativeModule::unloadInternal ()
{
	if(image)
		image->release (),
		image = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NativeModule::isLoadedInternal () const
{
	return image != nullptr;
}
