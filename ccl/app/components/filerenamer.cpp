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
// Filename    : ccl/app/components/filerenamer.cpp
// Description : File Renamer
//
//************************************************************************************************

#include "ccl/app/components/filerenamer.h"

#include "ccl/app/paramcontainer.h"

#include "ccl/base/asyncoperation.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/message.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/ialert.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/cclerror.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileRenamer")
	XSTRING (ObjectAlreadyExists, "An object with this name already exists.")
	XSTRING (FileAlreadyExists, "A file with this name already exists.")
	XSTRING (FolderAlreadyExists, "A folder with this name already exists.")
	XSTRING (CouldNotRenameFile, "Could not rename this file.")
	XSTRING (CouldNotRenameFolder, "Could not rename this folder.")
	XSTRING (ShowExtension, "Show Extension")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum RenamerTags
	{
		kName = 100,
		kShowExtension = 200
	};
}

//************************************************************************************************
// Renamer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Renamer, Component)
DEFINE_STRINGID_MEMBER_ (Renamer, kWarnExists, "warnExists")

//////////////////////////////////////////////////////////////////////////////////////////////////

Renamer::Renamer (StringRef oldName)
: oldName (oldName),
  initialName (oldName),
  renameSucceeded (false)
{
	nameParam = paramList.addString (CSTR ("Name"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Renamer::~Renamer ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Renamer::setOldName (StringRef name, bool updateInitialName)
{
	oldName = name;
	if(updateInitialName)
		initialName = name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Renamer::tryRename (StringRef _newName)
{
	if(_newName != nullptr)
		newName = _newName;
	
	makeLegalName (newName);
	newName.trimWhitespace ();

	if(newName == oldName)
		return true;

	if(newName.isEmpty ())
		return false;

	if(doesAlreadyExist (newName))
	{
		if(alreadyExistsMessage.isEmpty ())
			alreadyExistsMessage = XSTR (ObjectAlreadyExists); // fallback message
		
		// we need to defer the alert dialog (allow other dialog to end first)
		retain ();
		Message* m = new Message (kWarnExists);
		m->post (this);
		
		return false;
	}
	else
	{
		renameSucceeded = performRename (newName);
		return true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* Renamer::runDialogAsync (StringRef title)
{
	SharedPtr<AsyncOperation> dialogOperation = NEW AsyncOperation ();
	dialogOperation->setState (IAsyncInfo::kStarted);
	
	nameParam->fromString (initialName, Tag::kName);
	
	SharedPtr<Renamer> This = this;

	DialogBox dialog;
	Promise (dialog->runWithParametersAsync (CCLSTR ("RenameDialog"), paramList, title)).then ([This, dialogOperation] (IAsyncOperation& operation)
	{
		if(operation.getState () == IAsyncInfo::kCompleted && operation.getResult ().asInt () == DialogResult::kOkay)
		{
			This->newName = This->nameParam->getValue ().asString ();
		}
		
		dialogOperation->setResult (operation.getResult ());
		dialogOperation->setState (operation.getState ());
	});

	return dialogOperation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Renamer::runDialog (StringRef title)
{
	newName = initialName;

	while(true)
	{
		// ask for new name
		nameParam->fromString (newName, Tag::kName);
		if(DialogBox ()->runWithParameters (CCLSTR ("RenameDialog"), paramList, title) != DialogResult::kOkay)
			break;

		if(tryRename (nameParam->getValue ().asString ()))
			return renameSucceeded;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Renamer::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kWarnExists)
	{
		Promise (Alert::warnAsync (alreadyExistsMessage));
		release ();
	}
	
	SuperClass::notify (subject, msg);
}

//************************************************************************************************
// FileRenamer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (FileRenamer, Renamer)

StringRef FileRenamer::strFileAlreadyExists ()   { return XSTR (FileAlreadyExists); }
StringRef FileRenamer::strFolderAlreadyExists () { return XSTR (FolderAlreadyExists); }

//////////////////////////////////////////////////////////////////////////////////////////////////

FileRenamer::FileRenamer (UrlRef oldPath, bool canEditExtension)
: oldPath (oldPath),
  canEditExtension (canEditExtension),
  editExtension (false)
{
	UrlDisplayString oldName (oldPath, IUrl::kStringDisplayName);
	setOldName (oldName);
	
	setAlreadyExistsMessage (oldPath.isFolder () ? strFolderAlreadyExists () : strFileAlreadyExists ());

	if(canEditExtension)
		paramList.addParam (XSTR_REF (ShowExtension).getKey (), Tag::kShowExtension);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileRenamer::paramChanged (IParameter* param)
{
	if(param->getTag () == Tag::kShowExtension)
	{
		bool wantExtension = param->getValue ().asBool ();
		if(wantExtension != editExtension)
		{
			editExtension = wantExtension;
			String name (getNameParam ().getValue ().asString ());
			if(editExtension)
			{
				// add old extension
				String ext;
				oldPath.getExtension (ext);
				name << CCLSTR (".") << ext;
			}
			else
			{
				// remove extension
				Url url;
				url.setName (name);
				url.getName (name, false);
			}
			getNameParam ().setValue (name);

			// update old name with / without extension
			String oldName;
			oldPath.getName (oldName, editExtension);
			setOldName (oldName);
		}
		return true;
	}
	return Renamer::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileRenamer::makeNewPath (Url& newPath, StringRef newName)
{
	newPath = oldPath;
	newPath.ascend ();
	newPath.descend (newName, oldPath.isFolder () ? IUrl::kFolder : IUrl::kFile);

	if(!editExtension)
	{
		String ext;
		oldPath.getExtension (ext);
		newPath.setExtension (ext, false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url* FileRenamer::createNewPath ()
{
	Url* newPath (NEW Url);
	makeNewPath (*newPath, getNewName ());
	return newPath;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileRenamer::doesAlreadyExist (StringRef newName)
{
	if(!oldPath.isCaseSensitive () && newName.compare (getOldName (), false) == Text::kEqual)
		return false; // allow changing case when filesystem is non-case-sensitive

	Url newPath;
	makeNewPath (newPath, newName);
	return System::GetFileSystem ().fileExists (newPath) !=0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileRenamer::performRename (StringRef newName)
{
	Url newPath;
	makeNewPath (newPath, newName);

	SignalSource signalSource (Signals::kFileSystem);
	signalSource.signal (Message (Signals::kReleaseFile, oldPath.asUnknown ()));

	ErrorContextGuard guard;

	bool result = System::GetFileSystem ().moveFile (newPath, oldPath) != 0;
	if(!result)
		Alert::errorWithContext (oldPath.isFolder () ? XSTR (CouldNotRenameFolder) : XSTR (CouldNotRenameFile));

	if(System::GetFileSystem ().isLocalFile (newPath))
		signalSource.signal (Message (Signals::kFileMoved, oldPath.asUnknown (), newPath.asUnknown (), result));
	return result;
}
