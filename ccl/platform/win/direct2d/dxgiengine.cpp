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
// Filename    : ccl/platform/win/direct2d/dxgiengine.cpp
// Description : DXGI (DirectX Graphics Infrastructure) Engine
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "dxgiengine.h"

#include <wincodec.h>

#include "ccl/platform/win/system/system.win.h"

#include "ccl/public/gui/graphics/ibitmap.h"
#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/public/gui/framework/ialert.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/translation.h"

/*
	Platform Update for Windows 7
	http://msdn.microsoft.com/en-us/library/windows/desktop/jj863687%28v=vs.85%29.aspx

	How to render by using a Direct2D device context
	http://msdn.microsoft.com/en-us/library/windows/desktop/hh780339%28v=vs.85%29.aspx

	Enhancing presentation with the flip model, dirty rectangles, and scrolled areas
	http://msdn.microsoft.com/en-us/library/windows/desktop/hh706345%28v=vs.85%29.aspx

	Improving performance with multiple swap chains per rendering device
	http://msdn.microsoft.com/en-us/library/windows/desktop/hh706347%28v=vs.85%29.aspx

	Reduce latency with DXGI 1.3 swap chains
	http://msdn.microsoft.com/en-us/library/windows/apps/dn448914.aspx

	Handling device removed scenarios in Direct3D 11
	http://msdn.microsoft.com/en-us/library/windows/apps/dn458383.aspx
*/

#pragma comment (lib, "D3D11.lib")
#pragma comment (lib, "d2d1.lib")
#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "d3dcompiler.lib")

using namespace CCL;
using namespace Win32;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Win32")
	XSTRING (DXGIUserWarning, "Graphics hardware acceleration issue detected.")
END_XSTRINGS

//************************************************************************************************
// DXGIEngine::StrokeStyle
//************************************************************************************************

class DXGIEngine::StrokeStyle: public Object
{
public:
	StrokeStyle (ID2D1StrokeStyle* strokeStyle, int32 penStyle)
	: strokeStyle (strokeStyle),
	  penStyle (penStyle)
	{}

	PROPERTY_OBJECT (ComPtr<ID2D1StrokeStyle>, strokeStyle, StrokeStyle)
	PROPERTY_VARIABLE (int32, penStyle, PenStyle)

	// Object
	int compare (const Object& obj) const override
	{
		const StrokeStyle& other = static_cast<const StrokeStyle&> (obj);
		return penStyle - other.penStyle;
	}
};

//************************************************************************************************
// DXGIEngine
//************************************************************************************************

DXGIEngine::DXGIEngine ()
: gdiCompatible (false),
  flipModelEnabled (false),
  currentClientDevice (nullptr),
  beginDrawCount (0),
  primaryColor (Colors::kBlack),
  lastError (S_OK),
  warningShown (false)
{
	strokeStyles.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DXGIEngine::~DXGIEngine ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DXGIEngine::startup ()
{
	HRESULT hr = E_FAIL;

	// *** Create Direct3D device ***
	D3D_DRIVER_TYPE driverTypes[] = {D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP};
	for(int i = 0; i < ARRAY_COUNT(driverTypes); i++)
	{
		D3D_FEATURE_LEVEL featureLevelSupported;
		ComPtr<ID3D11DeviceContext> immediateContext;
		UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
		#if (0 && DEBUG)
		flags |= D3D11_CREATE_DEVICE_DEBUG;
		#endif
		hr = ::D3D11CreateDevice (nullptr/*adapter*/, driverTypes[i], NULL/*software*/, flags,
								  nullptr/*featureLevels*/, 0/*#featureLevels*/, D3D11_SDK_VERSION,
								  direct3dDevice, &featureLevelSupported, immediateContext);
		immediateContext.as (direct3dDeviceContext);
		if(SUCCEEDED(hr))
			break;
	}

	if(!direct3dDevice || !direct3dDeviceContext)
		return false;

	direct3dDevice.as (dxgiDevice);
	if(!dxgiDevice)
		return false;

	#if DEBUG_LOG
	dumpFeatureSupport ();
	#endif

	// *** Create Direct2D objects ***
	hr = ::D2D1CreateFactory<ID2D1Factory1> (D2D1_FACTORY_TYPE_SINGLE_THREADED, direct2dFactory);
	if(FAILED (hr))
		return false;

	hr = direct2dFactory->CreateDevice (dxgiDevice, direct2dDevice);
	if(FAILED (hr))
		return false;

	hr = direct2dDevice->CreateDeviceContext (D2D1_DEVICE_CONTEXT_OPTIONS_NONE, direct2dDeviceContext);
	if(FAILED (hr))
		return false;

	hr = direct2dDeviceContext->CreateSolidColorBrush (D2DInterop::toColorF (primaryColor), primaryBrush);
	if(FAILED (hr))
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DXGIEngine::shutdown ()
{
	tempAlphaBitmap.release ();
	tempNonAlphaBitmap.release ();
	primaryBrush.release ();
	strokeStyles.removeAll ();

	direct2dDeviceContext.release ();
	direct2dDevice.release ();
	direct2dFactory.release ();

	dxgiDevice.release ();
	direct3dDevice.release ();
	direct3dDeviceContext.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DXGIEngine::handleDeviceLost ()
{
	HRESULT hr = direct3dDevice->GetDeviceRemovedReason ();

	shutdown ();
	startup ();

	reportError ("Device lost", hr, true); // report when restart is done (otherwise the alert icon cannot be created)
	warningShown = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DXGIEngine::dumpFeatureSupport ()
{
	D3D11_FEATURE_DATA_D3D11_OPTIONS options {};
	HRESULT hr = direct3dDevice->CheckFeatureSupport (D3D11_FEATURE_D3D11_OPTIONS, &options, sizeof(options));

	if(FAILED (hr))
	{
		Debugger::printf ("D3D feature check failed: 0x%08x", hr);
		return;
	}

	Debugger::println ("Direct3D Features:");
	Debugger::printf ("  OutputMergerLogicOp: %i\n", options.OutputMergerLogicOp);
	Debugger::printf ("  UAVOnlyRenderingForcedSampleCount: %i\n", options.UAVOnlyRenderingForcedSampleCount);
	Debugger::printf ("  DiscardAPIsSeenByDriver: %i\n", options.DiscardAPIsSeenByDriver);
	Debugger::printf ("  FlagsForUpdateAndCopySeenByDriver: %i\n", options.FlagsForUpdateAndCopySeenByDriver);
	Debugger::printf ("  ClearView: %i\n", options.ClearView);
	Debugger::printf ("  CopyWithOverlap: %i\n", options.CopyWithOverlap);
	Debugger::printf ("  ConstantBufferPartialUpdate: %i\n", options.ConstantBufferPartialUpdate);
	Debugger::printf ("  ConstantBufferOffsetting: %i\n", options.ConstantBufferOffsetting);
	Debugger::printf ("  MapNoOverwriteOnDynamicConstantBuffer: %i\n", options.MapNoOverwriteOnDynamicConstantBuffer);
	Debugger::printf ("  MapNoOverwriteOnDynamicBufferSRV: %i\n", options.MapNoOverwriteOnDynamicBufferSRV);
	Debugger::printf ("  MultisampleRTVWithForcedSampleCountOne: %i\n", options.MultisampleRTVWithForcedSampleCountOne);
	Debugger::printf ("  SAD4ShaderInstructions: %i\n", options.SAD4ShaderInstructions);
	Debugger::printf ("  ExtendedDoublesShaderInstructions: %i\n", options.ExtendedDoublesShaderInstructions);
	Debugger::printf ("  ExtendedResourceSharing: %i\n", options.ExtendedResourceSharing);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DXGIEngine::reportError (CStringPtr message, HRESULT hr, bool warn)
{
	CCL_WARN ("[DXGI] %s (HRESULT = 0x%08X)\n", message, hr);

	static bool notifyPending = false; // don't reenter!
	if(warn && !warningShown && !notifyPending)
	{
		ScopedVar<bool> scope (notifyPending, true);

		#if DEBUG
		String message;
		Win32::formatSystemDebugMessage (message, hr);
		#else
		String message (XSTR (DXGIUserWarning));
		message << MutableCString ().appendFormat (" (0x%08X)", hr);
		#endif
		if(Alert::notify (message, Alert::kWarning))
			warningShown = true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DXGIEngine::beginDraw ()
{
	if(beginDrawCount++ == 0)
		direct2dDeviceContext->BeginDraw ();

	CCL_PRINTF ("DXGIEngine beginDraw %d\n", beginDrawCount)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT DXGIEngine::endDraw ()
{
	HRESULT hr = S_OK;

	CCL_PRINTF ("DXGIEngine endDraw %d\n", beginDrawCount)
	ASSERT (beginDrawCount > 0)
	if(--beginDrawCount == 0)
	{
		hr = direct2dDeviceContext->EndDraw ();
		SOFT_ASSERT (SUCCEEDED (hr), "D2D end draw failed!!!\n")
		if(FAILED (hr)) // calling flush resets the error state
		{
			#if DEBUG
			D2DError::print (hr);
			#endif
			direct2dDeviceContext->Flush ();
		}
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID2D1StrokeStyle* DXGIEngine::getStrokeStyle (int penStyle)
{
	ASSERT ((penStyle & Pen::kPenTypeMask) == Pen::kSolid)

	ID2D1StrokeStyle* direct2dStrokeStyle = nullptr;
	if(penStyle != 0)
	{
		StrokeStyle* strokeStyle = static_cast<StrokeStyle*> (strokeStyles.search (StrokeStyle (nullptr, penStyle)));
		if(strokeStyle)
			direct2dStrokeStyle = strokeStyle->getStrokeStyle ();
		else
		{
			D2D1_CAP_STYLE capStyle = D2D1_CAP_STYLE_FLAT;
			D2D1_LINE_JOIN lineJoin = D2D1_LINE_JOIN_MITER;

			if(penStyle & Pen::kLineCapSquare)
				capStyle = D2D1_CAP_STYLE_SQUARE;
			else if(penStyle & Pen::kLineCapRound)
				capStyle = D2D1_CAP_STYLE_ROUND;

			if(penStyle & Pen::kLineJoinBevel)
				lineJoin = D2D1_LINE_JOIN_BEVEL;
			else if(penStyle & Pen::kLineJoinRound)
				lineJoin = D2D1_LINE_JOIN_ROUND;

			D2D1_STROKE_STYLE_PROPERTIES strokeProperties = { capStyle, capStyle, D2D1_CAP_STYLE_FLAT, lineJoin, 0.f, D2D1_DASH_STYLE_SOLID, 0.f };
			HRESULT hr = direct2dFactory->CreateStrokeStyle (strokeProperties, nullptr, 0, &direct2dStrokeStyle);
			ASSERT (SUCCEEDED (hr))
			if(FAILED (hr))
			{
				#if DEBUG
				D2DError::print (hr);
				#endif
				return nullptr;
			}
			else
				strokeStyles.addSorted (NEW StrokeStyle (direct2dStrokeStyle, penStyle));
		}
	}
	return direct2dStrokeStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DXGIEngine::isFlipModel () const
{
	if(isFlipModelEnabled ())
		return true;
#if 0
	return isGdiCompatible () == false; // flip model cannot be mixed with GDI!
#else
	return false;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UINT DXGIEngine::getSwapChainFlags () const
{
	return isGdiCompatible () ? DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDXGISwapChain1* DXGIEngine::createSwapChainForWindow (HWND hwnd)
{
	DXGI_SWAP_EFFECT swapChainMode = isFlipModel () ? DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL : DXGI_SWAP_EFFECT_DISCARD;

	ComPtr<IDXGIAdapter> dxgiAdapter;
	HRESULT hr = dxgiDevice->GetAdapter (dxgiAdapter);

	ComPtr<IDXGIFactory2> dxgiFactory;
	hr = dxgiAdapter->GetParent (__uuidof(IDXGIFactory2), dxgiFactory);

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
	swapChainDesc.Width = 0;                           // use automatic sizing
	swapChainDesc.Height = 0;
	swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // this is the most common swapchain format
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;                // don't use multi-sampling
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;                     // use double buffering to enable flip
	swapChainDesc.Scaling = isFlipModel () ? DXGI_SCALING_NONE : DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = swapChainMode;
	swapChainDesc.Flags = getSwapChainFlags ();
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

	ComPtr<IDXGISwapChain1> swapChain;
	hr = dxgiFactory->CreateSwapChainForHwnd (dxgiDevice, hwnd, &swapChainDesc, nullptr, nullptr, swapChain);
	setLastError (hr);

    // Disallows DXGI to monitor an application's message queue for the alt-enter key sequence
    hr = dxgiFactory->MakeWindowAssociation (hwnd, DXGI_MWA_NO_WINDOW_CHANGES);

	return swapChain.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID2D1Bitmap1* DXGIEngine::createBitmapForSwapChain (IDXGISwapChain1* swapChain)
{
	// Direct2D needs the dxgi version of the backbuffer surface pointer.
    ComPtr<IDXGISurface> dxgiBackBuffer;
	HRESULT hr = swapChain->GetBuffer (0, __uuidof(IDXGISurface), dxgiBackBuffer);
	if(FAILED (hr))
		return nullptr;

	FLOAT dpiX = 0.f, dpiY = 0.f; //96?

	D2D1_BITMAP_OPTIONS options = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
	if(isGdiCompatible ())
		options |= D2D1_BITMAP_OPTIONS_GDI_COMPATIBLE;

	const D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1 (
		options, D2D1::PixelFormat (DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), dpiX, dpiY);

	ComPtr<ID2D1Bitmap1> swapChainDirect2dBitmap;
	hr = direct2dDeviceContext->CreateBitmapFromDxgiSurface (dxgiBackBuffer, bitmapProperties, swapChainDirect2dBitmap);
	setLastError (hr);

	return swapChainDirect2dBitmap.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID2D1Bitmap1* DXGIEngine::takeScreenshot (HWND hwnd, IDXGISwapChain1* swapChain)
{
	if(swapChain == nullptr)
		return nullptr;

	ComPtr<IDXGIOutput> output;
	ComPtr<IDXGIOutput1> output1;
	HRESULT hr = swapChain->GetContainingOutput (output);
	setLastError (hr);
	output.as (output1);
	if(!output1)
		return nullptr;

	RECT clientRect = {0};
	::GetClientRect (hwnd, &clientRect);
	POINT offset = {0};
	::ClientToScreen (hwnd, &offset);

	D2D_RECT_U screenRect = D2D1::RectU (offset.x, offset.y, offset.x + clientRect.right, offset.y + clientRect.bottom);
	return takeScreenshot (output1, screenRect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID2D1Bitmap1* DXGIEngine::takeScreenshot (IDXGIOutput1* output, const D2D_RECT_U& screenRect)
{
	ComPtr<IDXGIOutputDuplication> outputDuplication;
	HRESULT hr = output->DuplicateOutput (direct3dDevice, outputDuplication);
	setLastError (hr);
	if(FAILED (hr))
		return nullptr;

	ComPtr<ID2D1Bitmap1> destBitmap;

	DXGI_OUTDUPL_FRAME_INFO frameInfo = {0};
	ComPtr<IDXGIResource> desktopResource;
	hr = outputDuplication->AcquireNextFrame (500, &frameInfo, desktopResource);
	if(SUCCEEDED (hr))
	{
		ComPtr<IDXGISurface> dxgiSurface;
		desktopResource.as (dxgiSurface);
		if(dxgiSurface)
		{
			D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1 (
				D2D1_BITMAP_OPTIONS_NONE,
				D2D1::PixelFormat (DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));

			ComPtr<ID2D1Bitmap1> surfaceBitmap;
			hr = direct2dDeviceContext->CreateBitmapFromDxgiSurface (dxgiSurface, bitmapProperties, surfaceBitmap);
			if(surfaceBitmap)
			{
				bitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_CPU_READ;
				D2D_SIZE_U size = D2D1::SizeU (screenRect.right - screenRect.left, screenRect.bottom - screenRect.top);

				hr = direct2dDeviceContext->CreateBitmap (size, nullptr, 0, &bitmapProperties, destBitmap);
				if(SUCCEEDED (hr))
				{
					D2D1_POINT_2U dstPoint = D2D1::Point2U (0, 0);
					hr = destBitmap->CopyFromBitmap (&dstPoint, surfaceBitmap, &screenRect);
					ASSERT (SUCCEEDED (hr))
					setLastError (hr);
					if(FAILED (hr))
						destBitmap.release ();
				}
			}
		}

		dxgiSurface.release ();
		desktopResource.release ();
		hr = outputDuplication->ReleaseFrame ();
		setLastError (hr);
		ASSERT (SUCCEEDED (hr))
	}

	return destBitmap.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

D2D1_BITMAP_PROPERTIES1 DXGIEngine::getBitmapProperties (ID2D1Bitmap1* bitmap)
{
	ASSERT (bitmap)
	D2D1_BITMAP_PROPERTIES1 properties;
	properties.pixelFormat = bitmap->GetPixelFormat ();
	bitmap->GetDpi (&properties.dpiX, &properties.dpiY);
	properties.bitmapOptions = bitmap->GetOptions ();
	properties.colorContext = nullptr;
	return properties;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID2D1Bitmap1* DXGIEngine::createBitmap (PointRef sizeInPixel, bool alphaChannelUsed, bool isTarget, float scaleFactor)
{
	D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1 (
		isTarget ? D2D1_BITMAP_OPTIONS_TARGET : D2D1_BITMAP_OPTIONS_NONE,
		D2D1::PixelFormat (DXGI_FORMAT_B8G8R8A8_UNORM, alphaChannelUsed ? D2D1_ALPHA_MODE_PREMULTIPLIED : D2D1_ALPHA_MODE_IGNORE));

	bitmapProperties.dpiX = DpiScale::getDpi (scaleFactor);
	bitmapProperties.dpiY = DpiScale::getDpi (scaleFactor);

	ComPtr<ID2D1Bitmap1> bitmap;
	HRESULT hr = direct2dDeviceContext->CreateBitmap (D2D1::SizeU (sizeInPixel.x, sizeInPixel.y), nullptr, 0, &bitmapProperties, bitmap);
	setLastError (hr);
	ASSERT (SUCCEEDED (hr))
	if(FAILED (hr))
		return nullptr;
	return bitmap.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID2D1Bitmap1* DXGIEngine::createBitmapWithOptions (ID2D1Bitmap1* sourceBitmap, D2D1_BITMAP_OPTIONS desiredOptions, bool mustCopy)
{
	ASSERT (sourceBitmap)
	if(!sourceBitmap)
		return nullptr;

	// check if source already supports given options
	if(mustCopy == false && (sourceBitmap->GetOptions () & desiredOptions) != 0)
	{
		sourceBitmap->AddRef ();
		return sourceBitmap;
	}

	// copy source to new bitmap with desired options
	D2D1_SIZE_U size = sourceBitmap->GetPixelSize ();
	D2D1_BITMAP_PROPERTIES1 bitmapProperties = getBitmapProperties (sourceBitmap);
	bitmapProperties.bitmapOptions = desiredOptions;

	ComPtr<ID2D1Bitmap1> destBitmap;
	HRESULT hr = direct2dDeviceContext->CreateBitmap (size, nullptr, 0, &bitmapProperties, destBitmap);
	setLastError (hr);
	ASSERT (SUCCEEDED (hr))
	if(FAILED (hr))
		return nullptr;

	D2D1_POINT_2U dstPoint = D2D1::Point2U (0, 0);
	D2D1_RECT_U srcRect = D2D1::RectU (0, 0, size.width, size.height);
	hr = destBitmap->CopyFromBitmap (&dstPoint, sourceBitmap, &srcRect);
	ASSERT (SUCCEEDED (hr))
	return destBitmap.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID2D1Bitmap1* DXGIEngine::createBitmapForCPUReadAccess (ID2D1Bitmap1* sourceBitmap)
{
	return createBitmapWithOptions (sourceBitmap, D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_CPU_READ);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DXGIEngine::clearBitmap (ID2D1Bitmap1* bitmap)
{
	ASSERT (bitmap)
	if(!bitmap)
		return false;

	ComPtr<ID2D1Image> oldTarget;
	direct2dDeviceContext->GetTarget (oldTarget);
	ASSERT (oldTarget == 0 || currentClientDevice != nullptr)

	ASSERT (bitmap->GetOptions () & D2D1_BITMAP_OPTIONS_TARGET)

	D2D1::Matrix3x2F oldTransform;
	if(currentClientDevice)
	{
		currentClientDevice->suspend (true);
		direct2dDeviceContext->GetTransform (&oldTransform);
		direct2dDeviceContext->SetTransform (D2D1::Matrix3x2F::Identity ());
	}

	direct2dDeviceContext->SetTarget (bitmap);
	beginDraw ();
	direct2dDeviceContext->Clear (nullptr);
	HRESULT hr = endDraw ();
	ASSERT (SUCCEEDED (hr))
	direct2dDeviceContext->SetTarget (oldTarget);

	if(currentClientDevice)
	{
		direct2dDeviceContext->SetTransform (&oldTransform);
		currentClientDevice->suspend (false);
	}

	return SUCCEEDED (hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DXGIEngine::lockBitmap (BitmapLockData& data, ID2D1Bitmap1* bitmap, int mode)
{
	ASSERT (bitmap)
	if(!bitmap)
		return false;

	ASSERT (mode == IBitmap::kLockRead)
	if(mode != IBitmap::kLockRead) // write access not allowed!
		return false;

	ComPtr<ID2D1Bitmap1> softwareBitmap = createBitmapForCPUReadAccess (bitmap);
	if(softwareBitmap == 0)
		return false;

	D2D1_MAPPED_RECT mappedRect = {0};
	HRESULT hr = softwareBitmap->Map (D2D1_MAP_OPTIONS_READ, &mappedRect);
	setLastError (hr);
	if(FAILED (hr))
		return false;

	ASSERT (softwareBitmap->GetPixelFormat ().format == DXGI_FORMAT_B8G8R8A8_UNORM)
	D2D1_SIZE_U pixelSize = softwareBitmap->GetPixelSize ();

	// fill BitmapData
	data.width = (int)pixelSize.width;
	data.height = (int)pixelSize.height;
	data.format = IBitmap::kRGBAlpha;
	data.scan0 = mappedRect.bits;
	data.rowBytes = mappedRect.pitch;
	data.bitsPerPixel = 32;
	data.mode = mode;
	data.nativeData = softwareBitmap.detach ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DXGIEngine::unlockBitmap (BitmapLockData& data)
{
	ID2D1Bitmap1* softwareBitmap = reinterpret_cast<ID2D1Bitmap1*> (data.nativeData);
	ASSERT (softwareBitmap != nullptr)
	if(softwareBitmap == nullptr)
		return false;

	HRESULT hr = softwareBitmap->Unmap ();
	setLastError (hr);
	ASSERT (SUCCEEDED (hr))
	softwareBitmap->Release ();
	data.nativeData = nullptr;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ID2D1Bitmap1* DXGIEngine::getScratchBitmap (const D2D1_SIZE_U& size, const D2D1_PIXEL_FORMAT& format)
{
	ASSERT (format.format == DXGI_FORMAT_B8G8R8A8_UNORM)

	// TODO: adjust size to larger tiles
	ComPtr<ID2D1Bitmap1>& tempBitmap = format.alphaMode == D2D1_ALPHA_MODE_IGNORE ? tempNonAlphaBitmap : tempAlphaBitmap;
	if(tempBitmap)
	{
		D2D1_SIZE_U currentSize = tempBitmap->GetPixelSize ();
		if(currentSize.width < size.width || currentSize.height < size.height)
			tempBitmap.release ();
	}

	if(tempBitmap == 0)
	{
		D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1 (D2D1_BITMAP_OPTIONS_TARGET, format);
		HRESULT hr = direct2dDeviceContext->CreateBitmap (size, nullptr, 0, &bitmapProperties, tempBitmap);
		setLastError (hr);
		ASSERT (SUCCEEDED (hr))
	}

	return tempBitmap;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DXGIEngine::scrollBitmap (ID2D1Bitmap1* bitmap, RectRef rect, PointRef delta)
{
	ASSERT (bitmap)
	if(!bitmap)
		return false;

	if(rect.isEmpty () || delta.isNull ())
		return true;

	// create temporary bitmap (and keep it allocated)
	D2D1_SIZE_U size = D2D1::SizeU (rect.getWidth (), rect.getHeight ());
	ID2D1Bitmap1* tempBitmap = getScratchBitmap (size, bitmap->GetPixelFormat ());
	if(tempBitmap == nullptr)
		return false;

	// copy to temporary bitmap
	D2D1_POINT_2U dstPoint = D2D1::Point2U (0, 0);
	D2D1_RECT_U srcRect = D2DInterop::toRectU (rect);
	HRESULT hr = tempBitmap->CopyFromBitmap (&dstPoint, bitmap, &srcRect);
	ASSERT (SUCCEEDED (hr))

	// copy back at new position
	Rect dstRect (rect);
	dstRect.offset (delta);
	dstPoint = D2D1::Point2U (dstRect.left, dstRect.top);
	srcRect = D2D1::RectU (0, 0, rect.getWidth (), rect.getHeight ());
	hr = bitmap->CopyFromBitmap (&dstPoint, tempBitmap, &srcRect);
	setLastError (hr);
	ASSERT (SUCCEEDED (hr))
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DXGIEngine::copyFromWICBitmap (ID2D1Bitmap1* bitmap, IWICBitmap* wicBitmap)
{
	ASSERT (bitmap && wicBitmap)
	if(!bitmap || !wicBitmap)
		return false;

	// check bitmap sizes
	UINT srcWidth = 0, srcHeight = 0;
	wicBitmap->GetSize (&srcWidth, &srcHeight);
	D2D1_SIZE_U dstSize = bitmap->GetPixelSize ();
	ASSERT (srcWidth == dstSize.width && srcHeight == dstSize.height)
	if(srcWidth != dstSize.width || srcHeight != dstSize.height)
		return false;

	// we assume that pixel format is always compatible (32 bit)
	//WICPixelFormatGUID srcFormat = {0};
	//wicBitmap->GetPixelFormat (&srcFormat);
	//D2D1_PIXEL_FORMAT dstFormat = bitmap->GetPixelFormat ();

	// lock WIC bitmap
	ComPtr<IWICBitmapLock> bitmapLock;
	HRESULT hr = wicBitmap->Lock (nullptr, WICBitmapLockRead, bitmapLock);
	setLastError (hr);
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

	// copy to Direct2D bitmap
	hr = bitmap->CopyFromMemory (nullptr, dataPointer, stride);
	setLastError (hr);
	ASSERT (SUCCEEDED (hr))
	return SUCCEEDED (hr);
}
