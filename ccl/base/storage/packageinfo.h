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
// Filename    : ccl/base/storage/packageinfo.h
// Description : Package Meta Information
//
//************************************************************************************************

#ifndef _ccl_packageinfo_h
#define _ccl_packageinfo_h

#include "ccl/base/storage/attributes.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/public/storage/istorage.h"
#include "ccl/public/storage/metainfo.h"
#include "ccl/public/system/ipackagemetainfo.h"

namespace CCL {

interface IPackageFile;
class ArchiveHandler;

//************************************************************************************************
// PackageResource
//************************************************************************************************

class PackageResource: public Object
{
public:
	DECLARE_CLASS (PackageResource, Object)

	PackageResource (StringID id = nullptr, StringRef fileName = nullptr, IStorable* data = nullptr);

	PROPERTY_MUTABLE_CSTRING (id, ID)
	PROPERTY_STRING (fileName, FileName)
	PROPERTY_SHARED_AUTO (IStorable, data, Data)
};

//************************************************************************************************
// PackageInfo
/** Meta Information saved with packages. */
//************************************************************************************************

class PackageInfo: public PersistentAttributes,
				   public IPackageInfo
{
public:
	DECLARE_CLASS (PackageInfo, PersistentAttributes)

	PackageInfo ();
	PackageInfo (const IAttributeList& attributes);

	static const String kFileName;		///< name of info file inside package
	static const CString kRootName;		///< name of root tag

	// Resources
	void addResource (PackageResource* resource);
	PackageResource* addResource (StringID id, StringRef fileName, IStorable* data);
	
	PackageResource* getResource (StringID id) const;
	IStorable* CCL_API getResourceData (StringID id) const override; // IPackageInfo
	template <class T> T* getResourceData (StringID id) const;

	// Common Attributes
	METAINFO_ATTRIBUTE_STRING (PackageID, Meta::kPackageID)

	String getStringWithAlternative (StringID id, StringID altId) const;

	bool loadFromPackage (UrlRef path, int options = 0);
	bool saveWithPackage (UrlRef path, UIDRef cid = kNullUID) const;

	bool loadFromPackage (IPackageFile& package);
	bool saveWithPackage (IPackageFile& package) const;

	bool loadFromHandler (ArchiveHandler& handler);
	bool saveWithHandler (ArchiveHandler& handler) const;

	bool toXml (IStream& xmlStream) const;

	CLASS_INTERFACE (IPackageInfo, PersistentAttributes)

protected:
	ObjectList resources;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T> 
T* PackageInfo::getResourceData (StringID id) const
{ return unknown_cast<T> (getResourceData (id)); }

inline String PackageInfo::getStringWithAlternative (StringID id, StringID altId) const
{ String string = getString (id); if(string.isEmpty ()) string = getString (altId); return string; } 

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_packageinfo_h
