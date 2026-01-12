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
// Filename    : ccl/app/documents/autosaver.cpp
// Description : Autosave Documents
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/documents/autosaver.h"
#include "ccl/app/documents/documentmanager.h"
#include "ccl/app/documents/document.h"
#include "ccl/app/documents/documentversions.h"

#include "ccl/app/safety/appsafety.h"

#include "ccl/base/asyncoperation.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

#define DEBUG_IMPATIENTLY (0 && DEBUG)
#define ASK_KEEP_BACKUP 0

//////////////////////////////////////////////////////////////////////////////////////////////////

Configuration::BoolValue AutoSaver::enabled ("Application.AutoSaver", "enabled", false);
Configuration::IntValue AutoSaver::period ("Application.AutoSaver", "period", 5 * 60); // seconds

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Autosave")
	XSTRING (NewerBackupFound, "A backup that is newer than the original file has been found.")
	XSTRING (EmergencyBackupFound, "A backup of the original file has been found.")
	XSTRING (DoYouWantToUseTheBackup, "Do you want to open the backup?")
	XSTRING (OriginalFileWillBeKeptAsVersion, "The original file will be kept as a version then.")
	XSTRING (DoYouWantToKeepTheBackup, "Do you want to keep the backup as a version?")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (IAutoSaveHook, 0xD8CB389D, 0x52F2, 0x48A3, 0x9E, 0x5B, 0x93, 0x7A, 0x6B, 0xF6, 0x5F, 0xD4)

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (AutoSaver, kSetupLevel)
{
	AutoSaver::instance ();
	return true;
}

//************************************************************************************************
// AutoSaver::Suspender
//************************************************************************************************

AutoSaver::Suspender::Suspender ()
: autoSaver (AutoSaver::instance ())
{ 
	wasSuspended = autoSaver.isSuspended ();
	autoSaver.setSuspended (true); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AutoSaver::Suspender::~Suspender ()
{
	autoSaver.setSuspended (wasSuspended);
}

//************************************************************************************************
// AutoSaver
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AutoSaver, Object)
DEFINE_COMPONENT_SINGLETON (AutoSaver)

const String AutoSaver::kAutosaveExtension = CCLSTR ("autosave");

//////////////////////////////////////////////////////////////////////////////////////////////////

AutoSaver::AutoSaver ()
: Component (CCLSTR ("AutoSaver")),
  manager (DocumentManager::instance ()),
  timer (nullptr),
  nextTime (0),
  saveTimeout (5 * 60 * 1000),
  softUserTimeout (8 * 1000),
  hardUserTimeout (1 * 1000),
  gracePeriod (30 * 1000),
  numFilesToKeep (10),
  autoSaving (false),
  overwrite (false),
  suspended (false)
{
	#if DEBUG_IMPATIENTLY
	saveTimeout = 5 * 1000;
	softUserTimeout = 4 * 1000;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AutoSaver::initialize (IUnknown* context)
{
	#if !DEBUG_IMPATIENTLY
	setSaveTimeout (1000 * period.getValue ());
	#endif
	enable (enabled.getValue ());

	enabled.addObserver (this);
	period.addObserver (this);

	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AutoSaver::terminate ()
{
	enabled.removeObserver (this);
	period.removeObserver (this);
	enable (false);

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AutoSaver::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged && subject == &enabled)
		enable (enabled.getValue ());
	else if(msg == kChanged && subject == &period)
	{
		setSaveTimeout (1000 * period.getValue ());
		if(timer) // restart timer if already enabled
		{
			enable (false);
			enable (true);
		}
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AutoSaver::enable (bool state)
{
	bool enabled = timer != nullptr;
	if(state != enabled)
	{
		CCL_PRINTF ("Autosave %sabled\n", state ? "En" : "Dis")
		if(timer)
		{
			timer->removeTask (this);
			timer->release ();
			timer = nullptr;
		}
		else
		{
			nextTime = System::GetSystemTicks () + saveTimeout;

			unsigned int period = ccl_min (saveTimeout / 2, hardUserTimeout);
			period = ccl_bound<unsigned int> (period, 1000, 60000);

			ASSERT (timer == nullptr)
			timer = System::GetGUI ().createTimer (period);
			if(timer)
				timer->addTask (this);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AutoSaver::resetTimer ()
{
	nextTime = System::GetSystemTicks () + saveTimeout;
	System::GetGUI ().updateUserActivity ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AutoSaver::onTimer (ITimer*)
{
	if(isSuspended ())
		return;

	if(manager.isSaveDisabled ())
		return;

	int64 now = System::GetSystemTicks ();
	int64 over = now - nextTime;
	if(over >= 0)
	{
		// it's time to save
		bool urgent = over > gracePeriod;
		int requiredUserTimeOut = softUserTimeout;
		if(urgent)
		{
			// take another gracePeriod seconds to change gradually from soft to hard timeout
			requiredUserTimeOut = int(softUserTimeout - (softUserTimeout - hardUserTimeout) * float(over - gracePeriod) / gracePeriod);
			requiredUserTimeOut = ccl_bound (requiredUserTimeOut, hardUserTimeout, softUserTimeout);
		}

		int64 userTimeOut = now - (int64)(System::GetGUI ().getLastUserActivity () * 1000.);

		CCL_PRINTF ("want autosave now (%s) user timeout %.1f (of %.1f)\n", urgent ? "URGENT" : "grace period", userTimeOut/1000.f, requiredUserTimeOut/1000.f)

		if(userTimeOut >= requiredUserTimeOut)
			if(canSaveNow (urgent))
			{
				// or: for each document?
				if(Document* document = manager.getActiveDocument ())
					checkDocument (*document);

				nextTime = System::GetSystemTicks () + saveTimeout;
			}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AutoSaver::canSaveNow (bool urgent)
{
	if(isSuspended ())
		return false;

	if(manager.isSaveDisabled ())
		return false;
	
	if(System::GetDesktop ().isInMode (IDesktop::kMenuLoopMode|IDesktop::kProgressMode|IDesktop::kModalMode|IDesktop::kTextInputMode))
		return false;

	// check mouse or modifier keys
	KeyState keys;
	System::GetGUI ().getKeyState (keys);
	if(keys.isSet (KeyState::kMouseMask|KeyState::kModifierMask))
	{
		CCL_PRINTLN ("Can't autosave now: mouse buttons or modifiers still pressed!")
		System::GetGUI ().updateUserActivity ();
		return false;
	}

	if(autoSaveHook)
		return autoSaveHook->canAutoSaveNow (urgent);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool AutoSaver::checkDocument (Document& document)
{
	if(document.needsAutoSave ())
		return doSave (document);

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void AutoSaver::makeAutoSavePath (Url& path, const Document& document)
{
	path = document.getPath ();
	if(overwrite)
		return;

	Url newPath (path);
	if(System::GetSystem ().isProcessSandboxed ())
	{
		// In sandboxed applications not every path is writable. Specifically the opened
		// file might be in an otherwise not writable location, so that an .autosave
		// file next to it cannot be written. To work around this limitation, we use a common
		// autosave location inside the sandbox that is independent from the original file path.
		System::GetSystem ().getLocation (newPath, System::kAppSettingsFolder);
		newPath.descend ("AutosaveData");
		String temp;
		path.getName (temp);
		newPath.descend (temp);
		newPath.setFileType (path.getFileType ());
	}

	// an imported document is saved in the new document format, so the extension needs to be adjusted
	if(document.isImported () && newPath.getFileType () != document.getDocumentClass ()->getFileType ())
		newPath.setFileType (document.getDocumentClass ()->getFileType (), true);

	newPath.setExtension (kAutosaveExtension, false);
	path = newPath;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

bool AutoSaver::isAutoSaveFile (UrlRef path)
{
	String extension;
	path.getExtension (extension) ;
	return extension == kAutosaveExtension;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

bool AutoSaver::doSave (Document& document)
{
	if(isSuspended ())
		return false;

	CCL_PRINTLN ("start AutoSave ...")
	
	SafetyGuard safetyGuard (SafetyID::kAutoSaveAction);

	Suspender guard;

	// begin notification
	manager.signalDocumentEvent (document, Document::kBeforeAutoSave);
	if(autoSaveHook)
		autoSaveHook->onAutoSave (true);

	Url autoSavePath;
	makeAutoSavePath (autoSavePath, document);

	AutoPtr<Url> existingFile;
	if(!overwrite)
	{
		if(System::GetFileSystem ().fileExists (autoSavePath))
		{
			if(numFilesToKeep > 0 && DocumentVersions::isSupported ())
			{
				DocumentVersions versions (document.getPath ());
				versions.moveDocumentToHistory (&autoSavePath, &DocumentVersions::strAutosaveSnapshotSuffix);
				versions.purgeOldest (DocumentVersions::strAutosaveSnapshotSuffix, numFilesToKeep);
			}
			else
			{
				existingFile = NEW Url (autoSavePath);

				// move existing autosave file out of the way
				existingFile->makeUnique ();
				if(!System::GetFileSystem ().moveFile (*existingFile, autoSavePath))
				{
					CCL_WARN ("Could not move old Autosave file!", 0)
				}
			}
		}
	}

	#if DEBUG_LOG
	UrlFullString s (autoSavePath);
	MutableCString cs (s);
	CCL_PRINTF ("Autosave: %s\n", cs)
	#endif

	Url oldPath (document.getPath ());
	bool wasDirty = document.isDirty () != 0;

	bool wasAutoSave = document.isAutoSave ();
	document.isAutoSave (true);

	{
		ScopedVar<bool> scope (autoSaving, true);
		document.saveAs (autoSavePath);
	}

	// delete old autosave file
	if(!overwrite && existingFile)
	{
		if(!System::GetFileSystem ().removeFile (*existingFile))
		{
			CCL_WARN ("Could not delete old Autosave file!", 0)
		}
	}

	document.setPath (oldPath);
	if(overwrite)
	{
		document.setDirty (false);
		DocumentManager::instance ().updateDirtyState (&document);
	}
	else if(wasDirty)
		document.setDirty (true);

	document.isAutoSave (wasAutoSave);
	document.setAutoSavedNow ();

	// end notification
	if(autoSaveHook)
		autoSaveHook->onAutoSave (false);

	manager.signalDocumentEvent (document, Document::kAutoSaveFinished);

	CCL_PRINTLN ("... AutoSave done")
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AutoSaver::removeAutoSaveFile (Document& document)
{
	if(overwrite)
		return;

	Suspender guard;

	Url autoSavePath;
	makeAutoSavePath (autoSavePath, document);

	if(System::GetFileSystem ().fileExists (autoSavePath))
	{
		if(numFilesToKeep > 0 && DocumentVersions::isSupported ())
		{
			DocumentVersions versions (document.getPath ());
			versions.moveDocumentToHistory (&autoSavePath, &DocumentVersions::strAutosaveSnapshotSuffix);
			versions.purgeOldest (DocumentVersions::strAutosaveSnapshotSuffix, numFilesToKeep);
		}
		else
			System::GetFileSystem ().removeFile (autoSavePath);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* AutoSaver::tryAutoSavedFile (Document& document, bool isEmergency)
{
	if(overwrite)
		return AsyncOperation::createFailed ();

	Suspender guard;

	// check if autosave file exists
	Url autoSavePath;
	makeAutoSavePath (autoSavePath, document);

	if(System::GetFileSystem ().fileExists (autoSavePath))
	{
		// check if it's newer
		FileInfo autoSaveInfo;
		FileInfo documentInfo;
		if(!isEmergency
			&& System::GetFileSystem ().getFileInfo (autoSaveInfo, autoSavePath)
			&& System::GetFileSystem ().getFileInfo (documentInfo, document.getPath ()))
		{
			if(autoSaveInfo.modifiedTime < documentInfo.modifiedTime)
				return AsyncOperation::createFailed ();
		}

		bool isOtherType = document.isImported () && autoSavePath.getFileType () != document.getPath ().getFileType ();

		// ask user
		String text;
		document.getPath ().getName (text);
		text << "\n\n" << (isEmergency ? XSTR (EmergencyBackupFound) : XSTR (NewerBackupFound));
		text << "\n" << XSTR (DoYouWantToUseTheBackup);
		if(!isOtherType)
			text << " " << XSTR (OriginalFileWillBeKeptAsVersion);

		AutoPtr<AsyncOperation> result = NEW AsyncOperation; // result is captured by value. When initializing the copied autoptr, AsyncOperation is retained.
		Promise p (Alert::askAsync (text));
		p.then ([&document, result, isOtherType, autoSavePath, isEmergency] (IAsyncOperation& operation)
		{
			if(operation.getResult ().asInt () == Alert::kYes)
			{
				// move original document to history folder, but not if it's another file type (leave the original in place)
				if(isOtherType || !DocumentVersions::isSupported () || DocumentVersions (document.getPath ()).moveDocumentToHistory (nullptr, &DocumentVersions::strDocumentSnapshotSuffix))
				{
					// move autosave file to original location
					Url restoredPath (document.getPath ());
					if(isOtherType)
					{
						// adjust filetype for imported document
						restoredPath.setFileType (document.getDocumentClass ()->getFileType (), true);
						document.setPath (restoredPath);
					}

					if(System::GetSystem ().isProcessSandboxed ())
					{
						if(System::GetFileSystem ().copyFile (restoredPath, autoSavePath))
						{
							System::GetFileSystem ().removeFile (autoSavePath);
							result->setState (IAsyncInfo::kCompleted);
						}
					}
					else if(System::GetFileSystem ().moveFile (restoredPath, autoSavePath))
						result->setState (IAsyncInfo::kCompleted);

					if(result->getState () != IAsyncInfo::kCompleted)
					{
						CCL_WARN ("Could not move autosave file to document location", 0)
						result->setState (IAsyncInfo::kFailed);
					}
				}
				else
				{
					CCL_WARN ("Could not move document to snapshots", 0)
					result->setState (IAsyncInfo::kFailed);
				}
			}
			else if(!isEmergency) // in case of emergency, leave autosave file alone if user doesn't load it (last resort)
			{
				#if ASK_KEEP_BACKUP
				// ask if user wants to keep the autosave file as a version, otherwise delete it
				Promise (Alert::askAsync (XSTR (DoYouWantToKeepTheBackup))).then ([&document, result, autoSavePath] (IAsyncOperation& operation)
				{
					bool keep = operation.getState () == IAsyncInfo::kCompleted && operation.getResult ().asInt () == Alert::kYes;
					if(keep)
						DocumentVersions (document.getPath ()).moveDocumentToHistory (&autoSavePath, &DocumentVersions::strAutosaveSnapshotSuffix);
					else
						System::GetFileSystem ().removeFile (autoSavePath);

					result->setState (IAsyncInfo::kCompleted);
				});
				#else
				DocumentVersions (document.getPath ()).moveDocumentToHistory (&autoSavePath, &DocumentVersions::strAutosaveSnapshotSuffix);
				result->setState (IAsyncInfo::kCompleted);
				#endif
			}
			else
				result->setState (IAsyncInfo::kFailed); // if emergency & result != kYes complete asyncOperation, otherwise it will leak
		});

		return result.detach ();
	}

	return AsyncOperation::createFailed ();
}
