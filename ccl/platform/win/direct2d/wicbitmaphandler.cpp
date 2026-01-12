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
// Filename    : ccl/platform/win/direct2d/wicbitmaphandler.cpp
// Description : WIC (Windows Imaging Component) Bitmap Handler
//
//************************************************************************************************

#include "ccl/platform/win/direct2d/wicbitmaphandler.h"

#include "ccl/platform/win/system/comstream.h"

#include "ccl/public/gui/graphics/rect.h"

#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/storage/filetype.h"
#include "ccl/public/base/buffer.h"

#include "core/gui/corebitmapprimitives.h"

#ifndef ARGB
typedef DWORD ARGB;
#endif

#define CCL_WICPixelFormat_RGBAlpha GUID_WICPixelFormat32bppPBGRA

#pragma comment (lib, "windowscodecs.lib")

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// WICBitmapHandler
//************************************************************************************************

DEFINE_SINGLETON (WICBitmapHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

WICBitmapHandler::WICBitmapHandler ()
{
	// Note: windowscodecs.dll is available since Windows XP
	HRESULT hr = ::CoCreateInstance (CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, factory);
	ASSERT (SUCCEEDED (hr))
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WICBitmapHandler::~WICBitmapHandler ()
{
	factory.release (); // place for a breakpoint
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWICBitmapSource* WICBitmapHandler::createSourceFromStream (CCL::IStream& stream)
{
	ASSERT (factory.isValid ())
	if(!factory)
		return nullptr;

	AutoPtr<CCL::Win32::ComStream> comStream = NEW CCL::Win32::ComStream (&stream);

	// create decoder
	ComPtr<IWICBitmapDecoder> decoder;
	HRESULT hr = factory->CreateDecoderFromStream (comStream, nullptr, WICDecodeMetadataCacheOnDemand, decoder);
	SOFT_ASSERT (SUCCEEDED (hr), "Failed to create image decoder from stream!\n")
	if(!decoder)
		return nullptr;

	// retrieve the first frame of the image from the decoder
	ComPtr<IWICBitmapFrameDecode> frame;
	hr = decoder->GetFrame (0, frame);
	ASSERT (SUCCEEDED (hr))
	if(!frame)
		return nullptr;

	// convert the source to 32 bpp RGBA (converter is an IWICBitmapSource itself)
	ComPtr<IWICFormatConverter> converter;
	hr = factory->CreateFormatConverter (converter);
	ASSERT (SUCCEEDED (hr))
	if(!converter)
		return nullptr;

	hr = converter->Initialize (frame, CCL_WICPixelFormat_RGBAlpha, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom);
	ASSERT (SUCCEEDED (hr))
	if(FAILED (hr))
		return nullptr;

	return converter.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWICBitmap* WICBitmapHandler::createBitmap (int width, int height)
{
	ASSERT (factory.isValid ())
	if(!factory)
		return nullptr;

	ComPtr<IWICBitmap> bitmap;
	HRESULT hr = factory->CreateBitmap (width, height, CCL_WICPixelFormat_RGBAlpha, WICBitmapCacheOnLoad, bitmap);
	ASSERT (SUCCEEDED (hr))
	if(FAILED (hr))
		return nullptr;

	return bitmap.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWICBitmap* WICBitmapHandler::createBitmapFromSource (IWICBitmapSource* bitmapSource)
{
	ASSERT (factory.isValid ())
	if(!factory)
		return nullptr;

	ComPtr<IWICBitmap> bitmap;
	HRESULT hr = factory->CreateBitmapFromSource (bitmapSource, WICBitmapCacheOnLoad, bitmap);
	ASSERT (SUCCEEDED (hr))
	if(FAILED (hr))
		return nullptr;

	return bitmap.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWICBitmapSource* WICBitmapHandler::createClippedSource (IWICBitmapSource* bitmapSource, const Rect& rect)
{
	ASSERT (factory.isValid ())
	if(!factory)
		return nullptr;

	ComPtr<IWICBitmapClipper> clipper;
	HRESULT hr = factory->CreateBitmapClipper (clipper);
	ASSERT (SUCCEEDED (hr))
	if(FAILED (hr))
		return nullptr;

	WICRect wiRect = {rect.left, rect.top, rect.getWidth (), rect.getHeight ()};

	hr = clipper->Initialize (bitmapSource, &wiRect);
	if(FAILED (hr))
		return nullptr;

	return clipper.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WICBitmapHandler::lockBitmap (BitmapLockData& data, IWICBitmap* bitmap, int mode)
{
	if(!bitmap)
		return false;

	UINT width = 0, height = 0;
	bitmap->GetSize (&width, &height);
	WICRect rect = {0, 0, (INT)width, (INT)height};

	ComPtr<IWICBitmapLock> bitmapLock;
	DWORD flags = WICBitmapLockRead;
	if(mode == IBitmap::kLockWrite)
		flags |= WICBitmapLockWrite;
	HRESULT hr = bitmap->Lock (&rect, flags, bitmapLock);
	ASSERT (SUCCEEDED (hr))
	if(FAILED (hr))
		return false;

	UINT bufferSize = 0;
	BYTE* dataPointer = nullptr;
	hr = bitmapLock->GetDataPointer (&bufferSize, &dataPointer);
	ASSERT (SUCCEEDED (hr))

	UINT stride = 0;
	hr = bitmapLock->GetStride (&stride);
	ASSERT (SUCCEEDED (hr))

	// fill BitmapData
	data.width = (int)width;
	data.height = (int)height;
	data.format = IBitmap::kRGBAlpha;
	data.scan0 = dataPointer;
	data.rowBytes = stride;
	data.bitsPerPixel = 32;
	data.mode = mode;
	data.nativeData = bitmapLock.detach ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WICBitmapHandler::unlockBitmap (BitmapLockData& data)
{
	IWICBitmapLock* bitmapLock = reinterpret_cast<IWICBitmapLock*> (data.nativeData);
	ASSERT (bitmapLock != nullptr)
	if(bitmapLock == nullptr)
		return false;

	bitmapLock->Release ();
	data.nativeData = nullptr;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WICBitmapHandler::scrollBitmap (IWICBitmap* bitmap, RectRef rect, PointRef delta)
{
	BitmapLockData data;
	if(!lockBitmap (data, bitmap, IBitmap::kLockWrite))
		return false;

	Core::BitmapPrimitives32::scrollRect (data, rect, delta);

	unlockBitmap (data);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WICBitmapHandler::copyBitmap (IWICBitmap* dstBitmap, IWICBitmapSource* bitmapSource)
{
	// check bitmap compatibility
	UINT dstWidth = 0, dstHeight = 0, srcWidth = 0, srcHeight = 0;
	dstBitmap->GetSize (&dstWidth, &dstHeight);
	bitmapSource->GetSize (&srcWidth, &srcHeight);
	ASSERT (dstWidth == srcWidth && dstHeight == srcHeight)
	if(dstWidth != srcWidth || dstHeight != srcHeight)
		return false;

	WICPixelFormatGUID dstFormat = {0}, srcFormat = {0};
	dstBitmap->GetPixelFormat (&dstFormat);
	bitmapSource->GetPixelFormat (&srcFormat);
	ASSERT (dstFormat == srcFormat)
	if(dstFormat != srcFormat)
		return false;

	ComPtr<IWICBitmapLock> bitmapLock;
	HRESULT hr = dstBitmap->Lock (nullptr, WICBitmapLockWrite, bitmapLock);
	ASSERT (SUCCEEDED (hr))
	if(FAILED (hr))
		return false;

	UINT bufferSize = 0;
	BYTE* dataPointer = nullptr;
	hr = bitmapLock->GetDataPointer (&bufferSize, &dataPointer);
	ASSERT (SUCCEEDED (hr))
	UINT stride = 0;
	hr = bitmapLock->GetStride (&stride);
	ASSERT (SUCCEEDED (hr))

	hr = bitmapSource->CopyPixels (nullptr, stride, bufferSize, dataPointer);
	ASSERT (SUCCEEDED (hr))
	return SUCCEEDED (hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWICBitmap* WICBitmapHandler::createBitmapFromHBITMAP (HBITMAP hBitmap)
{
	ComPtr<IWICBitmap> bitmap;
	factory->CreateBitmapFromHBITMAP (hBitmap, NULL, WICBitmapUsePremultipliedAlpha, bitmap);
	return bitmap.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT WICBitmapHandler::createDIBSectionFromBitmapSource (HBITMAP& hDIBBitmap, IWICBitmapSource* pToRenderBitmapSource)
{
	// Copied from WICViewerGDI Windows 7 SDK Sample:

	HRESULT hr = S_OK;
	hDIBBitmap = nullptr;

    // Get image attributes and check for valid image
    UINT width = 0;
    UINT height = 0;
    void* pvImageBits = nullptr;

    // Check BitmapSource format
    WICPixelFormatGUID pixelFormat = {};
    hr = pToRenderBitmapSource->GetPixelFormat (&pixelFormat);

    if(SUCCEEDED (hr))
    {
        hr = (pixelFormat == CCL_WICPixelFormat_RGBAlpha) ? S_OK : E_FAIL;
		#if DEBUG
		if(FAILED (hr))
			CCL_DEBUGGER ("Unsupported pixel format!")
		#endif
    }

    if(SUCCEEDED (hr))
    {
        hr = pToRenderBitmapSource->GetSize (&width, &height);
    }

    // Create a DIB section based on Bitmap Info
    // BITMAPINFO Struct must first be setup before a DIB can be created.
    // Note that the height is negative for top-down bitmaps
    if(SUCCEEDED (hr))
    {
        BITMAPINFO bminfo;
        ZeroMemory (&bminfo, sizeof(bminfo));
        bminfo.bmiHeader.biSize         = sizeof(BITMAPINFOHEADER);
        bminfo.bmiHeader.biWidth        = width;
        bminfo.bmiHeader.biHeight       = -(LONG)height;
        bminfo.bmiHeader.biPlanes       = 1;
        bminfo.bmiHeader.biBitCount     = 32;
        bminfo.bmiHeader.biCompression  = BI_RGB;

        // Get a DC for the full screen
        HDC hdcScreen = NULL;//GetDC (NULL);

        //hr = hdcScreen ? S_OK : E_FAIL;

        if(SUCCEEDED (hr))
        {
            hDIBBitmap = CreateDIBSection (hdcScreen, &bminfo, DIB_RGB_COLORS,
                &pvImageBits, NULL, 0);

            //ReleaseDC (NULL, hdcScreen);

            hr = hDIBBitmap ? S_OK : E_FAIL;
        }
    }

    UINT cbStride = 0;
    if(SUCCEEDED (hr))
    {
        // Size of a scan line represented in bytes: 4 bytes each pixel
        hr = UIntMult (width, sizeof(ARGB), &cbStride);
    }

    UINT cbImage = 0;
    if(SUCCEEDED (hr))
    {
        // Size of the image, represented in bytes
        hr = UIntMult (cbStride, height, &cbImage);
    }

    // Extract the image into the HBITMAP
    if(SUCCEEDED (hr))
    {
        hr = pToRenderBitmapSource->CopyPixels (
            nullptr,
            cbStride,
            cbImage,
            reinterpret_cast<BYTE*>(pvImageBits));
    }

    // Image Extraction failed, clear allocated memory
    if(FAILED (hr))
    {
        ::DeleteObject (hDIBBitmap);
        hDIBBitmap = NULL;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WICBitmapHandler::saveToStream (CCL::IStream& stream, IWICBitmapSource* wicBitmap, const FileType& format)
{
	ComPtr<IWICBitmapEncoderInfo> encoderInfo;
	if(getEncoderInfoByFormat (encoderInfo, format) == false)
		return false;

	ComPtr<IWICBitmapEncoder> encoder;
	HRESULT hr = encoderInfo->CreateInstance (encoder);
    if(FAILED(hr) || encoder == 0)
		return false;

	AutoPtr<CCL::IStream> seekableStream = System::GetFileUtilities ().createSeekableStream (stream, true);

	AutoPtr<CCL::Win32::ComStream> comStream = NEW CCL::Win32::ComStream (seekableStream);
	hr = encoder->Initialize (comStream, WICBitmapEncoderNoCache);

	ComPtr<IWICBitmapFrameEncode> frameEncode;
	encoder->CreateNewFrame (frameEncode, nullptr);
	if(frameEncode == 0)
		return false;

	hr = frameEncode->Initialize (nullptr);
	if(SUCCEEDED (hr))
		hr = frameEncode->WriteSource (wicBitmap, nullptr);

	if(SUCCEEDED (hr))
		hr = frameEncode->Commit ();

	frameEncode = nullptr;
	if(SUCCEEDED (hr))
		hr = encoder->Commit ();

	return SUCCEEDED (hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WICBitmapHandler::getEncoderInfoByFormat (ComPtr<IWICBitmapEncoderInfo>& result, const FileType& format)
{
	ComPtr<IEnumUnknown> enumerator;
	HRESULT hr = factory->CreateComponentEnumerator (WICEncoder, WICComponentEnumerateDefault, enumerator);
    if(FAILED(hr))
		return false;

	if(enumerator)
	{
		enumerator->Reset ();
		Array<WCHAR> charBuffer (0, false);
		UINT strLength = 0;

		while(true)
		{
			ULONG n = 0;
			ComPtr<::IUnknown> unknown;

			enumerator->Next (1, unknown, &n);
			if(n == 0)
				break;

			if(unknown)
			{
				ComPtr<IWICBitmapEncoderInfo> encoderInfo;
				unknown->QueryInterface (__uuidof (IWICBitmapEncoderInfo), encoderInfo);
				if(encoderInfo)
				{
					//  encoderInfo->MatchesMimeType (mimeType, &matches); is not implemented in most cases... so:
					if(format.getMimeType ().isEmpty () == false)
					{
						strLength = 0;
						hr = encoderInfo->GetMimeTypes (0,nullptr, &strLength);
						if(SUCCEEDED(hr) && strLength > 0)
						{
							if(charBuffer.getSize () < strLength + 1)
								charBuffer.resize (strLength + 1);
							if(SUCCEEDED (encoderInfo->GetMimeTypes (strLength, charBuffer, &strLength)))
							{
								String mimeTypes (charBuffer.getAddress ());
								if(mimeTypes.contains (format.getMimeType ()))
								{
									result = encoderInfo;
									return true;
								}
							}
						}
					}

					if(format.getExtension ().isEmpty () == false)
					{
						// try by extension as well...
						strLength = 0;
						hr = encoderInfo->GetFileExtensions (0,nullptr, &strLength);
						if(SUCCEEDED(hr) && strLength > 0)
						{
							if(charBuffer.getSize () < strLength + 1)
								charBuffer.resize (strLength + 1);
							if(SUCCEEDED (encoderInfo->GetFileExtensions (strLength, charBuffer, &strLength)))
							{
								String extensions (charBuffer.getAddress ());
								if(extensions.contains (format.getExtension ()))
								{
									result = encoderInfo;
									return true;
								}
							}
						}
					}
				}
			}
		}
	}

	return false;
}
