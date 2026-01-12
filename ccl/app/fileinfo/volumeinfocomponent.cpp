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
// Filename    : ccl/app/fileinfo/volumeinfocomponent.cpp
// Description : Volume Info Component
//
//************************************************************************************************

#include "ccl/app/fileinfo/volumeinfocomponent.h"
#include "ccl/app/utilities/pathclassifier.h"

#include "ccl/app/utilities/imagefile.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/packageinfo.h"
#include "ccl/base/storage/archivehandler.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum VolumeInfoTags
	{
		kVolumeLabel = 100,
		kVolumeSpaceUsed,
		kVolumeSpaceNotUsed,
		kVolumeSpaceFree,
		kVolumeTotalSize,
		kVolumeTimeFree
	};

	enum PackageInfoTags
	{
		kPackageVendor = 200,
		kPackageDescription,
		kPackageCopyright,
		kPackageWebsite,
		kPackageIcon
	};
}

//************************************************************************************************
// VolumeInfoComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (VolumeInfoComponent, StandardFileInfo)

//////////////////////////////////////////////////////////////////////////////////////////////////

VolumeInfoComponent::VolumeInfoComponent ()
: StandardFileInfo (CCLSTR ("VolumeFileInfo"), CSTR ("VolumeFileInfo")),
  currentPath (nullptr),
  valid (false),
  volumeType (VolumeInfo::kUnknown)
{
	paramList.addString (CSTR ("volumeLabel"), Tag::kVolumeLabel);
	paramList.addFloat (0, 100, CSTR ("spaceUsed"), Tag::kVolumeSpaceUsed);
	paramList.addFloat (0, 100, CSTR ("spaceNotUsed"), Tag::kVolumeSpaceNotUsed);
	paramList.addString (CSTR ("spaceFree"), Tag::kVolumeSpaceFree);
	paramList.addString (CSTR ("totalSize"), Tag::kVolumeTotalSize);
	paramList.addString (CSTR ("timeFree"), Tag::kVolumeTimeFree);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VolumeInfoComponent::~VolumeInfoComponent ()
{
	if(currentPath)
		currentPath->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VolumeInfoComponent::accepts (UrlRef path)
{
	return PathClassifier::classify (path) == PathClassifier::kNativeVolume;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VolumeInfoComponent::setFile (UrlRef path)
{
	if(!accepts (path)) // otherwise file preview sticks with this component!
		return false;

	if(!currentPath)
		currentPath = NEW Url;
	currentPath->assign (path);

	while(!System::GetFileSystem ().fileExists (*currentPath))
		if(!currentPath->ascend ())
			break;

	SuperClass::setFile (*currentPath);
	update ();

	signal (Message (kPropertyChanged));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VolumeInfoComponent::update ()
{
	valid = currentPath != nullptr;
	if(!valid)
		return;

	valid = System::GetFileSystem ().getVolumeInfo (info, *currentPath) != 0;
	if(!valid)
		return;

	volumeType = info.type;

	double spaceUsed = 0;
	if(info.bytesTotal > 0)
		spaceUsed = (double)(info.bytesTotal - info.bytesFree) / (double)info.bytesTotal;

	String spaceFree;
	if(info.bytesFree > 0)
		spaceFree = Format::ByteSize::print ((double)info.bytesFree);

	String totalSize;
	if(info.bytesTotal > 0)
		totalSize = Format::ByteSize::print ((double)info.bytesTotal);

	String label = PathClassifier::getVolumeLabel (*currentPath, info);

	paramList.byTag (Tag::kVolumeLabel)->fromString (label);
	paramList.byTag (Tag::kVolumeSpaceUsed)->setNormalized ((float)spaceUsed);
	paramList.byTag (Tag::kVolumeSpaceNotUsed)->setNormalized (1.f - (float)spaceUsed);
	paramList.byTag (Tag::kVolumeSpaceFree)->fromString (spaceFree);
	paramList.byTag (Tag::kVolumeTotalSize)->fromString (totalSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VolumeInfoComponent::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "isLocal")
	{
		var = volumeType == VolumeInfo::kLocal;
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// PackageVolumeInfo
//************************************************************************************************

String PackageVolumeInfo::defaultVolumeSubType;
void PackageVolumeInfo::setDefaultVolumeSubType (StringRef defaultType)
{
	defaultVolumeSubType = defaultType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageVolumeInfo::isPackageVolume (UrlRef path, StringRef subType)
{
	if(PathClassifier::classify (path) != PathClassifier::kPackageVolume)
		return false;

	if(!subType.isEmpty ()) // limit to given sub type
	{
		VolumeInfo info;
		System::GetFileSystem ().getVolumeInfo (info, path);
		if(info.subType != subType)
			return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (PackageVolumeInfo, VolumeInfoComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageVolumeInfo::PackageVolumeInfo ()
: volumeSubType (defaultVolumeSubType)
{
	setName (CCLSTR ("PackageVolumeInfo"));
	setFormName (CSTR ("PackageVolumeInfo"));

	paramList.addString (CSTR ("vendor"), Tag::kPackageVendor);
	paramList.addString (CSTR ("description"), Tag::kPackageDescription);
	paramList.addString (CSTR ("copyright"), Tag::kPackageCopyright);
	paramList.addString (CSTR ("website"), Tag::kPackageWebsite);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageVolumeInfo::accepts (UrlRef path)
{
	return isPackageVolume (path, volumeSubType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageVolumeInfo::update ()
{
	if(currentPath == nullptr)
		return;

	SuperClass::update ();

	String vendor;
	String description;
	String copyright;
	String website;

	PackageInfo info;
	AutoPtr<ImageFile> iconFile = NEW ImageFile;
	info.addResource (Meta::kPackageIcon, CCLSTR ("packageicon.png"), iconFile); // obsolete!
	// PLEASE NOTE: @2x naming convention doesn't work here, because package resources are loaded via stream!
	AutoPtr<ImageFile> iconFile2 = NEW ImageFile (ImageFile::kIconSet);
	info.addResource (Meta::kPackageIconSet, CCLSTR (Meta::kPackageIconSetFileName), iconFile2);

	AutoPtr<IPackageVolume> volume = System::GetPackageHandler ().openPackageVolume (currentPath->getHostName ());
	if(volume)
	{
		IFileSystem* fileSystem = volume->getPackage ()->getFileSystem ();
		ASSERT (fileSystem)
		ArchiveHandler handler (*fileSystem);
		if(info.loadFromHandler (handler))
		{
			vendor = info.getString (Meta::kPackageVendor);
			description = info.getString (Meta::kPackageDescription);
			copyright = info.getString (Meta::kPackageCopyright);
			website = info.getString (Meta::kPackageWebsite);
		}
	}

	if(iconFile2->getImage ())
		fileIcon->setImage (iconFile2->getImage ());
	else if(iconFile->getImage ())
		fileIcon->setImage (iconFile->getImage ());

	paramList.byTag (Tag::kPackageVendor)->fromString (vendor);
	paramList.byTag (Tag::kPackageDescription)->fromString (description);
	paramList.byTag (Tag::kPackageCopyright)->fromString (copyright);
	paramList.byTag (Tag::kPackageWebsite)->fromString (website);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageVolumeInfo::setDisplayAttributes (IImage* icon, StringRef title)
{
	// keep icon read from package!
	return false;
}
