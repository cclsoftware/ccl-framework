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
// Filename    : ccl/app/documents/documentrenamer.cpp
// Description : Document Renamer
//
//************************************************************************************************

#include "ccl/app/documents/documentrenamer.h"
#include "ccl/app/documents/documentmanager.h"
#include "ccl/app/documents/document.h"
#include "ccl/app/documents/documentmetainfo.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/message.h"

#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Documents")
	XSTRING (RenameFileFailed, "Could not rename %(1)!\n\nThe file is in use.")
	XSTRING (RenameFolderFailed, "Could not rename the folder %(1)!\n\nA file in this folder is in use.")
END_XSTRINGS

//************************************************************************************************
// DocumentPathHelper
//************************************************************************************************

DocumentPathHelper::DocumentPathHelper (UrlRef documentFile)
{
	documentFile.getName (fileName, false);

	parentFolder = documentFile;
	parentFolder.ascend ();

	DocumentClass* documentClass = DocumentManager::instance ().findDocumentClass (documentFile.getFileType ());
	bool expectOwnFolder = documentClass ? documentClass->needsFolder () : true;
	if(expectOwnFolder)
	{
		String folderName;
		parentFolder.getName (folderName);

		hasOwnFolder = folderName == LegalFolderName (fileName);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentPathHelper::getDedicatedFolder (Url& folder) const
{
	if(hasOwnFolder)
	{
		folder = parentFolder;
		return true;
	}
	return false;
}

//************************************************************************************************
// DocumentRenamer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DocumentRenamer, Renamer)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef DocumentRenamer::strRenameFileFailed () {return XSTR (RenameFileFailed);}
StringRef DocumentRenamer::strRenameFolderFailed () {return XSTR (RenameFolderFailed);}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentRenamer::DocumentRenamer (Document& document)
: document (document),
  hasOwnFolder (false)
{
	// get full paths of old file & folder
	DocumentPathHelper pathHelper (document.getPath ());
	oldFolder = pathHelper.getParentFolder ();
	oldFile = document.getPath ();

	// get name of old file 
	setOldName (pathHelper.getFileName ());

	fileType = oldFile.getFileType ();
	hasOwnFolder = pathHelper.hasDedicatedFolder ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentRenamer::makeLegalName (String& name)
{
	name = LegalFileName (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentRenamer::doesAlreadyExist (StringRef newName)
{
	if(!oldFolder.isCaseSensitive () && newName.compare (getOldName (), false) == Text::kEqual)
		return false; // allow changing case when filesystem is non-case-sensitive

	Url newPath (oldFolder);

	// check new file in old folder
	newPath.descend (newName);
	newPath.setExtension (fileType.getExtension (), false);
	if(System::GetFileSystem ().fileExists (newPath))
	{
		setAlreadyExistsMessage (FileRenamer::strFileAlreadyExists ());
		return true;
	}

	if(hasOwnFolder)
	{
		// check new folder
		newPath = oldFolder;
		newPath.setName (LegalFolderName (newName));
		if(System::GetFileSystem ().fileExists (newPath))
		{
			setAlreadyExistsMessage (FileRenamer::strFolderAlreadyExists ());
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentRenamer::renameFolder (UrlRef newFolder)
{
	ASSERT (hasOwnFolder)
	
	if(!System::GetFileSystem ().moveFile (newFolder, oldFolder))
	{
		showErrorMessage (strRenameFolderFailed (), oldFolder);
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentRenamer::performRename (StringRef newName)
{
	Url newFolder (oldFolder);

	if(hasOwnFolder)
	{
		// we also want to rename the folder
		// todo: check if other documents in this folder are open...
		LegalFolderName newFolderName (newName); // e.g. remove trailing dots
		newFolder.setName (newFolderName);

		if(renameFolder (newFolder) == false)
			return false;

		// old file is now in new folder (with old name)
		oldFile = newFolder;
		oldFile.descend (getOldName ());
		oldFile.setExtension (fileType.getExtension (), false);
	}

	// rename the document file
	Url newFile (newFolder);
	newFile.descend (newName);
	newFile.setExtension (fileType.getExtension (), false);

	if(System::GetFileSystem ().fileExists (oldFile)
		&& !System::GetFileSystem ().moveFile (newFile, oldFile))
	{
		if(hasOwnFolder)
		{
			// we must continue, folder is already renamed
			newFile = oldFile; // old file in new folder
		}
		else
		{
			showErrorMessage (strRenameFileFailed (), oldFile);
			return false;
		}
	}

	// set new path & title
	document.setPath (newFile);

	// update title in meta info as well, reset (version) description
	UnknownPtr<IAttributeList> metaAttribs (document.getMetaInfo ());
	if(metaAttribs)
	{
		DocumentMetaInfo (*metaAttribs).setTitle (document.getTitle ());
		DocumentMetaInfo (*metaAttribs).setDescription (String::kEmpty);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentRenamer::showErrorMessage (StringRef pattern, UrlRef path)
{
	String text;
	UrlDisplayString pathString (path);
	text.appendFormat (pattern, pathString);
	Alert::error (text);
}
