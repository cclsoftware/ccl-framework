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
// Filename    : ccl/app/utilities/multisprite.cpp
// Description : MultiSprite
//
//************************************************************************************************

#include "ccl/app/utilities/multisprite.h"

#include "ccl/app/controls/usercontrol.h"

#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/igraphicslayer.h"
#include "ccl/public/gui/framework/iwindow.h"

using namespace CCL;

//************************************************************************************************
// MultiSprite
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MultiSprite, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

MultiSprite::MultiSprite (IView* view)
: view (view),
  style (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

MultiSprite::MultiSprite (UserControl* control)
: view (control ? *control : (IView*)nullptr),
  style (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

MultiSprite::~MultiSprite ()
{
	removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* MultiSprite::getView () const
{
	return view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsRootLayer* MultiSprite::getRootLayer () const
{
	auto window = view ? view->getIWindow () : nullptr;
	auto layer = window ? ViewBox (window).getGraphicsLayer () : nullptr;
	return UnknownPtr<IGraphicsRootLayer> (layer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const MultiSprite::SpriteList& MultiSprite::getSprites () const
{
	return sprites;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiSprite::addSprite (ISprite* sprite)
{
	sprites.append (sprite);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiSprite::prependSprite (ISprite* sprite)
{
	sprites.prepend (sprite);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiSprite::removeSprite (ISprite* sprite)
{
	sprites.remove (sprite);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiSprite::removeAll ()
{
	ListForEach (sprites, ISprite*, sprite)
		sprite->release ();
	EndFor
	sprites.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect MultiSprite::calcJoinedSize () const
{
	Rect totalSize;
	totalSize.setReallyEmpty ();
	ListForEach (sprites, ISprite*, sprite)
		totalSize.join (sprite->getSize ());
	EndFor
	return totalSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MultiSprite::construct (IView* view, RectRef size, IDrawable* drawable, int style)
{
	ASSERT (0)
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RectRef CCL_API MultiSprite::getSize () const
{
	tempRect = calcJoinedSize ();
	return tempRect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDrawable* CCL_API MultiSprite::getDrawable () const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MultiSprite::isVisible () const
{
	return (style & kVisible) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MultiSprite::show ()
{
	style |= kVisible;

	IGraphicsRootLayer::UpdateSuspender suspender (collectLayerUpdates () ? getRootLayer () : nullptr, true);
	ListForEach (sprites, ISprite*, sprite)
		sprite->show ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MultiSprite::hide ()
{
	style &= ~kVisible;

	ListForEach (sprites, ISprite*, sprite)
		sprite->hide ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MultiSprite::move (RectRef size)
{
	ASSERT (0) // should not be used for MultiSprite!
	moveTo (size.getLeftTop ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MultiSprite::moveTo (PointRef position)
{
	// keep relative sprite positions on move
	Rect totalSize = calcJoinedSize ();
	Point oldPos = totalSize.getLeftTop ();
	totalSize.moveTo (position);
	Point delta = totalSize.getLeftTop () - oldPos;

	ListForEach (sprites, ISprite*, sprite)
		Point p (sprite->getSize ().getLeftTop ());
		p += delta;
		sprite->moveTo (p);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MultiSprite::scrolled (PointRef delta)
{
	ListForEach (sprites, ISprite*, sprite)
		sprite->scrolled (delta);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MultiSprite::refresh ()
{
	ListForEach (sprites, ISprite*, sprite)
		sprite->refresh ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MultiSprite::takeOpacity (IDrawable* drawable)
{
	ListForEach (sprites, ISprite*, sprite)
		sprite->takeOpacity (drawable);
	EndFor
}

//************************************************************************************************
// MultiDrawable
//************************************************************************************************

MultiDrawable::~MultiDrawable ()
{
	while(DrawItem* item = drawItems.removeFirst ())
		delete item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect MultiDrawable::getTotalSize ()
{
	Rect size;
	size.setReallyEmpty ();

	IntrusiveListForEach (drawItems, DrawItem, item)
		size.join (item->getSize ());
	EndFor
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MultiDrawable::draw (const DrawArgs& args)
{
	args.graphics.clearRect (args.size);

	Rect size;
	DrawArgs childArgs (args.graphics, size, args.updateRgn);

	IntrusiveListForEach (drawItems, DrawItem, item)
		size = item->getSize ();
		item->getDrawable ()->draw (childArgs);
	EndFor
}
