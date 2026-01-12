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
// Filename    : ccl/platform/android/gui/fileselector.android.cpp
// Description : platform-specific file selector implementation
//
//************************************************************************************************

#include "ccl/gui/dialogs/fileselector.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/asyncoperation.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/istringdict.h"

#include "ccl/platform/android/gui/frameworkactivity.h"

namespace CCL {

//************************************************************************************************
// AndroidFileSelector
//************************************************************************************************

class AndroidFileSelector: public NativeFileSelector
{
public:
	DECLARE_CLASS (AndroidFileSelector, NativeFileSelector)

	PROPERTY_SHARED_AUTO (AsyncOperation, asyncOperation, AsyncOperation)

	void onResult (StringRef uriString, StringRef displayName);

	// NativeFileSelector
	bool runPlatformSelector (int type, StringRef title, int filterIndex, IWindow* window) override;
	IAsyncOperation* runPlatformSelectorAsync (int type, StringRef title, int filterIndex, IWindow* window) override;
	int CCL_API getSaveBehavior () const override;

	static SharedPtr<AndroidFileSelector> currentInstance;
};

//************************************************************************************************
// AndroidFolderSelector
//************************************************************************************************

class AndroidFolderSelector: public NativeFolderSelector
{
public:
	DECLARE_CLASS (AndroidFolderSelector, NativeFolderSelector)

	PROPERTY_SHARED_AUTO (AsyncOperation, asyncOperation, AsyncOperation)

	void onResult (StringRef uriString, StringRef displayName);

	// NativeFolderSelector
	bool runPlatformSelector (StringRef title, IWindow* window) override;
	IAsyncOperation* runPlatformSelectorAsync (StringRef title, IWindow* window) override;

	static SharedPtr<AndroidFolderSelector> currentInstance;
};

} // namespace CCL

using namespace CCL;
using namespace Android;

//************************************************************************************************
// AndroidFileSelector
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (AndroidFileSelector, NativeFileSelector, "FileSelector")
DEFINE_CLASS_UID (AndroidFileSelector, 0xacfd316a, 0x371d, 0x4ba2, 0x9b, 0x7e, 0x45, 0xce, 0xc8, 0x7a, 0x2c, 0xbf) // ClassID::FileSelector

//////////////////////////////////////////////////////////////////////////////////////////////////

SharedPtr<AndroidFileSelector> AndroidFileSelector::currentInstance;

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API AndroidFileSelector::getSaveBehavior () const
{
	return kSaveCreatesFile;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidFileSelector::runPlatformSelector (int type, StringRef title, int filterIndex, IWindow* window)
{
	CCL_WARN ("synchronous FileSelector not supported!", 0)
	Promise p (runPlatformSelectorAsync (type, title, filterIndex, window));
	ASSERT (0)
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* AndroidFileSelector::runPlatformSelectorAsync (int type, StringRef title, int filterIndex, IWindow* window)
{
	ASSERT (!currentInstance)

	ASSERT (type != kOpenMultipleFiles)
	bool create = type == kSaveFile;

	currentInstance = this;

	FrameworkActivity* activity = FrameworkActivity::getCurrentActivity ();

	auto isMimeTypeSupported = [&] (const FileType& fileType)
	{
		JniCCLString mimeTypeString (fileType.getMimeType ());
		return FrameworkActivityClass.isMimeTypeSupported (*activity, mimeTypeString);
	};

	String mimeType;
	String extension;
	if(!filters.isEmpty ())
	{
		if(filters.count () == 1)
		{
			Boxed::FileType* fileType = ccl_cast<Boxed::FileType> (filters.first ());
			if(fileType)
			{
				if(isMimeTypeSupported (*fileType))
					mimeType = fileType->getMimeType ();

				if(create)
					extension = fileType->getExtension ();
			}
		}
		else
		{
			auto getMainMimeType = [] (const FileType& fileType)
			{
				String type (fileType.getMimeType ());
				int index = type.index ("/");
				if(index >= 0)
					type.truncate (index);
				return type;
			};

			// we can only pass one mime type: for multiple filetypes, check if they have a common "main" type
			for(auto fileType : iterate_as<Boxed::FileType> (filters))
			{
				if(!isMimeTypeSupported (*fileType))
				{
					mimeType.empty (); // at least one type is not supported: fall back to "no filter"
					break;
				}

				if(mimeType.isEmpty ())
					mimeType = getMainMimeType (*fileType);
				else if(mimeType != getMainMimeType (*fileType))
				{
					mimeType.empty (); // different main type: fall back to "no filter"
					break;
				}
			}
			if(!mimeType.isEmpty ())
				mimeType << CCLSTR ("/*");
		}
	}

	retain (); // release in onResult

	// on create, append extension to suggested filename (Android doesn't do it automatically)
	String fileName (getInitialFileName ());
	if(!extension.isEmpty () && !fileName.isEmpty ())
	{
		extension.prepend (".");
		if(!fileName.endsWith (extension))
			fileName << extension;
	}

	JniCCLString mimeTypeString (mimeType);
	JniCCLString initialFileNameString (fileName);
	if(!FrameworkActivityClass.runFileSelector (*activity, create, mimeTypeString, initialFileNameString))
		return AsyncOperation::createFailed ();

	asyncOperation.share (NEW AsyncOperation);
	asyncOperation->setState (AsyncOperation::kStarted);	
	return asyncOperation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidFileSelector::onResult (StringRef uriString, StringRef displayName)
{
	ASSERT (currentInstance == this)
	currentInstance = 0;

	SharedPtr<AsyncOperation> asyncOperation (this->asyncOperation);
	this->asyncOperation = 0;

	bool result = !uriString.isEmpty ();
	if(result)
	{
		Url* url = NEW Url (uriString);
		if(!displayName.isEmpty ())
			url->getParameters ().setEntry (CCLSTR (UrlParameter::kDisplayName), displayName);

		paths.add (url);
	}

	// the client code (from other module) that created us with ccl_new, still owns a refCount and must cleanup using ccl_release (must by the final call)
	// if the client code has already released us before (although that wouldn't make much sense), this would be destroyed here, so we don't access 'this' anymore
	release ();

	if(asyncOperation)
	{
		asyncOperation->setResult (result);
		asyncOperation->setState (AsyncOperation::kCompleted);
	}
}

//************************************************************************************************
// AndroidFolderSelector
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (AndroidFolderSelector, NativeFolderSelector, "FolderSelector")
DEFINE_CLASS_UID (AndroidFolderSelector, 0x898fbf4d, 0x15d, 0x4754, 0x93, 0xa, 0xf1, 0x7a, 0xa7, 0x0, 0x82, 0xfc) // ClassID::FolderSelector

//////////////////////////////////////////////////////////////////////////////////////////////////

SharedPtr<AndroidFolderSelector> AndroidFolderSelector::currentInstance;

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidFolderSelector::runPlatformSelector (StringRef title, IWindow* window)
{
	CCL_WARN ("synchronous FolderSelector not supported!", 0)
	Promise p (runPlatformSelectorAsync (title, window));
	ASSERT (0)
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* AndroidFolderSelector::runPlatformSelectorAsync (StringRef title, IWindow* window)
{
	ASSERT (!currentInstance)
	currentInstance = this;

	FrameworkActivity* activity = FrameworkActivity::getCurrentActivity ();

	retain (); // release in onResult

	// on create, append extension to suggested filename (Android doesn't do it automatically)
	String initialPath;
	getInitialPath ().toDisplayString (initialPath);

	JniCCLString initialPathString (initialPath);
	if(!FrameworkActivityClass.runFolderSelector (*activity, initialPathString))
		return AsyncOperation::createFailed ();

	asyncOperation.share (NEW AsyncOperation);
	asyncOperation->setState (AsyncOperation::kStarted);
	return asyncOperation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidFolderSelector::onResult (StringRef uriString, StringRef displayName)
{
	ASSERT (currentInstance == this)
	currentInstance = 0;

	SharedPtr<AsyncOperation> asyncOperation (this->asyncOperation);
	this->asyncOperation = 0;

	bool result = !uriString.isEmpty ();
	if(result)
	{
		path = NEW Url (uriString);
		if(!displayName.isEmpty ())
			path->getParameters ().setEntry (CCLSTR (UrlParameter::kDisplayName), displayName);
	}

	// the client code (from other module) that created us with ccl_new, still owns a refCount and must cleanup using ccl_release (must by the final call)
	// if the client code has already released us before (although that wouldn't make much sense), this would be destroyed here, so we don't access 'this' anymore
	release ();

	if(asyncOperation)
	{
		asyncOperation->setResult (result);
		asyncOperation->setState (AsyncOperation::kCompleted);
	}
}

//************************************************************************************************
// File/folder selector Java native methods
//************************************************************************************************

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, FrameworkActivity, onFileSelectorResult, jstring _uriString, jstring _displayName)
{
	ASSERT (AndroidFileSelector::currentInstance)
	if(AndroidFileSelector* fileSelector = AndroidFileSelector::currentInstance)
	{
		String uriString;
		fromJavaString (uriString, env, _uriString);

		String displayName;
		fromJavaString (displayName, env, _displayName);

		fileSelector->onResult (uriString, displayName);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, FrameworkActivity, onFolderSelectorResult, jstring _uriString, jstring _displayName)
{
	ASSERT (AndroidFolderSelector::currentInstance)
	if(AndroidFolderSelector* folderSelector = AndroidFolderSelector::currentInstance)
	{
		String uriString;
		fromJavaString (uriString, env, _uriString);

		String displayName;
		fromJavaString (displayName, env, _displayName);

		folderSelector->onResult (uriString, displayName);
	}
}
