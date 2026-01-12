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
// Filename    : ccl/platform/android/system/java/OutputStreamWrapper.java
// Description : OutputStream Wrapper
//
//************************************************************************************************

package dev.ccl.cclsystem;

import androidx.annotation.Keep;

//************************************************************************************************
// OutputStreamWrapper
/** Wraps a java OutputStream around a native CCL::IStream (passed as native pointer). 
	The native side will forward the write calls to IStream::write (). */
//************************************************************************************************

@Keep
public class OutputStreamWrapper extends java.io.OutputStream
{
	private final long nativeStreamPtr;

	public OutputStreamWrapper (long nativeStreamPtr)
	{
		this.nativeStreamPtr = nativeStreamPtr;
	}

	// note: the single byte method must be overriden, the other ones can be overriden for better performance
	@Override public void write (byte[] bytes, int offset, int length)	{ writeBufferNative (nativeStreamPtr, bytes, offset, length); }
	@Override public void write (byte[] bytes)							{ writeBufferNative (nativeStreamPtr, bytes, 0, bytes.length); }
	@Override public void write (int b)									{ writeByteNative (nativeStreamPtr, b); }

	public native void writeBufferNative (long nativeStreamPtr, byte[] bytes, int offset, int length);
	public native void writeByteNative (long nativeStreamPtr, int b);
}
