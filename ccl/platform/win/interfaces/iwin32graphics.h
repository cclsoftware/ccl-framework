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
// Filename    : ccl/platform/win/interfaces/iwin32graphics.h
// Description : Win32 Graphics Interface
//
//************************************************************************************************

#ifndef _ccl_iwin32graphics_h
#define _ccl_iwin32graphics_h

#include "ccl/public/base/iunknown.h"

#include "ccl/platform/win/cclwindows.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// IWin32Graphics
//************************************************************************************************

interface IWin32Graphics: CCL::IUnknown
{
	virtual HDC CCL_API getHDC () = 0;
	
	virtual void CCL_API releaseHDC (HDC hdc, const RECT* rect = nullptr) = 0;
	
	DECLARE_IID (IWin32Graphics)
};

//************************************************************************************************
// IWin32Bitmap
//************************************************************************************************

interface IWin32Bitmap: CCL::IUnknown
{
	virtual tbool CCL_API isAlphaPixelFormat () const = 0;

	virtual HBITMAP CCL_API getHBITMAP () = 0;

	/**	Detach HBITMAP, i.e. the caller takes ownership.
		The IWin32Bitmap object will be unusable by others afterwards. */
	virtual HBITMAP CCL_API detachHBITMAP () = 0;

	DECLARE_IID (IWin32Bitmap)
};

//************************************************************************************************
// HDCGetter
//************************************************************************************************

struct HDCGetter
{
	IWin32Graphics& graphics;
	HDC hdc;
	RECT rect;

	HDCGetter (IWin32Graphics& graphics, const RECT& rect)
	: graphics (graphics),
	  hdc (graphics.getHDC ()),
	  rect (rect)
	{}

	~HDCGetter ()
	{
		graphics.releaseHDC (hdc, &rect);
	}

	operator HDC () { return hdc; }
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_iwin32graphics_h
