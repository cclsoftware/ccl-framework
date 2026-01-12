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
// Filename    : ccl/platform/win/gui/oledragndrop.cpp
// Description : OLE Drag-and-Drop
//
//************************************************************************************************

#define DEBUG_LOG 0
#define USE_DRAGDROP_HELPERS 1 // use Shell visual feedback
#define WRITE_DROPFILES_WIDE 1

#include "ccl/platform/win/gui/oledragndrop.h"
#include "ccl/platform/win/gui/dragndrop.win.h"
#include "ccl/platform/win/gui/touchhelper.h"
#include "ccl/platform/win/gui/windowhelper.h"

#include "ccl/gui/keyevent.h"
#include "ccl/gui/system/dragndrop.h"
#include "ccl/gui/system/clipboard.h"
#include "ccl/gui/windows/nativewindow.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/base/streamer.h"

namespace CCL {
namespace Win32 {

// helper functions:
static HGLOBAL GlobalClone (HGLOBAL hglobIn);
static ::IUnknown* GetCanonicalIUnknown (::IUnknown* punk);

static int resultToEffect (int result)
{
	switch (result)
	{
	case IDragSession::kDropNone:			return DROPEFFECT_NONE;
	case IDragSession::kDropCopyShared:		return DROPEFFECT_COPY;
	case IDragSession::kDropCopyReal:		return DROPEFFECT_COPY;
	case IDragSession::kDropMove:			return DROPEFFECT_MOVE;
	}
	return DROPEFFECT_NONE;
}

static int effectToResult (int effect)
{
	int dragResult = 0;
	if(effect & DROPEFFECT_COPY)
		dragResult |= IDragSession::kDropCopyReal; // todo: distinguish kDropCopyShared...???
	if(effect & DROPEFFECT_MOVE)
		dragResult |= IDragSession::kDropMove;
	return dragResult;
}

//************************************************************************************************
// DropTarget
//************************************************************************************************

DropTarget::DropTarget (Window* window)
: window (window),
  currentSession (nullptr),
  targetHelper (nullptr)
{
	ASSERT (window != nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DropTarget::~DropTarget ()
{
	if(targetHelper)
		targetHelper->Release ();
	
	ASSERT (currentSession == nullptr)
	if(currentSession)
		currentSession->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DropTarget::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_COM_INTERFACE (IDropTarget)
	return Object::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DropTarget::enableTargetHelper (bool state, POINTL pt, DWORD* pdwEffect)
{
	if(state != (targetHelper != nullptr))
	{
		if(state)
		{
			#if USE_DRAGDROP_HELPERS
			::CoCreateInstance (CLSID_DragDropHelper, nullptr, CLSCTX_INPROC_SERVER, IID_IDropTargetHelper, (void**)&targetHelper);
			ASSERT (targetHelper != nullptr)
			if(targetHelper)
			{
				HWND hwnd = (HWND)window->getSystemWindow ();
				ASSERT (hwnd != nullptr)
				IDataObject* dataObject = currentSession->getDataObject ();
				targetHelper->DragEnter (hwnd, dataObject, (POINT*)&pt, *pdwEffect);
			}
			#endif
		}
		else
			releaseTargetHelper ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DropTarget::releaseTargetHelper ()
{
	if(targetHelper)
	{
		targetHelper->DragLeave ();
		targetHelper->Release ();
		targetHelper = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP DropTarget::DragEnter (IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
	#if DEBUG_LOG
	CCL::Debugger::printf ("Drag Enter (%d/%d)...\n", pt.x, pt.y);
	#endif

	// prepare new drag session
	ASSERT (currentSession == nullptr)
	if(currentSession)
		currentSession->release ();

	currentSession = NEW CCL::WindowsDragSession (pDataObj);
	//currentSession->setDropResult (*pdwEffect); // hmm...?

	CCL::DragEvent e (*currentSession, CCL::DragEvent::kDragEnter);
	VKey::fromSystemModifiers (e.keys, grfKeyState);
	
	e.where (pt.x, pt.y); // in screen coordinates
	Win32Window::cast (window)->screenPixelToClientCoord (e.where);

	if(window->onDragEnter (e))
		*pdwEffect = resultToEffect (currentSession->getResult ());
	else
		*pdwEffect = DROPEFFECT_NONE;

	enableTargetHelper (!currentSession->hasVisualFeedback (), pt, pdwEffect);

	EnforceWindowOrder ();
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP DropTarget::DragOver (DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
	#if DEBUG_LOG
	CCL::Debugger::printf ("Drag Over (%d/%d)...\n", pt.x, pt.y);
	#endif

	ASSERT (currentSession != nullptr)
	if(!currentSession)
		return E_UNEXPECTED;

	CCL::DragEvent e (*currentSession, CCL::DragEvent::kDragOver);
	VKey::fromSystemModifiers (e.keys, grfKeyState);
	
	e.where (pt.x, pt.y); // in screen coordinates
	Win32Window::cast (window)->screenPixelToClientCoord (e.where);

	if(window->onDragOver (e))
		*pdwEffect = resultToEffect (currentSession->getTotalResult ());
	else
		*pdwEffect = DROPEFFECT_NONE;

	enableTargetHelper (!currentSession->hasVisualFeedback (), pt, pdwEffect);
	if(targetHelper)
		targetHelper->DragOver ((POINT*)&pt, *pdwEffect);
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP DropTarget::DragLeave ()
{
	#if DEBUG_LOG
	CCL::Debugger::printf ("Drag Leave!\n");
	#endif

	releaseTargetHelper ();

	ASSERT (currentSession != nullptr)
	if(!currentSession)
		return E_UNEXPECTED;

	CCL::DragEvent e (*currentSession, CCL::DragEvent::kDragLeave);
	window->onDragLeave (e);

	if(currentSession)
		currentSession->release ();
	currentSession = nullptr;

	EnforceWindowOrder ();
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP DropTarget::Drop (IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
	#if DEBUG_LOG
	CCL::Debugger::printf ("Drop (%d/%d)...\n", pt.x, pt.y);
	#endif

	if(targetHelper)
		targetHelper->Drop (pDataObj, (POINT*)&pt, *pdwEffect);

	ASSERT (currentSession != nullptr)
	if(!currentSession)
		return E_UNEXPECTED;

	CCL::DragEvent e (*currentSession, CCL::DragEvent::kDrop);
	VKey::fromSystemModifiers (e.keys, grfKeyState);
	
	e.where (pt.x, pt.y); // in screen coordinates
	Win32Window::cast (window)->screenPixelToClientCoord (e.where);

	if(window->onDrop (e))
		*pdwEffect = resultToEffect (currentSession->getTotalResult ());
	else
		*pdwEffect = DROPEFFECT_NONE;

	return DragLeave (); // we must cleanup here!
}

//************************************************************************************************
// DataObject::DataEntry
//************************************************************************************************

DataObject::DataEntry::DataEntry ()
{
	memset (&format, 0, sizeof(FORMATETC));
	memset (&medium, 0, sizeof(STGMEDIUM));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DataObject::DataEntry::~DataEntry ()
{
	if(format.ptd)
		::CoTaskMemFree (format.ptd);
	::ReleaseStgMedium (&medium);
}

//************************************************************************************************
// DataObject
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DataObject, Object)
DEFINE_IID_ (IDataObjectPrivate, 0x82BBB40B, 0xDA8B, 0x4CE0, 0x90, 0x09, 0x91, 0x0A, 0x3F, 0xB8, 0x3A, 0xB5)

//////////////////////////////////////////////////////////////////////////////////////////////////

DataObject::DataObject (CCL::WindowsDragSession* session)
: session (session)
{
	entries.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::WindowsDragSession* DataObject::getSession () const
{
	return session;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::tresult CCL_API DataObject::queryInterface (CCL::UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (IDataObjectPrivate)
	QUERY_COM_INTERFACE (IDataObject)
	return Object::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT DataObject::lookup (DataEntry*& result, const FORMATETC& format, BOOL add)
{
	CCL_LOGSCOPE ("DataObject::lookup")
	result = nullptr;

	/* Comparing two DVTARGETDEVICE structures is hard, so we don't even try */
	if(format.ptd != nullptr)
		return DV_E_DVTARGETDEVICE;

	ForEach (entries, DataEntry, e)
		if (e->format.cfFormat == format.cfFormat &&
			e->format.dwAspect == format.dwAspect &&
			e->format.lindex   == format.lindex)
		{
			if(add || (e->format.tymed & format.tymed))
			{
				result = e;
				return S_OK;
			}
			else
				return DV_E_TYMED;
		}
	EndFor

	if(!add)
		return DV_E_FORMATETC;

	result = NEW DataEntry;
	result->format = format;
	entries.add (result);
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT DataObject::AddRefStgMedium (STGMEDIUM* pstgmIn, STGMEDIUM* pstgmOut, BOOL fCopyIn)
{
	CCL_LOGSCOPE ("DataObject::AddRefStgMedium")
    HRESULT hres = S_OK;
    STGMEDIUM stgmOut = *pstgmIn;

    if (pstgmIn->pUnkForRelease == nullptr &&
        !(pstgmIn->tymed & (TYMED_ISTREAM | TYMED_ISTORAGE))) 
	{
        if (fCopyIn) 
		{
            /* Object needs to be cloned */
            if (pstgmIn->tymed == TYMED_HGLOBAL) 
			{
                stgmOut.hGlobal = GlobalClone (pstgmIn->hGlobal);
                if (!stgmOut.hGlobal) 
                    hres = E_OUTOFMEMORY;
            }
			else 
                hres = DV_E_TYMED; /* Don't know how to clone GDI objects */
        }
		else
            stgmOut.pUnkForRelease = static_cast<IDataObject*>(this);
    }

    if (SUCCEEDED(hres)) 
	{
        switch (stgmOut.tymed) 
		{
        case TYMED_ISTREAM:
            stgmOut.pstm->AddRef ();
            break;

        case TYMED_ISTORAGE:
            stgmOut.pstg->AddRef ();
            break;
        }

        if (stgmOut.pUnkForRelease)
            stgmOut.pUnkForRelease->AddRef ();

        *pstgmOut = stgmOut;
    }

    return hres;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT DataObject::getPaths (STGMEDIUM* pmedium)
{
	CCL_LOGSCOPE ("DataObject::getPaths")

	if(!session->containsNativePaths ())
	{
		CCL_PRINTLN ("  (have no native paths)")
		return DV_E_FORMATETC;
	}

	// write into a memory stream: DROPFILES header followed by a double 0-terminated list of paths
	DROPFILES dropFiles = {0};
	dropFiles.pFiles = sizeof(DROPFILES); // offset of path list
	dropFiles.fWide  = WRITE_DROPFILES_WIDE ? TRUE : FALSE;

	MemoryStream memStream;
	Streamer streamer (memStream);
	streamer.write (&dropFiles, sizeof(DROPFILES));

	ObjectList paths;
	paths.objectCleanup ();
	session->getNativePaths (paths);

	ForEach (paths, Url, url)
		#if WRITE_DROPFILES_WIDE
		NativePath nativePath (*url);
		streamer.writeString (nativePath.path, true); // including terminating 0
		#else
		MutableCString nativePathStr (UrlDisplayString (*url), Text::kSystemEncoding);
		streamer.write (nativePathStr.str (), nativePathStr.length () + 1); // including terminating 0
		#endif
	EndFor

	streamer.writeChar (0); // final terminating 0

	HGLOBAL	memoryHandle = ::GlobalAlloc (GMEM_MOVEABLE, memStream.getBytesWritten ()); 
	void* memory = ::GlobalLock (memoryHandle);
	if(!memory)
		return E_UNEXPECTED;

	memcpy (memory, memStream.getMemoryAddress (), memStream.getBytesWritten ());
	::GlobalUnlock (memoryHandle);
	pmedium->tymed = TYMED_HGLOBAL;
	pmedium->hGlobal = memoryHandle;
	pmedium->pUnkForRelease = nullptr;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT DataObject::getText (FORMATETC* pformatetcIn, STGMEDIUM* pmedium)
{
	CCL_LOGSCOPE ("DataObject::getText")

	CLIPFORMAT format = pformatetcIn->cfFormat;
	if(format == CF_TEXT || format == CF_UNICODETEXT)
	{
		if(pformatetcIn->tymed != TYMED_HGLOBAL)
			return DV_E_TYMED;

		// search session->getItems () for an object convertible to a string.
		// otherwise deliver an empty string (prevents crashing of some plugins if we have neither paths nor text)
		String string;
		ForEachUnknown (session->getItems (), unk)
			if(Clipboard::instance ().toText (string, unk))
				break;
		EndFor

		MutableCString cString;
		SIZE_T size = 0;
		if(format == CF_UNICODETEXT)
			size = (string.length () + 1) * sizeof(uchar); // one for terminating 0
		else
		{
			cString.append (string, Text::kSystemEncoding);
			size = cString.length () + 1;
		}

		HGLOBAL	memoryHandle = GlobalAlloc (GMEM_MOVEABLE, size);
		void* memory = GlobalLock (memoryHandle);
		if(memory)
		{
			if(format == CF_UNICODETEXT)
				memcpy (memory, StringChars (string), size);
			else
				memcpy (memory, cString.str (), size);

			GlobalUnlock (memoryHandle);
			pmedium->hGlobal = memoryHandle;
			pmedium->tymed = TYMED_HGLOBAL;
			pmedium->pUnkForRelease = nullptr;
			return S_OK;
		}
		return E_OUTOFMEMORY;
	}
	return DV_E_FORMATETC;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP DataObject::GetData (FORMATETC* pformatetcIn, STGMEDIUM* pmedium)
{
	CCL_LOGSCOPE ("DataObject::GetData")
	CCL_PRINTF ("%sGetData format: %d, medium: %d\n", CCL_INDENT, pformatetcIn->cfFormat, pformatetcIn->tymed);

	CLIPFORMAT format = pformatetcIn->cfFormat;
	HRESULT result = E_UNEXPECTED;

	// when asked for multiple medium flags, e.g. TYMED_HGLOBAL | TYMED_ISTREAM, we can choose one (and indicate that in pmedium->tymed)
	if(format == CF_HDROP && (pformatetcIn->tymed & TYMED_HGLOBAL) != 0)
		result = getPaths (pmedium);
	else if(format == CF_TEXT || format == CF_UNICODETEXT)
		result = getText (pformatetcIn, pmedium);

	if(result != S_OK)
	{
		DataEntry* e = nullptr;
		result = lookup (e, *pformatetcIn, FALSE);
		if(result == S_OK)
			result = AddRefStgMedium (&e->medium, pmedium, FALSE);
	}

	CCL_PRINTF ("%sGetData result %d\n", CCL_INDENT, result);
    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP DataObject::GetDataHere (FORMATETC* pformatetcm, STGMEDIUM* pmedium)
{
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP DataObject::QueryGetData (FORMATETC* pformatetc)
{
	CCL_LOGSCOPE ("DataObject::QueryGetData")
	CCL_PRINTF ("%sQueryGetData format: %d, medium: %d\n", CCL_INDENT, pformatetc->cfFormat, pformatetc->tymed);

	CLIPFORMAT format = pformatetc->cfFormat;
	HRESULT result = E_UNEXPECTED;

	if(format == CF_HDROP && (pformatetc->tymed & TYMED_HGLOBAL) != 0)
	{
		if(session->containsNativePaths ())
			result = S_OK;
		else
		{
			CCL_PRINTLN ("  (have no native paths)")
			result = DV_E_FORMATETC;
		}
	}
	else if((format == CF_TEXT || format == CF_UNICODETEXT) && pformatetc->tymed == TYMED_HGLOBAL)
		result = S_OK; // we can always provide (empty) text
	else
	{
		DataEntry* e = nullptr;
		result = lookup (e, *pformatetc, FALSE);
	}
	CCL_PRINTF ("%sQueryGetData result %d\n", CCL_INDENT, result);
    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP DataObject::GetCanonicalFormatEtc (FORMATETC* pformatectIn, FORMATETC* pformatetcOut)
{
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP DataObject::SetData (FORMATETC* pformatetc, STGMEDIUM* pmedium, BOOL fRelease)
{
	CCL_LOGSCOPE ("DataObject::SetData")
	if(!fRelease)
		return E_NOTIMPL;

    DataEntry* pde;
    HRESULT hres = lookup (pde, *pformatetc, TRUE);
    if (SUCCEEDED(hres)) 
	{
        if (pde->medium.tymed) 
		{
            ReleaseStgMedium (&pde->medium);
            ZeroMemory (&pde->medium, sizeof(STGMEDIUM));
        }

        if (fRelease) 
		{
            pde->medium = *pmedium;
            hres = S_OK;
        }
		else 
            hres = AddRefStgMedium (pmedium, &pde->medium, TRUE);

		pde->format.tymed = pde->medium.tymed;    /* Keep in sync */

        /* Subtlety!  Break circular reference loop */
        if (GetCanonicalIUnknown(pde->medium.pUnkForRelease) ==
            GetCanonicalIUnknown(static_cast<IDataObject*>(this)))
		{
            pde->medium.pUnkForRelease->Release ();
            pde->medium.pUnkForRelease = nullptr;
        }
    }
    return hres;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP DataObject::EnumFormatEtc (DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc)
{
	CCL_LOGSCOPE ("DataObject::EnumFormatEtc")

	if(dwDirection == DATADIR_GET)
	{
		*ppenumFormatEtc = NEW CCL::Win32::EnumFormatEtc (session);
		return S_OK;
	}
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP DataObject::DAdvise (FORMATETC* pformatetc, DWORD advf, IAdviseSink* pAdvSink, DWORD* pdwConnection)
{
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP DataObject::DUnadvise (DWORD dwConnection)
{
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP DataObject::EnumDAdvise (IEnumSTATDATA** ppenumAdvise)
{
	return E_NOTIMPL;
}

//************************************************************************************************
// EnumFormatEtc
//************************************************************************************************

DEFINE_CLASS_HIDDEN (EnumFormatEtc, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

EnumFormatEtc::EnumFormatEtc (WindowsDragSession* session)
: current (0)
{
	if(session->containsNativePaths ())
		addFormat (CF_HDROP);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EnumFormatEtc::EnumFormatEtc (const EnumFormatEtc& other)
: current (other.current)
{
	VectorForEach (other.formats, FORMATETC, formatEtc)
		formats.add (formatEtc);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EnumFormatEtc::addFormat (CLIPFORMAT format)
{
	CCL_LOGSCOPE ("EnumFormatEtc::addFormat")
	FORMATETC formatEtc = {0};
	formatEtc.cfFormat  = format;
	formatEtc.dwAspect  = DVASPECT_CONTENT;
	formatEtc.lindex    = -1;
	formatEtc.tymed     = TYMED_HGLOBAL;
	formats.add (formatEtc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::tresult CCL_API EnumFormatEtc::queryInterface (CCL::UIDRef iid, void** ptr)
{
	QUERY_COM_INTERFACE (IEnumFORMATETC)
	return Object::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP EnumFormatEtc::Next (ULONG celt, FORMATETC* pFormatEtc, ULONG *pceltFetched)
{
	CCL_LOGSCOPE ("EnumFormatEtc::Next")
	if(pceltFetched)
   	   *pceltFetched = 0;
	
   if(celt <= 0 || pFormatEtc == nullptr || current >= formats.count ())
      return S_FALSE;

   if(pceltFetched == nullptr && celt != 1) // pceltFetched can be nullptr only for 1 item request
      return S_FALSE;

	ULONG cReturn = celt;
	while(current < formats.count () && cReturn > 0)
	{
		*pFormatEtc++ = formats[current++];
		--cReturn;
	}
	if(pceltFetched)
		*pceltFetched = celt - cReturn;

    return (cReturn == 0) ? S_OK : S_FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP EnumFormatEtc::Skip (ULONG celt)
{
	CCL_LOGSCOPE ("EnumFormatEtc::Skip")
	if((current + int(celt)) >= formats.count ())
		return S_FALSE;
	current += celt;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP EnumFormatEtc::Reset (void)
{
	CCL_LOGSCOPE ("EnumFormatEtc::Reset")
	current = 0;
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP EnumFormatEtc::Clone (IEnumFORMATETC** ppenum)
{
	CCL_LOGSCOPE ("EnumFormatEtc::Clone")
	if(ppenum == nullptr)
		return E_POINTER;
      
  *ppenum = NEW EnumFormatEtc (*this);
  return S_OK;
}

//************************************************************************************************
// DropSource
//************************************************************************************************

DropSource::DropSource (CCL::WindowsDragSession* session)
: session (session),
  sourceHelper (nullptr)
{
	#if USE_DRAGDROP_HELPERS
	::CoCreateInstance (CLSID_DragDropHelper, nullptr, CLSCTX_INPROC_SERVER, IID_IDragSourceHelper, (void**)&sourceHelper);
	ASSERT (sourceHelper != nullptr)
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DropSource::~DropSource ()
{
	if(sourceHelper)
		sourceHelper->Release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragSourceHelper* DropSource::getHelper ()
{
	return sourceHelper;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::tresult CCL_API DropSource::queryInterface (CCL::UIDRef iid, void** ptr)
{
	QUERY_COM_INTERFACE (IDropSource)
	return Object::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP DropSource::QueryContinueDrag (BOOL fEscapePressed, DWORD grfKeyState)
{
	if(fEscapePressed)
	{
		session->setCanceled (true);
		return DRAGDROP_S_CANCEL;
	}
	
	if(session->getInputDevice () == IDragSession::kTouchInput)
	{
		if(!TouchHelper::isTouchDragging ())
			return DRAGDROP_S_DROP;
	}
	else if((grfKeyState & (MK_LBUTTON|MK_RBUTTON)) == 0)
		return DRAGDROP_S_DROP;

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP DropSource::GiveFeedback (DWORD dwEffect)
{
	session->setResult (effectToResult (dwEffect));

	// TBD: use custom cursors for per-monitor DPI???
	#if 0
	if(dwEffect == DROPEFFECT_NONE)
	{
		::SetCursor (::LoadCursor (0, IDC_NO));
		return S_OK;
	}
	#endif

	return DRAGDROP_S_USEDEFAULTCURSORS;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// helper functions (copied from MSDN)
//////////////////////////////////////////////////////////////////////////////////////////////////

HGLOBAL GlobalClone (HGLOBAL hglobIn)
{
    HGLOBAL hglobOut = NULL;
	LPVOID pvIn = ::GlobalLock (hglobIn);
    if(pvIn)
	{
		SIZE_T cb = ::GlobalSize (hglobIn);
		HGLOBAL hglobOut = ::GlobalAlloc (GMEM_FIXED, cb);
        if(hglobOut)
			::CopyMemory (hglobOut, pvIn, cb);
		::GlobalUnlock (hglobIn);
    }
    return hglobOut;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

::IUnknown* GetCanonicalIUnknown (::IUnknown* punk)
{
	::IUnknown *punkCanonical;
	if (punk && SUCCEEDED (punk->QueryInterface (IID_IUnknown, (LPVOID*)&punkCanonical)))
		punkCanonical->Release ();
	else 
		punkCanonical = punk;
	return punkCanonical;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Win32
} // namespace CCL
