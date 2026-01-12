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
// Filename    : ccl/platform/win/gui/activex.h
// Description : Active X Control Embedding
//
//************************************************************************************************

#ifndef _ccl_activex_h
#define _ccl_activex_h

#include "ccl/base/object.h"

#include "ccl/public/gui/graphics/rect.h"

#include "ccl/platform/win/system/cclcom.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// ActiveX Helper Methods
//************************************************************************************************

namespace ActiveX
{
	/** Creates a connection between an object's connection point and a client's sink. */
	HRESULT Advise (::IUnknown* pUnkCP, ::IUnknown* pUnk, const IID& iid, LPDWORD pdw);
	
	/** Terminates the connection established through Advise().*/
	HRESULT Unadvise (::IUnknown* pUnkCP, const IID& iid, DWORD dw);
}

//************************************************************************************************
// AbstractIDispatch
//************************************************************************************************

class AbstractIDispatch: public IDispatch
{
public:
	struct InvokeArgs
	{
		DISPID dispIdMember;
		REFIID riid;
		LCID lcid;
		WORD wFlags;
		DISPPARAMS* pDispParams;
		VARIANT* pVarResult;
		EXCEPINFO* pExcepInfo;
		UINT* puArgErr;

		// Note: arguments are in reverse order, i.e. right-most comes first!
		int getArgCount () const			{ return pDispParams->cArgs; }
		VARIANT& getArg (int index) const	{ return pDispParams->rgvarg[pDispParams->cArgs-1-index]; }

		#if DEBUG
		void dump ();
		#endif
	};

	virtual HRESULT Invoke (InvokeArgs& args)
	{ return E_NOTIMPL; }

	// IDispatch
	STDMETHODIMP GetTypeInfoCount (UINT* pctinfo) override
	{ return E_NOTIMPL; }

	STDMETHODIMP GetTypeInfo (UINT iTInfo, LCID lcid, ::ITypeInfo** ppTInfo) override
	{ return E_NOTIMPL; }

	STDMETHODIMP GetIDsOfNames (REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId) override
	{ return E_NOTIMPL; }

	STDMETHODIMP Invoke (DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
						 DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) override
	{
		InvokeArgs args = {dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr};
		return Invoke (args);
	}
};

//************************************************************************************************
// ActiveXFrame
//************************************************************************************************

class ActiveXFrame: public Object,
					public ::IOleInPlaceFrame
{
public:
	ActiveXFrame (HWND hwnd);
	~ActiveXFrame ();

	// CCL::IUnknown
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;

	// IUnknown
	DELEGATE_COM_IUNKNOWN

	// IOleWindow (<- IOleInPlaceUIWindow)
	STDMETHODIMP GetWindow (HWND* phwnd) override;
	STDMETHODIMP ContextSensitiveHelp (BOOL fEnterMode) override;

	// IOleInPlaceUIWindow (<- IOleInPlaceFrame)
	STDMETHODIMP GetBorder (LPRECT lprectBorder) override;
	STDMETHODIMP RequestBorderSpace (LPCBORDERWIDTHS pborderwidths) override;
	STDMETHODIMP SetBorderSpace (LPCBORDERWIDTHS pborderwidths) override;
	STDMETHODIMP SetActiveObject (IOleInPlaceActiveObject* pActiveObject, LPCOLESTR pszObjName) override;

	// IOleInPlaceFrame
	STDMETHODIMP InsertMenus (HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths) override;
	STDMETHODIMP SetMenu (HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject) override;
	STDMETHODIMP RemoveMenus (HMENU hmenuShared) override;
	STDMETHODIMP SetStatusText (LPCOLESTR pszStatusText) override;
	STDMETHODIMP EnableModeless (BOOL fEnable) override;
	STDMETHODIMP TranslateAccelerator (LPMSG lpmsg, WORD wID) override;

protected:
	HWND hwnd;
};

//************************************************************************************************
// ActiveXContainer
//************************************************************************************************

class ActiveXContainer: public Object,
						public ::IOleClientSite,
						public ::IOleInPlaceSite
{
public:
	ActiveXContainer (HWND hwnd, RectRef size);
	~ActiveXContainer ();

	PROPERTY_VARIABLE (HWND, hwnd, Hwnd)
	PROPERTY_OBJECT (Rect, size, Size)

	// CCL::IUnknown
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;

	// IUnknown
	DELEGATE_COM_IUNKNOWN

	// IOleClientSite
	STDMETHODIMP SaveObject () override;
	STDMETHODIMP GetMoniker (DWORD dwAssign, DWORD dwWhichMoniker, IMoniker** ppmk) override;
	STDMETHODIMP GetContainer (IOleContainer** ppContainer) override;
	STDMETHODIMP ShowObject () override;
	STDMETHODIMP OnShowWindow (BOOL fShow) override;
	STDMETHODIMP RequestNewObjectLayout () override;

	// IOleWindow (<- IOleInPlaceSite)
	STDMETHODIMP GetWindow (HWND* phwnd) override;
	STDMETHODIMP ContextSensitiveHelp (BOOL fEnterMode) override;

	// IOleInPlaceSite
	STDMETHODIMP CanInPlaceActivate () override;
	STDMETHODIMP OnInPlaceActivate () override;
	STDMETHODIMP OnUIActivate () override;
	STDMETHODIMP GetWindowContext (IOleInPlaceFrame** ppFrame, IOleInPlaceUIWindow** ppDoc, 
								   LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo) override;
	STDMETHODIMP Scroll (SIZE scrollExtant) override;
	STDMETHODIMP OnUIDeactivate (BOOL fUndoable) override;
	STDMETHODIMP OnInPlaceDeactivate () override;
	STDMETHODIMP DiscardUndoState () override;
	STDMETHODIMP DeactivateAndUndo () override;
	STDMETHODIMP OnPosRectChange (LPCRECT lprcPosRect) override;

protected:
	ActiveXFrame* frame;
};

//************************************************************************************************
// ActiveXEmbedder
//************************************************************************************************

class ActiveXEmbedder
{
public:
	bool construct (ActiveXContainer* container, ::IUnknown* unknown);

	bool activate ();
	void close ();

	void resize (RectRef newSize);

protected:
	AutoPtr<ActiveXContainer> container;
	ComPtr<::IOleObject> object;
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_activex_h
