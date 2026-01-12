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
// Filename    : ccl/app/presets/presettrader.cpp
// Description : Preset Trader Component
//
//************************************************************************************************

#include "ccl/app/presets/presettrader.h"
#include "ccl/app/presets/presetcomponent.h"
#include "ccl/app/presets/presetsystem.h"
#include "ccl/app/presets/presetfile.h"

#include "ccl/app/params.h"
#include "ccl/app/utilities/fileoperations.h"

#include "ccl/base/collections/stringlist.h"
#include "ccl/base/boxedtypes.h"

#include "ccl/public/app/presetmetainfo.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/gui/framework/ifileselector.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/cclerror.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Presets")
	XSTRING (ImportPreset, "Import Preset")
	XSTRING (ExportPreset, "Export Preset")
	XSTRING (ExportAsX, "Export %(1)")
	XSTRING (ExportPresetFailed, "Preset file export failed.")
	XSTRING (ImportingPreset, "Importing Preset")
	XSTRING (LoadPreset, "Load Preset")
	XSTRING (LoadPresetFile, "Load Preset File")
	XSTRING (OnePresetFileWasImported, "One preset file was imported.")
	XSTRING (NPresetFilesWereImported, "%(1) preset files were imported.")
	XSTRING (ButItsNotAPresetFor, "But it's not a preset for %(1)!")
	XSTRING (ButNoneisAPresetFor, "But none of them is a preset for %(1)!")
	XSTRING (TheseNFilesCouldntBeImported, "These %(1) files could not be imported:")
	XSTRING (ThisFileCouldntBeImported, "This file could not be imported:")
END_XSTRINGS

//************************************************************************************************
// PresetTrader
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PresetTrader, Component)
IMPLEMENT_COMMANDS (PresetTrader, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (PresetTrader)
	DEFINE_COMMAND ("Presets", "Import Preset",	PresetTrader::onImportPreset)
	DEFINE_COMMAND ("Presets", "Export Preset",	PresetTrader::onExportPreset)
	DEFINE_COMMAND ("Presets", "Load Preset File",	PresetTrader::onLoadPreset)
	DEFINE_COMMAND ("Presets", nullptr,				PresetTrader::onExportPresetAs)
END_COMMANDS (PresetTrader)

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetTrader::PresetTrader (PresetComponent& presetComponent)
: Component (CCLSTR ("PresetTrader")),
  presetComponent (presetComponent)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetTrader::collectFileTypes (bool forExport)
{
	if(IUnknown* target = presetComponent.getTarget ())
	{
		// collect filetypes for target
		fileTypes.getContent ().removeAll ();
		int flags = forExport ? IPresetFileHandler::kCanExport : IPresetFileHandler::kCanImport;
		System::GetPresetFileRegistry ().collectFileTypes (fileTypes, target, flags);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetTrader::runFileSelector (UnknownList& urls, int fileSelectorType, StringRef title, const FileType* fileType, IAttributeList* metaInfo)
{
	if(!fileTypes.getContent ().isEmpty ())
	{
		AutoPtr<IFileSelector> fs (ccl_new<IFileSelector> (CCL::ClassID::FileSelector));

		if(fileType)
			fs->addFilter (*fileType);
		else
		{
			VectorForEach (fileTypes.getContent (), const FileType&, type)
				fs->addFilter (type);
			EndFor

			if(!fileTypes.getContent ().isEmpty ())
				fileType = &fileTypes.getContent ().at (0); // use first filetype for finding handler
		}

		if(fileType && metaInfo)
		{
			if(IPresetFileHandler* handler = System::GetPresetFileRegistry ().getHandlerForFileType (*fileType))
			{
				Url folder;
				tbool result = (fileSelectorType == IFileSelector::kSaveFile)
					? handler->getWriteLocation (folder, metaInfo)
					: handler->getReadLocation (folder, metaInfo, 0);
				if(result)
				{
					// ascend max. 2 levels if folder doesn't exist
					for(int i = 0; i < 2; i++)
						if(System::GetFileSystem ().fileExists (folder))
							break;
						else
							folder.ascend ();
					fs->setFolder (folder);
				}
			}
		}

		if(fs->run (fileSelectorType, title))
			for(int i = 0, num = fs->countPaths (); i < num; i++)
				if(IUrl* url = fs->getPath (i))
					urls.add (url, true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUrl* PresetTrader::selectFile (int fileSelectorType, StringRef title, const FileType* fileType, IAttributeList* metaInfo)
{
	UnknownList urls;
	runFileSelector (urls, fileSelectorType, title, fileType, metaInfo);

	UnknownPtr<IUrl> url (urls.getFirst ());
	if(url)
		url->retain ();

	return url;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PresetTrader::getExportTitle (const FileType& fileType)
{
	String title;
	Variant args[] = {fileType.getDescription ()};
	title.appendFormat (XSTR (ExportAsX), args, 1);
	return title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetTrader::exportPreset (StringRef title, const FileType* fileType)
{
	AutoPtr<IAttributeList> metaInfo (presetComponent.createMetaInfo ());

	AutoPtr<IUrl> url (selectFile (IFileSelector::kSaveFile, title, fileType, metaInfo));
	if(url)
	{
		String presetName;
		url->getName (presetName, false);
		PresetMetaAttributes (*metaInfo).setTitle (presetName);

		if(!fileType)
			fileType = &url->getFileType ();

		IPresetFileHandler* handler = System::GetPresetFileRegistry ().getHandlerForFileType (*fileType);
		if(!handler)
		{
			handler = System::GetPresetFileRegistry ().getDefaultHandler ();
			if(!handler)
				return false;

			fileType = &handler->getFileType ();
		}

		ErrorContextGuard errorContext;

		url->setFileType (*fileType, false);
		bool result = presetComponent.writePreset (*url, *metaInfo, *handler, IPresetNotificationSink::kExportPreset) != 0;
		if(result == false)
			Alert::errorWithContext (XSTR (ExportPresetFailed));

	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetTrader::importPresets (const IUnknownList& urls, PresetComponent* targetComponent)
{
	IPresetFileHandler* handler = targetComponent ? &targetComponent->getPresetHandler () : System::GetPresetFileRegistry ().getDefaultHandler ();
	if(handler == nullptr)
		return false;

	// determine destination folder for presetComponent
	Url componentFolder;
	AutoPtr<IAttributeList> componentMetaInfo;
	if(targetComponent)
	{
		componentMetaInfo = targetComponent->createMetaInfo ();
		handler->getWriteLocation (componentFolder, componentMetaInfo);
	}

	// resolve content of folders
	struct FileList: public ObjectList
	{
		FileList (const IUnknownList& urls)
		{
			objectCleanup (true);

			ForEachUnknown (urls, obj)
				UnknownPtr<IUrl> url (obj);
				if(url)
					addUrl (*url);
			EndFor
		}

		void addUrl (UrlRef path)
		{
			if(path.isFolder())
				ForEachFile (System::GetFileSystem ().newIterator (path), p)
					addUrl (*p); // recursion
				EndFor
			else
				add (NEW Url (path));
		}
	};
	FileList fileList (urls);

	StringList failedNames;

	// check the selected preset files and create file copy tasks
	BatchOperation copier;
	copier.setCancelEnabled (false);

	ForEach (fileList, Url, url)
		String fileName;
		url->getName (fileName);

		// try to open preset from original location
		AutoPtr<IPreset> sourcePreset = System::GetPresetManager ().openPreset (*url);
		IAttributeList* presetMetaInfo = sourcePreset ? sourcePreset->getMetaInfo () : nullptr;
		if(presetMetaInfo)
		{
			// determine destination path for this preset (can be for another target)
			Url destPath;
			if(handler->getWriteLocation (destPath, presetMetaInfo))
			{
				// copy into our location if not already there
				Url sourceFolder (*url);
				sourceFolder.ascend ();
				if(sourceFolder != destPath)
				{
					destPath.descend (fileName, IUrl::kFile);

					FileCopyTask* copyTask = NEW FileCopyTask;
					copyTask->setSourcePath (*url);
					copyTask->setDestPath (destPath);
					copier.addTask (copyTask);
				}
			}
		}
		else
			failedNames.add (fileName);
	EndFor

	int numImported = 0;
	int numFailed = failedNames.count ();
	tbool presetRestored = false;

	// copy files
	if(copier.run (XSTR (ImportingPreset)))
	{
		ForEach (copier, FileCopyTask, copyTask)
			UrlRef destPath = copyTask->getDestPath (); // (may have been changed to a unique name)

			AutoPtr<IPreset> newPreset = System::GetPresetManager ().openPreset (destPath);
			if(newPreset)
			{
				// notify preset manager about the new preset
				System::GetPresetManager ().onPresetCreated (destPath, *newPreset);
				numImported++;

				// try to load first preset until succeeded
				if(targetComponent && !presetRestored)
				{
					// only try presets for our component
					Url destFolder (destPath);
					destFolder.ascend ();
					if(destFolder == componentFolder)
					presetRestored = targetComponent->restorePreset (newPreset);
				}
			}
		EndFor
	}

	// build alert message
	int alertType = -1; // no alert
	String text;

	if(numImported > 0)
	{
		// sucessfully imported
		alertType = Alert::kInformation;

		Variant args[] = { numImported };
		text.appendFormat (numImported == 1
			? XSTR (OnePresetFileWasImported)
			: XSTR (NPresetFilesWereImported),
			args, ARRAY_COUNT (args));

		if(targetComponent && !presetRestored)
		{
			// but for another preset target
			alertType = Alert::kWarning;

			Variant args[] = { PresetMetaAttributes (*componentMetaInfo).getClassName () };
			text << "\n";
			text.appendFormat (numImported == 1
				? XSTR (ButItsNotAPresetFor)
				: XSTR (ButNoneisAPresetFor)
				, args, ARRAY_COUNT (args));
		}
	}

	if(numFailed > 0)
	{
		// failed to import
		alertType = (alertType == -1) ? Alert::kError : Alert::kWarning;

		Variant args[] = { numFailed };
		if(!text.isEmpty ())
			text << "\n\n";
		text.appendFormat (numFailed == 1
			? XSTR (ThisFileCouldntBeImported)
			: XSTR (TheseNFilesCouldntBeImported),
			args, ARRAY_COUNT (args));

		text << "\n\n";

		StringBuilder listWriter (text);
		failedNames.addToBuilder (listWriter);
	}

	if(alertType >= 0)
		switch(alertType)
		{
		case Alert::kWarning : Alert::warn (text); break;
		case Alert::kError : Alert::error (text); break;
		default : Alert::info (text); break;
		}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetTrader::onLoadPreset (CmdArgs args)
{
	if(IUnknown* target = presetComponent.getTarget ())
	{
		if(!args.checkOnly ())
		{
			AutoPtr<IAttributeList> componentMetaInfo (presetComponent.createMetaInfo ());
			collectFileTypes (false);

			AutoPtr<IFileSelector> fs (ccl_new<IFileSelector> (CCL::ClassID::FileSelector));
			VectorForEach (fileTypes.getContent (), const FileType&, type)
				fs->addFilter (type);
			EndFor

			if(fs->run (IFileSelector::kOpenFile, XSTR (LoadPreset)))
				if(IUrl* url = fs->getPath (0))
					presetComponent.restorePreset (*url);
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetTrader::onImportPreset (CmdArgs cmd)
{
	if(!cmd.checkOnly ())
	{
		if(System::GetDesktop ().closePopupAndDeferCommand (this, cmd))
			return true;

		AutoPtr<IAttributeList> componentMetaInfo (presetComponent.createMetaInfo ());

		// select files
		UnknownList urls;
		collectFileTypes (false);
		runFileSelector (urls, IFileSelector::kOpenMultipleFiles, XSTR (ImportPreset), nullptr, componentMetaInfo);

		return importPresets (urls, &presetComponent);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetTrader::onExportPreset (CmdArgs cmd)
{
	if(!cmd.checkOnly ())
	{
		if(System::GetDesktop ().closePopupAndDeferCommand (this, cmd))
			return true;

		// user selects export format in fileselector
		collectFileTypes (true);
		exportPreset (XSTR (ExportPreset), nullptr);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetTrader::onExportPresetAs (CmdArgs cmd)
{
	if(cmd.name.startsWith ("Export "))
	{
		String extension (cmd.name.subString (7, -1));

		const FileType* fileType = nullptr;
		collectFileTypes (true);

		VectorForEach (fileTypes.getContent (), const FileType&, type)
			if(type.getExtension () == extension)
			{
				fileType = &type;
				break;
			}
		EndFor

		if(fileType)
		{
			if(!cmd.checkOnly ())
				exportPreset (getExportTitle (*fileType), fileType);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetTrader::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IParameter::kExtendMenu)
	{
		UnknownPtr<IMenu> menu (msg.getArg (0));
		if(menu)
		{
			if(menu->countItems () > 0)
				menu->addSeparatorItem ();

			// import entry
			collectFileTypes (false);
			if(!fileTypes.getContent ().isEmpty ())
			{
				menu->addCommandItem (CommandWithTitle (CSTR ("Presets"), CSTR ("Load Preset File"), XSTR (LoadPresetFile)), this, true);
				menu->addCommandItem (CommandWithTitle (CSTR ("Presets"), CSTR ("Import Preset"), XSTR (ImportPreset)), this, true);
			}

			// export entry for each filetype
			collectFileTypes (true);
			VectorForEach (fileTypes.getContent (), const FileType&, type)
				MutableCString cmdName (CSTR ("Export "));
				cmdName.append (type.getExtension ());

				menu->addCommandItem (CommandWithTitle (CSTR ("Presets"), cmdName, getExportTitle (type)), this, true);
			EndFor
		}
	}
	SuperClass::notify (subject, msg);
}
