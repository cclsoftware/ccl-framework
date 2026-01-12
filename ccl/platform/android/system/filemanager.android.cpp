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
// Filename    : ccl/platform/android/system/filemanager.android.cpp
// Description : Android file manager
//
//************************************************************************************************

#include "ccl/platform/android/system/filemanager.android.h"
#include "ccl/platform/android/system/system.android.h"

#include "ccl/base/storage/urlencoder.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/text/istringdict.h"
#include "ccl/public/text/translation.h"

using namespace CCL;

BEGIN_XSTRINGS ("FileManager")
	XSTRING (InternalStorage, "Internal Storage")
END_XSTRINGS

//************************************************************************************************
// AndroidFileManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (FileManager, AndroidFileManager)

static const String kUrlPrefixExternalStorage ("content://com.android.externalstorage.documents/");
static const String kUrlPrefixGoogleDrive ("content://com.google.android.apps.docs.storage/");
static const String kUrlPrefixOneDrive ("content://com.microsoft.skydrive.content.StorageAccessProvider/");
static const String kUrlPrefixDropBox ("content://com.dropbox.product.android.dbapp.document_provider.documents/");

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidFileManager::AndroidFileManager ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AndroidFileManager::getFileDisplayString (String& string, UrlRef url, int type) const
{
	if(!FileManager::getFileDisplayString (string, url, type))
		return false;

	if(type == Url::kStringDisplayPath)
	{
		String urlString;
		url.getUrl (urlString);

		String displayName;
		FileManager::getFileDisplayString (displayName, url, type);

		if(urlString.startsWith (kUrlPrefixExternalStorage))
		{
			url.getName (string);

			string = String ("/").append (UrlEncoder ().decode (string));
			string.replace ("primary:", String::kEmpty);
		}
		else if(urlString.startsWith (kUrlPrefixGoogleDrive))
			string = String ("/Google Drive/").append (displayName);
		else if(urlString.startsWith (kUrlPrefixOneDrive))
			string = String ("/OneDrive/").append (displayName);
		else if(urlString.startsWith (kUrlPrefixDropBox))
			string = String ("/DropBox/").append (displayName);
		else
		{
			// check folders returned by Android::Context.getExternalFilesDir (see AndroidSystemInformation::getNativeLocation)
			// example: replace "/storage/emulated/0/Android/data/com.vendorname.appname/files/Documents/"
			// with "InternalStorage - ApplicationName/Documents/"
			for(System::FolderType folderType : { System::kUserDocumentFolder, System::kUserMusicFolder, System::kUserDownloadsFolder })
			{
				Url folder;
				if(AndroidSystemInformation::getInstance ().getNativeLocation (folder, folderType))
				{
					folder.ascend (); // out of e.g. "Documents"

					String folderString;
					folder.getUrl (folderString);
					if(urlString.startsWith (folderString))
					{
						string = String (Url::strPathChar) << XSTR (InternalStorage) << (" - ") << AndroidSystemInformation::getInstance ().getAppProductFolderName () << Url::strPathChar;
						string << urlString.subString (folderString.length ());
						return true;
					}
				}
			}
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API AndroidFileManager::getFileLocationType (UrlRef url) const
{
	String urlString;
	url.getUrl (urlString);

	if(urlString.startsWith (kUrlPrefixGoogleDrive))
		return FileLocationType::kGoogleDrive;
	else if(urlString.startsWith (kUrlPrefixOneDrive))
		return FileLocationType::kOneDrive;
	else if(urlString.startsWith (kUrlPrefixDropBox))
		return FileLocationType::kDropBox;

	return FileManager::getFileLocationType (url);
}
