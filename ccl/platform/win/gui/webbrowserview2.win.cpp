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
#include "ccl/base/message.h"
#include "ccl/base/singleton.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/configuration.h"

#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

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
// WebView2EventHandler
/** Dispatches one of the persistent ICoreWebView2 event handler interfaces (NavigationStarting,
	NavigationCompleted, HistoryChanged, NewWindowRequested, ContextMenuRequested, ...) to a
	receiver method - all of them share the same (ICoreWebView2* sender, TArg* args) shape. */
//************************************************************************************************

template <typename THandler, typename TArg, typename TReceiver>
class WebView2EventHandler: public Object,
							public THandler
{
public:
	typedef void (TReceiver::*Callback) (TArg*);

	WebView2EventHandler (TReceiver& receiver, Callback callback)
	: receiver (receiver),
	  callback (callback)
	{}

	DELEGATE_COM_IUNKNOWN
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override
	{
		QUERY_COM_INTERFACE (THandler)
		return Object::queryInterface (iid, ptr);
	}

	// THandler
	STDMETHODIMP Invoke (ICoreWebView2* sender, TArg* arg) override
	{
		(receiver.*callback) (arg);
		return S_OK;
	}

protected:
	TReceiver& receiver;
	Callback callback;
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
	void takeFocus () override;
	void updateSize () override;
	void copyText () override;
	tresult CCL_API navigate (UrlRef url) override;
	tresult CCL_API refresh () override;
	tresult CCL_API goBack () override;
	tresult CCL_API goForward () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	ComPtr<ICoreWebView2Controller> controller;
	ComPtr<ICoreWebView2> webView;
	bool attached;
	Url pendingUrl;

	EventRegistrationToken navigationStartingToken;
	EventRegistrationToken navigationCompletedToken;
	EventRegistrationToken historyChangedToken;
	EventRegistrationToken newWindowRequestedToken;
	EventRegistrationToken contextMenuRequestedToken;

	// deferred context menu display, see onContextMenuRequested()
	ComPtr<ICoreWebView2Deferral> contextMenuDeferral;

	void popupContextMenu (PointRef screenPosInPixel, bool textSelected);
	void updateCurrentPage ();
	void onNavigationStarting (ICoreWebView2NavigationStartingEventArgs* args);
	void onNavigationCompleted (ICoreWebView2NavigationCompletedEventArgs* args);
	void onHistoryChanged (::IUnknown* args);
	void onNewWindowRequested (ICoreWebView2NewWindowRequestedEventArgs* args);
	void onContextMenuRequested (ICoreWebView2ContextMenuRequestedEventArgs* args);
};

} // namespae Win32
} // namespace CCL

using namespace CCL;
using namespace Win32;

NativeWebControl* CreateLegacyIEWebControl (WebBrowserView& owner); // webbrowserview.win.cpp

//////////////////////////////////////////////////////////////////////////////////////////////////

static Configuration::BoolValue webView2Enabled ("CCL.Win32.WebView2", "Enabled", true);
bool IsWebView2EnabledAndInstalled ()
{
	if(!webView2Enabled)
		return false;

	static int webView2Installed = -1;
	if(webView2Installed == -1)
	{
		LPWSTR versionInfo = nullptr;
		HRESULT hr = ::GetAvailableCoreWebView2BrowserVersionString (nullptr, &versionInfo);
		ComDeleter<wchar_t> versionInfoDeleter (versionInfo);
		webView2Installed = SUCCEEDED (hr) && versionInfo != nullptr ? 1 : 0;
	}
	return webView2Installed == 1;
}

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
	if(IsWebView2EnabledAndInstalled ())
		return NEW WebView2Control (owner);
	else
		return CreateLegacyIEWebControl (owner);
}

//************************************************************************************************
// WebView2Control
//************************************************************************************************

WebView2Control::WebView2Control (WebBrowserView& owner)
: NativeWebControl (owner),
  attached (false),
  navigationStartingToken {},
  navigationCompletedToken {},
  historyChangedToken {},
  newWindowRequestedToken {},
  contextMenuRequestedToken {}
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
		ASSERT (controller.isValid ())
		if(!controller.isValid ())
			return;

		HRESULT hr = controller->get_CoreWebView2 (webView);
		ASSERT (SUCCEEDED (hr))
		if(FAILED (hr))
			return;

		ComPtr<ICoreWebView2Settings> settings;
		if(SUCCEEDED (webView->get_Settings (&settings)))
			settings->put_AreDevToolsEnabled (FALSE);

		auto* navigationStartingHandler = NEW WebView2EventHandler<ICoreWebView2NavigationStartingEventHandler, ICoreWebView2NavigationStartingEventArgs, WebView2Control> (*this, &WebView2Control::onNavigationStarting);
		webView->add_NavigationStarting (navigationStartingHandler, &navigationStartingToken);
		navigationStartingHandler->release ();

		auto* navigationCompletedHandler = NEW WebView2EventHandler<ICoreWebView2NavigationCompletedEventHandler, ICoreWebView2NavigationCompletedEventArgs, WebView2Control> (*this, &WebView2Control::onNavigationCompleted);
		webView->add_NavigationCompleted (navigationCompletedHandler, &navigationCompletedToken);
		navigationCompletedHandler->release ();

		auto* historyChangedHandler = NEW WebView2EventHandler<ICoreWebView2HistoryChangedEventHandler, ::IUnknown, WebView2Control> (*this, &WebView2Control::onHistoryChanged);
		webView->add_HistoryChanged (historyChangedHandler, &historyChangedToken);
		historyChangedHandler->release ();

		auto* newWindowRequestedHandler = NEW WebView2EventHandler<ICoreWebView2NewWindowRequestedEventHandler, ICoreWebView2NewWindowRequestedEventArgs, WebView2Control> (*this, &WebView2Control::onNewWindowRequested);
		webView->add_NewWindowRequested (newWindowRequestedHandler, &newWindowRequestedToken);
		newWindowRequestedHandler->release ();

		// context menu forwarding needs a Runtime version new enough to expose ICoreWebView2_11;
		// fall back to the built-in Edge menu on older Runtimes rather than losing the menu entirely.
		// Note: leave AreDefaultContextMenusEnabled at its default TRUE - if set to FALSE, WebView2
		// never raises ContextMenuRequested at all; onContextMenuRequested() suppresses the native
		// menu per-invocation via args->put_Handled(TRUE) instead.
		ComPtr<ICoreWebView2_11> webView11;
		webView.as (webView11);
		if(webView11)
		{
			auto* contextMenuRequestedHandler = NEW WebView2EventHandler<ICoreWebView2ContextMenuRequestedEventHandler, ICoreWebView2ContextMenuRequestedEventArgs, WebView2Control> (*this, &WebView2Control::onContextMenuRequested);
			webView11->add_ContextMenuRequested (contextMenuRequestedHandler, &contextMenuRequestedToken);
			contextMenuRequestedHandler->release ();
		}

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

	if(webView)
	{
		webView->remove_NavigationStarting (navigationStartingToken);
		webView->remove_NavigationCompleted (navigationCompletedToken);
		webView->remove_HistoryChanged (historyChangedToken);
		webView->remove_NewWindowRequested (newWindowRequestedToken);

		ComPtr<ICoreWebView2_11> webView11;
		webView.as (webView11);
		if(webView11)
			webView11->remove_ContextMenuRequested (contextMenuRequestedToken);
	}

	webView.release ();
	if(controller)
	{
		HRESULT hr = controller->Close ();
		ASSERT (SUCCEEDED (hr))
		controller.release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebView2Control::takeFocus ()
{
	if(controller)
		controller->MoveFocus (COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebView2Control::copyText ()
{
	// Synthesizes a Ctrl+C keystroke through the OS input queue, after moving real keyboard focus
	// onto the WebView2.
	if(controller)
	{
		controller->MoveFocus (COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);

		INPUT inputs[4] = {};

		inputs[0].type = INPUT_KEYBOARD;
		inputs[0].ki.wVk = VK_CONTROL;

		inputs[1].type = INPUT_KEYBOARD;
		inputs[1].ki.wVk = WORD('C');

		inputs[2].type = INPUT_KEYBOARD;
		inputs[2].ki.wVk = WORD('C');
		inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

		inputs[3].type = INPUT_KEYBOARD;
		inputs[3].ki.wVk = VK_CONTROL;
		inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

		::SendInput (4, inputs, sizeof(INPUT));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebView2Control::popupContextMenu (PointRef screenPosInPixel, bool _textSelected)
{
	Window* window = owner.getWindow ();
	ASSERT (window != nullptr)
	if(window == nullptr)
		return;

	ScopedVar<bool> scope (textSelected, _textSelected);
	Point p (screenPosInPixel);
	Win32Window::cast (window)->screenPixelToClientCoord (p);
	window->popupContextMenu (p);

	takeFocus (); // restore focus
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebView2Control::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "contextMenuDeferred")
	{
		Point screenPosInPixel (msg[0].asInt (), msg[1].asInt ());
		bool hasSelection = msg[2].asBool ();

		ComPtr<ICoreWebView2Deferral> deferral (contextMenuDeferral);
		contextMenuDeferral.release ();

		popupContextMenu (screenPosInPixel, hasSelection);

		if(deferral)
			deferral->Complete ();
	}
	else
		Object::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebView2Control::updateSize ()
{
	if(controller && attached)
	{
		Rect size (getSizeInWindow ());
		float scaleFactor = owner.getWindow ()->getContentScaleFactor ();
		DpiScale::toPixelRect (size, scaleFactor);
		
		RECT r = {};
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

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebView2Control::refresh ()
{
	if(webView)
		return static_cast<tresult> (webView->Reload ());
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebView2Control::goBack ()
{
	if(webView)
		return static_cast<tresult> (webView->GoBack ());
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebView2Control::goForward ()
{
	if(webView)
		return static_cast<tresult> (webView->GoForward ());
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebView2Control::updateCurrentPage ()
{
	if(!webView)
		return;

	LPWSTR sourceChars = nullptr;
	webView->get_Source (&sourceChars);
	ComDeleter<wchar_t> sourceDeleter (sourceChars);

	currentUrl.setUrl (String (sourceChars));

	LPWSTR titleChars = nullptr;
	webView->get_DocumentTitle (&titleChars);
	ComDeleter<wchar_t> titleDeleter (titleChars);

	currentTitle = String (titleChars);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebView2Control::onNavigationStarting (ICoreWebView2NavigationStartingEventArgs* args)
{
	if(getOptions ().isCustomStyle (Styles::kWebBrowserViewBehaviorRestrictToLocal) == false)
		return;

	// restrict to local pages, open externally otherwise

	LPWSTR uriChars = nullptr;
	args->get_Uri (&uriChars);
	ComDeleter<wchar_t> uriDeleter (uriChars);
	String urlString (uriChars);

	Url url (urlString);

	bool accepted = url.getProtocol ().isEmpty () || url.getProtocol ().compare (CCLSTR ("file"), false) == Text::kEqual;
	if(accepted == false)
	{
		args->put_Cancel (TRUE);
		System::GetSystemShell ().openUrl (url);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebView2Control::onNavigationCompleted (ICoreWebView2NavigationCompletedEventArgs* args)
{
	updateCurrentPage ();
	signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebView2Control::onHistoryChanged (::IUnknown* args)
{
	if(!webView)
		return;

	BOOL canBack = FALSE;
	BOOL canForward = FALSE;
	webView->get_CanGoBack (&canBack);
	webView->get_CanGoForward (&canForward);

	flagCanBack (canBack != FALSE);
	flagCanForward (canForward != FALSE);

	signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebView2Control::onNewWindowRequested (ICoreWebView2NewWindowRequestedEventArgs* args)
{
	args->put_Handled (TRUE); // never let WebView2 open its own popup window

	LPWSTR uriChars = nullptr;
	args->get_Uri (&uriChars);
	ComDeleter<wchar_t> uriDeleter (uriChars);

	System::GetSystemShell ().openUrl (Url (String (uriChars)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebView2Control::onContextMenuRequested (ICoreWebView2ContextMenuRequestedEventArgs* args)
{
	args->put_Handled (TRUE); // suppress the built-in Edge menu, CCL shows its own instead

	BOOL hasSelection = FALSE;
	ComPtr<ICoreWebView2ContextMenuTarget> target;
	args->get_ContextMenuTarget (&target);
	if(target)
		target->get_HasSelection (&hasSelection);

	POINT location = {};
	args->get_Location (&location); // logical pixels (DIPs), relative to the WebView2 bounds

	double rasterizationScale = 1.;
	ComPtr<ICoreWebView2Controller3> controller3;
	controller.as (controller3);
	if(controller3)
		controller3->get_RasterizationScale (&rasterizationScale);

	RECT bounds = {};
	if(controller)
		controller->get_Bounds (&bounds); // physical pixels, see updateSize()

	POINT screenPoint = {bounds.left + int(location.x * rasterizationScale), bounds.top + int(location.y * rasterizationScale)};
	::ClientToScreen ((HWND)owner.getWindow ()->getSystemWindow (), &screenPoint);

	// Context menu runs a modal message loop and calling it synchronously from inside a WebView2
	// event handler risks reentering WebView2 internals (it warns/asserts about this at runtime).
	// Defer the actual menu display via GetDeferral() until back in the normal message loop.
	if(contextMenuDeferral)
	{
		contextMenuDeferral->Complete (); // don't leave a previous request dangling
		contextMenuDeferral.release ();
	}
	args->GetDeferral (&contextMenuDeferral);

	(NEW Message ("contextMenuDeferred", screenPoint.x, screenPoint.y, hasSelection))->post (this);
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
	Url userDataFolder;
	System::GetSystem ().getLocation (userDataFolder, System::kAppSettingsFolder);
	NativePath userDataPath (userDataFolder);

	// Note: Looks like the completion handler is always called from within the create function
	auto* handler = NEW AsyncCallbackHandler<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler, ICoreWebView2Environment>;
	HRESULT hr = ::CreateCoreWebView2EnvironmentWithOptions (nullptr, userDataPath, nullptr, return_shared (handler));
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

	auto* handler = NEW AsyncCallbackHandler<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, ICoreWebView2Controller>;
	HRESULT hr = environment->CreateCoreWebView2Controller (parentWindow, return_shared (handler));
	if(FAILED (hr))
	{
		handler->destroy ();
		return nullptr;
	}

	return ITypedAsyncOperation<ICoreWebView2Controller>::cast (handler);
}
