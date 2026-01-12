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
// Filename    : ccl/extras/packages/unifiedpackagesource.h
// Description : Unified Package Source
//
//************************************************************************************************

#ifndef _ccl_unifiedpackagesource_h
#define _ccl_unifiedpackagesource_h

#include "ccl/base/storage/url.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/collections/unknownlist.h"

namespace CCL {

namespace Alert {
struct Event;}

namespace Packages {
class UnifiedPackage;
interface IUnifiedPackageSource;

//************************************************************************************************
// IUnifiedPackageSink
/** Package sink used in conjunction with IUnifiedPackageSource. */
//************************************************************************************************

interface IUnifiedPackageSink: IUnknown
{
	enum UpdateFlags
	{
		kPackageAdded = 1<<0,
		kPackageRemoved = 1<<1,
		kPackageChanged = 1<<2,

		kRescan = kPackageAdded|kPackageRemoved|kPackageChanged
	};

	virtual void addPackage (UnifiedPackage* package) = 0;

	virtual void requestUpdate (IUnifiedPackageSource& source, int updateFlags) = 0;

	DECLARE_IID (IUnifiedPackageSink)
};

//************************************************************************************************
// IUnifiedPackageSource
/** Package source used to retrieve UnifiedPackage instances. */
//************************************************************************************************

interface IUnifiedPackageSource: IUnknown
{
	enum PackageSourceFlags
	{
		kLocalSource = 1 << 0		// source does only contain local packages
	};

	virtual void addSink (IUnifiedPackageSink* sink) = 0;

	virtual void removeSink (IUnifiedPackageSink* sink) = 0;

	virtual void retrievePackages (UrlRef url, bool refresh = false) = 0;

	virtual UnifiedPackage* createFromFile (UrlRef url) = 0;

	virtual int getFlags () const = 0;

	DECLARE_IID (IUnifiedPackageSource)
};

//************************************************************************************************
// UnifiedPackageUrl
/** Url used to address UnifiedPackage's in IUnifiedPackageSource's. */
//************************************************************************************************

class UnifiedPackageUrl: public Url
{
public:
	UnifiedPackageUrl ();

	static const String Protocol;
};

//************************************************************************************************
// UnifiedPackageSourceBase
//************************************************************************************************

class UnifiedPackageSourceBase: public IUnifiedPackageSource
{
public:
	UnifiedPackageSourceBase ();

	static String getLocalizedPackageOrigin (int origin);

	// IUnifiedPackageSource
	void addSink (IUnifiedPackageSink* sink) override;
	void removeSink (IUnifiedPackageSink* sink) override;
	UnifiedPackage* createFromFile (UrlRef url) override;
	int getFlags () const override;

protected:
	InterfaceList<IUnifiedPackageSink> sinks;
	ObjectArray packageCache;
	int flags;

	UnifiedPackage* lookupPackage (StringRef id) const;
	UnifiedPackage* createPackage (StringRef id = nullptr);
	void invalidatePackageCache ();

	void announcePackage (UnifiedPackage* package) const;
	void reportEvent (const CCL::Alert::Event& e);
	void requestUpdate (int updateFlags = IUnifiedPackageSink::kRescan);
};

//************************************************************************************************
// UnifiedPackageSource
/** Base class for IUnifiedPackageSource implementations. */
//************************************************************************************************

template <class BaseClass = Unknown>
class UnifiedPackageSource: public BaseClass,
							public UnifiedPackageSourceBase
{
public:
	using BaseClass::BaseClass;
	
	CLASS_INTERFACE (IUnifiedPackageSource, BaseClass)
};

} // namespace Packages
} // namespace CCL

#endif // _ccl_unifiedpackagesource_h
