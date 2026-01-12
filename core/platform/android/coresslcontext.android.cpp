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
// Filename    : core/platform/android/coresslcontext.android.cpp
// Description : SSL context based on java SSLContext / SSLEngine
//
//************************************************************************************************

#include "core/platform/android/coresslcontext.android.h"

#include "core/system/coredebug.h"

#define DEBUG_LOG 0

namespace Core {

//************************************************************************************************
// dev.ccl.core.SSLChannel
//************************************************************************************************

DECLARE_JNI_CLASS (SSLChannel, CORE_CLASS_PREFIX "SSLChannel")
	DECLARE_JNI_CONSTRUCTOR (construct, Java::JniIntPtr, jstring)
	DECLARE_JNI_METHOD (void, close)
	DECLARE_JNI_METHOD (void, handshake)
	DECLARE_JNI_METHOD (int, read, jbyteArray, int)
	DECLARE_JNI_METHOD (int, write, jbyteArray, int)
END_DECLARE_JNI_CLASS (SSLChannel)

DEFINE_JNI_CLASS (SSLChannel)
	DEFINE_JNI_CONSTRUCTOR (construct, "(JLjava/lang/String;)V")
	DEFINE_JNI_METHOD (close, "()V")
	DEFINE_JNI_METHOD (handshake, "()V")
	DEFINE_JNI_METHOD (read, "([BI)I")
	DEFINE_JNI_METHOD (write, "([BI)I")
END_DEFINE_JNI_CLASS

} // namespace

using namespace Core;
using namespace Platform;
using namespace Java;

//************************************************************************************************
// AndroidSSLContext
//************************************************************************************************

struct AndroidSSLContext::LogBuffer
{
	char str[64] = {0};

	LogBuffer(void* buffer, int size)
	{
		for(int i = 0; i < get_min (size, ARRAY_COUNT (str) - 1); i++)
			str[i] = ((char*)buffer)[i];
	}
};

//************************************************************************************************
// AndroidSSLContext
//************************************************************************************************

AndroidSSLContext::AndroidSSLContext ()
: ioHandler (0),
  javaPlainInput (JniAccessor (), kJavaBufferSize),
  javaPlainOutput (JniAccessor (), kJavaBufferSize),
  plainInputRemaining (0),
  plainInputStart (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

jobject AndroidSSLContext::getSSLChannel ()
{
	if(!sslChannel)
	{
		JniAccessor jni;
		JniString peerNameString (jni, peerName);

		sslChannel.assign (jni, jni.newObject (SSLChannel, SSLChannel.construct, (JniIntPtr)this, peerNameString.getString ()));
	}
	return sslChannel;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidSSLContext::setIOHandler (ISSLContextIOHandler* _ioHandler)
{
	ioHandler = _ioHandler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidSSLContext::setPeerName (CStringPtr peerName)
{
	ASSERT (!sslChannel)
	this->peerName = peerName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult AndroidSSLContext::handshake ()
{
	jobject channel = getSSLChannel ();
	if(!channel)
		return kSSLFailed;

	SSLChannel.handshake (channel);
	return kSSLSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult AndroidSSLContext::close ()
{
	if(sslChannel)
		SSLChannel.close (sslChannel);

	return kSSLSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult AndroidSSLContext::write (const void* buffer, int size, int& bytesWritten)
{
	jobject channel = getSSLChannel ();
	if(!channel)
		return kSSLFailed;

	if(size > javaPlainOutput.getLength ())
		javaPlainOutput.reallocate (size);

	javaPlainOutput.setData (buffer, size);

	bytesWritten = SSLChannel.write (channel, javaPlainOutput, size);
	if(bytesWritten >= 0)
		return kSSLSuccess;

	return kSSLFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult AndroidSSLContext::read (void* buffer, int size, int& bytesRead)
{
	jobject channel = getSSLChannel ();
	if(!channel)
		return kSSLFailed;

	if(plainInputRemaining > 0)
	{
		// return (only) remaining data from last java call
		int toCopy = get_min (plainInputRemaining, size);
		javaPlainInput.getData (buffer, plainInputStart, toCopy);

		plainInputRemaining -= toCopy;
		plainInputStart += toCopy;
		bytesRead = toCopy;

		#if DEBUG_LOG
		DebugPrintf ("AndroidSSLContext::read (%d) %d of %d buffered: %s", size, toCopy, plainInputRemaining, LogBuffer (buffer, bytesRead).str);
		#endif
		return kSSLSuccess;
	}

	// read at least kJavaBufferSize bytes from java
	const int bytesRequested = get_max<int> (kJavaBufferSize, size);
	if(bytesRequested > javaPlainInput.getLength ())
		javaPlainInput.reallocate (bytesRequested);

	int bytesReceived = SSLChannel.read (channel, javaPlainInput, bytesRequested);
	if(bytesReceived >= 0)
	{
		bytesRead = get_min (size, bytesReceived);
		javaPlainInput.getData (buffer, bytesRead);

		plainInputRemaining = bytesReceived - bytesRead;
		plainInputStart = bytesRead;

		#if DEBUG_LOG
		DebugPrintf ("AndroidSSLContext::read (%d) %d-> %d: %s", size, bytesRequested, bytesReceived, LogBuffer (buffer, bytesRead).str);
		#endif

		return kSSLSuccess;
	}

	return kSSLFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AndroidSSLContext::writeEncrypted (jbyteArray data, int size)
{
	if(!ioHandler)
		return -1;

	JniAccessor jni;
	Java::JniByteArray array (jni, data);

	ASSERT (size <= array.getLength ())
	if(size > encryptedOutput.getSize ())
		encryptedOutput.resize (size);

	array.getData (encryptedOutput, size);

	int bytesWritten = 0;
	SSLResult result = ioHandler->write (encryptedOutput, size, bytesWritten);

	#if DEBUG_LOG
	DebugPrintf ("AndroidSSLContext::writeEncrypted (%d): %d", size, bytesWritten);
	#endif

	if(result == kSSLFailed)
		return -1;

	return bytesWritten;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AndroidSSLContext::readEncrypted (jbyteArray data, int start, int size)
{
	if(!ioHandler)
		return -1;

	if(size > encryptedInput.getSize ())
		encryptedInput.resize (size);

	int bytesRead = 0;
	SSLResult result = ioHandler->read (encryptedInput, size, bytesRead);

	#if DEBUG_LOG
	DebugPrintf ("AndroidSSLContext::readEncrypted (%d): %d", size, bytesRead);
	#endif

	if(result == kSSLFailed)
		return -1;

	ASSERT (bytesRead <= size)
	if(bytesRead > 0)
	{
		JniAccessor jni;
		Java::JniByteArray array (jni, data);

		ASSERT (size <= array.getLength ())
		array.setData (encryptedInput, start, bytesRead);
	}
	return bytesRead;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CORE (int, SSLChannel, writeEncrypted, JniIntPtr nativeContext, jbyteArray data, int count)
{
	AndroidSSLContext* sslContext = JniCast<AndroidSSLContext>::fromIntPtr (nativeContext);
	ASSERT (sslContext)
	if(sslContext)
		return sslContext->writeEncrypted (data, count);
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CORE (int, SSLChannel, readEncrypted, JniIntPtr nativeContext, jbyteArray data, int start, int count)
{
	AndroidSSLContext* sslContext = JniCast<AndroidSSLContext>::fromIntPtr (nativeContext);
	ASSERT (sslContext)
	if(sslContext)
		return sslContext->readEncrypted (data, start, count);
	return -1;
}
