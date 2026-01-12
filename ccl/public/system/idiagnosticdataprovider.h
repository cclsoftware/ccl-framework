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
// Filename    : ccl/public/system/idiagnosticdataprovider.h
// Description : Diagnostics Provider Interface
//
//************************************************************************************************

#ifndef _ccl_idiagnosticdataprovider_h
#define _ccl_idiagnosticdataprovider_h

#include "ccl/public/storage/filetype.h"

namespace CCL {

//************************************************************************************************
// DiagnosticDescription
//************************************************************************************************

struct DiagnosticDescription
{
	DEFINE_ENUM (DiagnosticCategory)
	{
		kErrorInformation = 1,
		kSystemInformation = 1 << 1,
		kPlugInInformation = 1 << 2,
		kApplicationLogs = 1 << 3,
		kApplicationSettings = 1 << 4
	};

	DiagnosticCategory categoryFlags;
	String fileName;
	FileType fileType;
	String subFolder;

	DiagnosticDescription (DiagnosticCategory categoryFlags = 0, StringRef fileName = nullptr)
	: categoryFlags (categoryFlags),
	  fileName (fileName)
	{}
};

//************************************************************************************************
// IDiagnosticDataProvider
//************************************************************************************************

interface IDiagnosticDataProvider: IUnknown
{
	virtual int CCL_API countDiagnosticData () const = 0;

	virtual tbool CCL_API getDiagnosticDescription (DiagnosticDescription& description, int index) const = 0;

	virtual IStream* CCL_API createDiagnosticData (int index) = 0;

	DECLARE_IID (IDiagnosticDataProvider)
};

DEFINE_IID (IDiagnosticDataProvider, 0xf5b5984b, 0x2049, 0x4f75, 0xb1, 0xd9, 0x51, 0x53, 0x75, 0x48, 0xd6, 0x16)

} // namespace CCL

#endif // _ccl_idiagnosticdataprovider_h
