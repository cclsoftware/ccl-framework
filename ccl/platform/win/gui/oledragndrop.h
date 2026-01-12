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
// Filename    : ccl/platform/win/gui/oledragndrop.h
// Description : OLE Drag-and-Drop
//
//************************************************************************************************

#ifndef _ccl_oledragndrop_h
#define _ccl_oledragndrop_h

#include "ccl/base/collections/objectlist.h"
#include "ccl/public/collections/vector.h"

#include "ccl/platform/win/system/cclcom.h"

namespace CCL {
class Window;
class WindowsDragSession; }

namespace CCL {
namespace Win32 {

//************************************************************************************************
// DropTarget
//************************************************************************************************

class DropTarget: public CCL::Object,
				  public ::IDropTarget
{
public:
	DropTarget (CCL::Window* window);
	~DropTarget ();

	// CCL::IUnknown
	CCL::tresult CCL_API queryInterface (CCL::UIDRef iid, void** ptr) override;

	// IUnknown
	DELEGATE_COM_IUNKNOWN

	// IDropTarget
	STDMETHODIMP DragEnter (IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;
	STDMETHODIMP DragOver  (DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;
	STDMETHODIMP DragLeave () override;
	STDMETHODIMP Drop      (IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;

protected:
	void enableTargetHelper (bool state, POINTL pt, DWORD* pdwEffect);
	void releaseTargetHelper ();

	CCL::Window* window;
	CCL::WindowsDragSession* currentSession;
	IDropTargetHelper* targetHelper;
};

//************************************************************************************************
// IDataObjectPrivate
//************************************************************************************************

interface IDataObjectPrivate: CCL::IUnknown
{
	virtual CCL::WindowsDragSession* CCL_API getSession () const = 0;

	DECLARE_IID (IDataObjectPrivate)
};

DEFINE_IID (IDataObjectPrivate, 0x82BBB40B, 0xDA8B, 0x4CE0, 0x90, 0x09, 0x91, 0x0A, 0x3F, 0xB8, 0x3A, 0xB5)

//************************************************************************************************
// DataObject
//************************************************************************************************

class DataObject: public CCL::Object,
				  public IDataObjectPrivate,
				  public ::IDataObject
{
public:
	DECLARE_CLASS (DataObject, Object)

	DataObject (CCL::WindowsDragSession* session = nullptr);

	// IDataObjectPrivate
	CCL::WindowsDragSession* CCL_API getSession () const override;

	// CCL::IUnknown
	CCL::tresult CCL_API queryInterface (CCL::UIDRef iid, void** ptr) override;

	// IUnknown
	DELEGATE_COM_IUNKNOWN
	UNKNOWN_REFCOUNT

	// IDataObject
	STDMETHODIMP GetData (FORMATETC* pformatetcIn, STGMEDIUM* pmedium) override;
	STDMETHODIMP GetDataHere (FORMATETC* pformatetcm, STGMEDIUM* pmedium) override;
	STDMETHODIMP QueryGetData (FORMATETC* pformatetc) override;
	STDMETHODIMP GetCanonicalFormatEtc (FORMATETC* pformatectIn, FORMATETC* pformatetcOut) override;
	STDMETHODIMP SetData (FORMATETC* pformatetc, STGMEDIUM* pmedium, BOOL fRelease) override;
	STDMETHODIMP EnumFormatEtc (DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc) override;
	STDMETHODIMP DAdvise (FORMATETC* pformatetc, DWORD advf, IAdviseSink* pAdvSink, DWORD* pdwConnection) override;
	STDMETHODIMP DUnadvise (DWORD dwConnection) override;
	STDMETHODIMP EnumDAdvise (IEnumSTATDATA** ppenumAdvise) override;

protected:
	CCL::WindowsDragSession* session;
	CCL::ObjectList entries;

	////////////////////////////////////
	// DataEntry
	////////////////////////////////////
	struct DataEntry: public CCL::Object
	{
		FORMATETC format;
		STGMEDIUM medium;

		DataEntry ();
		~DataEntry ();
	};

	HRESULT lookup (DataEntry*& result, const FORMATETC& format, BOOL add);
	HRESULT AddRefStgMedium (STGMEDIUM* pstgmIn, STGMEDIUM* pstgmOut, BOOL fCopyIn);
	HRESULT getPaths (STGMEDIUM* pmedium);
	HRESULT getText (FORMATETC* pformatetcIn, STGMEDIUM* pmedium);
};

//************************************************************************************************
// EnumFormatEtc
//************************************************************************************************

class EnumFormatEtc: public CCL::Object,
					 public ::IEnumFORMATETC
{
public:
	DECLARE_CLASS (EnumFormatEtc, Object)

	EnumFormatEtc (CCL::WindowsDragSession* session = nullptr);
	EnumFormatEtc (const EnumFormatEtc&);

	// CCL::IUnknown
	CCL::tresult CCL_API queryInterface (CCL::UIDRef iid, void** ptr) override;

	// IUnknown
	DELEGATE_COM_IUNKNOWN

	// IEnumFORMATETC
	STDMETHODIMP Next (ULONG celt, FORMATETC* pFormatEtc, ULONG *pceltFetched) override;
	STDMETHODIMP Skip (ULONG celt) override;
	STDMETHODIMP Reset (void) override;
	STDMETHODIMP Clone (IEnumFORMATETC** ppenum) override;

protected:
	CCL::Vector<FORMATETC> formats;
	int current;

	void addFormat (CLIPFORMAT format);
};

//************************************************************************************************
// DropSource
//************************************************************************************************

class DropSource: public CCL::Object,
				  public ::IDropSource
{
public:
	DropSource (CCL::WindowsDragSession* session);
	~DropSource ();

	IDragSourceHelper* getHelper ();

	// CCL::IUnknown
	CCL::tresult CCL_API queryInterface (CCL::UIDRef iid, void** ptr) override;

	// IUnknown
	DELEGATE_COM_IUNKNOWN

	// IDropSource
	STDMETHODIMP QueryContinueDrag (BOOL fEscapePressed, DWORD grfKeyState) override;
	STDMETHODIMP GiveFeedback (DWORD dwEffect) override;

protected:
	CCL::WindowsDragSession* session;
	IDragSourceHelper* sourceHelper;
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_oledragndrop_h
