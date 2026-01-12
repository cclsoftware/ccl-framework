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
// Filename    : ccl/base/security/fingerprint.h
// Description : Fingerprint
//
//************************************************************************************************

#ifndef _ccl_fingerprint_h
#define _ccl_fingerprint_h

#include "ccl/base/storage/url.h"

#include "ccl/public/base/iasyncoperation.h"

namespace CCL {
namespace Security {

//************************************************************************************************
// FileFingerprint
//************************************************************************************************

class FileFingerprint: public Object
{
public:
	DECLARE_CLASS (FileFingerprint, Object)

	FileFingerprint (UrlRef filePath = Url ());
	~FileFingerprint ();
	
	IAsyncOperation* calculate ();

protected:
	class FingerprintCalculator;
	friend class FingerprintCalculator;

	FingerprintCalculator* calculator;
	Url filePath;
};

//************************************************************************************************
// AppFingerprint
//************************************************************************************************

class AppFingerprint: public FileFingerprint
{
public:
	DECLARE_CLASS (AppFingerprint, FileFingerprint)

	AppFingerprint ();
};

} // namespace Security
} // namespace CCL

#endif // _ccl_fingerprint_h
