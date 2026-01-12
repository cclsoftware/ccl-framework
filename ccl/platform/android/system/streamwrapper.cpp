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
// Filename    : ccl/platform/android/system/streamwrapper.cpp
// Description : OutputStreamWrapper native implementation
//
//************************************************************************************************

#include "ccl/platform/android/cclandroidjni.h"

#include "ccl/public/base/istream.h"
#include "ccl/public/base/buffer.h"

using namespace CCL;
using namespace CCL::Android;

//************************************************************************************************
// dev.ccl.OutputStreamWrapper Java native methods
//************************************************************************************************

DECLARE_JNI_CLASS_METHOD_CCLSYSTEM (void, OutputStreamWrapper, writeBufferNative, JniIntPtr nativeStreamPtr, jbyteArray bytes, int offset, int length)
{
	if(IStream* stream = JniCast<IStream>::fromIntPtr (nativeStreamPtr))
	{
		JniByteArray byteArray (env, bytes);
		Buffer buffer (length);
		byteArray.getData (buffer, offset, length);
		stream->write (buffer, length);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLSYSTEM (void, OutputStreamWrapper, writeByteNative, JniIntPtr nativeStreamPtr, int data)
{
	if(IStream* stream = JniCast<IStream>::fromIntPtr (nativeStreamPtr))
	{
		char byte (data);
		stream->write (&byte, 1);
	}
}
