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
// Filename    : ccl/public/gui/graphics/igraphicslayer.h
// Description : Graphics Layer Interface
//
//************************************************************************************************

#ifndef _ccl_igraphicslayer_h
#define _ccl_igraphicslayer_h

#include "ccl/public/gui/graphics/types.h"

namespace CCL {

struct UpdateRgn;
interface IAnimation;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (RootLayer, 0x82116a5c, 0x3bc4, 0x46d4, 0xba, 0x22, 0x6e, 0xc5, 0xd7, 0xb6, 0x1a, 0xa3)
	DEFINE_CID (GraphicsLayer, 0x7d5edf01, 0x7830, 0x420c, 0xbe, 0x12, 0x54, 0x12, 0xe5, 0xc2, 0x20, 0x69)
	DEFINE_CID (TiledLayer, 0xCA2C929C, 0xF5B9, 0x6148, 0x81, 0x05, 0xA6, 0x65, 0x1F, 0xBF, 0x58, 0x74)
}

//************************************************************************************************
// IGraphicsLayer
/**
	\ingroup gui_graphics 
	
	Graphics layers are surfaces which allow content to be rendered, transformed, and animated efficiently 
	by the system compositor with hardware-accelleration if supported by the underlying platform. 
	On Windows, layers are implemented via DirectComposition, and with Core Animation on macOS/iOS.
	On Linux, layers are implemented using wl_subsurface.

	Graphics layers are rendered above any view elements that are not rendered as layer content.
	Layers can be nested. Sublayers are rendered above parent layers and previous siblings.

	The position of a layer, relative to its parent, is determined by its offset.
	
	Layers are always clipped at the window boundaries.
	Additional clipping behavior depends on \a Modes flags.
	When \a kClipToBounds is set, contents of the layer and all sublayers are clipped at the layer boundaries, determined by its size and offset.
	
	While the layer's offset affects the position of the layer itself, an additional transform can be used to rotate, translate, or scale the layer content.
	This transform does not affect the offset, size, or clipping rectangle of the layer. */
//************************************************************************************************

interface IGraphicsLayer: IUnknown
{
	enum Modes
	{
		kIgnoreAlpha  = 1<<0,	///< Ignore alpha values in layer content, draw opaque black background.
		kClipToBounds = 1<<1	///< Clip to the boundaries of the layer. If not set, layer's content may exceed its boundaries.
	};

	/** Construct a graphics layer.
		The layer will become visible on screen in the next update cycle of the compositor or when flush is called.
		@param content The initial layer content, either an IBitmap or an IGraphicsLayerContent implementation.
		@param bounds The boundaries of the layer. The top-left corner of this rect is used as the initial offset. Width and height of this rect determine the intial size. 
		@param mode A combination of \a Modes flags. 
		@param contentScaleFactor The initial content scale factor of the layer. */
	virtual tresult CCL_API construct (IUnknown* content, RectRef bounds = Rect (), int mode = 0, float contentScaleFactor = 1.f) = 0;

	/** Set the layer content, either an IBitmap or an IGraphicsLayerContent implementation. */
	virtual tresult CCL_API setContent (IUnknown* content) = 0;

	/** Set the position of the layer, relative to its parent. */
	virtual void CCL_API setOffset (PointRef offset) = 0;

	/** Set the horizontal position of the layer, relative to its parent. */
	virtual void CCL_API setOffsetX (float offsetX) = 0;

	/** Set the vertical position of the layer, relative to its parent. */
	virtual void CCL_API setOffsetY (float offsetY) = 0;

	/** Set the size of the layer. */
	virtual void CCL_API setSize (Coord width, Coord height) = 0;

	/** Set mode flags, see \a Modes. */
	virtual void CCL_API setMode (int mode) = 0;

	/** Set the opacity of the layer content. */
	virtual void CCL_API setOpacity (float opacity) = 0;

	/** Set a transform that is applied to the layer content. */
	virtual void CCL_API setTransform (TransformRef transform) = 0;

	/** Set the points to pixels scaling factor. */
	virtual void CCL_API setContentScaleFactor (float factor) = 0;

	/** Invalidate the layer. Multiple changes are collected and will become visible on screen in the next update cycle of the compositor or when flush is called. */
	virtual void CCL_API setUpdateNeeded () = 0;

	/** Invalidate part of the layer. Multiple changes are collected and will become visible on screen in the next update cycle of the compositor or when flush is called. */
	virtual void CCL_API setUpdateNeeded (RectRef rect) = 0;

	/** Get the parent layer. */
	virtual IGraphicsLayer* CCL_API getParentLayer () = 0;

	/** Add a sublayer. New layers are always drawn on top of previous sublayers. */
	virtual tresult CCL_API addSublayer (IGraphicsLayer* layer) = 0;

	/** Remove a sublayer. If currently visible, the layer will become invisible in the next update cycle of the compositor or when flush is called. */
	virtual tresult CCL_API removeSublayer (IGraphicsLayer* layer) = 0;

	/** Change layer order, so that the given child layer is drawn right after the given \a sibling. */
	virtual tresult CCL_API placeAbove (IGraphicsLayer* layer, IGraphicsLayer* sibling) = 0;
	
	/** Change layer order, so that the given child layer is drawn right before the given \a sibling. */
	virtual tresult CCL_API placeBelow (IGraphicsLayer* layer, IGraphicsLayer* sibling) = 0;
	
	/** Get the sibling after the given child layer. */
	virtual IGraphicsLayer* CCL_API getNextSibling (IGraphicsLayer* layer) const = 0;
	
	/** Get the sibling before the given child layer. */
	virtual IGraphicsLayer* CCL_API getPreviousSibling (IGraphicsLayer* layer) const = 0;

	/** Add an animation. */
	virtual tresult CCL_API addAnimation (StringID propertyId, const IAnimation* animation) = 0;

	/** Remove an animation. */
	virtual tresult CCL_API removeAnimation (StringID propertyId) = 0;

	/** While an animation is running, get a property of the currently displayed state. */
	virtual tbool CCL_API getPresentationProperty (Variant& value, StringID propertyId) const = 0;

	/** Commit pending changes to graphics hardware. */
	virtual tresult CCL_API flush () = 0;

	/** Stop using tiles for drawing, only paint into visibleRect instead. */
	virtual void CCL_API suspendTiling (tbool suspend, const Rect* visibleRect) = 0;

	/** Set tile size in points */
	virtual void CCL_API setTileSize (int size) = 0;

	/** Set background color */
	virtual void CCL_API setBackColor (const Color& color) = 0;	

	// Property identifiers
	DECLARE_STRINGID_MEMBER (kOpacity)		///< float
	DECLARE_STRINGID_MEMBER (kOffsetX)		///< float
	DECLARE_STRINGID_MEMBER (kOffsetY)		///< float
	DECLARE_STRINGID_MEMBER (kOffset)		///< IUIValue (Point or PointF)
	DECLARE_STRINGID_MEMBER (kTransform)	///< IUIValue (Transform)

	DECLARE_IID (IGraphicsLayer)
};

DEFINE_IID (IGraphicsLayer, 0x297ac812, 0x2b1b, 0x4c61, 0xbe, 0xbc, 0x3, 0x7, 0x4a, 0xd0, 0x4b, 0xfa)
DEFINE_STRINGID_MEMBER (IGraphicsLayer, kOpacity, "opacity")
DEFINE_STRINGID_MEMBER (IGraphicsLayer, kOffsetX, "offsetX")
DEFINE_STRINGID_MEMBER (IGraphicsLayer, kOffsetY, "offsetY")
DEFINE_STRINGID_MEMBER (IGraphicsLayer, kOffset, "offset")
DEFINE_STRINGID_MEMBER (IGraphicsLayer, kTransform, "transform")

//************************************************************************************************
// IGraphicsRootLayer
/**
	\ingroup gui_graphics */
//************************************************************************************************

interface IGraphicsRootLayer: IUnknown
{
	struct UpdateSuspender;

	/** Suspend committing pending changes to graphics hardware in IGraphicsLayer::flush for all deep sublayers. */
	virtual tbool CCL_API suspendUpdates (tbool suspend) = 0;

	DECLARE_IID (IGraphicsRootLayer)
};

DEFINE_IID (IGraphicsRootLayer, 0x39f30d50, 0x6729, 0x49e1, 0x9d, 0xcb, 0xb3, 0x3a, 0x56, 0xac, 0xc7, 0x96)

//************************************************************************************************
// IGraphicsRootLayer::UpdateSuspender
/**
	\ingroup gui_graphics */
//************************************************************************************************

struct IGraphicsRootLayer::UpdateSuspender
{
	IGraphicsRootLayer* rootLayer;
	bool wasSuspended;
	UpdateSuspender (IGraphicsRootLayer* rootLayer, bool suspend = true)
	: rootLayer (rootLayer),
	  wasSuspended (false)
	{
		if(rootLayer)
			wasSuspended = rootLayer->suspendUpdates (suspend);
	}
	~UpdateSuspender ()
	{
		if(rootLayer)
			rootLayer->suspendUpdates (wasSuspended);
	}
};

//************************************************************************************************
// IGraphicsLayerContent
/**
	\ingroup gui_graphics */
//************************************************************************************************

interface IGraphicsLayerContent: IUnknown
{
	typedef GraphicsContentHint LayerHint;

	/** Get hint how content will be drawn into the layer. Used for optimizations (e.g. if background should be cleared). */
	virtual LayerHint CCL_API getLayerHint () const = 0;

	virtual void CCL_API drawLayer (IGraphics& graphics, const UpdateRgn& updateRgn, PointRef offset = Point ()) = 0;

	DECLARE_IID (IGraphicsLayerContent)
};

DEFINE_IID (IGraphicsLayerContent, 0xa16e5d92, 0xe47d, 0x4d54, 0xb8, 0x7a, 0xbc, 0xa, 0x1c, 0x3a, 0xd3, 0x4a)

} // namespace CCL

#endif // _ccl_igraphicslayer_h
