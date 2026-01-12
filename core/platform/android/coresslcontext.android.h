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
// Filename    : core/platform/android/coresslcontext.android.h
// Description : SSL context based on java SSLContext / SSLEngine
//
//************************************************************************************************

#ifndef _coresslcontext_android_h
#define _coresslcontext_android_h

#include "core/platform/shared/coreplatformsslcontext.h"
#include "core/platform/shared/jni/corejnihelper.h"

#include "core/public/corestringbuffer.h"
#include "core/public/corebuffer.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// AndroidSSLContext
//************************************************************************************************

class AndroidSSLContext: public ISSLContext
{
public:
	AndroidSSLContext ();

	// ISSLContext
	void setIOHandler (ISSLContextIOHandler* ioHandler) override;
	void setPeerName (CStringPtr peerName) override;
	SSLResult handshake () override;
	SSLResult close () override;
	SSLResult write (const void* buffer, int size, int& bytesWritten) override;
	SSLResult read (void* buffer, int size, int& bytesRead) override;

	int writeEncrypted (jbyteArray data, int size);
	int readEncrypted (jbyteArray data, int start, int size);

protected:
	ISSLContextIOHandler* ioHandler;
	CStringBuffer<128> peerName;
	Java::JniObject sslChannel;
	IO::Buffer encryptedOutput;
	IO::Buffer encryptedInput;

	Java::JniByteArray javaPlainInput;
	Java::JniByteArray javaPlainOutput;
	int plainInputRemaining;
	int plainInputStart;

	enum { kJavaBufferSize = 1024 };

	jobject getSSLChannel ();
	struct LogBuffer;
};

typedef AndroidSSLContext SSLContext;

} // namespace Platform
} // namespace Core

#endif // _coresslcontext_android_h
