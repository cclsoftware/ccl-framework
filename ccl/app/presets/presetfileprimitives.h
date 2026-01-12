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
// Filename    : ccl/app/presets/presetfileprimitives.h
// Description : Preset File Primitives
//
//************************************************************************************************

#ifndef _ccl_presetfileprimitives_h
#define _ccl_presetfileprimitives_h

#include "ccl/public/app/ipreset.h"

#include "ccl/app/components/filerenamer.h"

namespace CCL {

class Url;

//************************************************************************************************
// PresetFilePrimitives
//************************************************************************************************

class PresetFilePrimitives
{
public:
	/** Chooses a default handler: PresetCollectionHandler if mediator implements IPresetCollector, or PresetPackageHandler otherwise. */
	static IPresetFileHandler& getDefaultHandler (IPresetMediator* presetMediator);

	static bool getWriteLocation (IUrl& folder, const FileType& fileType, IAttributeList* metaInfo);

	static bool getTempLocation (IUrl& folder);

	//////////////////////////////////////////////////////////////////////////////////////////////////

	/** Descends into subfolder from meta attributes, if any. */
	static void descendSubFolder (Url& url, IAttributeList& metaInfo);

	/** Descends with a valid fileName from presetName and filetype from handler. */
	static void descendPresetName (Url& url, StringRef presetName, IPresetFileHandler& handler, bool makeUnique);

	/** Make a unique preset name from the given base name and meta attributes; if no fileType is given, the name alone will be unique. */
	static String makeUniquePresetName (StringRef baseName, IAttributeList* metaInfo, const FileType* fileType = nullptr);

	/** Lets handler create a preset file and presetMediator store the data into it. */
	static tbool writePreset (UrlRef url, IAttributeList& metaInfo, IPresetFileHandler& handler, IPresetMediator& presetMediator, int notificationHint);

	static bool makeRelativePresetUrl (Url& presetUrl, IAttributeList* metaInfo);
	
	static bool makeRelativePresetUrl (IPresetFileHandler& handler, Url& presetUrl, IAttributeList* metaInfo);
	
	static String determineRelativeSubFolder (IPresetFileHandler& handler, IAttributeList& metaInfo, UrlRef presetUrl);

	static String determineRelativeSubFolder (IPresetFileHandler& handler, IAttributeList& metaInfo, StringRef subFolder);

	static const String kDefaultPresetFileName;
};

//************************************************************************************************
// PresetRenamer
//************************************************************************************************

class PresetRenamer: public Renamer
{
public:
	PresetRenamer (IPreset& preset);

	// Renamer
	bool doesAlreadyExist (StringRef newName) override;
	bool performRename (StringRef newName) override;

protected:
	IPreset& preset;
};

} // namespace CCL

#endif // _ccl_presetfileprimitives_h
