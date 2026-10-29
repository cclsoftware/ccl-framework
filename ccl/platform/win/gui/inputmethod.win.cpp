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
// Filename    : ccl/platform/win/gui/inputmethod.win.cpp
// Description : Windows Input Method
//
//************************************************************************************************

#include "ccl/gui/system/inputmethod.h"

#include "ccl/gui/graphics/mutableregion.h"
#include "ccl/gui/views/view.h"

#include "ccl/platform/win/gui/dpihelper.h"
#include "ccl/platform/win/system/cclcom.h"

#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/graphics/itextlayout.h"

#include <initguid.h>
#include <TextStor.h>
#include <InputScope.h>
#include <msctf.h>
#include <tsattrs.h>

namespace CCL {

class WindowsInputMethodManager;

//************************************************************************************************
// WindowsTextStore
//************************************************************************************************

class WindowsTextStore: public Object,
						public ::ITextStoreACP2,
						public ::ITfContextOwnerCompositionSink,
						public ::ITfInputScope
{
public:
	DELEGATE_COM_IUNKNOWN
	DECLARE_CLASS_ABSTRACT (WindowsTextStore, Object)

	WindowsTextStore (ITextInputClient* client);

	tresult initialize ();
	tresult terminate ();

	tresult enable (bool state);

	void selectionChanged ();
	void textChanged ();
	void visibleRectChanged ();

	// ITextStoreACP2
	STDMETHODIMP AdviseSink (REFIID riid, ::IUnknown* punk, DWORD dwMask) override;
	STDMETHODIMP UnadviseSink (::IUnknown* punk) override;
	STDMETHODIMP RequestLock (DWORD dwLockFlags, HRESULT* phrSession) override;
	STDMETHODIMP GetStatus (TS_STATUS* pdcs) override;
	STDMETHODIMP QueryInsert (LONG acpInsertStart, LONG acpInsertEnd, ULONG cch, LONG* pacpResultStart, LONG* pacpResultEnd) override;
	STDMETHODIMP GetSelection (ULONG ulIndex, ULONG ulCount, TS_SELECTION_ACP* pSelection, ULONG* pcFetched) override;
	STDMETHODIMP SetSelection (ULONG ulCount, const TS_SELECTION_ACP* pSelection) override;
	STDMETHODIMP GetText (LONG acpStart, LONG acpEnd, WCHAR* pchPlain, ULONG cchPlainReq, ULONG* pcchPlainOut, TS_RUNINFO* prgRunInfo, ULONG ulRunInfoReq, ULONG* pulRunInfoOut, LONG* pacpNext) override;
	STDMETHODIMP SetText (DWORD dwFlags, LONG acpStart, LONG acpEnd, const WCHAR* pchText, ULONG cch, TS_TEXTCHANGE* pChange) override;
	STDMETHODIMP GetFormattedText (LONG acpStart, LONG acpEnd, ::IDataObject** ppDataObject) override { return E_NOTIMPL; }
	STDMETHODIMP GetEmbedded (LONG acpPos, REFGUID rguidService, REFIID riid, ::IUnknown** ppunk) override { return E_NOTIMPL; }
	STDMETHODIMP QueryInsertEmbedded (const GUID* pguidService, const FORMATETC* pFormatEtc, BOOL* pfInsertable) override { return E_NOTIMPL; }
	STDMETHODIMP InsertEmbedded (DWORD dwFlags, LONG acpStart, LONG acpEnd, ::IDataObject* pDataObject, TS_TEXTCHANGE* pChange) override { return E_NOTIMPL; }
	STDMETHODIMP InsertTextAtSelection (DWORD dwFlags, const WCHAR* pchText, ULONG cch, LONG* pacpStart, LONG* pacpEnd, TS_TEXTCHANGE* pChange) override;
	STDMETHODIMP InsertEmbeddedAtSelection (DWORD dwFlags, ::IDataObject* pDataObject, LONG* pacpStart, LONG* pacpEnd, TS_TEXTCHANGE* pChange) override { return E_NOTIMPL; };
	STDMETHODIMP RequestSupportedAttrs (DWORD dwFlags, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs) override;
	STDMETHODIMP RequestAttrsAtPosition (LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs, DWORD dwFlags) override;
	STDMETHODIMP RequestAttrsTransitioningAtPosition (LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs, DWORD dwFlags) override { return E_NOTIMPL; }
	STDMETHODIMP FindNextAttrTransition (LONG acpStart, LONG acpHalt, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs, DWORD dwFlags, LONG* pacpNext, BOOL* pfFound, LONG* plFoundOffset) override;
	STDMETHODIMP RetrieveRequestedAttrs (ULONG ulCount, TS_ATTRVAL* paAttrVals, ULONG* pcFetched) override;
	STDMETHODIMP GetEndACP (LONG* pacp) override;
	STDMETHODIMP GetActiveView (TsViewCookie* pvcView) override;
	STDMETHODIMP GetACPFromPoint (TsViewCookie vcView, const POINT* pt, DWORD dwFlags, LONG* pacp) override { return E_NOTIMPL; }
	STDMETHODIMP GetTextExt (TsViewCookie vcView, LONG acpStart, LONG acpEnd, RECT* prc, BOOL* pfClipped) override;
	STDMETHODIMP GetScreenExt (TsViewCookie vcView, RECT* prc) override;

	// ITfContextOwnerCompositionSink
	STDMETHODIMP OnStartComposition (ITfCompositionView* pComposition, BOOL* pfOk) override;
	STDMETHODIMP OnUpdateComposition (ITfCompositionView* pComposition, ITfRange* pRangeNew) override;
	STDMETHODIMP OnEndComposition (ITfCompositionView* pComposition) override;

	// ITfInputScope
	STDMETHODIMP GetInputScopes (InputScope** pprgInputScopes, UINT* pcCount) override;
	STDMETHODIMP GetPhrase (BSTR** ppbstrPhrases, UINT* pcCount) override { return E_NOTIMPL; }
	STDMETHODIMP GetRegularExpression (BSTR* pbstrRegExp) override { return E_NOTIMPL; }
	STDMETHODIMP GetSRGS (BSTR* pbstrSRGS) override { return E_NOTIMPL; }
	STDMETHODIMP GetXML (BSTR* pbstrXML) override { return E_NOTIMPL; }

	CLASS_INTERFACES (Object)

private:
	WindowsInputMethodManager& manager;
	ITextInputClient* client;

	TS_ATTRVAL attributes[8];
	int numAttributes;

	Win32::ComPtr<::ITfDocumentMgr> document;
	Win32::ComPtr<::ITfContext> primaryContext;
	::TfEditCookie editCookie;

	Win32::ComPtr<::ITextStoreACPSink> notificationSink;

	String preeditText;
	int preeditStart;

	int prepareAttributes (int numFilterAttributes, const TS_ATTRID* filterAttributes);
};

//************************************************************************************************
// WindowsInputMethod
//************************************************************************************************

class WindowsInputMethod: public Object,
						  public IInputMethod
{
public:
	WindowsInputMethod (ITextInputClient* client);
	~WindowsInputMethod ();

	// IInputMethod
	tresult CCL_API enable (tbool state) override { return textStore->enable (state); }
	void CCL_API textChanged () override { textStore->textChanged (); }
	void CCL_API selectionChanged () override { textStore->selectionChanged (); }
	void CCL_API visibleRectChanged () override { textStore->visibleRectChanged (); }

	CLASS_INTERFACE (IInputMethod, Object)

protected:
	SharedPtr<ITextInputClient> client;
	AutoPtr<WindowsTextStore> textStore;
};

//************************************************************************************************
// WindowsInputMethodManager
//************************************************************************************************

class WindowsInputMethodManager: public InputMethodManager
{
public:
	WindowsInputMethodManager ();
	~WindowsInputMethodManager ();

	::ITfThreadMgr* getThreadManager () const { return tsfThreadManager; }
	::TfClientId getClientId () const { return tsfClientId; }

	// InputMethodManager
	IInputMethod* createInputMethod (ITextInputClient& client) override;

private:
	Win32::ComPtr<::ITfThreadMgr> tsfThreadManager;
	::TfClientId tsfClientId;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// WindowsInputMethodManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (InputMethodManager, WindowsInputMethodManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsInputMethodManager::WindowsInputMethodManager ()
: tsfClientId (0)
{
	HRESULT hr = ::CoCreateInstance (CLSID_TF_ThreadMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfThreadMgr, tsfThreadManager);
	ASSERT (SUCCEEDED (hr))
	if(SUCCEEDED (hr))
		tsfThreadManager->Activate (&tsfClientId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsInputMethodManager::~WindowsInputMethodManager ()
{
	if(tsfThreadManager)
		tsfThreadManager->Deactivate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IInputMethod* WindowsInputMethodManager::createInputMethod (ITextInputClient& client)
{
	return NEW WindowsInputMethod (&client);
}

//************************************************************************************************
// WindowsTextStore
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WindowsTextStore, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsTextStore::WindowsTextStore (ITextInputClient* client)
: manager (static_cast<WindowsInputMethodManager&> (InputMethodManager::instance ())),
  client (client),
  numAttributes (0),
  editCookie (0),
  preeditStart (0)
{
	::memset (attributes, 0, sizeof(attributes));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WindowsTextStore::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_COM_INTERFACE (::ITextStoreACP2)
	QUERY_COM_INTERFACE (::ITfContextOwnerCompositionSink)
	QUERY_COM_INTERFACE (::ITfInputScope)
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult WindowsTextStore::initialize ()
{
	if(document)
		return kResultUnexpected;

	::ITfThreadMgr* threadManager = manager.getThreadManager ();
	if(!threadManager)
		return kResultFailed;

	threadManager->CreateDocumentMgr (document);

	document->CreateContext (manager.getClientId (), 0, static_cast<::ITextStoreACP2*> (this), primaryContext, &editCookie);
	document->Push (primaryContext);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult WindowsTextStore::terminate ()
{
	if(!document)
		return kResultUnexpected;

	document->Pop (TF_POPF_ALL);

	primaryContext = nullptr;
	document = nullptr;

	editCookie = 0;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult WindowsTextStore::enable (bool state)
{
	HWND hwnd = NULL;
	if(const IView* view = client->getInputView ())
		if(const IWindow* window = view->getIWindow ())
			hwnd = (HWND)window->getSystemWindow ();

	if(!hwnd)
		return kResultFailed;

	::ITfThreadMgr* threadManager = manager.getThreadManager ();
	if(!threadManager)
		return kResultFailed;

	::ITfDocumentMgr* oldDocument = nullptr;
	threadManager->AssociateFocus (hwnd, state ? document : nullptr, &oldDocument);
	if(oldDocument)
		oldDocument->Release ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsTextStore::selectionChanged ()
{
	if(notificationSink)
		notificationSink->OnSelectionChange ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsTextStore::textChanged ()
{
	if(notificationSink)
	{
		TS_TEXTCHANGE textChange = {0, 0, 0};
		notificationSink->OnTextChange (0, &textChange);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsTextStore::visibleRectChanged ()
{
	if(notificationSink)
		notificationSink->OnLayoutChange (TS_LC_CHANGE, 0x01);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP WindowsTextStore::AdviseSink (REFIID riid, ::IUnknown* punk, DWORD dwMask)
{
	if(!::IsEqualGUID (riid, ::IID_ITextStoreACPSink))
		return TS_E_NOOBJECT;

	if(FAILED (punk->QueryInterface (::IID_ITextStoreACPSink, notificationSink)))
		return E_NOINTERFACE;

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP WindowsTextStore::UnadviseSink (::IUnknown* punk)
{
	notificationSink = nullptr;

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP WindowsTextStore::RequestLock (DWORD dwLockFlags, HRESULT* phrSession)
{
	if(!notificationSink)
		return E_UNEXPECTED;

	*phrSession = notificationSink->OnLockGranted (dwLockFlags);
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP WindowsTextStore::GetStatus (TS_STATUS* pdcs)
{
	pdcs->dwDynamicFlags = 0;
	pdcs->dwStaticFlags = 0;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP WindowsTextStore::QueryInsert (LONG acpInsertStart, LONG acpInsertEnd, ULONG cch, LONG* pacpResultStart, LONG* pacpResultEnd)
{
	*pacpResultStart = acpInsertStart;
	*pacpResultEnd = acpInsertEnd;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP WindowsTextStore::GetSelection (ULONG ulIndex, ULONG ulCount, TS_SELECTION_ACP* pSelection, ULONG* pcFetched)
{
	*pcFetched = 0;
	if((ulCount > 0) && ((ulIndex == 0) || (ulIndex == TS_DEFAULT_SELECTION)))
	{
		int caret = 0, selection = 0;
		client->getSelection (caret, selection, ITextInputClient::kSelectionMerged);

		pSelection[0].acpStart = ccl_min (caret, selection);
		pSelection[0].acpEnd = ccl_max (caret, selection);
		pSelection[0].style.ase = caret < selection ? TS_AE_START : TS_AE_END;
		pSelection[0].style.fInterimChar = FALSE;
		*pcFetched = 1;
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP WindowsTextStore::SetSelection (ULONG ulCount, const TS_SELECTION_ACP* pSelection)
{
	if(ulCount > 0)
	{
		if(pSelection[0].style.ase == TS_AE_START)
			client->setSelection (pSelection[0].acpStart, pSelection[0].acpEnd);
		else
			client->setSelection (pSelection[0].acpEnd, pSelection[0].acpStart);
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP WindowsTextStore::GetText (LONG acpStart, LONG acpEnd, WCHAR* pchPlain, ULONG cchPlainReq, ULONG* pcchPlainOut, TS_RUNINFO* prgRunInfo, ULONG ulRunInfoReq, ULONG* pulRunInfoOut, LONG* pacpNext)
{
	if(cchPlainReq == 0 && ulRunInfoReq == 0)
		return S_OK;

	if(acpEnd == -1)
		acpEnd = client->getTextLength ();

	acpEnd = min (acpEnd, acpStart + int(cchPlainReq));

	String text;
	if(acpStart != acpEnd && client->getText (text, acpStart, acpEnd - acpStart) != kResultOk)
		return E_FAIL;

	for(int i = 0; i < acpEnd - acpStart; i++)
		pchPlain[i] = text[i];

	*pcchPlainOut = acpEnd - acpStart;
	if(ulRunInfoReq)
	{
		prgRunInfo[0].uCount = acpEnd - acpStart;
		prgRunInfo[0].type = TS_RT_PLAIN;
		*pulRunInfoOut = 1;
	}

	*pacpNext = acpEnd;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP WindowsTextStore::SetText (DWORD dwFlags, LONG acpStart, LONG acpEnd, const WCHAR* pchText, ULONG cch, TS_TEXTCHANGE* pChange)
{
	if(acpStart > client->getTextLength ())
		return TS_E_INVALIDPOS;

	TS_SELECTION_ACP selection = {acpStart, acpEnd, {TS_AE_END, FALSE}};
	HRESULT hr = SetSelection (1, &selection);
	if(FAILED (hr))
		return hr;

	return InsertTextAtSelection (0, pchText, cch, nullptr, nullptr, pChange);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP WindowsTextStore::RequestSupportedAttrs (DWORD dwFlags, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs)
{
	if(!prepareAttributes (cFilterAttrs, paFilterAttrs))
		return S_FALSE;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP WindowsTextStore::RequestAttrsAtPosition (LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs, DWORD dwFlags)
{
	if(!prepareAttributes (cFilterAttrs, paFilterAttrs))
		return S_FALSE;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP WindowsTextStore::FindNextAttrTransition (LONG acpStart, LONG acpHalt, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs, DWORD dwFlags, LONG* pacpNext, BOOL* pfFound, LONG* plFoundOffset)
{
	*pacpNext = 0;
	*pfFound = FALSE;
	*plFoundOffset = 0;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP WindowsTextStore::RetrieveRequestedAttrs (ULONG ulCount, TS_ATTRVAL* paAttrVals, ULONG* pcFetched)
{
	*pcFetched = 0;
	for(int i = 0; i < int(ulCount) && i < numAttributes; i++)
	{
		paAttrVals[i] = attributes[i];
		(*pcFetched)++;
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP WindowsTextStore::GetEndACP (LONG* pacp)
{
	*pacp = client->getTextLength ();
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP WindowsTextStore::GetActiveView (TsViewCookie* pvcView)
{
	*pvcView = 0;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP WindowsTextStore::GetTextExt (TsViewCookie vcView, LONG acpStart, LONG acpEnd, RECT* prc, BOOL* pfClipped)
{
	if(const IView* view = client->getInputView ())
	{
		SelectionRegion region;
		client->getTextBounds (region, acpStart, acpEnd - acpStart);

		Rect bounds = region.getBoundingBox ();
		if(bounds.isEmpty ())
			bounds.setEmpty ();

		Point leftTop = view->clientToScreen (bounds.getLeftTop ());
		Point rightBottom = view->clientToScreen (bounds.getRightBottom ());

		float scaleFactor = view->getIWindow ()->getContentScaleFactor ();

		DpiScale::toPixelPoint (leftTop, scaleFactor);
		DpiScale::toPixelPoint (rightBottom, scaleFactor);

		prc->left = leftTop.x;
		prc->top = leftTop.y;
		prc->right = rightBottom.x;
		prc->bottom = rightBottom.y;

		*pfClipped = FALSE;
		return S_OK;
	}
	return E_UNEXPECTED;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP WindowsTextStore::GetScreenExt (TsViewCookie vcView, RECT* prc)
{
	if(const IView* view = client->getInputView ())
	{
		Rect clientRect = view->getSize ();
		Point leftTop = view->clientToScreen (clientRect.getLeftTop ());
		Point rightBottom = view->clientToScreen (clientRect.getRightBottom ());

		float scaleFactor = view->getIWindow ()->getContentScaleFactor ();

		DpiScale::toPixelPoint (leftTop, scaleFactor);
		DpiScale::toPixelPoint (rightBottom, scaleFactor);

		prc->left = leftTop.x;
		prc->top = leftTop.y;
		prc->right = rightBottom.x;
		prc->bottom = rightBottom.y;

		return S_OK;
	}
	return E_UNEXPECTED;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP WindowsTextStore::InsertTextAtSelection (DWORD dwFlags, const WCHAR* pchText, ULONG cch, LONG* pacpStart, LONG* pacpEnd, TS_TEXTCHANGE* pChange)
{
	int caret = 0, selection = 0;
	client->getSelection (caret, selection, ITextInputClient::kSelectionMerged);

	int selectionStart = ccl_min (caret, selection);
	int selectionEnd = ccl_max (caret, selection);
	if(dwFlags & TS_IAS_QUERYONLY)
	{
		*pacpStart = selectionStart;
		*pacpEnd = selectionStart + cch;
		return S_OK;
	}

	int preeditEnd = preeditStart + preeditText.length ();
	if(selectionEnd > preeditEnd)
		client->replaceText (String::kEmpty, ccl_max (selectionStart, preeditEnd), selectionEnd - ccl_max (selectionStart, preeditEnd));

	preeditText = preeditText.subString (0, selectionStart - preeditStart).append (pchText, cch).append (preeditText.subString (selectionEnd - preeditStart, preeditEnd - selectionEnd));
	client->setPreeditText (preeditText);

	if(pacpStart)
		*pacpStart = selectionStart;

	if(pacpEnd)
		*pacpEnd = selectionStart + cch;

	if(pChange)
	{
		pChange->acpStart = selectionStart;
		pChange->acpOldEnd = selectionEnd;
		pChange->acpNewEnd = selectionStart + cch;
	}

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP WindowsTextStore::OnStartComposition (ITfCompositionView* pComposition, BOOL* pfOk)
{
	Win32::ComPtr<::ITfRange> range;
	if(FAILED (pComposition->GetRange (range)))
		return E_UNEXPECTED;

	Win32::ComPtr<::ITfRangeACP> acpRange;
	if(FAILED (range->QueryInterface (IID_PPV_ARGS (&acpRange))))
		return E_UNEXPECTED;

	LONG start = 0;
	LONG count = 0;
	acpRange->GetExtent (&start, &count);

	preeditStart = start;
	preeditText.empty ();

	*pfOk = TRUE;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP WindowsTextStore::OnUpdateComposition (ITfCompositionView* pComposition, ITfRange* pRangeNew)
{
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP WindowsTextStore::OnEndComposition (ITfCompositionView* pComposition)
{
	client->setPreeditText (String::kEmpty);
	client->applyText (preeditText);
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP WindowsTextStore::GetInputScopes (InputScope** pprgInputScopes, UINT* pcCount)
{
	*pprgInputScopes = reinterpret_cast<InputScope*> (CoTaskMemAlloc (sizeof(InputScope)));
	if(*pprgInputScopes == nullptr)
		return E_OUTOFMEMORY;

	*(*pprgInputScopes) = IS_TEXT;

	int inputType = client->getInputType ();
	int inputFlags = client->getInputFlags ();

	if(get_flag<int> (inputFlags, ITextInputClient::kSensitiveData))
		*(*pprgInputScopes) = IS_PASSWORD;

	switch(inputType)
	{
	case Styles::kKeyboardTypeNumeric : CCL_FALLTHROUGH
	case Styles::kKeyboardTypeNumericSigned : CCL_FALLTHROUGH
	case Styles::kKeyboardTypeDecimal : CCL_FALLTHROUGH
	case Styles::kKeyboardTypeDecimalSigned :
		*(*pprgInputScopes) = IS_NUMBER;
		if(get_flag<int> (inputFlags, ITextInputClient::kSensitiveData))
			*(*pprgInputScopes) = IS_NUMERIC_PASSWORD;
		break;
	case Styles::kKeyboardTypeEmail :
		*(*pprgInputScopes) = IS_EMAIL_SMTPEMAILADDRESS;
		break;
	case Styles::kKeyboardTypeUrl :
		*(*pprgInputScopes) = IS_URL;
		break;
	case Styles::kKeyboardTypePhoneNumber :
		*(*pprgInputScopes) = IS_TELEPHONE_FULLTELEPHONENUMBER;
		break;
	}

	*pcCount = 1;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int WindowsTextStore::prepareAttributes (int numFilterAttributes, const TS_ATTRID* filterAttributes)
{
	static const GUID supportedAttributes[7] =
	{
		GUID_PROP_INPUTSCOPE,
		TSATTRID_Font_Style_Height,
		TSATTRID_Font_FaceName,
		TSATTRID_Font_SizePts,
		TSATTRID_Text_ReadOnly,
		TSATTRID_Text_Orientation,
		TSATTRID_Text_VerticalWriting
	};

	const IView* view = client->getInputView ();
	if(!view)
		return 0;

	const IVisualStyle& visualStyle = view->getVisualStyle ();
	Font font = visualStyle.getTextFont ();

	::memset (attributes, 0, sizeof(attributes));
	numAttributes = 0;

	for(int i = 0; i < ARRAYSIZE(supportedAttributes); i++)
	{
		GUID attribute = supportedAttributes[i];

		if(numFilterAttributes)
		{
			bool found = false;
			for(int j = 0; j < numFilterAttributes; j++)
			{
				if(::IsEqualGUID (attribute, filterAttributes[j]))
				{
					found = true;
					break;
				}
			}

			if(!found)
				continue;
		}

		attributes[numAttributes].idAttr = attribute;
		attributes[numAttributes].dwOverlapId = i + 1;

		if(::IsEqualGUID (attribute, GUID_PROP_INPUTSCOPE))
		{
			::ITfInputScope* inputScope = nullptr;
			this->QueryInterface (IID_PPV_ARGS (&inputScope));

			attributes[numAttributes].varValue.vt = VT_UNKNOWN;
			attributes[numAttributes].varValue.punkVal = inputScope;
		}
		else if(::IsEqualGUID (attribute, TSATTRID_Font_Style_Height))
		{
			attributes[numAttributes].varValue.vt = VT_I4;
			attributes[numAttributes].varValue.lVal = ccl_round<0> (font.getSize () / 72.0 * 96.0);
		}
		else if(::IsEqualGUID (attribute, TSATTRID_Font_FaceName))
		{
			attributes[numAttributes].varValue.vt = VT_BSTR;
			attributes[numAttributes].varValue.bstrVal = NULL;
		}
		else if(::IsEqualGUID (attribute, TSATTRID_Font_SizePts))
		{
			attributes[numAttributes].varValue.vt = VT_I4;
			attributes[numAttributes].varValue.lVal = ccl_round<0> (font.getSize ());
		}
		else if(::IsEqualGUID (attribute, TSATTRID_Text_ReadOnly))
		{
			attributes[numAttributes].varValue.vt = VT_BOOL;
			attributes[numAttributes].varValue.bVal = FALSE;
		}
		else if(::IsEqualGUID (attribute, TSATTRID_Text_Orientation))
		{
			attributes[numAttributes].varValue.vt = VT_I4;
			attributes[numAttributes].varValue.lVal = 0;
		}
		else if(::IsEqualGUID (attribute, TSATTRID_Text_VerticalWriting))
		{
			attributes[numAttributes].varValue.vt = VT_BOOL;
			attributes[numAttributes].varValue.bVal = FALSE;
		}

		numAttributes++;
	}
	return numAttributes;
}

//************************************************************************************************
// WindowsInputMethod
//************************************************************************************************

WindowsInputMethod::WindowsInputMethod (ITextInputClient* client)
: client (client)
{
	textStore = NEW WindowsTextStore (client);
	textStore->initialize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsInputMethod::~WindowsInputMethod ()
{
	textStore->terminate ();
	textStore = nullptr;
}
