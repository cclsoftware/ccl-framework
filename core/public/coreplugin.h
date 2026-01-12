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
// Filename    : core/public/coreplugin.h
// Description : Core Plug-in API
//
//************************************************************************************************

#ifndef _coreplugin_h
#define _coreplugin_h

#include "core/public/coreproperty.h"

namespace Core {
namespace Plugins {

struct ClassInfo;

//************************************************************************************************
// Basic API Definitions
//************************************************************************************************

enum APIVersion
{
	kAPIVersion = 100	///< current API version
};

//************************************************************************************************
// IConstructor
/** Constructor interface. \ingroup core_plugin */
//************************************************************************************************

struct IConstructor: IPropertyHandler
{
	virtual void construct (const ClassInfo& classInfo) = 0;

	static const PropertyType kIID = FOUR_CHAR_ID ('C', 't', 'o', 'r');
};

//************************************************************************************************
// ClassInfo
/** Describes a single class. \ingroup core_plugin */
//************************************************************************************************

struct ClassInfo
{
	/** Class flags. */
	enum Flags
	{
		kDiscardable = 1<<0,	///< class information should not be cached (compatible with CCL)

		kHidden = 1<<16			///< hidden class (i.e. used internally, but not shown to user)
	};

	typedef void* (*CreateFunction) (InterfaceID iid);

	int flags;						///< class flags
	CStringPtr classType;			///< class type (ASCII)
	CStringPtr displayName;			///< class name displayed to user (UTF-8)
	CStringPtr classID;				///< class identifier (ASCII representation of 16 Byte GUID)
	CStringPtr classAttributes;		///< class attributes ({associated class id} \n key=value \n key2=value2...)
	CreateFunction createFunction;	///< function to create class instance

	template <class T> T* createInstance () const
	{
		T* instance = (T*)(*createFunction) (T::kIID);

		// call optional constructor interface
		if(IConstructor* ctor = GetInterface<IConstructor> (instance))
			ctor->construct (*this);

		return instance;
	}
};

//************************************************************************************************
// ClassInfoBundle
/** Describes a list of classes exported from a shared library. \ingroup core_plugin */
//************************************************************************************************

struct ClassInfoBundle
{
	int numClasses;					///< number of classes
	const ClassInfo** classInfos;	///< vector with class information
	CStringPtr versionInfo;			///< version information (key=value \n key2=value2...)
};

extern "C"
{
	/** Main "C" entry point for Plug-ins named "CoreGetClassInfoBundle". */
	typedef const ClassInfoBundle* (*GetClassInfoBundleProc) (int apiVersion);
}

//************************************************************************************************
// Helper Classes
//************************************************************************************************

/** Implementation helper for factory function. \ingroup core_plugin */
template <class Class, class Interface>
struct ClassFactory
{
	static void* createInstance (InterfaceID iid = Interface::kIID)
	{
		if(iid == Interface::kIID)
			return static_cast<Interface*> (NEW Class);
		return nullptr;
	}
};

//************************************************************************************************
// Helper Macros
//************************************************************************************************

#define DEFINE_CORE_CLASSINFO(VarName, flags, classType, displayName, classID, classAttributes, createFunction) \
	static const Core::Plugins::ClassInfo VarName = \
	{flags, classType, displayName, classID, classAttributes, createFunction};

#define DEFINE_CORE_VERSIONINFO(Name, Vendor, Version, Copyright, URL) \
	"name=" Name "\n" \
	"vendor=" Vendor "\n" \
	"version=" Version "\n" \
	"copyright=" Copyright "\n" \
	"url=" URL

#define BEGIN_CORE_CLASSINFO_BUNDLE_(bundleName) \
static const Core::Plugins::ClassInfo* bundleName##ClassArray[] = {

#define END_CORE_CLASSINFO_BUNDLE_(bundleName, versionInfo) }; \
static const Core::Plugins::ClassInfoBundle bundleName = \
{sizeof(bundleName##ClassArray)/sizeof(Core::Plugins::ClassInfo*), bundleName##ClassArray, versionInfo};

#define BEGIN_CORE_CLASSINFO_BUNDLE(versionInfo) \
	CORE_EXPORT const Core::Plugins::ClassInfoBundle* CoreGetClassInfoBundle (int apiVersion); \
	static Core::CStringPtr theVersionInfo = versionInfo; \
	BEGIN_CORE_CLASSINFO_BUNDLE_ (theClassBundle)

#define END_CORE_CLASSINFO_BUNDLE \
	END_CORE_CLASSINFO_BUNDLE_ (theClassBundle, theVersionInfo) \
	CORE_EXPORT const Core::Plugins::ClassInfoBundle* CoreGetClassInfoBundle (int apiVersion) { return apiVersion == Core::Plugins::kAPIVersion ? &theClassBundle : nullptr; }

#define ADD_CORE_CLASSINFO(VarName) &VarName

} // namespace Plugins
} // namespace Core

#endif // _coreplugin_h
