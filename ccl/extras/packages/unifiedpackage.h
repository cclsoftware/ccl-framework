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
// Filename    : ccl/extras/packages/unifiedpackage.h
// Description : Unified Package
//
//************************************************************************************************

#ifndef _ccl_unifiedpackage_h
#define _ccl_unifiedpackage_h

#include "ccl/base/object.h"

#include "ccl/public/text/cclstring.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/collections/vector.h"
#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/plugins/versionnumber.h"
#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/storage/filetype.h"

namespace CCL {
namespace Packages {

//************************************************************************************************
// UnifiedPackage
/**  
* @brief Unified representation of different types of packages from various sources.
*
* A package is an item containing files used to extend an application, unlock features or supply content.
* A UnifiedPackage may represent a package which is available on the local machine, or a package which 
* is available from remote sources.
*
* UnifiedPackage's can be retrieved from IUnifiedPackageSource implementations. 
* Implement IUnifiedPackageSink to do so.
*
* Implementations of IUnifiedPackageHandler may be used to perform actions on UnifiedPackage instances.
* Use PackageHandlerRegistry to find available handlers.
*
* @sa PackageManager
*/
//************************************************************************************************

class UnifiedPackage: public Object
{
public:
	DECLARE_CLASS (UnifiedPackage, Object)
	DECLARE_METHOD_NAMES (UnifiedPackage)

	UnifiedPackage (StringRef id = nullptr);
	UnifiedPackage (const UnifiedPackage& other);

	static void reportPackageWarning (StringRef message);

	UnifiedPackage& operator= (const UnifiedPackage& other);

	enum Flags
	{
		kMinimum = 1<<0,			//< package should be installed with a "minimal" install configuration
		kRecommended = 1<<1,		//< package should be installed with a "recommended" install configuration

		kRetrieveChilds = 1<<2,		//< children of this package need to be retrieved separately

		kLocalPackage = 1<<3,		//< package is located on this system
		kTopLevel = 1<<4,			//< package is not a child of another package
		kCritical = 1<<5,			//< package should always be displayed, never combine it with child packages
		kAllowLocalInstall = 1<<6,	//< allow installation from a local file

		kIsProduct = 1<<7			//< package is a product package
	};

	enum Origin
	{
		kUnknownOrigin = 0,
		kFactoryContentOrigin = 1 << 0,
		kPurchasedContentOrigin = 1 << 1,
		kSubscriptionContentOrigin = 1 << 2,
		kDevelopmentOrigin = 1 << 3
	};

	PROPERTY_FLAG (flags, kMinimum, isMinimum)
	PROPERTY_FLAG (flags, kRecommended, isRecommended)

	PROPERTY_FLAG (flags, kRetrieveChilds, retrieveChildren)

	PROPERTY_FLAG (flags, kLocalPackage, isLocalPackage)
	PROPERTY_FLAG (flags, kTopLevel, isTopLevel)
	PROPERTY_FLAG (flags, kCritical, isCritical)
	PROPERTY_FLAG (flags, kAllowLocalInstall, isLocalInstallationAllowed)
		
	PROPERTY_FLAG (flags, kIsProduct, isProduct)

	PROPERTY_STRING (id, Id)
	PROPERTY_STRING (title, Title)
	PROPERTY_STRING (description, Description)
	PROPERTY_SHARED_AUTO (IImage, icon, Icon)
	PROPERTY_STRING (vendor, Vendor)
	PROPERTY_STRING (website, Website)
	PROPERTY_OBJECT (VersionNumber, installedVersion, InstalledVersion)
	PROPERTY_OBJECT (VersionNumber, currentVersion, CurrentVersion)
	PROPERTY_STRING (type, Type)
	PROPERTY_VARIABLE (int, origin, Origin)
	PROPERTY_OBJECT (FileType, fileType, FileType)
	PROPERTY_VARIABLE (int64, size, Size)
	PROPERTY_STRING (licenseData, LicenseData)
	PROPERTY_STRING (authorizerId, AuthorizerId)

	const Vector<SharedPtr<UnifiedPackage>>& getChildren () const;
	void addChild (UnifiedPackage* child);
	void removeChild (UnifiedPackage* child);
	void removeChildren ();

	const Vector<String>& getDependencies () const;
	void addDependency (StringRef packageId);

	const Vector<String>& getTags () const;
	void addTag (StringRef tag);

	template<class T> T* getData (int index = -1) const;
	template<class T> T* getData (CStringRef id) const;
	template<class T> T* getUnknown (int index = -1) const;
	template<class T> void setData (T* data, CStringRef id = nullptr);
	void removeData ();

	// Object
	bool toString (String& string, int flags = 0) const override;
	
protected:
	struct DataItem
	{
		MutableCString id;
		SharedPtr<Object> object;

		bool operator == (const DataItem& other) const
		{
			return id == other.id && object == other.object;
		}
	};

	Vector<SharedPtr<UnifiedPackage>> children;
	Vector<DataItem> data;
	Vector<String> dependencies;
	Vector<String> tags;
	int flags;

	void mergeData (const UnifiedPackage& other);
	void mergeChildren (const UnifiedPackage& other);
	bool checkUniqueId (StringRef id) const;

	// Object
	tbool CCL_API invokeMethod (CCL::Variant& returnValue, CCL::MessageRef msg) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
T* UnifiedPackage::getData (int index) const
{
	int currentIndex = -1;
	T* val = nullptr;
	for(const DataItem& item : data)
	{
		if(T* instance = ccl_cast<T> (item.object))
		{
			val = instance;
			currentIndex++;
			if(currentIndex == index)
				return val;
		}
	}
	return index < 0 ? val : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
T* UnifiedPackage::getData (CStringRef id) const
{
	for(const DataItem& item : data)
	{
		if(item.id == id)
			if(T* instance = ccl_cast<T> (item.object))
				return instance;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
T* UnifiedPackage::getUnknown (int index) const
{
	int currentIndex = -1;
	UnknownPtr<T> val = 0;
	for(const DataItem& item : data)
	{
		if(UnknownPtr<T> instance = ccl_as_unknown (item.object))
		{
			val = instance;
			currentIndex++;
			if(currentIndex == index)
				return val;
		}
	}
	return index < 0 ? val : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
void UnifiedPackage::setData (T* val, CStringRef id)
{
	ASSERT (val != nullptr)
	SharedPtr<T> value (val);

	for(const DataItem& item : data)
	{
		if(item.id != id)
			continue;
		if(T* oldData = ccl_cast<T> (item.object))
		{
			if(val == oldData)
				return;
			data.remove (item);
			break;
		}
	}

	data.add ({ id, val });
}

} // namespace Packages
} // namespace CCL

#endif // _ccl_unifiedpackage_h
