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
// Filename    : ccl/platform/win/gui/webbrowserview2.win.cpp
// Description : Win32 WebView2 (Microsoft Edge) Integration
//
//************************************************************************************************

/*
	Get started with WebView2 in Win32 apps
	https://docs.microsoft.com/en-us/microsoft-edge/webview2/get-started/win32

	CreateCoreWebView2Environment
	https://docs.microsoft.com/en-us/microsoft-edge/webview2/reference/win32/webview2-idl?view=webview2-1.0.992.28#createcorewebview2environment

*/

#define DEBUG_LOG 1

#include "ccl/gui/system/webbrowserview.h"
#include "ccl/gui/windows/nativewindow.h"
#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/platform/win/gui/win32graphics.h"

#include "ccl/base/asyncoperation.h"
#include "ccl/base/singleton.h"
#include "ccl/base/storage/configuration.h"

#include "ccl/platform/win/system/cclcom.h"

#include "WebView2.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// AsyncCallbackHandler
//************************************************************************************************

template <typename CompletionHandlerInterface, typename ResultInterface>
class AsyncCallbackHandler: public AsyncOperation,
							public CompletionHandlerInterface
{
public:	
	~AsyncCallbackHandler ()
	{
		CCL_PRINTLN ("AsyncCallbackHandler dtor")
	}

	void destroy ()
	{
		while(release () > 0)
			;
	}

	DELEGATE_COM_IUNKNOWN
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override
	{
		QUERY_COM_INTERFACE (CompletionHandlerInterface)
		return AsyncOperation::queryInterface (iid, ptr);
	}

	// CompletionHandlerInterface
	STDMETHODIMP Invoke (HRESULT errorCode, ResultInterface* result) override
	{		
		setResult (Variant ().takeShared (reinterpret_cast<CCL::IUnknown*> (result)));
		setState (SUCCEEDED (errorCode) ? kCompleted : kFailed);
		release ();
		return S_OK;
	}
};

//************************************************************************************************
// WebView2Factory
//************************************************************************************************

class WebView2Factory: public Object,
					   public Singleton<WebView2Factory>
{
public:
	WebView2Factory ();
	~WebView2Factory ();

	bool isAvailable () const { return environment.isValid (); }

	ITypedAsyncOperation<ICoreWebView2Controller>* createCoreWebView2Controller (HWND parentWindow);

protected:
	ComPtr<ICoreWebView2Environment> environment;

	void construct ();
};

//************************************************************************************************
// WebView2Control
//************************************************************************************************

class WebView2Control: public NativeWebControl
{
public:
	WebView2Control (WebBrowserView& owner);

	// NativeWebControl
	void attachView () override;
	void detachView () override;
	void updateSize () override;
	tresult CCL_API navigate (UrlRef url) override;

protected:
	ComPtr<ICoreWebView2Controller> controller;
	ComPtr<ICoreWebView2> webView;
	bool attached;
	Url pendingUrl;
};

} // namespae Win32
} // namespace CCL

using namespace CCL;
using namespace Win32;

static Configuration::BoolValue webView2Enabled ("CCL.Win32.WebView2", "Enabled", false);
NativeWebControl* CreateLegacyIEWebControl (WebBrowserView& owner); // webbrowserview.win.cpp

//************************************************************************************************
// NativeWebControl
//************************************************************************************************

bool NativeWebControl::isAvailable ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeWebControl* NativeWebControl::createInstance (WebBrowserView& owner)
{
	if(webView2Enabled)
	{
		if(WebView2Factory::instance ().isAvailable ())
			return NEW WebView2Control (owner);
	}
	return CreateLegacyIEWebControl (owner);
}

//************************************************************************************************
// WebView2Control
//************************************************************************************************

WebView2Control::WebView2Control (WebBrowserView& owner)
: NativeWebControl (owner),
  attached (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebView2Control::attachView ()
{
	attached = true;

	Window* ownerWindow = owner.getWindow ();
	ASSERT (ownerWindow != nullptr)
	HWND hwnd = (HWND)ownerWindow->getSystemWindow ();

	// This call can take a while and is completed some time later in the main event loop
	SharedPtr<WebView2Control> keeper (this);
	Promise (WebView2Factory::instance ().createCoreWebView2Controller (hwnd)).then ([this, keeper] (IAsyncOperation& op)
	{			
		controller.fromUnknown (op.getResult ());
		HRESULT hr = controller->get_CoreWebView2 (webView);
		ASSERT (SUCCEEDED (hr))

		updateSize ();

		if(!pendingUrl.isEmpty ())
		{
			navigate (pendingUrl);
			pendingUrl = Url::kEmpty;
		}
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebView2Control::detachView ()
{
	attached = false;

	webView.release ();
	if(controller)
	{
		HRESULT hr = controller->Close ();
		ASSERT (SUCCEEDED (hr))
		controller.release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebView2Control::updateSize ()
{
	if(controller && attached)
	{
		Rect size (getSizeInWindow ());
		float scaleFactor = owner.getWindow ()->getContentScaleFactor ();
		DpiScale::toPixelRect (size, scaleFactor);
		
		RECT r = {0};
		GdiInterop::toSystemRect (r, size);
		HRESULT hr = controller->put_Bounds (r);
		ASSERT (SUCCEEDED (hr))
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebView2Control::navigate (UrlRef url)
{
	if(webView)
	{
		UrlFullString urlString (url, true);
		StringChars urlChars (urlString);

		HRESULT hr = webView->Navigate (urlChars);
		return static_cast<tresult> (hr);
	}
	else
	{
		// construction still in progress
		pendingUrl = url;
		return kResultOk;
	}	
}

//************************************************************************************************
// WebView2Factory
//************************************************************************************************

DEFINE_SINGLETON (WebView2Factory)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebView2Factory::WebView2Factory ()
{
	construct ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebView2Factory::~WebView2Factory ()
{
	CCL_PRINTLN ("WebView2Factory dtor")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebView2Factory::construct ()
{
	// Note: Looks like the completion handler is always called from within the create function
	auto handler = NEW AsyncCallbackHandler<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler, ICoreWebView2Environment>;
	HRESULT hr = ::CreateCoreWebView2Environment (return_shared (handler));
	if(FAILED (hr))
		handler->destroy ();
	else
	{
		Promise (handler).then ([this] (IAsyncOperation& op)
		{
			environment.fromUnknown (op.getResult ());
		});
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITypedAsyncOperation<ICoreWebView2Controller>* WebView2Factory::createCoreWebView2Controller (HWND parentWindow)
{
	ASSERT (environment)
	if(!environment)
		return nullptr;

	auto handler = NEW AsyncCallbackHandler<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, ICoreWebView2Controller>;
	HRESULT hr = environment->CreateCoreWebView2Controller (parentWindow, return_shared (handler));
	if(FAILED (hr))
	{
		handler->destroy ();
		return nullptr;
	}

	return ITypedAsyncOperation<ICoreWebView2Controller>::cast (handler);
}
