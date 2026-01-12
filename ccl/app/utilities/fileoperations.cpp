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
// Filename    : ccl/app/utilities/fileoperations.cpp
// Description : File Operations: Copy / Move / Delete
//
//************************************************************************************************

#include "ccl/app/utilities/fileoperations.h"

#include "ccl/base/storage/file.h"

#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/dialogbox.h"

#include "ccl/public/system/cclerror.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/text/translation.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileOperations")
	XSTRING (CopyingFiles, "Copying files")
	XSTRING (MovingFiles, "Moving files")
	XSTRING (DeletingFiles, "Deleting Files")
	XSTRING (CopyingX, "Copying %(1)")
	XSTRING (MovingX, "Moving %(1)")
	XSTRING (DeletingX, "Deleting %(1)")

	XSTRING (RenameFile, "Rename File")
	XSTRING (RenameFolder, "Rename Folder")
	XSTRING (DeleteFile, "Delete File")
	XSTRING (DeleteFolder, "Delete Folder")
	XSTRING (NewFolder, "New Folder")
	XSTRING (MoveToNewFolder, "Move to New Folder")
	XSTRING (NewFolderTitle, "Create new folder")
	XSTRING (NewFolderDefaultName, "New folder")

	XSTRING (Copy, "Copy")
	XSTRING (Move, "Move")
	XSTRING (CopyTo, "Copy to \"%(1)\"")
	XSTRING (MoveTo, "Move to \"%(1)\"")
	XSTRING (MoveToRoot, "Move to Root")
	XSTRING (CopyToRoot, "Copy to Root")
	XSTRING (MoveToFolder, "Move to Folder")

	XSTRING (DoYouWantToCopyThisFolderTo, "Do you want to copy this folder to \"%(1)\"?")
	XSTRING (DoYouWantToCopyThisFileTo, "Do you want to copy this file to \"%(1)\"?")
	XSTRING (DoYouWantToCopyTheseFilesTo, "Do you want to copy these files to \"%(1)\"?")

	XSTRING (DoYouWantToMoveThisFolderTo, "Do you want to move this folder to \"%(1)\"?")
	XSTRING (DoYouWantToMoveThisFileTo, "Do you want to move this file to \"%(1)\"?")
	XSTRING (DoYouWantToMoveTheseFilesTo, "Do you want to move these files to \"%(1)\"?")

	XSTRING (DoYouWantToDeleteThisFolder, "Do you want to delete this folder permanently?")
	XSTRING (DoYouWantToDeleteThisFile, "Do you want to delete this file permanently?")
	XSTRING (DoYouWantToDeleteTheseFiles, "Do you want to delete these files permanently?")

	XSTRING (SomeFilesCouldNotBeCopied, "Some files could not be copied.")
	XSTRING (SomeFilesCouldNotBeMoved, "Some files could not be moved.")
	XSTRING (SomeFilesCouldNotBeDeleted, "Some files could not be deleted.")
	XSTRING (CouldNotTrashFile_DoYouWantToPermanentlyDelete, "The file \"%(1)\" could not be moved to trash.\n\nDo you want to permanently delete it?")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////

#define DEFINE_FILESTRING(Method, folderStr, singularStr, pluralStr) \
	StringRef FileStrings::Method (int number, const IUrl* example) { return (number == 1) ? ((example && example->isFolder ()) ? XSTR(folderStr) : XSTR(singularStr)) : XSTR(pluralStr); }

#define DEFINE_FILESTRING_SIMPLE(Method, str) \
	StringRef FileStrings::Method () { return XSTR(str); }

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_FILESTRING (DoYouWantToCopyTheseFilesTo, DoYouWantToCopyThisFolderTo, DoYouWantToCopyThisFileTo, DoYouWantToCopyTheseFilesTo)
DEFINE_FILESTRING (DoYouWantToMoveTheseFilesTo, DoYouWantToMoveThisFolderTo, DoYouWantToMoveThisFileTo, DoYouWantToMoveTheseFilesTo)
DEFINE_FILESTRING (DoYouWantToDeleteTheseFiles, DoYouWantToDeleteThisFolder, DoYouWantToDeleteThisFile, DoYouWantToDeleteTheseFiles)

DEFINE_FILESTRING_SIMPLE (CopyingFiles, CopyingFiles)
DEFINE_FILESTRING_SIMPLE (MovingFiles, MovingFiles)
DEFINE_FILESTRING_SIMPLE (DeletingFiles, DeletingFiles)
DEFINE_FILESTRING_SIMPLE (RenameFile, RenameFile)
DEFINE_FILESTRING_SIMPLE (RenameFolder, RenameFolder)
DEFINE_FILESTRING_SIMPLE (DeleteFile, DeleteFile)
DEFINE_FILESTRING_SIMPLE (DeleteFolder, DeleteFolder)
DEFINE_FILESTRING_SIMPLE (NewFolder, NewFolder)
DEFINE_FILESTRING_SIMPLE (MoveToNewFolder, MoveToNewFolder)
DEFINE_FILESTRING_SIMPLE (NewFolderTitle, NewFolderTitle)
DEFINE_FILESTRING_SIMPLE (RenameFileTitle, RenameFile)
DEFINE_FILESTRING_SIMPLE (RenameFolderTitle, RenameFolder)
DEFINE_FILESTRING_SIMPLE (Copy, Copy)
DEFINE_FILESTRING_SIMPLE (Move, Move)
DEFINE_FILESTRING_SIMPLE (CopyTo, CopyTo)
DEFINE_FILESTRING_SIMPLE (MoveTo, MoveTo)
DEFINE_FILESTRING_SIMPLE (CopyToRoot, CopyToRoot)
DEFINE_FILESTRING_SIMPLE (MoveToRoot, MoveToRoot)
DEFINE_FILESTRING_SIMPLE (MoveToFolder, MoveToFolder)
DEFINE_FILESTRING_SIMPLE (DoYouWantToCopyThisFolderTo, DoYouWantToCopyThisFolderTo)
DEFINE_FILESTRING_SIMPLE (DoYouWantToMoveThisFolderTo, DoYouWantToMoveThisFolderTo)
DEFINE_FILESTRING_SIMPLE (DoYouWantToDeleteThisFolder, DoYouWantToDeleteThisFolder)
DEFINE_FILESTRING_SIMPLE (SomeFilesCouldNotBeCopied, SomeFilesCouldNotBeCopied)
DEFINE_FILESTRING_SIMPLE (SomeFilesCouldNotBeMoved, SomeFilesCouldNotBeMoved)
DEFINE_FILESTRING_SIMPLE (SomeFilesCouldNotBeDeleted, SomeFilesCouldNotBeDeleted)

//************************************************************************************************
// FileCopyTask
//************************************************************************************************

DEFINE_CLASS_HIDDEN (FileCopyTask, BatchOperation::Task)

//////////////////////////////////////////////////////////////////////////////////////////////////

String FileCopyTask::getProgressText ()
{
	return buildTextFromSourceFileName (XSTR (CopyingX));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileCopyTask::prepare ()
{
	destPath.makeUnique ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileCopyTask::perform (IProgressNotify* progress)
{
	return System::GetFileSystem ().copyFile (getDestPath (), getSourcePath (), 0, progress) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileCopyTask::onFinished ()
{
	if(succeeded ())
		File (getDestPath ()).signalCreated ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileCopyTask::onCanceled ()
{
	// remove successfully copied file
	if(succeeded ())
		System::GetFileSystem ().removeFile (getDestPath ());
}

//************************************************************************************************
// FileMoveTask
//************************************************************************************************

DEFINE_CLASS_HIDDEN (FileMoveTask, BatchOperation::Task)

//////////////////////////////////////////////////////////////////////////////////////////////////

String FileMoveTask::getProgressText ()
{
	return buildTextFromSourceFileName (XSTR (MovingX));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileMoveTask::prepare ()
{
	destPath.makeUnique ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileMoveTask::perform (IProgressNotify* progress)
{
	// request releasing the source file first
	File (getSourcePath ()).signalRelease ();

	bool succeeded = System::GetFileSystem ().moveFile (getDestPath (), getSourcePath (), 0, progress) != 0;
	if(succeeded)
	{
		File (getSourcePath ()).signalRemoved ();
		File (getDestPath ()).signalCreated ();
		// note: if we want to move it back on cancel, we should signal in onFinished ()
	}
	return succeeded;
}

//************************************************************************************************
// FileDeleteTask
//************************************************************************************************

int FileDeleteTask::bypassTrashState = FileDeleteTask::kMustAskBypassTrash;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (FileDeleteTask, BatchOperation::Task)

//////////////////////////////////////////////////////////////////////////////////////////////////


void FileDeleteTask::resetBypassTrashState ()
{
	bypassTrashState = kMustAskBypassTrash;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileDeleteTask::FileDeleteTask ()
: mode (IFileSystem::kDeleteToTrashBin)
{
	hasProgressInfo (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String FileDeleteTask::getProgressText ()
{
	String fileName;
	sourcePath.getName (fileName);

	String text;
	Variant args[] = { fileName };
	text.appendFormat (XSTR (DeletingX), args, 1);
	return text;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileDeleteTask::removeObject (UrlRef path, int mode)
{
	INativeFileSystem& fs = System::GetFileSystem ();
	if(path.isFolder ())
		return fs.removeFolder (path, mode) != 0;
	else
		return fs.removeFile (path, mode) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileDeleteTask::perform (IProgressNotify*)
{
	// don't try if write protected
	INativeFileSystem& fs = System::GetFileSystem ();
	if(fs.isWriteProtected (getSourcePath ()))
	{
		ccl_raise (String (fs.getErrorString (INativeFileSystem::kFileWriteProtected)) << " (" << UrlDisplayString (getSourcePath ()) << ")");
		return false;
	}

	// request releasing the file first
	bool local = File (getSourcePath ()).isLocal ();
	File (getSourcePath ()).signalRelease ();

	bool succeeded = removeObject (getSourcePath (), local ? getMode () : 0);
	if(!succeeded && local)
	{
		// note: we assume here that the reason for failure was related to the trash, and so a real delete could succeed...
		if(bypassTrashState == kMustAskBypassTrash)
		{
			UrlDisplayString pathString (getSourcePath ());
			String text;
			text.appendFormat (XSTR (CouldNotTrashFile_DoYouWantToPermanentlyDelete), pathString);

			bypassTrashState = (Alert::ask (text) == Alert::kYes) ? kBypassTrashAllowed : kBypassTrashDenied;
		}

		// try again deleting without trash
		if(bypassTrashState == kBypassTrashAllowed)
			succeeded = removeObject (getSourcePath (), 0);
	}

	if(succeeded && local)
		File (getSourcePath ()).signalRemoved ();
	
	return succeeded;
}

//************************************************************************************************
// FileTransferOperation
//************************************************************************************************

void FileTransferOperation::makeDestPath (BatchOperation::Task& task, UrlRef destFolder)
{
	String fileName;
	task.getSourcePath ().getName (fileName, true);

	Url destPath (destFolder);
	destPath.descend (fileName, task.getSourcePath ().getType ());

	task.setDestPath (destPath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BatchOperation::Task* FileTransferOperation::addFile (UrlRef path, Object* userData)
{
	Task* task = nullptr;

	switch(transferMode)
	{
	case kCopy:
		task = NEW FileCopyTask;
		task->setSourcePath (path);
		makeDestPath (*task, destFolder);
		break;

	case kMove:
		task = NEW FileMoveTask;
		task->setSourcePath (path);
		makeDestPath (*task, destFolder);
		break;

	case kDelete:
		task = NEW FileDeleteTask;
		task->setSourcePath (path);
		break;

	default:
		ASSERT (0)
		return nullptr;
	}

	task->setUserData (userData);
	addTask (task);
	return task;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileTransferOperation::prepare ()
{
	return BatchOperationComponent (*this).runListDialog (nullptr, createUserMessageText ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* FileTransferOperation::prepareAsync ()
{
	SharedPtr<AsyncOperation> prepareOperation = NEW AsyncOperation;
	prepareOperation->setState (IAsyncInfo::kStarted);
	
	if(isSilent ())
		prepareOperation->setState (IAsyncInfo::kCompleted);
	else
	{
		Promise p (Alert::askAsync (createUserMessageText ()));
		p.then ([prepareOperation] (IAsyncOperation& operation)
		{
			if(operation.getResult ().asInt () == Alert::kNo || operation.getResult ().asInt () == Alert::kCancel)
			{
				prepareOperation->setState (IAsyncInfo::kCanceled);
			}
			else
			{
				prepareOperation->setState(IAsyncInfo::kCompleted);
			}
		});
	}

	return prepareOperation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String FileTransferOperation::createUserMessageText ()
{
	// ask user if files should be copied, moved, deleted
	String text;
	String destName;

	const Url* first = nullptr; // example url for choosing file / folder string
	if(Task* task = (Task*)tasks.at (0))
		first = &task->getSourcePath ();

	switch(transferMode)
	{
	case kCopy:
		destFolder.getName (destName, true);
		text.appendFormat (FileStrings::DoYouWantToCopyTheseFilesTo (tasks.count (), first), destName);
		break;

	case kMove:
		destFolder.getName (destName, true);
		text.appendFormat (FileStrings::DoYouWantToMoveTheseFilesTo (tasks.count (), first), destName);
		break;

	case kDelete:
		text = FileStrings::DoYouWantToDeleteTheseFiles (tasks.count (), first);
		break;
	}

	FileDeleteTask::resetBypassTrashState ();

	return text;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileTransferOperation::onFinished (bool allSucceeded)
{
	if(!allSucceeded)
	{
		String text;
		switch(transferMode)
		{
		case kCopy:		text = XSTR (SomeFilesCouldNotBeCopied); break;
		case kMove:		text = XSTR (SomeFilesCouldNotBeMoved); break;
		case kDelete:	text = XSTR (SomeFilesCouldNotBeDeleted); break;
		}
		Alert::errorWithContext (text);
	}
}

//************************************************************************************************
// NewFolderOperation
//************************************************************************************************

bool NewFolderOperation::run (UrlRef parentFolder)
{
	// ask for name
	String folderName (XSTR (NewFolderDefaultName));

	// make unique before offering
	Url newPath (parentFolder);
	newPath.descend (folderName, IUrl::kFolder);
	newPath.makeUnique ();
	newPath.getName (folderName);

	if(DialogBox ()->askForString (folderName, CSTR ("Name"), XSTR (NewFolderTitle), CCLSTR ("NewFolder")))
	{
		folderName = LegalFileName (folderName);

		// create folder
		Url newPath (parentFolder);
		newPath.descend (folderName, IUrl::kFolder);

		newPath.makeUnique ();
		if(System::GetFileSystem ().createFolder (newPath))
		{
			if(System::GetFileSystem ().isLocalFile (parentFolder))
				File (newPath).signalCreated ();
			return true;
		}
	}
	return false;
}
