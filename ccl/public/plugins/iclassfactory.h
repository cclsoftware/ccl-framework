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
// Filename    : ccl/public/plugins/iclassfactory.h
// Description : Class Factory Interface
//
//************************************************************************************************

#ifndef _ccl_iclassfactory_h
#define _ccl_iclassfactory_h

#include "ccl/public/text/cclstring.h"

namespace CCL {

interface IClassFactory;
interface IAttributeList;

//************************************************************************************************
// Plug-In module 'C' interface
//************************************************************************************************

extern "C"
{	
	/** Main "C" entry point for Plug-Ins named "CCLGetClassFactory". */
	typedef IClassFactory* (CCL_API *CCLGetClassFactoryProc) ();
}

//************************************************************************************************
// VersionDesc
/** Plug-In version description.
	\ingroup base_plug */
//************************************************************************************************

struct VersionDesc
{
	String name;				///< friendly name
	String version;				///< version string
	String vendor;				///< vendor string
	String copyright;			///< copyright string
	String url;					///< vendor or product website

	VersionDesc (StringRef name = nullptr, 
				 StringRef version = nullptr, 
				 StringRef vendor = nullptr, 
				 StringRef copyright = nullptr, 
				 StringRef url = nullptr)
	: name (name),
	  version (version),
	  vendor (vendor),
	  copyright (copyright),
	  url (url)
	{}
};

//************************************************************************************************
// ClassDesc
/** Plug-In class description. 
	\ingroup base_plug */
//************************************************************************************************

struct ClassDesc
{
	/** Class flags. */
	enum Flags
	{
		kDiscardable = 1<<0,		///< class description should not be cached
		kSingleton = 1<<1			///< class instance is a singleton
	};

	int flags;						///< class flags
	UIDBytes classID;				///< unique class identifier
	String category;				///< class category
	String name;					///< class name
	String subCategory;				///< class subcategory (optional)
	String description;				///< class description (optional)

	ClassDesc (UIDRef classID = kNullUID,
			   StringRef category = nullptr,
			   StringRef name = nullptr,
			   StringRef subCategory = nullptr,
			   StringRef description = nullptr,
			   int flags = 0)
   : flags (flags),
     classID (classID),
	 category (category),
	 name (name),
	 subCategory (subCategory),
	 description (description)
	{}
};

//************************************************************************************************
// IClassFactory
/** Class factory interface.
	\ingroup base_plug  */
//************************************************************************************************

interface IClassFactory: IClassAllocator
{
	/** Get version description of this factory. */
	virtual void CCL_API getVersion (VersionDesc& version) const = 0;

	/** Returns number of exported classes. */
	virtual int CCL_API getNumClasses () const = 0;

	/** Get description of class at specified index. */
	virtual tbool CCL_API getClassDescription (ClassDesc& description, int index) const = 0;

	/** Get additional class attributes (optional). */
	virtual tbool CCL_API getClassAttributes (IAttributeList& attributes, UIDRef cid, StringID language) const = 0;

	DECLARE_IID (IClassFactory)
};

DEFINE_IID (IClassFactory, 0xbeac2a68, 0x8c44, 0x4ce3, 0xb4, 0x46, 0x58, 0x9b, 0xd9, 0x21, 0x5c, 0xee)

//************************************************************************************************
// IClassFactoryUpdate
/** Class factory update interface.
	\ingroup base_plug  */
//************************************************************************************************

interface IClassFactoryUpdate: IUnknown
{
	/** Update classes exported by factory. */
	virtual tresult CCL_API updateClasses () = 0;

	/** Check if factory has discardable classes, e.g. when number of exported classes is currently zero. */
	virtual tbool CCL_API hasDiscardableClasses () const = 0;

	DECLARE_IID (IClassFactoryUpdate)
};

DEFINE_IID (IClassFactoryUpdate, 0x9454b49f, 0x6ce1, 0x4973, 0xb4, 0x0, 0xdf, 0xb1, 0x1a, 0x55, 0x94, 0xf6)

//************************************************************************************************
// IPluginInstance
/** Plug-in instance interface to store factory data.
	\ingroup base_plug  */
//************************************************************************************************

interface IPluginInstance: IUnknown
{
	/** Instance data. */
	typedef IUnknown* Token;

	/** The host factory associates data with this instance. */
	virtual void CCL_API setFactoryToken (Token token) = 0;

	/** Returns associated instance data. */
	virtual Token CCL_API getFactoryToken () const = 0;

	DECLARE_IID (IPluginInstance)
};

DEFINE_IID (IPluginInstance, 0x332263d1, 0x4590, 0x4bd2, 0x99, 0x49, 0xa, 0x62, 0xc, 0x35, 0xb, 0xd5)

//************************************************************************************************
// Plug-in Meta Classes
//************************************************************************************************

/** Category for meta classes. */
#define PLUG_CATEGORY_METACLASS CCLSTR ("MetaClass")

/** \ingroup base_plug  */
namespace Meta 
{
	/** UID of meta class. */
	DEFINE_STRINGID (kMetaClassID, "Class:MetaClassID")

	/** UID of alternative class. */
	DEFINE_STRINGID (kAlternativeClassID, "Class:AltClassID")

	/** Alternative class name (e.g. legacy name when class was renamed). */
	DEFINE_STRINGID (kAlternativeClassName, "Class:AltClassName")

	/** UID of associated component class. */
	DEFINE_STRINGID (kComponentClassID, "Class:ComponentClassID")

	/** Image resource identifier. */
	DEFINE_STRINGID (kClassImageResource, "Class:ImageResource")

	/** Text resource identifier. */
	DEFINE_STRINGID (kClassTextResource, "Class:TextResource")
}

//************************************************************************************************
// IPluginMetaClass
/** Plug-in meta class interface.
	\ingroup base_plug  */
//************************************************************************************************

interface IPluginMetaClass: CCL::IUnknown
{
	/** Get location of associated resource. */
	virtual tresult CCL_API getResourceLocation (IUrl& url, StringID id, StringID language) = 0;

	DECLARE_IID (IPluginMetaClass)
};

DEFINE_IID (IPluginMetaClass, 0x8ad19611, 0x9f28, 0x4d7b, 0xa3, 0xdd, 0xf9, 0x4c, 0x18, 0x42, 0xc4, 0x95)

} // namespace CCL

#endif // _ccl_iclassfactory_h
