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
// Filename    : ccl/platform/win/gui/webbrowserview.win.cpp
// Description : Win32 Web Browser View
//
//************************************************************************************************

/*
	WebBrowser Customization
	http://msdn.microsoft.com/en-us/library/aa770041%28v=VS.85%29.aspx
*/

#define DEBUG_LOG 0

#include "ccl/gui/system/webbrowserview.h"
#include "ccl/gui/windows/nativewindow.h"

#include "ccl/public/system/iexecutable.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/guiservices.h"

#include "ccl/platform/win/gui/activex.h"
#include "ccl/platform/win/gui/dpihelper.h"
#include "ccl/platform/win/gui/screenscaling.h"
#include "ccl/platform/win/system/registry.h"

#include <Exdispid.h>
#include <Mshtmhst.h>
#include <shlwapi.h>

namespace CCL {
namespace Win32 {

//************************************************************************************************
// IEWebBrowserControl
//************************************************************************************************

class IEWebBrowserControl: public NativeWebControl,
						   private ActiveXEmbedder
{
public:
	IEWebBrowserControl (WebBrowserView& owner, ::IWebBrowser2* webBrowser);
	~IEWebBrowserControl ();

	class HostContainer;
	class EventHandler;
	class SecurityManager;

	void updateCurrentPage ();
	void popupContextMenu (PointRef screenPosInPixel, bool textSelected);

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

protected:
	ComPtr<::IWebBrowser2> webBrowser;
	AutoPtr<EventHandler> eventHandler;
	DWORD adviseCookie;
	bool systemScalingActive;

	bool isBusy ();
};

//************************************************************************************************
// IEWebBrowserControl::HostContainer
//************************************************************************************************

class IEWebBrowserControl::HostContainer: public ActiveXContainer,
										  public IDocHostUIHandler,
										  public IDocHostShowUI,
										  public IServiceProvider
{
public:
	HostContainer (IEWebBrowserControl& owner, HWND hwnd, RectRef size);

	// CCL::IUnknown
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;

	// IUnknown
	DELEGATE_COM_IUNKNOWN

	// IDocHostUIHandler
	STDMETHODIMP ShowContextMenu (DWORD dwID, POINT *ppt, ::IUnknown *pcmdtReserved, IDispatch *pdispReserved) override;
	STDMETHODIMP GetHostInfo (DOCHOSTUIINFO *pInfo) override;
	STDMETHODIMP ShowUI (DWORD dwID, IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget, IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc) override;
	STDMETHODIMP HideUI () override;
	STDMETHODIMP UpdateUI () override;
	STDMETHODIMP EnableModeless (BOOL fEnable) override;
	STDMETHODIMP OnDocWindowActivate (BOOL fActivate) override;
	STDMETHODIMP OnFrameWindowActivate (BOOL fActivate) override;
	STDMETHODIMP ResizeBorder (LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow) override;
	STDMETHODIMP TranslateAccelerator (LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID) override;
	STDMETHODIMP GetOptionKeyPath (LPOLESTR *pchKey, DWORD dw) override;
	STDMETHODIMP GetDropTarget (IDropTarget *pDropTarget, IDropTarget **ppDropTarget) override;
	STDMETHODIMP GetExternal (IDispatch **ppDispatch) override;
	STDMETHODIMP TranslateUrl (DWORD dwTranslate, LPWSTR pchURLIn, LPWSTR *ppchURLOut) override;
	STDMETHODIMP FilterDataObject (IDataObject *pDO, IDataObject **ppDORet) override;

	// IDocHostShowUI
	STDMETHODIMP ShowMessage (HWND hwnd, LPOLESTR lpstrText, LPOLESTR lpstrCaption, DWORD dwType, LPOLESTR lpstrHelpFile, DWORD dwHelpContext, LRESULT *plResult) override;
	STDMETHODIMP ShowHelp (HWND hwnd, LPOLESTR pszHelpFile, UINT uCommand, DWORD dwData, POINT ptMouse, IDispatch *pDispatchObjectHit) override;

	// IServiceProvider
	STDMETHODIMP QueryService (REFGUID guidService, REFIID riid, void** ppvObject) override;

protected:
	IEWebBrowserControl& owner;
};

//************************************************************************************************
// IEWebBrowserControl::EventHandler
//************************************************************************************************

class IEWebBrowserControl::EventHandler: public Object,
										 public AbstractIDispatch
{
public:
	EventHandler (IEWebBrowserControl& owner);
	~EventHandler ();

	// CCL::IUnknown
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;

	// IUnknown
	DELEGATE_COM_IUNKNOWN

	// AbstractIDispatch
	HRESULT Invoke (InvokeArgs& args) override;

protected:
	IEWebBrowserControl& owner;
};

//************************************************************************************************
// IEWebBrowserControl::SecurityManager
//************************************************************************************************

class IEWebBrowserControl::SecurityManager: public Object,
					                        public ::IInternetSecurityManager
{
public:
	SecurityManager ();

	// CCL::IUnknown
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;

	// IUnknown
	DELEGATE_COM_IUNKNOWN

	// IInternetSecurityManager
	STDMETHODIMP SetSecuritySite (IInternetSecurityMgrSite *pSite) override;
	STDMETHODIMP GetSecuritySite (IInternetSecurityMgrSite **ppSite) override;
	STDMETHODIMP MapUrlToZone (LPCWSTR pwszUrl, DWORD *pdwZone, DWORD dwFlags) override;
	STDMETHODIMP GetSecurityId (LPCWSTR pwszUrl, BYTE *pbSecurityId, DWORD *pcbSecurityId, DWORD_PTR dwReserved) override;
	STDMETHODIMP ProcessUrlAction (LPCWSTR pwszUrl,DWORD dwAction, BYTE *pPolicy, DWORD cbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved) override;
    STDMETHODIMP QueryCustomPolicy (LPCWSTR pwszUrl, REFGUID guidKey, BYTE **ppPolicy, DWORD *pcbPolicy, BYTE *pContext,DWORD cbContext, DWORD dwReserved) override;
	STDMETHODIMP SetZoneMapping (DWORD dwZone,LPCWSTR lpszPattern, DWORD dwFlags) override;
    STDMETHODIMP GetZoneMappings (DWORD dwZone, IEnumString **ppenumString, DWORD dwFlags) override;
};

} // namespace Win32
} // namespace CCL

using namespace CCL;
using namespace Win32;

//////////////////////////////////////////////////////////////////////////////////////////////////

void InitIEBrowserEmulationVersion () // called by UserInterface::startupPlatform()
{
	static const uint32 kIEVersion = 11000; // Internet Explorer 11
	// see https://msdn.microsoft.com/en-us/library/ee330730%28VS.85%29.aspx#browser_emulation
	// http://en.wikipedia.org/wiki/Internet_Explorer_versions#Windows

	Url exePath;
	System::GetExecutableLoader ().getMainImage ().getPath (exePath);
	String fileName;
	exePath.getName (fileName);

	Registry::Accessor acc (Registry::kKeyCurrentUser, "Software\\Microsoft\\Internet Explorer\\Main\\FeatureControl");
	bool result = acc.writeDWORD (kIEVersion, "FEATURE_BROWSER_EMULATION", fileName);
	ASSERT (result == true)
}

NativeWebControl* CreateLegacyIEWebControl (WebBrowserView& owner) // called as fallback if WebView2 not enabled or available
{
	::IWebBrowser2* webBrowser = nullptr;
	::CoCreateInstance (CLSID_WebBrowser, nullptr, CLSCTX_INPROC, IID_IWebBrowser2, (void**)&webBrowser);
	ASSERT (webBrowser != nullptr)
	if(webBrowser == nullptr)
		return nullptr;

	return NEW IEWebBrowserControl (owner, webBrowser);
}

//************************************************************************************************
// NativeWebControl
//************************************************************************************************

#if 0 // moved to webbrowserview2.win.cpp
NativeWebControl* NativeWebControl::createInstance (WebBrowserView& owner)
{
	return CreateLegacyIEWebControl (owner);
}
#endif

//************************************************************************************************
// IEWebBrowserControl
//************************************************************************************************

IEWebBrowserControl::IEWebBrowserControl (WebBrowserView& owner, ::IWebBrowser2* webBrowser)
: NativeWebControl (owner),
  webBrowser (webBrowser),
  adviseCookie (0),
  systemScalingActive (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IEWebBrowserControl::~IEWebBrowserControl ()
{
	CCL_PRINTLN ("IEWebBrowserControl dtor")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IEWebBrowserControl::attachView ()
{
	webBrowser->put_Silent (VARIANT_TRUE); // do not display script error dialogs

	Window* ownerWindow = owner.getWindow ();
	ASSERT (ownerWindow != nullptr)
	HWND hwnd = (HWND)ownerWindow->getSystemWindow ();

	// determine if system scaling can be used, must be supported by OS and parent window
	this->systemScalingActive = gDpiInfo.canSwitchDpiAwarenessContext () &&
								gDpiInfo.canSwitchDpiHostingBehavior () &&
								ownerWindow->getStyle ().isCustomStyle (Styles::kWindowBehaviorPluginViewHost);

	Rect size (getSizeInWindow ());
	float scaleFactor = systemScalingActive ? gDpiInfo.getSystemDpiFactor () : owner.getWindow ()->getContentScaleFactor ();
	DpiScale::toPixelRect (size, scaleFactor);

	DpiAwarenessScope dpiScope (gDpiInfo, systemScalingActive ? kDpiContextSystemAware : kDpiContextDefault);
	ActiveXEmbedder::construct (NEW HostContainer (*this, hwnd, size), webBrowser);
	ActiveXEmbedder::activate ();

	eventHandler = NEW EventHandler (*this);
	ActiveX::Advise (webBrowser, eventHandler, DIID_DWebBrowserEvents2, &adviseCookie);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IEWebBrowserControl::detachView ()
{
	ActiveX::Unadvise (webBrowser, DIID_DWebBrowserEvents2, adviseCookie);

	ActiveXEmbedder::close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IEWebBrowserControl::takeFocus ()
{
	HWND hwndBrowser = NULL;
	ComPtr<IOleWindow> oleWindow;
	webBrowser.as (oleWindow);
	if(oleWindow)
		oleWindow->GetWindow (&hwndBrowser);

	struct CB
	{
		static BOOL CALLBACK FindIEServer (HWND hwnd, LPARAM lParam)
		{
			HWND* result = reinterpret_cast<HWND*> (lParam);

			WCHAR className[33] = {0};
			::GetClassNameW (hwnd, className, 32);
			if(StrCmpI (className, L"Internet Explorer_Server") == 0)
				*result = hwnd;

			if(*result == nullptr)
				EnumChildWindows (hwnd, FindIEServer, lParam); // recurse

			return (*result == nullptr) ? true : false;
		}
	};

	HWND toFocus = NULL;
	EnumChildWindows (hwndBrowser, CB::FindIEServer, reinterpret_cast<LPARAM> (&toFocus));
	if(toFocus)
		::SetFocus (toFocus);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IEWebBrowserControl::updateSize ()
{
	Rect size (getSizeInWindow ());
	float scaleFactor = systemScalingActive ? gDpiInfo.getSystemDpiFactor () : owner.getWindow ()->getContentScaleFactor ();
	DpiScale::toPixelRect (size, scaleFactor);

	DpiAwarenessScope dpiScope (gDpiInfo, systemScalingActive ? kDpiContextSystemAware : kDpiContextDefault);
	ActiveXEmbedder::resize (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IEWebBrowserControl::updateCurrentPage ()
{
	String urlString;
	BSTR nativeUrl = nullptr;
	webBrowser->get_LocationURL (&nativeUrl);
	if(nativeUrl)
	{
		urlString.appendNativeString (nativeUrl);
		::SysFreeString (nativeUrl);
	}

	String titleString;
	BSTR nativeTitle = nullptr;
	webBrowser->get_LocationName (&nativeTitle);
	if(nativeTitle)
	{
		titleString.appendNativeString (nativeTitle);
		::SysFreeString (nativeTitle);
	}

	this->currentUrl.setUrl (urlString);
	this->currentTitle = titleString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IEWebBrowserControl::popupContextMenu (PointRef screenPosInPixel, bool _textSelected)
{
	Window* window = owner.getWindow ();
	ASSERT (window != nullptr)
	if(window == nullptr)
		return;

	// make sure to reset from system-aware when calling back into the framework
	DpiAwarenessScope dpiScope (gDpiInfo, kDpiContextDefault);
	ScopedVar<bool> scope (textSelected, _textSelected);
	Point p (screenPosInPixel);
	Win32Window::cast (window)->screenPixelToClientCoord (p);
	window->popupContextMenu (p);

	takeFocus (); // restore focus
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IEWebBrowserControl::copyText ()
{
	ComPtr<IDispatch> document;
	webBrowser->get_Document (document);
	ComPtr<IOleCommandTarget> target;
	document.as (target);
	if(target)
		target->Exec (nullptr, OLECMDID_COPY, OLECMDEXECOPT_DONTPROMPTUSER, nullptr, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IEWebBrowserControl::isBusy ()
{
	VARIANT_BOOL v = VARIANT_FALSE;
	webBrowser->get_Busy (&v);
	return v == VARIANT_TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API IEWebBrowserControl::navigate (UrlRef url)
{
	UrlFullString urlString (url, true);
	NativeString<BSTR> bStr (urlString);

	if(isBusy () == true) // cancel previous navigation
		webBrowser->Stop ();

	return static_cast<tresult> (webBrowser->Navigate (bStr, nullptr, nullptr, nullptr, nullptr));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API IEWebBrowserControl::refresh ()
{
	return static_cast<tresult> (webBrowser->Refresh ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API IEWebBrowserControl::goBack ()
{
	return static_cast<tresult> (webBrowser->GoBack ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API IEWebBrowserControl::goForward ()
{
	return static_cast<tresult> (webBrowser->GoForward ());
}

//************************************************************************************************
// IEWebBrowserControl::HostContainer
//************************************************************************************************

IEWebBrowserControl::HostContainer::HostContainer (IEWebBrowserControl& owner, HWND hwnd, RectRef size)
: ActiveXContainer (hwnd, size),
  owner (owner)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API IEWebBrowserControl::HostContainer::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_COM_INTERFACE (IDocHostUIHandler)
	QUERY_COM_INTERFACE (IDocHostShowUI)
	QUERY_COM_INTERFACE (IServiceProvider)
	return ActiveXContainer::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::HostContainer::ShowContextMenu (DWORD dwID, POINT *ppt, ::IUnknown *pcmdtReserved, IDispatch *pdispReserved)
{
	CCL_PRINTLN ("ShowContextMenu")

	Point screenPosInPixel;
	if(ppt != nullptr)
	{
		// make sure we get a physical pixel position, not a DPI-virtualized logical coordinate
		POINT p2 = {ppt->x, ppt->y};
		gDpiInfo.logicalToPhysicalPoint (HWND_DESKTOP, &p2);
		screenPosInPixel (p2.x, p2.y);
	}

	if(dwID == CONTEXT_MENU_TEXTSELECT || dwID == CONTEXT_MENU_DEFAULT)
		owner.popupContextMenu (screenPosInPixel, dwID == CONTEXT_MENU_TEXTSELECT);

	return S_OK; // suppress IE menu
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::HostContainer::GetHostInfo (DOCHOSTUIINFO *pInfo)
{
	CCL_PRINTLN ("GetHostInfo")

	pInfo->cbSize = sizeof(DOCHOSTUIINFO);
	pInfo->dwFlags =	DOCHOSTUIFLAG_NO3DBORDER |
						DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE |
						DOCHOSTUIFLAG_NOPICS |
						DOCHOSTUIFLAG_LOCAL_MACHINE_ACCESS_CHECK |
						DOCHOSTUIFLAG_DISABLE_UNTRUSTEDPROTOCOL;

	if(Win32::gDpiInfo.isDpiAware ())
		pInfo->dwFlags |= DOCHOSTUIFLAG_DPI_AWARE;

	pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;
	pInfo->pchHostCss = nullptr;
	pInfo->pchHostNS = nullptr;

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::HostContainer::ShowUI (DWORD dwID, IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget, IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc)
{
	CCL_PRINTLN ("ShowUI")
	// MSDN: To avoid a memory leak, forward calls to IDocHostUIHandler::ShowUI to the default UI handler.
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::HostContainer::HideUI ()
{
	CCL_PRINTLN ("HideUI")
	// MSDN: To avoid a memory leak, forward calls to IDocHostUIHandler::HideUI to the default UI handler.
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::HostContainer::UpdateUI ()
{
	CCL_PRINTLN ("UpdateUI")
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::HostContainer::EnableModeless (BOOL fEnable)
{
	CCL_PRINTLN ("EnableModeless")
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::HostContainer::OnDocWindowActivate (BOOL fActivate)
{
	CCL_PRINTLN ("OnDocWindowActivate")
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::HostContainer::OnFrameWindowActivate (BOOL fActivate)
{
	CCL_PRINTLN ("OnFrameWindowActivate")
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::HostContainer::ResizeBorder (LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow)
{
	CCL_PRINTLN ("ResizeBorder")
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::HostContainer::TranslateAccelerator (LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID)
{
	CCL_PRINTLN ("TranslateAccelerator")
	return S_FALSE; // not handled
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::HostContainer::GetOptionKeyPath (LPOLESTR *pchKey, DWORD dw)
{
	CCL_PRINTLN ("GetOptionKeyPath")
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::HostContainer::GetDropTarget (IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
	CCL_PRINTLN ("GetDropTarget")
	if(ppDropTarget)
		*ppDropTarget = nullptr;
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::HostContainer::GetExternal (IDispatch **ppDispatch)
{
	CCL_PRINTLN ("GetExternal")
	// LATER TODO: provide IDispatch for script access from HTML document!
	if(ppDispatch)
		*ppDispatch = nullptr;
	return S_FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::HostContainer::TranslateUrl (DWORD dwTranslate, LPWSTR pchURLIn, LPWSTR *ppchURLOut)
{
	CCL_PRINTLN ("TranslateUrl")
	if(ppchURLOut)
		*ppchURLOut  = nullptr;
	return S_FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::HostContainer::FilterDataObject (IDataObject *pDO, IDataObject **ppDORet)
{
	CCL_PRINTLN ("FilterDataObject")
	if(ppDORet)
		*ppDORet = nullptr;
	return S_FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::HostContainer::ShowMessage (HWND hwnd, LPOLESTR lpstrText, LPOLESTR lpstrCaption, DWORD dwType, LPOLESTR lpstrHelpFile, DWORD dwHelpContext, LRESULT *plResult)
{
	CCL_PRINTLN ("ShowMessage")
	return S_OK; // ???
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::HostContainer::ShowHelp (HWND hwnd, LPOLESTR pszHelpFile, UINT uCommand, DWORD dwData, POINT ptMouse, IDispatch *pDispatchObjectHit)
{
	CCL_PRINTLN ("ShowHelp")
	return S_OK; // ???
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::HostContainer::QueryService (REFGUID guidService, REFIID riid, void** ppvObject)
{
	if(SID_SInternetSecurityManager == guidService)
	{
		if(riid == IID_IInternetSecurityManager )
		{
			(*ppvObject) = static_cast<::IInternetSecurityManager*> (NEW SecurityManager);
			return S_OK;
		}
		else
			return E_NOINTERFACE;
	}
	return E_NOTIMPL;
}

//************************************************************************************************
// IEWebBrowserControl::EventHandler
//************************************************************************************************

IEWebBrowserControl::EventHandler::EventHandler (IEWebBrowserControl& owner)
: owner (owner)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IEWebBrowserControl::EventHandler::~EventHandler ()
{
	CCL_PRINTLN ("EventHandler dtor")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API IEWebBrowserControl::EventHandler::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_COM_INTERFACE (IDispatch)
	return Object::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT IEWebBrowserControl::EventHandler::Invoke (InvokeArgs& args)
{
	#if DEBUG_LOG
	args.dump ();
	#endif

	if(args.dispIdMember == DISPID_COMMANDSTATECHANGE)
	{
		ASSERT (args.getArgCount () == 2)
		VARIANT& v1 = args.getArg (0);
		VARIANT& v2 = args.getArg (1);
		ASSERT (v1.vt == VT_I4)
		ASSERT (v2.vt == VT_BOOL)

		int command = v1.intVal;
		VARIANT_BOOL enable = v2.boolVal;

		switch(command)
		{
		case CSC_NAVIGATEFORWARD : owner.flagCanForward (enable != 0); break;
		case CSC_NAVIGATEBACK : owner.flagCanBack (enable != 0); break;
		}

		owner.deferChanged ();
	}
	else if(args.dispIdMember == DISPID_NAVIGATECOMPLETE2)
	{
		owner.updateCurrentPage ();
		owner.deferChanged ();
	}
	else if(args.dispIdMember == DISPID_BEFORENAVIGATE2)
	{
		/*
			BeforeNavigate2 event
			http://msdn.microsoft.com/en-us/library/aa768280%28v=VS.85%29.aspx
		*/

		ASSERT (args.getArgCount () == 7)

		// restrict to local pages, open externally otherwise
		if(owner.getOptions ().isCustomStyle (Styles::kWebBrowserViewBehaviorRestrictToLocal))
		{
			ConvertedVariant v (args.getArg (1));
			VariantString urlString (v);
			Url url (urlString);
			bool accepted = url.getProtocol ().isEmpty () || url.getProtocol ().compare (CCLSTR ("file"), false) == 0;
			if(accepted == false)
			{
				VARIANT& cancel = args.getArg (6);
				*cancel.pboolVal = VARIANT_TRUE;

				System::GetSystemShell ().openUrl (url);
			}
		}
	}
	else if(args.dispIdMember == DISPID_NEWWINDOW3)
	{
		/*
			NewWindow3 event
			https://msdn.microsoft.com/en-us/library/aa768288%28v=vs.85%29.aspx
		*/

		ASSERT (args.getArgCount () == 5)
		VARIANT& cancel = args.getArg (1);
		*cancel.pboolVal = VARIANT_TRUE;

		ConvertedVariant v (args.getArg (4));
		VariantString urlString (v);

		System::GetSystemShell ().openUrl (Url (urlString));
	}

	return AbstractIDispatch::Invoke (args);
}

//************************************************************************************************
// IEWebBrowserControl::SecurityManager
//************************************************************************************************

IEWebBrowserControl::SecurityManager::SecurityManager ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API IEWebBrowserControl::SecurityManager::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_COM_INTERFACE (IInternetSecurityManager)
	return Object::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::SecurityManager::SetSecuritySite (IInternetSecurityMgrSite *pSite)
{
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::SecurityManager::GetSecuritySite (IInternetSecurityMgrSite **ppSite)
{
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::SecurityManager::MapUrlToZone (LPCWSTR pwszUrl, DWORD *pdwZone, DWORD dwFlags)
{
	if(pdwZone == nullptr)
		return E_INVALIDARG;

	CCL::String url (pwszUrl);
	if(url.startsWith (CCLSTR ("file:"), false))
	{
		*pdwZone = URLZONE_LOCAL_MACHINE;
		return S_OK;
	}

	return INET_E_DEFAULT_ACTION;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::SecurityManager::GetSecurityId (LPCWSTR pwszUrl, BYTE *pbSecurityId, DWORD *pcbSecurityId, DWORD_PTR dwReserved)
{
	return INET_E_DEFAULT_ACTION;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::SecurityManager::ProcessUrlAction (LPCWSTR pwszUrl,DWORD dwAction, BYTE *pPolicy, DWORD cbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved)
{
	return INET_E_DEFAULT_ACTION;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::SecurityManager::QueryCustomPolicy (LPCWSTR pwszUrl, REFGUID guidKey, BYTE **ppPolicy, DWORD *pcbPolicy, BYTE *pContext,DWORD cbContext, DWORD dwReserved)
{
	return INET_E_DEFAULT_ACTION;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::SecurityManager::SetZoneMapping (DWORD dwZone,LPCWSTR lpszPattern, DWORD dwFlags)
{
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP IEWebBrowserControl::SecurityManager::GetZoneMappings (DWORD dwZone, IEnumString **ppenumString, DWORD dwFlags)
{
	return E_NOTIMPL;
}

