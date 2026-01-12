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
// Filename    : ccl/app/presets/presetfileexporter.cpp
// Description : Preset File Exporter
//
//************************************************************************************************

#include "ccl/app/presets/presetfileexporter.h"
#include "ccl/app/presets/presetfileprimitives.h"

#include "ccl/base/storage/packageinfo.h"
#include "ccl/base/storage/url.h"

using namespace CCL;

//************************************************************************************************
// PresetFileExporter
//************************************************************************************************

bool PresetFileExporter::create (IUnknownList& filePromises, IUnknown* object, IUnknown* context)
{
	AutoPtr<IPresetMediator> mediator = ObjectConverter::toInterface<IPresetMediator> (object);
	if(mediator && mediator->getPresetTarget ())
	{
		PresetFileExporter* promise = NEW PresetFileExporter;
		promise->setPresetMediator (mediator);
		filePromises.add (promise->asUnknown ());
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPresetFileHandler& PresetFileExporter::getHandler () const
{
	return PresetFilePrimitives::getDefaultHandler (presetMediator);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetFileExporter::getFileName (String& fileName) const
{
	if(presetMediator)
	{
		fileName = presetMediator->makePresetName (true);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetFileExporter::getFileType (FileType& fileType) const
{
	if(presetMediator)
	{
		fileType = getHandler ().getFileType ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PresetFileExporter::createFile (UrlRef destPath, IProgressNotify* progress)
{
	tbool result = false;
	if(presetMediator)
	{
		AutoPtr<PackageInfo> metaInfo (NEW PackageInfo);
		presetMediator->getPresetMetaInfo (*metaInfo);

		result = PresetFilePrimitives::writePreset (destPath, *metaInfo, getHandler (), *presetMediator, IPresetNotificationSink::kExportPreset);
	}
	return result ? kResultOk : kResultFailed;
}
