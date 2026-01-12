//************************************************************************************************
//
// CCL Crypt Tool
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
// Filename    : crypttool.h
// Description : CCL Crypt Tool
//
//************************************************************************************************

#ifndef _crypttool_h
#define _crypttool_h

#include "ccl/extras/tools/toolhelp.h"

#include "ccl/main/cclargs.h"

namespace CCL {

//************************************************************************************************
// CryptTool
//************************************************************************************************

class CryptTool: public CommandLineTool
{
public:
	CryptTool (ArgsRef args);

	int run ();

protected:
	ArgsRef args;

	enum CipherAction { kEncrypt, kDecrypt };
	int cipher (UrlRef outDataPath, UrlRef inDataPath, UrlRef cipherPath, CipherAction action);

	int generate (UrlRef privateKeyPath, UrlRef publicKeyPath);
	int sign (UrlRef outDataPath, UrlRef inDataPath, StringID rootName, UrlRef privateKeyPath);
	int signPackage (UrlRef outDataPath, UrlRef inDataPath, UrlRef privateKeyPath);
	int vendorSignPackage (UrlRef outDataPath, UrlRef inDataPath, UrlRef privateTokenPath);
	int createPublicToken (UrlRef outPath, UrlRef vendorPublicKeyPath, StringRef vendorName, 
						   UrlRef authorityPrivateKeyPath, StringID authorityKeyId);
	int createPrivateToken (UrlRef outPath, UrlRef vendorPrivateKeyPath, UrlRef vendorPublicTokenPath);
};

} // namespace CCL

#endif // _crypttool_h
