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
// Filename    : ccl/platform/android/system/contentprotocol.cpp
// Description : Android content protocol handler
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/android/system/system.android.h"
#include "ccl/platform/android/system/nativefilesystem.android.h"

#include "ccl/platform/android/interfaces/iframeworkactivity.h"

#include "ccl/base/storage/protocolhandler.h"

#include "ccl/public/systemservices.h"

#include <unistd.h>

namespace CCL {
namespace Android {

//************************************************************************************************
// android.os.ParcelFileDescriptor
//************************************************************************************************

DECLARE_JNI_CLASS (ParcelFileDescriptor, "android/os/ParcelFileDescriptor")
	DECLARE_JNI_METHOD (int, getFd)
	DECLARE_JNI_METHOD (void, close)
END_DECLARE_JNI_CLASS (ParcelFileDescriptor)

//************************************************************************************************
// android.os.ParcelFileDescriptor
//************************************************************************************************

DEFINE_JNI_CLASS (ParcelFileDescriptor)
	DEFINE_JNI_METHOD (getFd, "()I")
	DEFINE_JNI_METHOD (close, "()V")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL

using namespace CCL;
using namespace Android;

//************************************************************************************************
// ParcelFileDescriptorWrapper
/** Wraps a ParcelFileDecriptor forwarding stream calls and closes it when done. */
//************************************************************************************************

class ParcelFileDescriptorWrapper: public Unknown,
								   public IStream
{
public:
	ParcelFileDescriptorWrapper (jobject pfd, int mode)
	: pfd (JniAccessor (), pfd)
	{
		int fd = ParcelFileDescriptor.getFd (pfd);
		if(mode & IStream::kCreate)
			ftruncate (fd, 0);

		stream = AndroidNativeFileSystem::getInstance ().createStreamFromHandle (fd);
	}

	~ParcelFileDescriptorWrapper ()
	{
		ParcelFileDescriptor.close (pfd);
	}

	// IStream
	int CCL_API read (void* buffer, int size) override { return stream->read (buffer, size); }
	int CCL_API write (const void* buffer, int size) override { return stream->write (buffer, size); }
	int64 CCL_API tell () override { return stream->tell (); }
	tbool CCL_API isSeekable () const override { return stream->isSeekable (); }
	int64 CCL_API seek (int64 pos, int mode) override { return stream->seek (pos, mode); }

	CLASS_INTERFACE (IStream, Unknown)

private:
	JniObject pfd;
	AutoPtr<IStream> stream;
};

//************************************************************************************************
// ContentFileSystem
/** File system for Android "content" urls. */
//************************************************************************************************

class ContentFileSystem: public Unknown,
						 public AbstractFileSystem
{
public:
	ContentFileSystem ()
	{}

	// IFileSystem
	IStream* CCL_API openStream (UrlRef url, int mode = IStream::kOpenMode, IUnknown* context = nullptr) override
	{
		ASSERT (url.getProtocol () == CCLSTR ("content"))

		String modeString = AndroidNativeFileSystem::translateMode (mode);

		// get a ParcelFileDescriptor and wrap it
		if(IFrameworkActivity* activity = AndroidSystemInformation::getInstance ().getNativeActivity ())
			if(jobject parcelFileDesriptor = activity->openContentFile (url, modeString))
				return NEW ParcelFileDescriptorWrapper (parcelFileDesriptor, mode);

		return 0;
	}

	tbool CCL_API fileExists (UrlRef url) override
	{
		ASSERT (url.getProtocol () == CCLSTR ("content"))

		IFrameworkActivity* activity = AndroidSystemInformation::getInstance ().getNativeActivity ();
		if(!activity)
			return false;

		bool exists = activity->contentFileExists (url);
		CCL_PRINTF ("fileExists %d: %s\n", exists, MutableCString (UrlFullString (url)).str ())
		return exists;
	}

	tbool CCL_API getFileInfo (FileInfo& info, UrlRef url) override
	{
		IFrameworkActivity* activity = AndroidSystemInformation::getInstance ().getNativeActivity ();
		if(!activity)
			return false;

		return activity->getContentFileInfo (info, url);
	}

	CLASS_INTERFACE (IFileSystem, Unknown)
};

//************************************************************************************************
// ContentProtocol
/** Protocol handler for android "content" urls.
	Such urls are e.g. returned from a file selector, and must be resolved using a content resolver . */
//************************************************************************************************

class ContentProtocol: public Object,
					   public Singleton<ContentProtocol>
{
public:
	class Handler: public ProtocolHandler
	{
	public:
		Handler ()
		: fileSystem (NEW ContentFileSystem)
		{}

		// ProtocolHandler
		StringRef CCL_API getProtocol () const override
		{
			static const String protocol = CCLSTR ("content");
			return protocol;
		}

		IFileSystem* CCL_API getMountPoint (StringRef name) override
		{
			return fileSystem;
		}

	private:
		AutoPtr<ContentFileSystem> fileSystem;
	};

	ContentProtocol ()
	: handler (NEW Handler)
	{
		UnknownPtr<IProtocolHandlerRegistry> registry (&System::GetFileSystem ());
		ASSERT (registry != 0)
		if(registry)
			registry->registerProtocol (handler);
	}

	~ContentProtocol ()
	{
		UnknownPtr<IProtocolHandlerRegistry> registry (&System::GetFileSystem ());
		if(registry)
			registry->unregisterProtocol (handler);
	}

private:
	AutoPtr<Handler> handler;
};

DEFINE_SINGLETON (ContentProtocol)

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT (ContentProtocol)
{
	ContentProtocol::instance ();
	return true;
}
