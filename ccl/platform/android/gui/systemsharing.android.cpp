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
// Filename	: ccl/platform/android/gui/systemsharing.android.cpp
// Description : Platform-specific system sharing handler for Android
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/public/gui/framework/isystemsharing.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/asyncoperation.h"

#include "ccl/platform/android/gui/frameworkactivity.h"

namespace CCL {

//************************************************************************************************
// AndroidSystemSharingHandler
//************************************************************************************************

class AndroidSystemSharingHandler: public Object,
								   public ISystemSharingHandler
{
public:
	DECLARE_CLASS (AndroidSystemSharingHandler, Object)

	PROPERTY_SHARED_AUTO (AsyncOperation, asyncOperation, AsyncOperation)

	void onFinished ();

	static SharedPtr<AndroidSystemSharingHandler> currentInstance;

	// ISystemSharingHandler
	IAsyncOperation* CCL_API shareFile (UrlRef url, IWindow* window = nullptr) override;
	IAsyncOperation* CCL_API shareText (StringRef text, IWindow* window = nullptr) override;

	CLASS_INTERFACE (ISystemSharingHandler, Object)
};

} // namespace CCL

using namespace CCL;
using namespace Android;

//************************************************************************************************
// AndroidSystemSharingHandler
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (AndroidSystemSharingHandler, Object, "SystemSharingHandler")
DEFINE_CLASS_UID (AndroidSystemSharingHandler, 0x3421790e, 0x33c8, 0x430a, 0xa4, 0x98, 0x97, 0x1f, 0x0d, 0xb2, 0x56, 0x22) // ClassID::SystemSharingHandler

//////////////////////////////////////////////////////////////////////////////////////////////////

SharedPtr<AndroidSystemSharingHandler> AndroidSystemSharingHandler::currentInstance;

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API AndroidSystemSharingHandler::shareFile (UrlRef url, IWindow* window)
{
	ASSERT (!currentInstance)
	currentInstance = this;

	FrameworkActivity* activity = FrameworkActivity::getCurrentActivity ();

	retain (); // release in onFinished

	// on create, append extension to suggested filename (Android doesn't do it automatically)
	String urlString;
	url.getUrl (urlString);

	JniCCLString uriString (urlString);
	JniCCLString mimeTypeString ("application/octet-stream");
	if(!FrameworkActivityClass.runFileSharing (*activity, uriString, mimeTypeString))
		return AsyncOperation::createFailed ();

	asyncOperation.share (NEW AsyncOperation);
	asyncOperation->setState (AsyncOperation::kStarted);
	return asyncOperation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API AndroidSystemSharingHandler::shareText (StringRef text, IWindow* window)
{
	ASSERT (!currentInstance)
	currentInstance = this;

	FrameworkActivity* activity = FrameworkActivity::getCurrentActivity ();

	retain (); // release in onFinished

	JniCCLString textString (text);
	if(!FrameworkActivityClass.runTextSharing (*activity, textString))
		return AsyncOperation::createFailed ();

	asyncOperation.share (NEW AsyncOperation);
	asyncOperation->setState (AsyncOperation::kStarted);
	return asyncOperation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidSystemSharingHandler::onFinished ()
{
	ASSERT (currentInstance == this)
	currentInstance = nullptr;

	SharedPtr<AsyncOperation> asyncOperation (this->asyncOperation);
	this->asyncOperation = nullptr;

	// the client code (from other module) that created us with ccl_new, still owns a refCount and must cleanup using ccl_release (must by the final call)
	// if the client code has already released us before (although that wouldn't make much sense), this would be destroyed here, so we don't access 'this' anymore
	release ();

	if(asyncOperation)
		asyncOperation->setState (AsyncOperation::kCompleted);
}

//************************************************************************************************
// File exporter Java native methods
//************************************************************************************************

DECLARE_JNI_CLASS_METHOD_CCLGUI_NO_ARGS (void, FrameworkActivity, onSharingFinished)
{
	ASSERT (AndroidSystemSharingHandler::currentInstance)
	if(AndroidSystemSharingHandler* handler = AndroidSystemSharingHandler::currentInstance)
		handler->onFinished ();
}
