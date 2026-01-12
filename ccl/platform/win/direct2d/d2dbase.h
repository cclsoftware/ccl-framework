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
// Filename    : ccl/platform/win/direct2d/d2dbase.h
// Description : Direct2D Base Classes
//
//************************************************************************************************

#ifndef _ccl_direct2d_base_h
#define _ccl_direct2d_base_h

#include "ccl/platform/win/direct2d/d2dinterop.h"

#include "ccl/public/collections/intrusivelist.h"

#include "ccl/public/system/threadsync.h"

namespace CCL {
namespace Win32 {

class DXGIEngine;

//************************************************************************************************
// D2DResource
//************************************************************************************************

class D2DResource: public IntrusiveLink<D2DResource>
{
public:
	D2DResource ()
	: registered (false)
	{}

	~D2DResource ()
	{
		setRegistered (false);
	}

	virtual void discardDirect2dResource (bool isShutdown) = 0;

	static void discardAll (bool isShutdown)
	{
		IntrusiveListForEach (getList (), D2DResource, resource)
			resource->discardDirect2dResource (isShutdown);
		EndFor
	}

protected:
	void setRegistered (bool state)
	{
		if(state != registered)
		{
			Threading::ScopedLock lock (getCriticalSection ()); // must protect the list

			if(registered)
				getList ().remove (this);
			registered = state;
			if(registered)
				getList ().append (this);			
		}
	}

private:
	bool registered;

	typedef IntrusiveLinkedList<D2DResource> ResourceList;
	static ResourceList& getList ()
	{
		static ResourceList theResourceList;
		return theResourceList;
	}
	static Threading::CriticalSection& getCriticalSection ()
	{
		static Threading::CriticalSection criticalSection;
		return criticalSection;
	}
};

//************************************************************************************************
// D2DRenderTarget
//************************************************************************************************

class D2DRenderTarget
{
public:
	D2DRenderTarget (ID2D1DeviceContext* ownDeviceContext = nullptr);
	virtual ~D2DRenderTarget ();

	bool hasOutputImage () const { return outputImage.isValid (); }
	void setActive (D2DClientRenderDevice& device, bool state);

	void beginDraw ();
	HRESULT endDraw ();

	bool isValid () const				{ return target.isValid (); }
	ID2D1RenderTarget* getTarget ()		{ return target; }
	ID2D1RenderTarget* operator -> ()	{ return target; }
	ID2D1DeviceContext* getContext ();

	bool isGdiCompatible () const;
	ID2D1GdiInteropRenderTarget* getGdiTarget () { return gdiTarget; }

	ID2D1Brush* getBrushForColor (ColorRef color);
	ID2D1Brush* getUnderlyingBrush (BrushRef brush);
	ID2D1Brush* getBrushForPen (PenRef pen);
	ID2D1StrokeStyle* getStyleForPen (PenRef pen);

	virtual bool isAlphChannelUsed () const = 0;
	virtual float getContentScaleFactor () const = 0;
	
	float getDpi () const;
	D2D1_TEXT_ANTIALIAS_MODE getDefaultTextAntialiasMode () const;

protected:
	DXGIEngine& engine;
	ID2D1DeviceContext* ownDeviceContext;
	ComPtr<ID2D1RenderTarget> target;
	ComPtr<ID2D1GdiInteropRenderTarget> gdiTarget;

	ComPtr<ID2D1Image> outputImage;
	ComPtr<ID2D1Image> oldOutputImage;
	ComPtr<ID2D1DrawingStateBlock> oldDrawingState;
	D2DClientRenderDevice* oldClientDevice;
	float oldDpi;
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_direct2d_base_h
