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
// Filename    : core/extras/extensions/coresignature.h
// Description : Digital Signature
//
//************************************************************************************************

#ifndef _coresignature_h
#define _coresignature_h

#include "core/public/corebuffer.h"

namespace Core {
namespace Security {

typedef const IO::Buffer& MaterialRef;

//************************************************************************************************
// SignatureVerifier
//************************************************************************************************

struct SignatureVerifier
{
	virtual ~SignatureVerifier () {}

	virtual bool verifySignature (MaterialRef data, MaterialRef publicKey, MaterialRef signature) const = 0;
};

//************************************************************************************************
// SignatureVerifierRS256
//************************************************************************************************

class SignatureVerifierRS256: public SignatureVerifier
{
public:
	// SignatureVerifier
	bool verifySignature (MaterialRef data, MaterialRef publicKey, MaterialRef signature) const override;
};

} // namespace Core
} // namespace Security

#endif // _coresignature_h
