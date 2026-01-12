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
// Filename    : ccl/extras/packages/packagefilter.cpp
// Description : Package Filter
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/extras/packages/packagefilter.h"
#include "ccl/extras/packages/packagemanager.h"
#include "ccl/extras/packages/unifiedpackageaction.h"

#include "ccl/extras/extensions/installdata.h"

#include "ccl/base/message.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/text/translation.h"

using namespace CCL;
using namespace Packages;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("PackageFilter")
	XSTRING (PackageSearch, "Search")
	XSTRING (PackageOrigin, "Origin")
	XSTRING (PackageInstallState, "State")
	XSTRING (PackageFileType, "File Type")
	XSTRING (PackageType, "Type")
	XSTRING (PackageName, "Name")
	XSTRING (PackageTags, "Tags")
	XSTRING (FileTypeOther, "Other")
	XSTRING (InstallStateAny, "Any")
	XSTRING (InstallStateInstalled, "Installed")
	XSTRING (InstallStateAvailable, "Available")
	XSTRING (TypeAny, "Any")
END_XSTRINGS

//************************************************************************************************
// PackageFilterComponent
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PackageFilterComponent, Component);

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageFilterComponent::PackageFilterComponent (PackageManager* manager, StringRef name, StringRef title)
: Component (name, title),
  manager (manager),
  hidden (false),
  enabled (true)
{
	manager->addObserver (this);

	selectionParameter = paramList.addInteger (0, 0, "selection");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageFilterComponent::~PackageFilterComponent ()
{
	manager->removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageFilterComponent::update ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageFilterComponent::select (int index)
{
	selectionParameter->setValue (index, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageFilterComponent::select (StringRef value)
{
	int index = items.index (value);
	if(index >= 0)
		select (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageFilterComponent::reset ()
{
	select (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PackageFilterComponent::getSelection () const
{
	return selectionParameter->getValue ().asInt ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageFilterComponent::setHidden (bool state)
{
	hidden = state;
	signalPropertyChanged ("hidden", true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageFilterComponent::isHidden () const
{
	return hidden;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageFilterComponent::setEnabled (bool state)
{
	enabled = state;
	signalPropertyChanged ("enabled", true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageFilterComponent::isEnabled () const
{
	return enabled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageFilterComponent::matches (IUnknown* object) const
{
	if(enabled == false)
		return true;

	UnifiedPackage* package = unknown_cast<UnifiedPackage> (object);
	if(package == nullptr)
		return false;
	bool match = matches (*package);
	if(match == false)
	{
		CCL_PRINTF ("Package \"%s\" does not match filter \"%s\" / \"%s\"\n", MutableCString (package->getId ()).str (), MutableCString (getName ()).str (), MutableCString (title).str ())
	}
	return match;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageFilterComponent::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "title")
	{
		var = title;
		return true;
	}
	else if(propertyId == "hidden")
	{
		var = hidden;
		return true;
	}
	else if(propertyId == "enabled")
	{
		var = enabled;
		return true;
	}
	else if(propertyId == "numItems")
	{
		var = items.count ();
		return true;
	}
	else
	{
		int index = -1;
		CStringRef postfix = propertyId.subString (propertyId.index ("-") + 1);
		index = String (postfix).scanInt ();

		if(propertyId.startsWith ("item-"))
		{
			var.fromString (getItemTitle (index));
			return true;
		}
	}
	
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageFilterComponent::paramChanged (IParameter* param)
{
	if(param == selectionParameter)
	{
		deferChanged ();
		return true;
	}
	else
		return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PackageFilterComponent::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
		update ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageFilterComponent::addItem (StringRef title, int index)
{
	if(index < 0)
		items.addSorted (title);
	else
		items.insertAt (index, title);
	selectionParameter->setMax (items.count () - 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageFilterComponent::removeItem (int index)
{
	items.removeAt (index);
	selectionParameter->setMax (items.count () - 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PackageFilterComponent::getItemTitle (int index) const
{
	return items.at (index);
}

//************************************************************************************************
// MultiOptionPackageFilterComponent
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (MultiOptionPackageFilterComponent, PackageFilterComponent);

//////////////////////////////////////////////////////////////////////////////////////////////////

MultiOptionPackageFilterComponent::MultiOptionPackageFilterComponent (PackageManager* manager, StringRef name, StringRef title)
: PackageFilterComponent (manager, name, title)
{
	selectionParameter->setMin (-1);
	selectionParameter->setValue (-1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiOptionPackageFilterComponent::reset ()
{
	selectionParameter->setValue (-1);
	for(int i = 0; i < items.count (); i++)
	{
		if(IParameter* param = getItemParam (i))
			param->setValue (true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MultiOptionPackageFilterComponent::paramChanged (IParameter* param)
{
	CStringRef name = param->getName ();
	int index = -1;
	CStringRef postfix = name.subString (name.index ("-") + 1);
	index = String (postfix).scanInt ();

	if(param == selectionParameter)
	{
		for(int i = 0; i < items.count (); i++)
		{
			if(IParameter* param = getItemParam (i))
				param->setValue (i == selectionParameter->getValue ().asInt ());
		}
		signal (Message (kChanged));
		return true;
	}
	else if(param->getName ().startsWith ("item-"))
	{
		selectionParameter->setValue (-1);
		signal (Message (kChanged));
		return true;
	}
	else
		return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiOptionPackageFilterComponent::addItem (StringRef title, int index)
{
	MutableCString paramName ("item-");
	paramName.appendInteger (items.count ());
	if(!paramList.findParameter (paramName))
	{
		IParameter* param = paramList.addParam (paramName);
		param->setValue (true);
	}

	SuperClass::addItem (title, index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* MultiOptionPackageFilterComponent::getItemParam (int index) const
{
	return paramList.findParameter (MutableCString ("item-").appendInteger (index));
}

//************************************************************************************************
// PackageSearchComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PackageSearchComponent, PackageFilterComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageSearchComponent::PackageSearchComponent (PackageManager* manager)
: PackageFilterComponent (manager, "PackageSearch", XSTR (PackageSearch))
{
	searchParam = paramList.addString ("filterTerms");
	clearParam = paramList.addParam ("clearFilterTerms");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageSearchComponent::matches (const UnifiedPackage& package) const
{
	if(package.isTopLevel () == false)
		return true;

	StringRef term = searchParam->getValue ().asString ();
	if(term.isEmpty ())
		return true;

	for(StringRef str : { package.getTitle (), package.getDescription () })
		if(str.contains (term, false))
			return true;

	for(UnifiedPackage* child : package.getChildren ())
		for(StringRef str : { child->getTitle (), child->getDescription () })
			if(str.contains (term, false))
				return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageSearchComponent::update ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageSearchComponent::reset ()
{
	searchParam->setValue ("");
	deferChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageSearchComponent::paramChanged (IParameter* param)
{
	if(param == searchParam)
	{
		deferChanged ();
		return true;
	}
	else if(param == clearParam)
	{
		searchParam->setValue ("");
		deferChanged ();
		return true;
	}
	else
		return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API PackageSearchComponent::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "FilterListItem" && hidden == false)
		return getTheme ()->createView (MutableCString (manager->getName ()).append ("/PackageSearch"), data.asUnknown ());
	return SuperClass::createView (name, data, bounds);
}

//************************************************************************************************
// StaticFileTypePackageFilterComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StaticFileTypePackageFilterComponent, PackageFilterComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

StaticFileTypePackageFilterComponent::StaticFileTypePackageFilterComponent (PackageManager* manager)
: PackageFilterComponent (manager, "StaticFileTypePackageFilter", XSTR (PackageFileType))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StaticFileTypePackageFilterComponent::matches (const UnifiedPackage& package) const
{
	return matches (package, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StaticFileTypePackageFilterComponent::matches (const UnifiedPackage& package, bool includeChildrenWithoutFiles) const
{
	const FileType& fileType = package.getFileType ();

	// always show child packages without files
	if(package.isTopLevel () == false && fileType.isValid () == false)
		return true;

	// show top level packages with file packages matching the filter
	if(package.isTopLevel () && fileType.isValid () == false)
	{
		bool hasMatchingChild = false;
		for(UnifiedPackage* child : package.getChildren ())
		{
			if(matches (*child, false))
			{
				hasMatchingChild = true;
				break;
			}
		}
		if(hasMatchingChild)
			return true;
	}

	if(fileTypes.contains (fileType))
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticFileTypePackageFilterComponent::update ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticFileTypePackageFilterComponent::addFileType (const FileType& type)
{
	fileTypes.add (type);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Vector<FileType>& StaticFileTypePackageFilterComponent::getFileTypes () const
{
	return fileTypes;
}

//************************************************************************************************
// FileTypePackageFilterComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (FileTypePackageFilterComponent, MultiOptionPackageFilterComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileTypePackageFilterComponent::FileTypePackageFilterComponent (PackageManager* manager)
: MultiOptionPackageFilterComponent (manager, "FileTypePackageFilter", XSTR (PackageFileType))
{
	addItem (XSTR (FileTypeOther), 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileTypePackageFilterComponent::matches (const UnifiedPackage& package) const
{
	return matches (package, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileTypePackageFilterComponent::matches (const UnifiedPackage& package, bool includeChildrenWithoutFiles) const
{
	const FileType& fileType = package.getFileType ();

	// always show child packages without files
	if(includeChildrenWithoutFiles && package.isTopLevel () == false && fileType.getExtension ().isEmpty ())
		return true;

	// show top level packages with file packages matching the filter
	if(package.isTopLevel () && fileType.getExtension ().isEmpty ())
	{
		bool hasMatchingChild = false;
		for(UnifiedPackage* child : package.getChildren ())
		{
			if(matches (*child, false))
			{
				hasMatchingChild = true;
				break;
			}
		}
		if(hasMatchingChild)
			return true;
	}

	int index = fileTypes.index (fileType);
	if(index < 0)
		return !fileType.getExtension ().isEmpty () && getItemParam (fileTypes.count ())->getValue ().asBool ();
	return getItemParam (index)->getValue ().asBool ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileTypePackageFilterComponent::update ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileTypePackageFilterComponent::addFileType (const FileType& type, StringRef title)
{
	fileTypes.add (type);
	addItem (title, fileTypes.count () - 1);
}

//************************************************************************************************
// InstallStatePackageFilterComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (InstallStatePackageFilterComponent, PackageFilterComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

InstallStatePackageFilterComponent::InstallStatePackageFilterComponent (PackageManager* manager)
: PackageFilterComponent (manager, "PackageInstallStateFilter", XSTR (PackageInstallState)),
  strict (false),
  filterChildren (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool InstallStatePackageFilterComponent::matches (const UnifiedPackage& package) const
{
	if(selectionParameter->getValue ().asInt () == kAny)
		return true;

	if(package.isTopLevel () == false && !isFilteringChildren ())
		return true;

	ObjectArray actions;
	actions.objectCleanup ();
	manager->getActions (actions, package);

	bool other = true;
	// show installable packages that match the filter
	for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (actions))
	{
		if(action->getId () == UnifiedPackageHandler::kUninstall)
			return selectionParameter->getValue ().asInt () == kInstalled;
		else if(action->getId () == UnifiedPackageHandler::kInstall && action->getState () > UnifiedPackageAction::kDisabled)
			return selectionParameter->getValue ().asInt () == kAvailable;
	}

	bool hasInstallableChild = false;
	bool hasInstalledChild = false;
	for(UnifiedPackage* child : package.getChildren ())
	{
		ObjectArray actions;
		actions.objectCleanup ();
		manager->getActions (actions, *child);
		for(UnifiedPackageAction* action : iterate_as<UnifiedPackageAction> (actions))
		{
			if(action->getId () == UnifiedPackageHandler::kUninstall)
				hasInstalledChild = true;
			if(action->getId () == UnifiedPackageHandler::kInstall && action->getState () > UnifiedPackageAction::kDisabled)
				hasInstallableChild = true;

			if(hasInstalledChild && hasInstallableChild)
				break;
		}
		if(hasInstalledChild && hasInstallableChild)
			break;
	}

	// show top level packages which contain matching childs
	if(hasInstallableChild && (isStrict () == false || hasInstalledChild == false) && selectionParameter->getValue ().asInt () == kAvailable)
		return true;
	if((hasInstalledChild || (package.isTopLevel () && hasInstallableChild == false)) && selectionParameter->getValue ().asInt () == kInstalled)
		return true;
	if(hasInstallableChild == false && hasInstalledChild == false && package.isTopLevel () == false)
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InstallStatePackageFilterComponent::update ()
{
	if(items.count () == 0)
	{
		addItem (XSTR (InstallStateAny), 0);
		addItem (XSTR (InstallStateInstalled), 1);
		addItem (XSTR (InstallStateAvailable), 2);

		deferChanged ();
		deferSignal (NEW Message (kPropertyChanged));
	}
}

//************************************************************************************************
// OriginPackageFilterComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (OriginPackageFilterComponent, MultiOptionPackageFilterComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

OriginPackageFilterComponent::OriginPackageFilterComponent (PackageManager* manager)
: MultiOptionPackageFilterComponent (manager, "PackageOriginFilter", XSTR (PackageOrigin))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OriginPackageFilterComponent::matches (const UnifiedPackage& package) const
{
	if(!package.isTopLevel ())
		return true;

	if(package.isLocalPackage ())
		return true;

	int origins = package.getOrigin ();

	if(origins == UnifiedPackage::kUnknownOrigin)
	{
		int index = items.index (String ().appendIntValue (UnifiedPackage::kUnknownOrigin));
		if(IParameter* param = getItemParam (index))
			return param->getValue ().asBool ();
	}

	for(int i = 0; i < sizeof(int) * 8; i++)
	{
		int origin = 1 << i;
		if((origin & origins) == 0)
			continue;
		int index = items.index (String ().appendIntValue (origin));
		if(IParameter* param = getItemParam (index))
		{
			if(param->getValue ().asBool () == true)
				return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OriginPackageFilterComponent::update ()
{
	Vector<String> newItems;

	ObjectArray packages;
	packages.objectCleanup ();
	manager->getPackages (packages);

	bool anyUnknown = false;
	for(int i = 0; i < packages.count (); ++i)
	{
		UnifiedPackage* package = ccl_cast<UnifiedPackage> (packages.at (i));
		if(package->isLocalPackage ())
			continue;
		int origins = package->getOrigin ();
		for(int i = 0; i < sizeof(int) * 8; i++)
		{
			int origin = 1 << i;
			if((origin & origins) == 0)
				continue;
			if(newItems.contains (String ().appendIntValue (origin)) == false)
			{
				if(package->getOrigin () == UnifiedPackage::kUnknownOrigin && package->isTopLevel ())
					anyUnknown = true;
				else if(package->getOrigin () != UnifiedPackage::kUnknownOrigin)
					newItems.add (String ().appendIntValue (origin));
			}
		}
	}

	int currentIndex = selectionParameter->getValue ().asInt ();
	String currentTitle = (currentIndex >= 0 && currentIndex < items.count ()) ? items.at (currentIndex) : String::kEmpty;

	items.removeAll ();
	for(int i = 0; i < newItems.count (); ++i)
		if(items.contains (newItems[i]) == false && newItems[i].isEmpty () == false)
			addItem (newItems[i]);
	if(anyUnknown)
		addItem (String ().appendIntValue (UnifiedPackage::kUnknownOrigin), items.count ());

	currentIndex = items.index (currentTitle);
	if(currentIndex >= 0)
		selectionParameter->setValue (currentIndex);

	deferChanged ();
	deferSignal (NEW Message (kPropertyChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String OriginPackageFilterComponent::getItemTitle (int index) const
{
	int origin = UnifiedPackage::kUnknownOrigin;
	items.at (index).getIntValue (origin);
	return manager->getOriginTitle (origin);
}

//************************************************************************************************
// SinglePackageFilterComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SinglePackageFilterComponent, PackageFilterComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

SinglePackageFilterComponent::SinglePackageFilterComponent (PackageManager* manager, StringRef title)
: PackageFilterComponent (manager, "SinglePackageFilter", title),
  enabled (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SinglePackageFilterComponent::setPackageId (StringRef id)
{
	packageId = id;
	if(enabled)
		deferChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SinglePackageFilterComponent::enable (bool state)
{
	enabled = state;
	deferChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SinglePackageFilterComponent::matches (const UnifiedPackage& package) const
{
	if(enabled == false)
		return true;
	if(package.isTopLevel () == false)
		return true;
	return package.getId () == packageId;
}

//************************************************************************************************
// AppVersionPackageFilterComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AppVersionPackageFilterComponent, PackageFilterComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

AppVersionPackageFilterComponent::AppVersionPackageFilterComponent (PackageManager* manager, StringRef title)
: PackageFilterComponent (manager, "AppVersionPackageFilter", title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppVersionPackageFilterComponent::addSupportedVersion (StringRef identity, const VersionNumber& version)
{
	supportedVersions.add ({ identity, version });
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AppVersionPackageFilterComponent::matches (const UnifiedPackage& package) const
{
	bool foundManifest = false;
	bool canInstall = checkManifest (foundManifest, package);
	if(foundManifest == false && package.isTopLevel ())
	{
		for(UnifiedPackage* child : package.getChildren ())
		{
			canInstall |= checkManifest (foundManifest, *child);
		}
	}
	return foundManifest ? canInstall : true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AppVersionPackageFilterComponent::checkManifest (bool& foundManifest, const UnifiedPackage& package) const
{
	bool canInstall = false;
	Install::Manifest* manifest = nullptr;
	for(int i = 0; manifest = package.getData<Install::Manifest> (i); i++)
	{
		if(Install::File* file = manifest->findFile (package.getId ()))
		{
			foundManifest = true;
			for(const VersionItem& item : supportedVersions)
				canInstall |= (file->canInstallWithVersion (item.identity, item.version) == Install::File::kAppOK);
		}
	}
	return canInstall;
}

//************************************************************************************************
// TagPackageFilterComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TagPackageFilterComponent, MultiOptionPackageFilterComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

TagPackageFilterComponent::TagPackageFilterComponent (PackageManager* manager)
: MultiOptionPackageFilterComponent (manager, "PackageTags", XSTR (PackageTags))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TagPackageFilterComponent::matches (const UnifiedPackage& package) const
{
	bool anyChecked = false;
	for(int i = 0; i < items.count (); i++)
	{
		if(IParameter* param = getItemParam (i))
		{
			if(param->getValue ().asBool ())
			{
				anyChecked = true;
				break;
			}
		}
	}
	if(!anyChecked)
		return true;

	for(StringRef tag : package.getTags ())
	{
		int index = items.index (tag);
		if(IParameter* param = getItemParam (index))
			if(param->getValue ().asBool ())
				return true;
	}

	for(UnifiedPackage* child : package.getChildren ())
	{
		if(matches (*child))
			return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TagPackageFilterComponent::update ()
{
	Vector<String> newItems;

	ObjectArray packages;
	packages.objectCleanup ();
	manager->getPackages (packages);

	for(int i = 0; i < packages.count (); ++i)
	{
		UnifiedPackage* package = ccl_cast<UnifiedPackage> (packages.at (i));
		for(StringRef tag : package->getTags ())
		{
			if(newItems.contains (tag) == false)
				newItems.add (tag);
		}
	}

	items.removeAll ();
	for(int i = 0; i < newItems.count (); ++i)
		if(items.contains (newItems[i]) == false && newItems[i].isEmpty () == false)
			addItem (newItems[i]);

	reset ();

	deferChanged ();
	deferSignal (NEW Message (kPropertyChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TagPackageFilterComponent::reset ()
{
	selectionParameter->setValue (-1);
	for(int i = 0; i < items.count (); i++)
	{
		if(IParameter* param = getItemParam (i))
			param->setValue (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API TagPackageFilterComponent::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "FilterListItem" && hidden == false)
		return getTheme ()->createView (MutableCString (manager->getName ()).append ("/PackageTags"), data.asUnknown ());
	return SuperClass::createView (name, data, bounds);
}
