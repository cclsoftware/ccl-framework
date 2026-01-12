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
// Filename    : ccl/gui/views/sprite.h
// Description : Sprite
//
//************************************************************************************************

#ifndef _ccl_sprite_h
#define _ccl_sprite_h

#include "ccl/base/collections/objectlist.h"

#include "ccl/public/gui/framework/isprite.h"
#include "ccl/public/gui/framework/itimer.h"
#include "ccl/public/gui/graphics/igraphicslayer.h"

namespace CCL {

class View;
class Window;
class Bitmap;
class GraphicsDevice;
class TransparentWindow;

//************************************************************************************************
// Sprite
/** Sprites represent graphical objects animated on screen (e.g. selections, cursors, etc.) */
//************************************************************************************************

class Sprite: public Object,
			  public ISprite
{
public:
	DECLARE_CLASS (Sprite, Object)
	DECLARE_METHOD_NAMES (Sprite)

	/** Creates a sprite of a class best suited for the platform. */
	static Sprite* createSprite (View* view = nullptr, IDrawable* drawable = nullptr, const Rect& size = Rect (), int options = 0);

	Sprite (View* view = nullptr, IDrawable* drawable = nullptr, const Rect& size = Rect (), int options = 0);
	~Sprite ();

	View* getView () const;

	// ISprite
	RectRef CCL_API getSize () const override;
	IDrawable* CCL_API getDrawable () const override;
	tbool CCL_API isVisible () const override;
	void CCL_API show () override;
	void CCL_API hide () override;
	void CCL_API move (RectRef size) override;
	void CCL_API moveTo (PointRef position) override;
	void CCL_API scrolled (PointRef delta) override;
	void CCL_API refresh () override;
	void CCL_API takeOpacity (IDrawable* drawable) override;

	CLASS_INTERFACE (ISprite, Object)

protected:
	enum PrivateFlags 
	{ 
		kVisible  = 1<<16	///< visibility state
	};

	View* view;
	IDrawable* drawable;
	Rect size;
	int options;

	PROPERTY_FLAG (options, kVisible, visible)
	PROPERTY_FLAG (options, kKeepOnTop, keepOnTop)

	// ISprite
	tresult CCL_API construct (IView* view, RectRef size, IDrawable* drawable, int options) override;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// ManagedSprite
/** Base class for window- and layer-based sprites. */
//************************************************************************************************

class ManagedSprite: public Sprite,
					 public ITimerTask
{
public:
	DECLARE_CLASS_ABSTRACT (ManagedSprite, Sprite)

	ManagedSprite (View* view = nullptr, IDrawable* drawable = nullptr, const Rect& size = Rect (), int options = 0);

	// Sprite
	void CCL_API show () override;
	void CCL_API hide () override;
	void CCL_API move (RectRef size) override;
	void CCL_API refresh () override;

	CLASS_INTERFACE (ITimerTask, Sprite)

protected:
	enum UpdateReason { kOnShow, kOnHide, kOnMove, kOnRefresh, kOnIdle };
	virtual void update (UpdateReason reason) = 0;

	// ITimerTask
	void CCL_API onTimer (ITimer* timer) override;
};

//************************************************************************************************
// FloatingSprite
/** Sprite drawn in its own transparent window. */
//************************************************************************************************

class FloatingSprite: public ManagedSprite
{
public:
	DECLARE_CLASS (FloatingSprite, ManagedSprite)

	FloatingSprite (View* view = nullptr, IDrawable* drawable = nullptr, const Rect& size = Rect (), int options = 0);
	~FloatingSprite ();

	// ManagedSprite
	void CCL_API takeOpacity (IDrawable* drawable) override;

protected:
	TransparentWindow* window;
	Bitmap* offscreen;
	Rect oldSize;

	Bitmap& getOffscreen (int width, int height);
	TransparentWindow& getTransparentWindow ();
	void removeWindow ();
	void updateWindow (bool force = false);

	// ManagedSprite
	void update (UpdateReason reason) override;
};

//************************************************************************************************
// SublayerSprite
/** Sprite using GPU-accelerated graphics layer. */
//************************************************************************************************
	
class SublayerSprite: public ManagedSprite,
					  public IGraphicsLayerContent
{
public:
	DECLARE_CLASS (SublayerSprite, ManagedSprite)
	
	SublayerSprite (View* view = nullptr, IDrawable* drawable = nullptr, const Rect& size = Rect (), int options = 0);
	~SublayerSprite ();

	CLASS_INTERFACE (IGraphicsLayerContent, Sprite)
		
protected:
	AutoPtr<IGraphicsLayer> subLayer;
	Rect oldLayerRect;

	IGraphicsLayer* getParentLayer (Point& offset) const;

	// IGraphicsLayerContent
	void CCL_API drawLayer (IGraphics& graphics, const UpdateRgn& updateRgn, PointRef offset = Point ()) override;
	LayerHint CCL_API getLayerHint () const override;
	
	// ManagedSprite
	void update (UpdateReason reason) override;
};
	
} // namespace CCL

#endif // _ccl_sprite_h
