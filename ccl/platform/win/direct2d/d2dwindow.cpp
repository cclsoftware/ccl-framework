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
// Filename    : ccl/platform/win/direct2d/d2dwindow.cpp
// Description : Direct2D Window Render Target
//
//************************************************************************************************

#define DEBUG_LOG 0
#define D2D_DIRECTUPDATES_ENABLED 1

#include "ccl/platform/win/direct2d/d2dwindow.h"
#include "ccl/platform/win/direct2d/d3dsupport.h"
#include "ccl/platform/win/gui/win32graphics.h"

#include "ccl/gui/windows/nativewindow.h"

#include "ccl/base/message.h"

#include "ccl/public/base/buffer.h"
#include "ccl/public/gui/framework/itimer.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/systemservices.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// D2DWindowUpdater
//************************************************************************************************

class D2DWindowUpdater: public Object,
						public ITimerTask,
						public StaticSingleton<D2DWindowUpdater>
{
public:
	D2DWindowUpdater ()
	: lastFlushTime (0.)
	{}

	void addTarget (D2DWindowRenderTarget* target)
	{
		#if D2D_DIRECTUPDATES_ENABLED
		if(targets.isEmpty ())
			System::GetGUI ().addIdleTask (this);
		#endif

		targets.add (target);
	}

	void removeTarget (D2DWindowRenderTarget* target)
	{
		targets.remove (target);

		if(targets.isEmpty ())
		{
			cancelSignals ();
			#if D2D_DIRECTUPDATES_ENABLED
			System::GetGUI ().removeIdleTask (this);
			#endif
		}
	}

	void discardAll ()
	{
		VectorForEachFast (targets, D2DWindowRenderTarget*, target)
			target->discardSwapChain ();
			target->getWindow ().hasBeenDrawn (false); // reset state
		EndFor

		// invalidation must be deferred, because we might be in a draw call!
		(NEW Message ("invalidateAll"))->post (this);
	}

	// ITimerTask
	void CCL_API onTimer (ITimer* timer) override
	{
		static const double kPeriod = 1. / 60.; // 60 FPS

		double now = System::GetProfileTime ();
		if(now > lastFlushTime + kPeriod)
		{
			VectorForEachFast (targets, D2DWindowRenderTarget*, target)
				if(target->isFlushNeeded ())
					target->flush ();
			EndFor
			lastFlushTime = now;
		}
	}

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override
	{
		if(msg == "invalidateAll")
		{
			VectorForEachFast (targets, D2DWindowRenderTarget*, target)
				// force bitmap recreation
				float dpiFactor = target->getWindow ().getContentScaleFactor ();
				target->getWindow ().onDisplayPropertiesChanged (DisplayChangedEvent (dpiFactor, DisplayChangedEvent::kResolutionChanged));

				target->invalidate ();
			EndFor
		}
	}

	CLASS_INTERFACE (ITimerTask, Object)

protected:
	Vector<D2DWindowRenderTarget*> targets;
	double lastFlushTime;
};

} // namespace Win32
} // namespace CCL

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// D2DWindowRenderTarget
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (D2DWindowRenderTarget, NativeWindowRenderTarget)

//////////////////////////////////////////////////////////////////////////////////////////////////

D2DWindowRenderTarget::D2DWindowRenderTarget (Window& window)
: NativeWindowRenderTarget (window),
  flushNeeded (false),
  updateRegion (NEW GdiClipRegion)
{
	D2DWindowUpdater::instance ().addTarget (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

D2DWindowRenderTarget::~D2DWindowRenderTarget ()
{
	D2DWindowUpdater::instance ().removeTarget (this);

	discardSwapChain ();

	updateRegion->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DWindowRenderTarget::discardSwapChain ()
{
	for(D3DSurface* surface : surfaces)
		surface->destroy ();

	outputImage.release ();
	swapChainBitmap.release ();
	swapChain.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D2DWindowRenderTarget::isAlphChannelUsed () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float D2DWindowRenderTarget::getContentScaleFactor () const
{
	return window.getContentScaleFactor ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point D2DWindowRenderTarget::getPixelSize () const
{
	return PixelPoint (Point (window.getWidth (), window.getHeight ()), window.getContentScaleFactor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D2DWindowRenderTarget::isDirectUpdateEnabled () const
{
	return D2D_DIRECTUPDATES_ENABLED;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DWindowRenderTarget::handleError (CStringPtr message, HRESULT hr)
{
	// handle device removal
	if(hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		D2DWindowUpdater::instance ().discardAll ();

		NativeGraphicsEngine::instance ().recoverFromError (); // calls reportError		
	}
	else
		engine.reportError (message, hr, true);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DWindowRenderTarget::present (const DXGI_PRESENT_PARAMETERS* params)
{
	#define SYNC_INTERVAL 0 // SyncInterval=0 offloads CPU usage to DWM. Why???

	if(swapChain)
	{
		HRESULT hr = params ?
					swapChain->Present1 (SYNC_INTERVAL, 0, params) :
					swapChain->Present (SYNC_INTERVAL, 0);

		if(FAILED (hr))
			handleError ("Swap chain present failed", hr);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DWindowRenderTarget::flush ()
{
	setFlushNeeded (false);

	if(engine.isFlipModel ())
	{
		GdiClipRegion::RectList rectList (*updateRegion);
		if(rectList.rectCount == 0 && scrollRect.isEmpty ())
			return;

		Rect maxRect;
		window.getClientRect (maxRect);
		for(int i = 0; i < rectList.rectCount; i++)
			rectList.rects[i].bound (maxRect);

		rectList.removeEmptyRects ();
		if(rectList.rectCount > 0 || !scrollRect.isEmpty ())
		{
			DXGI_PRESENT_PARAMETERS params = {};
			if(rectList.rectCount > 0)
			{
				rectList.adjustToPixels (getContentScaleFactor ());

				params.DirtyRectsCount = rectList.rectCount;
				params.pDirtyRects = reinterpret_cast<RECT*> (rectList.rects);
			}

			if(!scrollRect.isEmpty ())
			{
				params.pScrollRect = reinterpret_cast<RECT*> (&scrollRect);
				params.pScrollOffset = reinterpret_cast<POINT*> (&scrollOffset);
			}

			//CCL_PROFILE_START (presentInFlush)
			present (&params);
			//CCL_PROFILE_STOP (presentInFlush)

			scrollRect.setEmpty ();
		}
		updateRegion->removeAll ();
	}
	else
		present ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D2DWindowRenderTarget::shouldCollectUpdates ()
{
	return isDirectUpdateEnabled () == false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMutableRegion* D2DWindowRenderTarget::getUpdateRegion ()
{
	setFlushNeeded (true);

	if(engine.isFlipModel ())
		return updateRegion;
	else
		return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D2DWindowRenderTarget::makeSwapChainBitmap (PointRef sizeInPixel)
{
	if(!swapChain)
		return false;

	ASSERT (!swapChainBitmap)
	ASSERT (sizeInPixel.x > 0 && sizeInPixel.y > 0)

	HRESULT hr = swapChain->ResizeBuffers (0, sizeInPixel.x, sizeInPixel.y, DXGI_FORMAT_UNKNOWN, engine.getSwapChainFlags ());
	if(FAILED (hr))
	{
		handleError (MutableCString ().appendFormat ("Swap chain resize to %ix%ipx failed", sizeInPixel.x, sizeInPixel.y), hr);
		return false;
	}

	swapChainBitmap = engine.createBitmapForSwapChain (swapChain);
	ASSERT (swapChainBitmap)
	if(!swapChainBitmap)
		return false;

	outputImage.share (swapChainBitmap);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DWindowRenderTarget::onRender ()
{
	HWND hwnd = (HWND)window.getSystemWindow ();
	ASSERT (hwnd != NULL)

	if(!swapChain.isValid ()) // create swap chain on first call
	{
		swapChain = engine.createSwapChainForWindow (hwnd);
		if(!swapChain.isValid ())
			return;

		if(!makeSwapChainBitmap (getPixelSize ()))
			return;
	}

	// render 3D content to offscreen bitmaps
	render3DContent ();

	// render 2D content - copy update region before BeginPaint()!
	Win32::GdiClipRegion renderRegion (hwnd);

	PAINTSTRUCT ps;
	::BeginPaint (hwnd, &ps);

	CCL_PROFILE_START (drawWindow)
	render (renderRegion);
	CCL_PROFILE_STOP (drawWindow)

	::EndPaint (hwnd, &ps);

	flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DWindowRenderTarget::render3DContent ()
{
	for(auto surface : surfaces)
	{
		if(!surface->isValid () && !surface->create (getContentScaleFactor ()))
			continue;
		
		if(!surface->isDirty ())
			continue;

		if(auto content = surface->getContent ())
		{
			D3DGraphicsContext context (*surface);
			content->renderContent (context);
		}

		surface->setDirty (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DWindowRenderTarget::render (GdiClipRegion& renderRegion)
{
	AutoPtr<NativeGraphicsDevice> nativeDevice = ensureGraphicsDevice (NativeGraphicsEngine::instance ().createWindowDevice (&window));
	AutoPtr<GraphicsDevice> graphicsDevice = NEW WindowGraphicsDevice (window, nativeDevice);
	window.setGraphicsDevice (graphicsDevice);

	// make sure to render 2D content behind transparent 3D surfaces
	for(auto surface : surfaces)
	{
		if(surface->getContent () == nullptr || surface->getContent ()->getContentHint () != kGraphicsContentTranslucent)
			continue;
		RectRef surfaceRect = surface->getViewPortRect ();
		if(renderRegion.rectVisible (surfaceRect))
			renderRegion.addRect (surfaceRect);
	}

	GdiClipRegion::RectList rectList (renderRegion);
	rectList.adjustToCoords (getContentScaleFactor ());
	for(int i = 0; i < rectList.rectCount; i++)
	{
		RectRef rect = rectList.rects [i];
		graphicsDevice->saveState ();
		graphicsDevice->addClip (rect);

		#if (0 && DEBUG) // to see which areas aren't cleared
		graphicsDevice->fillRect (rect, SolidBrush (Colors::kRed));
		#endif

		window.draw (UpdateRgn (rect));
		graphicsDevice->restoreState ();
	}

	// blend prerendered 3D surfaces to back buffer
	for(auto surface : surfaces)
	{
		if(renderRegion.rectVisible (surface->getViewPortRect ()))
			surface->blendToBackbuffer (getContext ());
	}

	// add rects to update region
	if(engine.isFlipModel ())
		updateRegion->addRectList (rectList);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DWindowRenderTarget::onSize ()
{
	if(!swapChain)
		return;

	Point sizeInPixel = getPixelSize ();
	ASSERT (sizeInPixel.x > 0 && sizeInPixel.y > 0)

	if(swapChainBitmap)
	{
		D2D1_SIZE_U currentSize = swapChainBitmap->GetPixelSize ();
		if(currentSize.width == sizeInPixel.x && currentSize.height == sizeInPixel.y)
			return;
	}

	CCL_PRINTF ("[Direct2D] window target resized : width = %d  height = %d [pixel]\n", sizeInPixel.x, sizeInPixel.y)

	for(D3DSurface* surface : surfaces)
		surface->destroy ();

	outputImage.release ();
	swapChainBitmap.release ();

	if(makeSwapChainBitmap (sizeInPixel))
		invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DWindowRenderTarget::invalidate ()
{
	// discard pending updates, full redraw follows
	setFlushNeeded (false);
	
	updateRegion->removeAll ();

	window.invalidate ();

	for(auto surface : surfaces)
		surface->setDirty (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DWindowRenderTarget::onScroll (RectRef inRect, PointRef inDelta)
{
	CCL_PRINTLN ("[Direct2D] window target scrolled")

	if(swapChain)
	{
		#if DEBUG_LOG
		dumpRect (inRect, "Rect to scroll");
		CCL_PRINTF ("delta x = %d y = %d\n", inDelta.x, inDelta.y)
		#endif

		Rect rect (inRect);
		Point delta (inDelta);

		float scaleFactor = getContentScaleFactor ();
		bool fractionalScaling = DpiScale::isIntAligned (scaleFactor) == false;
		if(fractionalScaling)
		{
			PixelRectF rectF (rect, scaleFactor);
			PixelPointF deltaF (delta, scaleFactor);
			if(rectF.isPixelAligned () == false || deltaF.isPixelAligned () == false)
			{
				// cannot scroll fractional pixels
				Rect r (rect);
				r.offset (delta);
				r.join (rect);
				window.invalidate (r);
				return;
			}
			rect = rectFToInt (rectF);
			delta = pointFToInt (deltaF);
		}
		else
		{
			DpiScale::toPixelRect (rect, scaleFactor);
			DpiScale::toPixelPoint (delta, scaleFactor);
		}

		if(engine.isFlipModel ())
		{
			// flush pending updates first
			if(isFlushNeeded ())
				flush ();

			scrollRect = rect;
			scrollRect.offset (delta);
			scrollOffset = delta;

			setFlushNeeded ();
		}
		else
			engine.scrollBitmap (swapChainBitmap, rect, delta);

		// invalidate areas
		if(fractionalScaling)
		{
			DpiScale::toCoordRect (rect, scaleFactor);
			DpiScale::toCoordPoint (delta, scaleFactor);
			window.finishScroll (rect, delta);
		}
		else
			window.finishScroll (inRect, inDelta);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DWindowRenderTarget::add3DSurface (Native3DSurface* _surface)
{
	auto* surface = ccl_cast<D3DSurface> (_surface);
	ASSERT (surface)
	surfaces.add (surface);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DWindowRenderTarget::remove3DSurface (Native3DSurface* _surface)
{
	auto* surface = ccl_cast<D3DSurface> (_surface);
	ASSERT (surface)
	surfaces.remove (surface);
}
