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
// Filename    : ccl/app/fileinfo/presetfileinfo.cpp
// Description : Preset File Info Component
//
//************************************************************************************************

#include "ccl/app/fileinfo/presetfileinfo.h"

#include "ccl/app/presets/presetsystem.h"

#include "ccl/public/app/ipreset.h"
#include "ccl/public/app/presetmetainfo.h"
#include "ccl/public/gui/iparameter.h"

#include "ccl/public/storage/iurl.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum PresetFileInfoTags
	{
		kPresetName = 100,
		kDescription,
		kHasDescription,
		kTypeDescription,
		kHasTypeDescription,
		kClassName,
		kHasClassName,
		kCategory,
		kCreator,
		kHasCreator
	};
}

//************************************************************************************************
// PresetFileInfo
//************************************************************************************************

REGISTER_FILEINFO (PresetFileInfo)
void PresetFileInfo::registerInfo () {} // force linkage

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetFileInfo::canHandleFile (UrlRef path)
{
	return !path.isFolder () && System::GetPresetManager ().supportsFileType (path.getFileType ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetFileInfo::PresetFileInfo ()
: StandardFileInfo (CCLSTR ("PresetFileInfo"), CSTR ("PresetFileInfo"))
{
	paramList.addString (CSTR ("presetName"), Tag::kPresetName);
	paramList.addString (CSTR ("description"), Tag::kDescription);
	paramList.addString (CSTR ("className"), Tag::kClassName);
	paramList.addParam (CSTR ("hasClassName"), Tag::kHasClassName);
	paramList.addString (CSTR ("category"), Tag::kCategory);
	paramList.addString (CSTR ("creator"), Tag::kCreator);
	paramList.addParam (CSTR ("hasDescription"), Tag::kHasDescription);
	paramList.addParam (CSTR ("hasCreator"), Tag::kHasCreator);

	paramList.addString (CSTR ("typeDescription"), Tag::kTypeDescription);
	paramList.addParam (CSTR ("hasTypeDescription"), Tag::kHasTypeDescription);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetFileInfo::setFile (UrlRef path)
{
	if(!isLocal (path))
		return false;

	AutoPtr<IPreset> preset (System::GetPresetManager ().openPreset (path));
	if(preset)
	{
		StandardFileInfo::setFile (path);

		String presetName (preset->getPresetName ());
		String description;
		String typeDescription;
		String className;
		String creator;
		String category;

		IAttributeList* metaInfo = preset->getMetaInfo ();
		if(metaInfo)
		{
			PresetMetaAttributes metaAttribs (*metaInfo);
			description = metaAttribs.getDescription ();
			typeDescription = metaAttribs.getTypeDescription ();
			className = metaAttribs.getClassName ();
			category = metaAttribs.getCategory ();
			creator = metaAttribs.getCreator ();
		}
		paramList.byTag (Tag::kPresetName)->setValue (presetName);
		paramList.byTag (Tag::kDescription)->setValue (description);
		paramList.byTag (Tag::kHasDescription)->setValue (!description.isEmpty ());
		paramList.byTag (Tag::kTypeDescription)->setValue (typeDescription);
		paramList.byTag (Tag::kHasTypeDescription)->setValue (!typeDescription.isEmpty ());

		paramList.byTag (Tag::kClassName)->setValue (className);
		paramList.byTag (Tag::kHasClassName)->setValue (!className.isEmpty ());
		paramList.byTag (Tag::kCategory)->setValue (category);
		paramList.byTag (Tag::kCreator)->setValue (creator);
		paramList.byTag (Tag::kHasCreator)->setValue (!creator.isEmpty ());
		return true;
	}
	return false;
}
