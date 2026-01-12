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
// Filename    : ccl/platform/win/gui/dragndrop.win.cpp
// Description : Windows-specific Drag-and-Drop stuff 
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/win/gui/dragndrop.win.h"

#include "ccl/base/asyncoperation.h"
#include "ccl/base/boxedtypes.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/objectconverter.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/imaging/multiimage.h"
#include "ccl/gui/graphics/graphicsdevice.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/gui.h"

#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

#include "ccl/platform/win/interfaces/iwin32graphics.h"
#include "ccl/platform/win/gui/oledragndrop.h"
#include "ccl/platform/win/gui/touchhelper.h"
#include "ccl/platform/win/gui/dpihelper.h"

using namespace CCL;

//************************************************************************************************
// DragSession
//************************************************************************************************

DragSession* DragSession::create (IUnknown* source, int inputDevice)
{
	return NEW WindowsDragSession (source, inputDevice);
}

//************************************************************************************************
// WindowsDragSession
//************************************************************************************************

DEFINE_CLASS (WindowsDragSession, DragSession)
DEFINE_CLASS_UID (WindowsDragSession, 0x5447ed24, 0x42cf, 0x43ed, 0x8a, 0x5b, 0xa9, 0x56, 0x4b, 0x93, 0xea, 0x5f) // ClassID::DragSession

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsDragSession::WindowsDragSession (IUnknown* source, int inputDevice)
: DragSession (source, inputDevice),
  dataObject (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsDragSession::WindowsDragSession (IDataObject* dataObject, int inputDevice)
: DragSession (inputDevice),
  dataObject (dataObject)
{
	convertNativeItems ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API WindowsDragSession::dragAsync ()
{
	AutoPtr<Win32::DropSource> source = NEW Win32::DropSource (this);
	AutoPtr<Win32::DataObject> dataObject = NEW Win32::DataObject (this);

	if(source->getHelper ())
	{
		AutoPtr<Bitmap> bitmap;

		if(dragImage)
		{
			Rect r;
			dragImage->getSize (r);

			// minimum icon size
			if(ccl_max (r.getWidth (), r.getHeight ()) < 32)
				r (0, 0, 32, 32);

			bitmap = NEW Bitmap (r.getWidth (), r.getHeight (), Bitmap::kRGBAlpha, Win32::gDpiInfo.getSystemDpiFactor ());

			// copy image to new bitmap...
			BitmapGraphicsDevice device (bitmap);
			ImageResolutionSelector::draw (device, dragImage, r);
		}

		// create transparent image because helper displays old image otherwise
		if(!bitmap)
			bitmap = NEW Bitmap (1, 1, Bitmap::kRGBAlpha);

		UnknownPtr<Win32::IWin32Bitmap> gdiBitmap (ccl_as_unknown (bitmap->getNativeBitmap ()));
		ASSERT (gdiBitmap != nullptr)
		if(gdiBitmap)
		{
			Point sizeInPixel (bitmap->getPixelSize ());
			SHDRAGIMAGE shdi = {0};
			shdi.sizeDragImage.cx = sizeInPixel.x;
			shdi.sizeDragImage.cy = sizeInPixel.y;
			shdi.ptOffset.x = sizeInPixel.x / 2;
			shdi.ptOffset.y = sizeInPixel.y;
			shdi.hbmpDragImage = gdiBitmap->detachHBITMAP ();
			shdi.crColorKey = CLR_NONE;

			HRESULT hr = source->getHelper ()->InitializeFromBitmap (&shdi, dataObject);
			SOFT_ASSERT ("Drag image could not be created", SUCCEEDED (hr))
			if(FAILED (hr))
				::DeleteObject (shdi.hbmpDragImage);  // helper takes ownership otherwise!
		}
	}

	GUI.hideTooltip ();

	DragGuard dragGuard (*this);

	if(inputDevice == kTouchInput)
	{
		if(Win32::TouchHelper::runDragLoop (*this))
			return AsyncOperation::createCompleted (getResult ());

		Win32::TouchHelper::setTouchDragging (true);
	}

	DWORD effect = 0;
	// Note: effect is not set correctly, therefore result is updated in DropSource::GiveFeedback().
	HRESULT result = ::DoDragDrop (dataObject, source, DROPEFFECT_COPY|DROPEFFECT_MOVE, &effect);
	if(result != DRAGDROP_S_DROP)
		setResult (kDropNone);

	if(inputDevice == kTouchInput)
		Win32::TouchHelper::setTouchDragging (false);

	return AsyncOperation::createCompleted (getResult ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsDragSession::convertNativeItems ()
{
	if(!dataObject)
		return;

	// check if we drag inside our application...
	Win32::ComUnknownPtr<Win32::IDataObjectPrivate> cclDataObject (dataObject);
	WindowsDragSession* otherSession = cclDataObject ? cclDataObject->getSession () : nullptr;
	if(otherSession)
	{
		this->sourceSession = otherSession;
		otherSession->setTargetSession (this);
		copyFrom (*otherSession);
		return; // we can exit here ;-)
	}

	// known formats:
	FORMATETC formatHDROP = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	FORMATETC formatUNICODE  = {CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	FORMATETC formatTEXT  = {CF_TEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

	STGMEDIUM medium = {0};

	// Try paths...
	if(dataObject->QueryGetData (&formatHDROP) == S_OK)
	{
		HRESULT result = dataObject->GetData (&formatHDROP, &medium);
		if(result == S_OK)
		{
			HDROP hDrop = (HDROP)medium.hGlobal;
			UINT count = ::DragQueryFile (hDrop, 0xFFFFFFFF, nullptr, 0);

			for(UINT i = 0; i < count; i++)
			{
				uchar path[Url::kMaxLength] = {0};
				::DragQueryFile (hDrop, i, path, Url::kMaxLength);

				int type = Url::kFile;
				DWORD attr = ::GetFileAttributes (path);
				if(attr != INVALID_FILE_ATTRIBUTES)
				{
					if(attr & FILE_ATTRIBUTE_DIRECTORY)
						type = Url::kFolder;
				}

				IUrl* url = NEW Url;
				url->fromNativePath (path, type);

				if(IUrl* packageUrl = System::GetFileUtilities ().translatePathInMountedFolder (*url))
				{
					url->release ();
					url = packageUrl;
				}
				items.add (url, false);
			}
		}
	}

	// Try Unicode Text...
	else if(dataObject->QueryGetData (&formatUNICODE) == S_OK)
	{
		HRESULT result = dataObject->GetData (&formatUNICODE, &medium);
		if(result == S_OK)
		{
			LPVOID text = ::GlobalLock (medium.hGlobal);
			int size = (int)::GlobalSize (medium.hGlobal);
			if(text && size > sizeof(uchar))
			{
				size -= sizeof(uchar); // exclude terminating null character

				// try to convert to an object...
				IUnknown* obj = ObjectConverter::instance ().importText (text, size, true);
				if(!obj)
				{
					Boxed::String* string = NEW Boxed::String;
					string->assign ((uchar*)text, size / sizeof(uchar));
					obj = ccl_as_unknown (string);
				}

				items.add (obj, false);
			}
		}
	}
	
	// Try ANSI text...
	else if(dataObject->QueryGetData (&formatTEXT) == S_OK)
	{
		HRESULT result = dataObject->GetData (&formatTEXT, &medium);
		if(result == S_OK)
		{
			LPVOID text = ::GlobalLock (medium.hGlobal);
			SIZE_T size = ::GlobalSize (medium.hGlobal);
			if(text && size > 1)
			{
				size--; // exclude terminating null character

				// try to convert to an object...
				IUnknown* obj = ObjectConverter::instance ().importText (text, (unsigned) size, false);
				if(!obj)
				{
					Boxed::String* string = NEW Boxed::String;
					string->appendCString (Text::kSystemEncoding, (const char*)text, (unsigned)size);
					obj = ccl_as_unknown (string);
				}

				items.add (obj, false);
			}
		}
	}
	#if DEBUG_LOG
	else
	{
		// Shell Clipboard Formats
		// http://msdn.microsoft.com/en-us/library/bb776902%28VS.85%29.aspx

		const WCHAR* formatNames[] =
		{
			CFSTR_SHELLIDLIST,
			CFSTR_SHELLIDLISTOFFSET,
			CFSTR_NETRESOURCES,
			CFSTR_FILEDESCRIPTORA,
			CFSTR_FILEDESCRIPTORW,
			CFSTR_FILECONTENTS,
			CFSTR_FILENAMEA,
			CFSTR_FILENAMEW,
			CFSTR_PRINTERGROUP,
			CFSTR_FILENAMEMAPA,
			CFSTR_FILENAMEMAPW,
			CFSTR_SHELLURL,
			CFSTR_INETURLA,
			CFSTR_INETURLW,
			CFSTR_PREFERREDDROPEFFECT,
			CFSTR_PERFORMEDDROPEFFECT,
			CFSTR_PASTESUCCEEDED,
			CFSTR_INDRAGLOOP,
			CFSTR_MOUNTEDVOLUME,
			CFSTR_PERSISTEDDATAOBJECT,
			CFSTR_TARGETCLSID,
			CFSTR_LOGICALPERFORMEDDROPEFFECT,
			CFSTR_AUTOPLAY_SHELLIDLISTS,
			CFSTR_UNTRUSTEDDRAGDROP,
			CFSTR_FILE_ATTRIBUTES_ARRAY,
			CFSTR_INVOKECOMMAND_DROPPARAM,
			CFSTR_SHELLDROPHANDLER,
			CFSTR_DROPDESCRIPTION
		};

		// Note: CFSTR_SHELLURL is the same as CF_TEXT describing the URL
		// e.g. when dragging a single FTP file from Windows Explorer.

		// check which formats are available
		Win32::ComPtr<IEnumFORMATETC> enumerator;
		dataObject->EnumFormatEtc (DATADIR_GET, enumerator);
		if(enumerator)
		{
			FORMATETC formats[1] = {0};
			int index = 0;
			while(enumerator->Next (1, formats, nullptr) == S_OK)
			{
				UINT cfFormat = formats[0].cfFormat;

				// compare with shell clipboard formats
				const WCHAR* cfFormatName = L"Unknown";
				if(cfFormat >= 0xC000 && cfFormat <= 0xFFFF)
					for(int i = 0; i < ARRAY_COUNT (formatNames); i++)
						if(cfFormat == ::RegisterClipboardFormat (formatNames[i]))
						{
							cfFormatName = formatNames[i];
							break;
						}

				CCL_PRINTF ("Supported Format %d: %04x (%ws)\n", ++index, (int)cfFormat, cfFormatName)
			}
		}
	}
	#endif

	// cleanup
	::ReleaseStgMedium (&medium);
}
