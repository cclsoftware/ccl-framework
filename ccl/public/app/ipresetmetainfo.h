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
// Filename    : ccl/public/app/ipresetmetainfo.h
// Description : Preset Meta Information
//
//************************************************************************************************

#ifndef _ccl_ipresetmetainfo_h
#define _ccl_ipresetmetainfo_h

#include "ccl/public/app/idocumentmetainfo.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Preset Meta Information
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Meta
{
	/** Filename of data file inside package [String] (e.g. "data.bin"). */
	DEFINE_STRINGID (kPresetDataFile, "Preset:DataFile")

	/** MIME type of data file [String] (e.g. "application/x-document-type"). */
	DEFINE_STRINGID (kPresetDataMimeType, "Preset:DataMimeType")

	/** Preset type description [String] (e.g. "Sound Preset"). */
	DEFINE_STRINGID (kPresetTypeDescription, "Preset:TypeDescription")

	/** Relative subfolder for sorting [String]. */
	DEFINE_STRINGID (kPresetSubFolder, "Preset:SubFolder")

	/** Preset handler location [String]. */
	DEFINE_STRINGID (kPresetHandlerLocation, "PresetHandler:Location")

	/** Preset handler vendor name [String]. */
	DEFINE_STRINGID (kPresetHandlerVendorName, "PresetHandler:Vendor")

	/** Fallback class ID [UID] needed to create default instance when preset can be handled by more than one class and preset has no own classID. */
	DEFINE_STRINGID (kFallbackClassID, "PresetHandler:FallbackClassID")
}

} // namespace CCL

#endif // _ccl_ipresetmetainfo_h
