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
// Filename    : ccl/platform/win/gui/activex.cpp
// Description : Active X Control Embedding
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/win/gui/activex.h"
#include "ccl/platform/win/gui/win32graphics.h"

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// ActiveX Helper Methods
//************************************************************************************************

HRESULT ActiveX::Advise (::IUnknown* pUnkCP, ::IUnknown* pUnk, const IID& iid, LPDWORD pdw)
{
	ComPtr<::IConnectionPointContainer> container;
	HRESULT hr = pUnkCP->QueryInterface (IID_IConnectionPointContainer, container);
	if(FAILED (hr))
		return hr;

	ComPtr<::IConnectionPoint> connectionPoint;
	hr = container->FindConnectionPoint (iid, connectionPoint);
	if(FAILED (hr))
		return hr;

	return connectionPoint->Advise (pUnk, pdw);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT ActiveX::Unadvise (::IUnknown* pUnkCP, const IID& iid, DWORD dw)
{
	ComPtr<::IConnectionPointContainer> container;
	HRESULT hr = pUnkCP->QueryInterface (IID_IConnectionPointContainer, container);
	if(FAILED (hr))
		return hr;

	ComPtr<::IConnectionPoint> connectionPoint;
	hr = container->FindConnectionPoint (iid, connectionPoint);
	if(FAILED (hr))
		return hr;

	return connectionPoint->Unadvise (dw);
}

//************************************************************************************************
// AbstractIDispatch
//************************************************************************************************

#if DEBUG
void AbstractIDispatch::InvokeArgs::dump ()
{
	Debugger::printf ("Invoke dispId = %d\n", dispIdMember);

	Debugger::printf ("  argc = %d\n", getArgCount ());
	for(int i = 0; i < getArgCount (); i++)
	{
		VARIANT& arg = getArg (i);
		Debugger::printf ("  arg[%d]: type = %d (%x)\n", i, (arg.vt & VT_TYPEMASK), arg.vt);
	}
}
#endif

//************************************************************************************************
// ActiveXEmbedder
//************************************************************************************************

bool ActiveXEmbedder::construct (ActiveXContainer* _container, ::IUnknown* unknown)
{
	ASSERT (!container && !object)

	unknown->QueryInterface (IID_IOleObject, object);
	if(!object)
	{
		_container->release ();
		return false;
	}

	container = _container;

	object->SetHostNames (L"CCL.ActiveXContainer", nullptr);
	object->SetClientSite (container);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ActiveXEmbedder::activate ()
{
	HRESULT hr = E_FAIL;
	if(object)
	{
		RECT rcClient = {0};
		GdiInterop::toSystemRect (rcClient, container->getSize ());
		HWND hwndParent = container->getHwnd ();
		hr = object->DoVerb (OLEIVERB_INPLACEACTIVATE, nullptr, container, 0, hwndParent, &rcClient);
	}
	return SUCCEEDED (hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActiveXEmbedder::close ()
{
	if(object)
		object->Close (OLECLOSE_NOSAVE);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActiveXEmbedder::resize (RectRef newSize)
{
	#if 0//DEBUG_LOG
	dumpRect (newSize, "ActiveXEmbedder::resize");
	#endif

	if(container)
		container->setSize (newSize);

	ComPtr<IOleInPlaceObject> object2;
	if(object)
		object->QueryInterface (IID_IOleInPlaceObject, object2);
	if(object2)
	{
		RECT rcPosRect = {0};
		RECT rcClipRect = {0};
		GdiInterop::toSystemRect (rcPosRect, newSize);
		GdiInterop::toSystemRect (rcClipRect, newSize);

		object2->SetObjectRects (&rcPosRect, &rcClipRect);
	}
}

//************************************************************************************************
// ActiveXFrame
//************************************************************************************************

ActiveXFrame::ActiveXFrame (HWND hwnd)
: hwnd (hwnd)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ActiveXFrame::~ActiveXFrame ()
{
	CCL_PRINTLN ("ActiveXFrame dtor")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ActiveXFrame::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_COM_INTERFACE (IOleWindow)
	QUERY_COM_INTERFACE (IOleInPlaceUIWindow)
	QUERY_COM_INTERFACE (IOleInPlaceFrame)
	return Object::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXFrame::GetWindow (HWND* phwnd)
{
	CCL_PRINTLN ("ActiveXFrame::GetWindow")
	*phwnd = hwnd;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXFrame::ContextSensitiveHelp (BOOL fEnterMode)
{
	CCL_PRINTLN ("ActiveXFrame::ContextSensitiveHelp")
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXFrame::GetBorder (LPRECT lprectBorder)
{
	CCL_PRINTLN ("ActiveXFrame::GetBorder")
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXFrame::RequestBorderSpace (LPCBORDERWIDTHS pborderwidths)
{
	CCL_PRINTLN ("ActiveXFrame::RequestBorderSpace")
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXFrame::SetBorderSpace (LPCBORDERWIDTHS pborderwidths)
{
	CCL_PRINTLN ("ActiveXFrame::SetBorderSpace")
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXFrame::SetActiveObject (IOleInPlaceActiveObject* pActiveObject, LPCOLESTR pszObjName)
{
	CCL_PRINTLN ("ActiveXFrame::SetActiveObject")
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXFrame::InsertMenus (HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
	CCL_PRINTLN ("ActiveXFrame::InsertMenus")
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXFrame::SetMenu (HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
	CCL_PRINTLN ("ActiveXFrame::SetMenu")
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXFrame::RemoveMenus (HMENU hmenuShared)
{
	CCL_PRINTLN ("ActiveXFrame::RemoveMenus")
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXFrame::SetStatusText (LPCOLESTR pszStatusText)
{
	CCL_PRINT ("ActiveXFrame::SetStatusText \"")
	CCL_PRINT (pszStatusText)
	CCL_PRINTLN ("\"")
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXFrame::EnableModeless (BOOL fEnable)
{
	CCL_PRINTF ("ActiveXFrame::EnableModeless %s\n", fEnable ? "TRUE" : "FALSE")
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXFrame::TranslateAccelerator (LPMSG lpmsg, WORD wID)
{
	CCL_PRINTLN ("ActiveXFrame::TranslateAccelerator")
	return E_NOTIMPL;
}

//************************************************************************************************
// ActiveXContainer
//************************************************************************************************

ActiveXContainer::ActiveXContainer (HWND hwnd, RectRef size)
: hwnd (hwnd),
  size (size),
  frame (nullptr)
{
	frame = NEW ActiveXFrame (hwnd);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ActiveXContainer::~ActiveXContainer ()
{
	CCL_PRINTLN ("ActiveXContainer dtor")

	frame->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ActiveXContainer::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_COM_INTERFACE (IOleClientSite)
	QUERY_COM_INTERFACE (IOleWindow)
	QUERY_COM_INTERFACE (IOleInPlaceSite)
	return Object::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXContainer::SaveObject ()
{
	CCL_PRINTLN ("ActiveXContainer::SaveObject")
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXContainer::GetMoniker (DWORD dwAssign, DWORD dwWhichMoniker, IMoniker** ppmk)
{
	CCL_PRINTLN ("ActiveXContainer::GetMoniker")
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXContainer::GetContainer (IOleContainer** ppContainer)
{
	CCL_PRINTLN ("ActiveXContainer::GetContainer")
	*ppContainer = nullptr;
	return E_NOINTERFACE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXContainer::ShowObject ()
{
	CCL_PRINTLN ("ActiveXContainer::ShowObject")
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXContainer::OnShowWindow (BOOL fShow)
{
	CCL_PRINTLN ("ActiveXContainer::OnShowWindow")
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXContainer::RequestNewObjectLayout ()
{
	CCL_PRINTLN ("ActiveXContainer::RequestNewObjectLayout")
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXContainer::GetWindow (HWND* phwnd)
{
	CCL_PRINTLN ("ActiveXContainer::GetWindow")
	*phwnd = hwnd;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXContainer::ContextSensitiveHelp (BOOL fEnterMode)
{
	CCL_PRINTLN ("ActiveXContainer::ContextSensitiveHelp")
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXContainer::CanInPlaceActivate ()
{
	CCL_PRINTLN ("ActiveXContainer::CanInPlaceActivate")
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXContainer::OnInPlaceActivate ()
{
	CCL_PRINTLN ("ActiveXContainer::OnInPlaceActivate")
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXContainer::OnUIActivate ()
{
	CCL_PRINTLN ("ActiveXContainer::OnUIActivate")
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXContainer::GetWindowContext (IOleInPlaceFrame** ppFrame, IOleInPlaceUIWindow** ppDoc,
												 LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
	CCL_PRINTLN ("ActiveXContainer::GetWindowContext")

	*ppFrame = frame;
	frame->retain ();

	*ppDoc = nullptr;

	GdiInterop::toSystemRect (*lprcPosRect, size);
	GdiInterop::toSystemRect (*lprcClipRect, size);

	lpFrameInfo->fMDIApp = FALSE;
	lpFrameInfo->hwndFrame = hwnd;
	lpFrameInfo->haccel = NULL;
	lpFrameInfo->cAccelEntries = 0;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXContainer::Scroll (SIZE scrollExtant)
{
	CCL_PRINTLN ("ActiveXContainer::Scroll")
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXContainer::OnUIDeactivate (BOOL fUndoable)
{
	CCL_PRINTLN ("ActiveXContainer::OnUIDeactivate")
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXContainer::OnInPlaceDeactivate ()
{
	CCL_PRINTLN ("ActiveXContainer::OnInPlaceDeactivate")
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXContainer::DiscardUndoState ()
{
	CCL_PRINTLN ("ActiveXContainer::DiscardUndoState")
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXContainer::DeactivateAndUndo ()
{
	CCL_PRINTLN ("ActiveXContainer::DeactivateAndUndo")
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP ActiveXContainer::OnPosRectChange (LPCRECT lprcPosRect)
{
	CCL_PRINTLN ("ActiveXContainer::OnPosRectChange")
	return S_OK;
}
