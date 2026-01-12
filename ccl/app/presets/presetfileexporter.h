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
// Filename    : ccl/app/presets/presetfileexporter.h
// Description : Preset File Exporter
//
//************************************************************************************************

#ifndef _ccl_presetfileexporter_h
#define _ccl_presetfileexporter_h

#include "ccl/public/app/ipreset.h"

#include "ccl/base/objectconverter.h"

namespace CCL {

//************************************************************************************************
// PresetFileExporter
/** Exports a preset file. */
//************************************************************************************************

class PresetFileExporter: public FilePromise
{
public:
	static bool create (IUnknownList& filePromises, IUnknown* object, IUnknown* context);

	PROPERTY_SHARED_AUTO (IPresetMediator, presetMediator, PresetMediator)

	// FilePromise
	tbool CCL_API getFileName (String& fileName) const override;
	tbool CCL_API getFileType (FileType& fileType) const override;
	tresult CCL_API createFile (UrlRef destPath, IProgressNotify* progress = nullptr) override;

private:
	IPresetFileHandler& getHandler () const;
};

} // namespace CCL

#endif // _ccl_presetfileexporter_h
