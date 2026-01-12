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
// Filename    : ccl/platform/shared/skia/skiastream.h
// Description : Skia Stream
//
//************************************************************************************************

#ifndef _ccl_skiastream_h
#define _ccl_skiastream_h

#include "ccl/platform/shared/skia/skiaglue.h"

#include "ccl/public/base/istream.h"

namespace CCL {

//************************************************************************************************
// SkiaStream
//************************************************************************************************

class SkiaStream: public SkStream
{
public:
	SkiaStream (IStream* stream);
	
	// SkStream
	size_t read (void* buffer, size_t size) override;
	bool isAtEnd () const override;
	bool hasLength () const override;
	size_t getLength () const override;

protected:
	SharedPtr<IStream> cclStream;
	size_t size;
};

} // namespace CCL

#endif // _ccl_skiastream_h
