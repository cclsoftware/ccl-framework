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
// Filename    : ccl/extras/packages/unifiedpackagesource.cpp
// Description : Unified Package Source
//
//************************************************************************************************

#include "ccl/extras/packages/unifiedpackagesource.h"
#include "ccl/extras/packages/unifiedpackage.h"

#include "ccl/public/system/alerttypes.h"
#include "ccl/public/text/translation.h"

using namespace CCL;
using namespace Packages;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("PackageSource")
	XSTRING (UnknownOrigin, "Other")
	XSTRING (FactoryContentOrigin, "$APPNAME Content")
	XSTRING (PurchasedContentOrigin, "Purchased Content")
	XSTRING (DevelopmentOrigin, "Development Content")
END_XSTRINGS

//************************************************************************************************
// IUnifiedPackageSink
//************************************************************************************************

DEFINE_IID_ (IUnifiedPackageSink, 0x75d9c361, 0x7b31, 0x4b91, 0x83, 0xe0, 0xeb, 0xb0, 0xd, 0xdc, 0xe, 0x46)

//************************************************************************************************
// IUnifiedPackageSource
//************************************************************************************************

DEFINE_IID_ (IUnifiedPackageSource, 0xbd220464, 0x464c, 0x42e7, 0x82, 0xe8, 0x9e, 0xe2, 0xcd, 0xab, 0xad, 0xaa)

//************************************************************************************************
// UnifiedPackageUrl
//************************************************************************************************

const String UnifiedPackageUrl::Protocol ("unifiedpackage");

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackageUrl::UnifiedPackageUrl ()
{
	type = kFolder;
	setProtocol (Protocol);
}

//************************************************************************************************
// UnifiedPackageSourceBase
//************************************************************************************************

UnifiedPackageSourceBase::UnifiedPackageSourceBase ()
: flags (0)
{
	packageCache.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String UnifiedPackageSourceBase::getLocalizedPackageOrigin (int origin)
{
	if(origin & UnifiedPackage::kFactoryContentOrigin) return XSTR (FactoryContentOrigin);
	if(origin & UnifiedPackage::kPurchasedContentOrigin) return XSTR (PurchasedContentOrigin);
	if(origin & UnifiedPackage::kDevelopmentOrigin) return XSTR (DevelopmentOrigin);
	return XSTR (UnknownOrigin);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackage* UnifiedPackageSourceBase::lookupPackage (StringRef id) const
{
	for(UnifiedPackage* package : iterate_as<UnifiedPackage> (packageCache))
	{
		if(package->getId () == id)
			return package;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackage* UnifiedPackageSourceBase::createPackage (StringRef id)
{
	UnifiedPackage* package = lookupPackage (id);
	if(package == nullptr)
	{
		package = NEW UnifiedPackage (id);
		packageCache.add (package);
	}
	return return_shared (package);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageSourceBase::invalidatePackageCache ()
{
	for(UnifiedPackage* package : iterate_as<UnifiedPackage> (packageCache))
		*package = UnifiedPackage (package->getId ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageSourceBase::addSink (IUnifiedPackageSink* sink)
{
	sinks.append (sink);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageSourceBase::removeSink (IUnifiedPackageSink* sink)
{
	sinks.remove (sink);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackage* UnifiedPackageSourceBase::createFromFile (UrlRef url)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageSourceBase::announcePackage (UnifiedPackage* package) const
{
	for(IUnifiedPackageSink* sink : sinks)
		sink->addPackage (package);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageSourceBase::reportEvent (const Alert::Event& e)
{
	for(IUnifiedPackageSink* sink : sinks)
	{
		UnknownPtr<Alert::IReporter> reporter (sink);
		if(reporter)
			reporter->reportEvent (e);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackageSourceBase::requestUpdate (int updateFlags)
{
	for(IUnifiedPackageSink* sink : sinks)
		sink->requestUpdate (*this, updateFlags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int UnifiedPackageSourceBase::getFlags () const
{
	return flags;
}
