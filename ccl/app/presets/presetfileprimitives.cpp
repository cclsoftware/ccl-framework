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
// Filename    : ccl/app/presets/presetfileprimitives.cpp
// Description : Preset File Primitives
//
//************************************************************************************************

#include "ccl/app/presets/presetfileprimitives.h"

#include "ccl/app/presets/presetfile.h"
#include "ccl/app/presets/presetcollection.h"
#include "ccl/app/presets/presetcomponent.h"
#include "ccl/app/presets/presetsystem.h"
#include "ccl/app/presets/simplepreset.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/app/presetmetainfo.h"
#include "ccl/public/text/translation.h"

using namespace CCL;

//************************************************************************************************
// PresetFilePrimitives
//************************************************************************************************
	 
const String PresetFilePrimitives::kDefaultPresetFileName (CCLSTR ("default"));

//////////////////////////////////////////////////////////////////////////////////////////////////

IPresetFileHandler& PresetFilePrimitives::getDefaultHandler (IPresetMediator* presetMediator)
{
	#if 1
	if(presetMediator && !presetMediator->getDefaultPresetType ().isEmpty ())
		if(IPresetFileHandler* handler = System::GetPresetFileRegistry ().getHandlerForMimeType (MutableCString (presetMediator->getDefaultPresetType ())))
			return *handler;
	#endif
	
	UnknownPtr<IPresetCollector> collector (presetMediator);
	return collector ? (IPresetFileHandler&)PresetCollectionHandler::instance () : PresetPackageHandler::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetFilePrimitives::getWriteLocation (IUrl& folder, const FileType& fileType, IAttributeList* metaInfo)
{
	// try handler for filetype
	IPresetFileHandler* presetHandler = System::GetPresetFileRegistry ().getHandlerForFileType (fileType);
	tbool result = presetHandler && presetHandler->getWriteLocation (folder, metaInfo);

	// fallback to default handler
	if(!result)
		if(presetHandler = System::GetPresetFileRegistry ().getDefaultHandler ())
			result = presetHandler->getWriteLocation (folder, metaInfo);

	return result != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetFilePrimitives::getTempLocation (IUrl& folder)
{
	if(IPresetFileHandler* defaultHandler = System::GetPresetFileRegistry ().getDefaultHandler ())
	{
		defaultHandler->getWriteLocation (folder);
		folder.descend ("(Temp)", Url::kFolder);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetFilePrimitives::descendSubFolder (Url& url, IAttributeList& metaInfo)
{
	PresetMetaAttributes metaAttributes (metaInfo);
	if(!metaAttributes.getSubFolder ().isEmpty ())
		url.descend (metaAttributes.getSubFolder (), IUrl::kFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetFilePrimitives::descendPresetName (Url& url, StringRef presetName, IPresetFileHandler& handler, bool makeUnique)
{
	url.descend (LegalFileName (presetName));
	url.setFileType (handler.getFileType (), false); // allow dots in the preset name

	if(makeUnique)
		url.makeUnique ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PresetFilePrimitives::makeUniquePresetName (StringRef name, IAttributeList* metaInfo, const FileType* fileType)
{
	String baseName (name);
	int64 suffix = 1;

	if(baseName.isEmpty ())
		baseName = CCLSTR ("Preset");
	else
	{
		// check if already ends with (x)
		if(baseName.endsWith (")"))
		{
			int openBracketIndex = baseName.lastIndex (" (");
			if(openBracketIndex > 0)
			{
				String number (baseName.subString (openBracketIndex + 2));
				if(number.getIntValue (suffix))
				{
					suffix ++;
					baseName.truncate (openBracketIndex);
				}
			}
		}
	}

	String presetName (baseName);
	while(System::GetPresetManager ().presetExists (metaInfo, presetName, fileType))
	{
		presetName = baseName;
		presetName << " (" << suffix++ << ")";
	}
	return presetName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool PresetFilePrimitives::writePreset (UrlRef url, IAttributeList& metaInfo, IPresetFileHandler& handler,
										 IPresetMediator& presetMediator, int notificationHint)
{
	AutoPtr<IPreset> preset (handler.createPreset (url, metaInfo));
	if(preset)
	{
		// notify target (before)
		UnknownPtr<IPresetNotificationSink> targetNotify (presetMediator.getPresetTarget ());
		if(targetNotify)
			targetNotify->onPresetStoring (*preset, notificationHint);

		tbool result = presetMediator.storePreset (*preset);
		
		// notify target (after)
		if(targetNotify)
			targetNotify->onPresetStored (*preset, notificationHint);

		if(result)
			System::GetPresetManager ().onPresetCreated (url, *preset);
		return result;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetFilePrimitives::makeRelativePresetUrl (Url& presetUrl, IAttributeList* metaInfo)
{
	// try all handlers
	IPresetFileRegistry& registry (System::GetPresetFileRegistry ());
	for(int h = 0, numhandlers = registry.countHandlers (); h < numhandlers; h++)
		if(makeRelativePresetUrl (*registry.getHandler (h), presetUrl, metaInfo))
			return true;

	// extra try for presets that were found in a folder structure different from their handler's rules (e.g. not "Vendor/ClassName")
	Url folder;
	SimplePresetHandler::getFactoryFolder (folder);
	if(presetUrl.makeRelative (folder))
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetFilePrimitives::makeRelativePresetUrl (IPresetFileHandler& handler, Url& presetUrl, IAttributeList* metaInfo)
{
	// try to find a root path (for given metaInfo) as ancestor of presetUrl
	int i = 0;
	Url folder;
	while(handler.getReadLocation (folder, metaInfo, i++))
		if(presetUrl.makeRelative (folder))
			return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PresetFilePrimitives::determineRelativeSubFolder (IPresetFileHandler& handler, IAttributeList& metaInfo, UrlRef presetUrl)
{
	String subFolder;
	Url url (presetUrl);
	if(makeRelativePresetUrl (url, &metaInfo))
	{
		Url base;
		url.makeAbsolute (base); // remove the "./"
		url.getPathName (subFolder);
	}
	return determineRelativeSubFolder (handler, metaInfo, subFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PresetFilePrimitives::determineRelativeSubFolder (IPresetFileHandler& handler, IAttributeList& metaInfo, StringRef subFolder)
{
	String classPrefix;
	if(!handler.getSubFolder (classPrefix, metaInfo))
		PresetPackageHandler::instance ().getSubFolder (classPrefix, metaInfo); // fallback to default subfolder structure "Vendor/ClassName"

	static String userPresetsPrefix (String (SimplePresetHandler::getUserPresetFolderName ()) << Url::strPathChar);

	auto extractRemainder = [] (StringRef subFolder, StringRef classPrefix) -> String
	{
		if(subFolder == classPrefix)
			return String::kEmpty; // directly in class folder
		else
		{
			// subFolder starts with prefix, check if the remainder contains another "/"
			String remainder (subFolder.subString (classPrefix.length ()));
			int separatorIndex = remainder.index (Url::strPathChar);
			if(separatorIndex >= 0)
			{
				// use only the part after the "/" as subFolder, ignoring additional characters before it (interpret them as variations of the prefix)
				ASSERT (remainder.length () > separatorIndex + 1) // otherwise the "/" would not make sense
				return remainder.subString (separatorIndex + 1);
			}
			else
				return String::kEmpty; // ignore additional characters after prefix
		}
	};

	if(subFolder.startsWith (classPrefix))
	{
		return extractRemainder (subFolder, classPrefix);
	}
	else if(&handler == &PresetPackageHandler::instance ())
	{
		// try alternative subFolder for class
		String alternativeClassPrefix;
		if(PresetPackageHandler::instance ().getAlternativeSubFolder (alternativeClassPrefix, metaInfo))
			if(subFolder.startsWith (alternativeClassPrefix))
				return extractRemainder (subFolder, alternativeClassPrefix);
	}

	if(subFolder.startsWith (userPresetsPrefix))
	{
		// we don't want "User Presets" to appear in subFolder strings
		return subFolder.subString (userPresetsPrefix.length ());
	}

	// no match with classPrefix: full subFolder
	return subFolder;
}

//************************************************************************************************
// PresetRenamer
//************************************************************************************************

PresetRenamer::PresetRenamer (IPreset& preset)
: Renamer (preset.getPresetName ()),
  preset (preset)
{
	setAlreadyExistsMessage (PresetComponent::getPresetExistsMessage ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetRenamer::doesAlreadyExist (StringRef newName)
{
	return System::GetPresetManager ().presetExists (preset.getMetaInfo (), newName) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetRenamer::performRename (StringRef newName)
{
	return System::GetPresetManager ().renamePreset (preset, newName) != 0;
}
