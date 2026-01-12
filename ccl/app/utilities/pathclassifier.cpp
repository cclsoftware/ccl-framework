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
// Filename    : ccl/app/utilities/pathclassifier.cpp
// Description : Path Classifier
//
//************************************************************************************************

#include "ccl/app/utilities/pathclassifier.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileInfo")
	XSTRING (UnknownDrive, "Unknown")
	XSTRING (LocalDrive, "Local")
	XSTRING (RemoteDrive, "Remote")
	XSTRING (OpticalDrive, "CD/DVD Drive")
	XSTRING (RemovableDrive, "Removable Drive")
END_XSTRINGS

//************************************************************************************************
// PathClassifier
//************************************************************************************************

bool PathClassifier::isRoot (UrlRef path)
{
	PathClass c = classify (path);
	return c == kNativeRoot || c == kPackageRoot;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathClassifier::isVolume (UrlRef path)
{
	PathClass c = classify (path);
	return c == kNativeVolume || c == kPackageVolume;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathClassifier::isRegular (UrlRef path)
{
	PathClass c = classify (path);
	return c == kFile || c == kFolder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathClassifier::isSameVolume (UrlRef path1, UrlRef path2)
{
	VolumeInfo v1;
	VolumeInfo v2;
	System::GetFileSystem ().getVolumeInfo (v1, path1);
	System::GetFileSystem ().getVolumeInfo (v2, path2);

	return v1.type == v2.type
		&& v1.label == v2.label
		&& v1.bytesTotal == v2.bytesTotal
		&& v1.serialNumber == v2.serialNumber;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathClassifier::needsExtraction (UrlRef path)
{
	// files in temporary packages need to be extracted
	if(path.getProtocol () == PackageUrl::Protocol)
	{
		if(AutoPtr<IPackageVolume> volume = System::GetPackageHandler ().openPackageVolume (path.getHostName ()))
			if(volume->getOptions () & IPackageVolume::kHidden)
				return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PathClassifier::isCompressedFile (UrlRef path)
{
	if(path.getProtocol () == PackageUrl::Protocol)
	{
		FileInfo info;
		if(System::GetFileSystem ().getFileInfo (info, path))
			if(info.flags & IPackageItem::kCompressed)
				return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PathClassifier::PathClass PathClassifier::classify (UrlRef path)
{
	if(path.isFile ())
		return kFile;

	if(path.isNativePath ())
	{
		if(path.getPath ().isEmpty ())
			return kNativeRoot;
		else if(path.isRootPath ())
			return kNativeVolume;
	}
	else if(path.getProtocol () == PackageUrl::Protocol)
	{
		if(path.getPath ().isEmpty ())
		{
			if(path.getHostName ().isEmpty ())
				return kPackageRoot;
			else
				return kPackageVolume;
		}
	}

	return kFolder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PathClassifier::getVolumeLabel (UrlRef path, const VolumeInfo& info)
{
	String title = info.label;

	if(title.isEmpty ())
	{
		switch(info.type)
		{
		case VolumeInfo::kLocal     : title = XSTR (LocalDrive); break;
		case VolumeInfo::kRemote    : title = XSTR (RemoteDrive); break;
		case VolumeInfo::kOptical   : title = XSTR (OpticalDrive); break;
		case VolumeInfo::kRemovable : title = XSTR (RemovableDrive); break;
		case VolumeInfo::kPackage   : title = XSTR (LocalDrive); break; //???
		default : title = XSTR (UnknownDrive); break;
		}
	}

	#if CCL_PLATFORM_WINDOWS
	if(path.isNativePath ())
	{
		char driveLetter[2] = {(char)path.getPath ().firstChar (), 0};
		if(driveLetter[0])
			title << " (" << driveLetter << ":)";
	}
	#endif

	return title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString PathClassifier::getVolumeIdentifier (UrlRef path, const VolumeInfo& info)
{
	MutableCString id = info.label;

	if(id.isEmpty ())
	{
		switch(info.type)
		{
		case VolumeInfo::kLocal     : id = XSTR_REF (LocalDrive).getKey (); break;
		case VolumeInfo::kRemote    : id = XSTR_REF (RemoteDrive).getKey (); break;
		case VolumeInfo::kOptical   : id = XSTR_REF (OpticalDrive).getKey (); break;
		case VolumeInfo::kRemovable : id = XSTR_REF (RemovableDrive).getKey (); break;
		case VolumeInfo::kPackage   : id = XSTR_REF (LocalDrive).getKey (); break; //???
		default : id = XSTR_REF (UnknownDrive).getKey (); break;
		}
	}

	#if CCL_PLATFORM_WINDOWS
	if(path.isNativePath ())
	{
		char driveLetter = (char)path.getPath ().firstChar ();
		if(driveLetter)
		{
			id += " (";
			id += driveLetter;
			id += ":)";
		}
	}
	#endif

	return id;
}
