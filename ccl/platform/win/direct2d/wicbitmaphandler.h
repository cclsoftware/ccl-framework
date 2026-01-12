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
// Filename    : ccl/platform/win/direct2d/wicbitmaphandler.h
// Description : WIC (Windows Imaging Component) Bitmap Loader / Saver
//
//************************************************************************************************

#ifndef _ccl_win32_wicbitmaphandler_h
#define _ccl_win32_wicbitmaphandler_h

#include "ccl/base/singleton.h"

#include "ccl/platform/win/system/cclcom.h"

#include "ccl/public/gui/graphics/ibitmap.h"

#include <wincodec.h>

namespace CCL {
interface IStream;
class FileType;

namespace Win32 {

//************************************************************************************************
// WICBitmapHandler
//************************************************************************************************

class WICBitmapHandler: public Object,
					    public Singleton<WICBitmapHandler>
{
public:
	WICBitmapHandler ();
	~WICBitmapHandler ();

	IWICBitmapSource* createSourceFromStream (CCL::IStream& stream);
	IWICBitmap* createBitmap (int width, int height);
	IWICBitmap* createBitmapFromSource (IWICBitmapSource* bitmapSource);
	IWICBitmapSource* createClippedSource (IWICBitmapSource* bitmapSource, const Rect& rect);

	bool lockBitmap (BitmapLockData& data, IWICBitmap* bitmap, int mode);
	bool unlockBitmap (BitmapLockData& data);
	bool scrollBitmap (IWICBitmap* bitmap, RectRef rect, PointRef delta);
	bool copyBitmap (IWICBitmap* dstBitmap, IWICBitmapSource* bitmapSource);
		
	IWICBitmap* createBitmapFromHBITMAP (HBITMAP hBitmap);
	HRESULT createDIBSectionFromBitmapSource (HBITMAP& hDIBBitmap, IWICBitmapSource* bitmapSource);

	bool saveToStream (IStream& stream, IWICBitmapSource* bitmap, const FileType& format);

	IWICImagingFactory* getFactory () { return factory; }
	
private:
	ComPtr<IWICImagingFactory> factory;

	bool getEncoderInfoByFormat (ComPtr<IWICBitmapEncoderInfo>& result, const FileType& format);
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_win32_wicbitmaploader_h
