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
// Filename    : ccl/platform/win/direct2d/dcompengine.h
// Description : DirectComposition Engine
//
//************************************************************************************************

#ifndef _ccl_dcompengine_h
#define _ccl_dcompengine_h

#include "ccl/gui/graphics/graphicslayerimpl.h"

#include "ccl/platform/win/system/cclcom.h"

#include <dcomp.h>
#include <d3d11_1.h>
#include <UIAnimation.h>

namespace CCL {
namespace Win32 {

//************************************************************************************************
// DirectCompositionEngine
//************************************************************************************************

class DirectCompositionEngine: public GraphicsLayerEngine,
							   public StaticSingleton<DirectCompositionEngine>
{
public:
	DirectCompositionEngine ();
	
	IDCompositionDevice* getDevice () const { return directCompositionDevice; }

	bool startup ();
	void shutdown ();
	bool checkDeviceState ();
	void handleDeviceLost ();
	bool suspendUpdates (bool suspend);

	IGraphicsLayer* createLayer (UIDRef classID);

	IDCompositionAnimation* createAnimation (const BasicAnimation& animation, float scaleFactor);
	IDCompositionAnimation* createAnimation (const AnimationDescription& description, double startValue, double endValue, float scaleFactor);
	IDCompositionTransform* createTransform (const TransformAnimation& animation, PointRef center, float scaleFactor);
	IDCompositionClip* createClip (const BasicAnimation& animation, float scaleFactor);

	// GraphicsLayerEngine
	void flush (bool force = false) override;
	double getNextEstimatedFrameTime () const override;

protected:
	ComPtr<IDCompositionDevice> directCompositionDevice;
	ComPtr<IUIAnimationManager2> animationManager;
	ComPtr<IUIAnimationTransitionLibrary2> transitionLibrary;
	bool updatesSuspended;
	bool commitPending;
	bool waitForCompletionPending;
};

//************************************************************************************************
// DCGraphicsLayer
//************************************************************************************************

class DCGraphicsLayer: public GraphicsLayer
{
public:
	DECLARE_CLASS (DCGraphicsLayer, GraphicsLayer)

	DCGraphicsLayer ();
	~DCGraphicsLayer ();
	
	virtual void handleDeviceLost (bool begin);

	// GraphicsLayer
	void CCL_API setContentScaleFactor (float factor) override;
	tresult CCL_API construct (IUnknown* content, RectRef bounds = Rect (), int mode = 0, float contentScaleFactor = 1.f) override;
	tresult CCL_API setContent (IUnknown* content) override;
	void CCL_API setSize (Coord width, Coord height) override;
	void CCL_API setMode (int mode) override;

protected:
	static IDCompositionDevice* getDevice ();

	SharedPtr<IUnknown> content;
	Rect contentRect;
	int mode;
	#if DEBUG
	MutableCString debugName;
	#endif

	ComPtr<IDCompositionVisual> visual;
	ComPtr<IDCompositionSurface> surface;
	ComPtr<IDCompositionEffectGroup> effects;

	PROPERTY_FLAG (mode, kIgnoreAlpha, isIgnoreAlpha)

	void handleDeviceLostForSublayers (bool begin);
	void reconstruct ();

	virtual IDCompositionSurface* createSurface () const;
	IDCompositionEffectGroup* getEffects ();
	void makeSurface ();
	void initContent ();
	void updateClip ();

	void drawContentDirect2D (IDXGISurface1* dxgiSurface, RectRef updateRectInPixel);
	void drawContent (NativeGraphicsDevice& nativeDevice, PointRef originPoint);

	// GraphicsLayer
	void applyProperty (int id) override;
	bool applyAnimation (int id, const Animation* animation) override;
	void attachSublayer (IGraphicsLayer* layer, bool state, IGraphicsLayer* sibling = nullptr, bool below = false) override;
	void updateContent () override;
};

//************************************************************************************************
// DCTiledGraphicsLayer
//************************************************************************************************

class DCTiledGraphicsLayer: public DCGraphicsLayer
{
public:
	DECLARE_CLASS (DCTiledGraphicsLayer, DCGraphicsLayer)

protected:
	Rect visibleRect;

	void snapToTiles (Rect& r) const;

	// DCGraphicsLayer
	IDCompositionSurface* createSurface () const override;
	void CCL_API setSize (Coord width, Coord height) override;
	void CCL_API setUpdateNeeded (RectRef rect) override;
	void updateAll (bool& updateDone) override;
	void updateContent () override;
};

//************************************************************************************************
// DCRootLayer
//************************************************************************************************

class DCRootLayer: public DCGraphicsLayer,
				   public IGraphicsRootLayer
{
public:
	DECLARE_CLASS (DCRootLayer, DCGraphicsLayer)

	DCRootLayer ();
	~DCRootLayer ();

	// IGraphicsRootLayer
	tbool CCL_API suspendUpdates (tbool suspend) override;

	// DCGraphicsLayer
	Rect getBounds () const override;
	void CCL_API setContentScaleFactor (float factor) override;
	void handleDeviceLost (bool begin) override;
	tresult CCL_API construct (IUnknown* content, RectRef bounds = Rect (), int mode = 0, float contentScaleFactor = 1.f) override;

	CLASS_INTERFACE (IGraphicsRootLayer, DCGraphicsLayer)

protected:
	ComPtr<IDCompositionTarget> target;
	HWND windowHandle;
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_dcompengine_h
