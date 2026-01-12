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
// Filename    : ccl/extras/packages/unifiedpackage.cpp
// Description : Unified Package
//
//************************************************************************************************

#include "ccl/extras/packages/unifiedpackage.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/configuration.h"

#include "ccl/public/systemservices.h"

#if DEBUG
#define PACKAGE_LOGGING_DEFAULT_ENABLED true
#else
#define PACKAGE_LOGGING_DEFAULT_ENABLED false
#endif

using namespace CCL;
using namespace Packages;

//************************************************************************************************
// UnifiedPackage
//************************************************************************************************

DEFINE_CLASS (UnifiedPackage, Object)
DEFINE_CLASS_NAMESPACE (UnifiedPackage, NAMESPACE_CCL)

static const Configuration::BoolValue packageLoggingEnabled ("Packages", "loggingEnabled", PACKAGE_LOGGING_DEFAULT_ENABLED);

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackage::UnifiedPackage (StringRef id)
: id (id),
  origin (kUnknownOrigin),
  size (0),
  flags (0)
{
	isTopLevel (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackage::UnifiedPackage (const UnifiedPackage& other)
{
	*this = other;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UnifiedPackage& UnifiedPackage::operator= (const UnifiedPackage& other)
{
	removeChildren ();
	removeData ();

	id = other.id;
	origin = other.origin;
	type = other.type;
	size = other.size;

	setIcon (other.icon);
	title = other.title;
	description = other.description;
	installedVersion = other.installedVersion;
	currentVersion = other.currentVersion;
	fileType = other.fileType;
	licenseData = other.licenseData;
	authorizerId = other.authorizerId;
	vendor = other.vendor;
	website = other.website;

	flags = other.flags;

	for(StringRef dependency : other.dependencies)
		addDependency (dependency);

	for(UnifiedPackage* child : other.children)
		addChild (child);

	data.addAll (other.data);

	tags = other.tags;

	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackage::mergeData (const UnifiedPackage& other)
{
	for(const DataItem& item : other.data)
	{
		if(!data.contains (item))
			data.add (item);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackage::mergeChildren (const UnifiedPackage& other)
{
	for(UnifiedPackage* child : other.children)
		addChild (child);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Vector<SharedPtr<UnifiedPackage>>& UnifiedPackage::getChildren () const
{
	return children;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackage::addChild (UnifiedPackage* child)
{
	ASSERT (child != nullptr)
	ASSERT (child != this)
	if(children.contains (child))
		return;

	if(!checkUniqueId (getId ()))
		return;
	if(child->getId () == getId ())
	{
		String message = "Trying to add a child package ";
		child->toString (message);
		message << " which has the same ID as the parent ";
		toString (message);
		reportPackageWarning (message);
		return;
	}

	child->isTopLevel (false);
	children.add (child);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackage::removeChild (UnifiedPackage* child)
{
	children.remove (child);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackage::removeChildren ()
{
	children.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackage::removeData ()
{
	data.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Vector<String>& UnifiedPackage::getDependencies () const
{
	return dependencies;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackage::addDependency (StringRef packageId)
{
	if(!dependencies.contains (packageId))
		dependencies.add (packageId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Vector<String>& UnifiedPackage::getTags () const
{
	return tags;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackage::addTag (StringRef tag)
{
	if(!tags.contains (tag))
		tags.add (tag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnifiedPackage::checkUniqueId (StringRef id) const
{
	for(UnifiedPackage* child : children)
	{
		ASSERT (child->getId () != id)
		if(child->getId () == id)
		{
			String message = "Child package ID is not unique: ";
			child->toString (message);
			reportPackageWarning (message);
			return false;
		}
		if(!child->checkUniqueId (id))
			return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UnifiedPackage::toString (String& string, int flags) const
{
	string << "{" << getId () << ", \"" << title << "\", flags: " << flags << "}";
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UnifiedPackage::reportPackageWarning (StringRef _message)
{
	#if RELEASE
	if(!packageLoggingEnabled)
		return;
	#endif

	String message = "[Packages] ";
	message.append (_message);
	System::DebugReportWarning (System::GetCurrentModuleRef (), message);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (UnifiedPackage)
	DEFINE_METHOD_ARGR ("isLocalPackage", "", "bool")
END_METHOD_NAMES (UnifiedPackage)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UnifiedPackage::invokeMethod (CCL::Variant& returnValue, CCL::MessageRef msg)
{
	if(msg == "isLocalPackage")
	{
		returnValue = isLocalPackage ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}
