//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/wasm/gui/webbrowserview.wasm.cpp
// Description : WebAssembly Web Browser View
//
//************************************************************************************************

#include "ccl/platform/wasm/gui/window.wasm.h"
#include "ccl/platform/wasm/gui/systemevent.wasm.h"

#include "ccl/gui/system/webbrowserview.h"

#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/guiservices.h"

#include <emscripten/html5.h>

namespace CCL {
namespace Wasm {

//////////////////////////////////////////////////////////////////////////////////////////////////
// JS Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (bool, isCrossOriginIsolated, (),
{
	return self.crossOriginIsolated || false;
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (bool, isSameOrigin, (CStringPtr urlString),
{
	try
	{
		const url = new URL (UTF8ToString (urlString), self.location.href);
		return url.origin === self.location.origin;
	}
	catch(e)
	{
		return false;
	}
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (bool, webViewCreateIFrame, (CStringPtr iframeId, CStringPtr parentId),
{
	const parent = CCL.getFrameElement (UTF8ToString (parentId));
	if(!parent)
		return false;

	const iframe = document.createElement ('iframe');
	const id = UTF8ToString (iframeId);
	iframe.id = id;
	iframe.style.position = 'absolute';
	iframe.style.border = 'none';
	iframe.style.width = '100%';
	iframe.style.height = '100%';
	iframe.style.left = '0';
	iframe.style.top = '0';

	iframe.addEventListener ('load', () =>
	{
		CCL.onJSEvent ('load', id);
	});

	iframe.addEventListener ('error', () =>
	{
		CCL.onJSEvent ('error', id);
	});

	parent.appendChild (iframe);
	return true;
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, webViewRemoveIFrame, (CStringPtr iframeId),
{
	const iframe = document.getElementById (UTF8ToString (iframeId));
	if(iframe && iframe.parentNode)
		iframe.parentNode.removeChild (iframe);
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, webViewNavigate, (CStringPtr iframeId, CStringPtr url),
{
	const iframe = document.getElementById (UTF8ToString (iframeId));
	if(iframe)
		iframe.src = UTF8ToString (url);
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (bool, webViewRefresh, (CStringPtr iframeId),
{
	try
	{
		const iframe = document.getElementById (UTF8ToString (iframeId));
		if(!iframe || !iframe.contentWindow)
			return false;

		iframe.contentWindow.location.reload ();
		return true;
	}
	catch(e)
	{
		return false;
	}
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (bool, webViewGoBack, (CStringPtr iframeId),
{
	try
	{
		const iframe = document.getElementById (UTF8ToString (iframeId));
		if(!iframe || !iframe.contentWindow)
			return false;

		iframe.contentWindow.history.back ();
		return true;
	}
	catch(e)
	{
		return false;
	}
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (bool, webViewGoForward, (CStringPtr iframeId),
{
	try
	{
		const iframe = document.getElementById (UTF8ToString (iframeId));
		if(!iframe || !iframe.contentWindow)
			return false;

		iframe.contentWindow.history.forward ();
		return true;
	}
	catch(e)
	{
		return false;
	}
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (bool, webViewCanGoBack, (CStringPtr iframeId),
{
	try
	{
		const iframe = document.getElementById (UTF8ToString (iframeId));
		if(!iframe || !iframe.contentWindow)
			return false;

		return iframe.contentWindow.history.length > 1;
	}
	catch(e)
	{
		return false;
	}
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, webViewUpdateSize, (CStringPtr iframeId, int x, int y, int width, int height),
{
	const iframe = document.getElementById (UTF8ToString (iframeId));
	if(iframe)
	{
		iframe.style.left = x + 'px';
		iframe.style.top = y + 'px';
		iframe.style.width = width + 'px';
		iframe.style.height = height + 'px';
	}
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, webViewFocus, (CStringPtr iframeId),
{
	const iframe = document.getElementById (UTF8ToString (iframeId));
	if(iframe)
		iframe.focus ();
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (bool, webViewGetUrl, (CStringPtr iframeId, char* buffer, int bufferSize),
{
	try
	{
		const iframe = document.getElementById (UTF8ToString (iframeId));
		if(!iframe || !iframe.contentWindow)
			return false;

		const url = iframe.contentWindow.location.href;
		stringToUTF8 (url, buffer, bufferSize);
		return true;
	}
	catch(e)
	{
		return false;
	}
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (bool, webViewGetTitle, (CStringPtr iframeId, char* buffer, int bufferSize),
{
	try
	{
		const iframe = document.getElementById (UTF8ToString (iframeId));
		if(!iframe || !iframe.contentWindow)
			return false;

		const title = iframe.contentWindow.document.title || "";
		stringToUTF8 (title, buffer, bufferSize);
		return true;
	}
	catch(e)
	{
		return false;
	}
});

} // namespace Wasm

//************************************************************************************************
// WasmWebControl
//************************************************************************************************

class WasmWebControl: public NativeWebControl,
					  public SystemEventHandler
{
public:
	WasmWebControl (WebBrowserView& owner);
	~WasmWebControl ();

	PROPERTY_MUTABLE_CSTRING (iframeId, IFrameId)

	// NativeWebControl
	void attachView () override;
	void detachView () override;
	void takeFocus () override;
	void updateSize () override;
	tresult CCL_API navigate (UrlRef url) override;
	tresult CCL_API refresh () override;
	tresult CCL_API goBack () override;
	tresult CCL_API goForward () override;

	// SystemEventHandler
	EventResult handleEvent (SystemEvent& e) override;

protected:
	void updateCurrentPage ();
	void updateNavigationState ();
	void onLoad ();
	void onError ();
};

} // namespace CCL

using namespace CCL;
using namespace Wasm;

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
	return NEW WasmWebControl (owner);
}

//************************************************************************************************
// WasmWebControl
//************************************************************************************************

WasmWebControl::WasmWebControl (WebBrowserView& owner)
: NativeWebControl (owner)
{
	UID uid;
	uid.generate ();
	MutableCString uidString;
	uid.toCString (uidString, UID::kCompact);
	iframeId = "webview";
	iframeId.append (uidString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmWebControl::~WasmWebControl ()
{
	detachView ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWebControl::attachView ()
{
	Window* ownerWindow = owner.getWindow ();
	if(!ownerWindow)
		return;

	WasmWindow* wasmWindow = WasmWindow::cast (ownerWindow);
	webViewCreateIFrame (iframeId, wasmWindow->getID ());
	SystemEventDelegate::instance ().registerEventHandler (iframeId, *this);
	updateSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWebControl::detachView ()
{
	SystemEventDelegate::instance ().unregisterEventHandler (*this);
	webViewRemoveIFrame (iframeId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWebControl::takeFocus ()
{
	webViewFocus (iframeId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWebControl::updateSize ()
{
	Rect size (getSizeInWindow ());
	webViewUpdateSize (iframeId, size.left, size.top, size.getWidth (), size.getHeight ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWebControl::updateCurrentPage ()
{
	char buffer[CORE_STRING_STACK_MAX];

	if(webViewGetUrl (iframeId, buffer, sizeof(buffer)))
		currentUrl.setUrl (String (Text::kUTF8, buffer));
	else
		currentUrl = Url::kEmpty;

	if(webViewGetTitle (iframeId, buffer, sizeof(buffer)))
		currentTitle = String (Text::kUTF8, buffer);
	else
		currentTitle.empty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWebControl::updateNavigationState ()
{
	bool canBack = webViewCanGoBack (iframeId);
	if(flagCanBack () != canBack)
	{
		flagCanBack (canBack);
		deferChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WasmWebControl::navigate (UrlRef url)
{
	UrlFullString urlString (url, true);
	MutableCString urlCString (urlString, Text::kUTF8);

	bool restrictToLocal = getOptions ().isCustomStyle (Styles::kWebBrowserViewBehaviorRestrictToLocal) || isCrossOriginIsolated ();
	if(restrictToLocal)
	{
		bool accepted = url.getProtocol ().isEmpty () || url.getProtocol ().compare (CCLSTR ("file"), false) == 0 || isSameOrigin (urlCString.str ());
		if(accepted == false)
		{
			System::GetSystemShell ().openUrl (url);
			return kResultFailed;
		}
	}

	webViewNavigate (iframeId, urlCString.str ());

	flagCanForward (false);
	deferChanged ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WasmWebControl::refresh ()
{
	return webViewRefresh (iframeId) ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WasmWebControl::goBack ()
{
	if(!flagCanBack ())
		return kResultFailed;

	bool succeeded = webViewGoBack (iframeId);
	if(succeeded)
		deferChanged ();
	return succeeded ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WasmWebControl::goForward ()
{
	if(!flagCanForward ())
		return kResultFailed;

	bool succeeded = webViewGoForward (iframeId);
	if(succeeded)
		deferChanged ();
	return succeeded ? kResultOk : kResultFailed;
}


//////////////////////////////////////////////////////////////////////////////////////////////////

EventResult WasmWebControl::handleEvent (SystemEvent& e)
{
	if(e.eventId == "load")
		onLoad ();
	else if(e.eventId == "error")
		onError ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWebControl::onLoad ()
{
	updateCurrentPage ();
	updateNavigationState ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWebControl::onError ()
{
	currentUrl = Url::kEmpty;
	currentTitle.empty ();
	deferChanged ();
}
