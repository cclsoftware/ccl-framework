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
// Filename    : ccl/public/security/iasn1contenthandler.h
// Description : ASN.1 Content Handler Interface
//
//************************************************************************************************

#ifndef _ccl_iasn1contenthandler_h
#define _ccl_iasn1contenthandler_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IStream;

namespace Security {
namespace Crypto {

//************************************************************************************************
// Crypto::IASN1ContentHandler
/** \ingroup base_io */
//************************************************************************************************

interface IASN1ContentHandler: IUnknown
{
	/** Receive an integer. */
	virtual tresult CCL_API integer (int i) = 0;

	/** Receive a stream of bytes. */
	virtual tresult CCL_API octetString (IStream& octets) = 0;

	/** Receive a character string. */
	virtual tresult CCL_API string (StringRef data) = 0;

	/** Receive an ASN.1 sequence (ordered collection of one or more types)  */
	virtual tresult CCL_API sequence (IStream& data) = 0;

	/** Receive an ASN.1 set (unordered collection of one or more types) */
	virtual tresult CCL_API set (IStream& data) = 0;

	/** Receive a context tag, return the corresponding default (typically universal) tag and whether this context tag is implicit or explicit*/
	virtual tresult CCL_API context (unsigned char& defaultTag, tbool& implicit, unsigned char contextTag) = 0;
	
	DECLARE_IID (IASN1ContentHandler)
};

DEFINE_IID (IASN1ContentHandler, 0xad63724b, 0xbabb, 0x6b4c, 0xb5, 0xa2, 0xe8, 0xee, 0x13, 0xf5, 0x5e, 0x15)

} // namespace Crypto
} // namespace Security
} // namespace CCL

#endif // _ccl_iasn1contenthandler_h
