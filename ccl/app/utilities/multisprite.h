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
// Filename    : ccl/app/utilities/multisprite.h
// Description : MultiSprite
//
//************************************************************************************************

#ifndef _ccl_multisprite_h
#define _ccl_multisprite_h

#include "ccl/base/object.h"

#include "ccl/public/collections/linkedlist.h"
#include "ccl/public/collections/intrusivelist.h"

#include "ccl/public/gui/framework/isprite.h"

namespace CCL {

interface IView;
interface IGraphicsRootLayer;
class UserControl;

//************************************************************************************************
// MultiSprite
/** A MultiSprite shows, hides and moves multiple sprites at once. */
//************************************************************************************************

class MultiSprite: public Object,
				   public ISprite
{
public:
	DECLARE_CLASS (MultiSprite, Object)

	MultiSprite (IView* view = nullptr);
	MultiSprite (UserControl* control);
	~MultiSprite ();

	PROPERTY_FLAG (style, kCollectLayerUpdates, collectLayerUpdates) ///< can improve performance for sublayer sprites

	IView* getView () const;

	typedef LinkedList<ISprite*> SpriteList;
	const SpriteList& getSprites () const;
	void addSprite (ISprite* sprite);
	void prependSprite (ISprite* sprite);
	void removeSprite (ISprite* sprite);
	void removeAll ();

	// ISprite
	RectRef CCL_API getSize () const override;  ///< calculates joined size of all sprites
	tbool CCL_API isVisible () const override;
	void CCL_API show () override;
	void CCL_API hide () override;
	void CCL_API moveTo (PointRef position) override;
	void CCL_API scrolled (PointRef delta) override;
	void CCL_API refresh () override;
	void CCL_API takeOpacity (IDrawable* drawable) override;

	CLASS_INTERFACE (ISprite, Object)

protected:
	enum Styles
	{
		kVisible = 1<<0, ///< visibility state (internal)
		kCollectLayerUpdates = 1<<1
	};

	IView* view;
	int style;
	SpriteList sprites;
	mutable Rect tempRect;

	IGraphicsRootLayer* getRootLayer () const;
	Rect calcJoinedSize () const;

	// ISprite
	tresult CCL_API construct (IView* view, RectRef size, IDrawable* drawable, int style) override;
	IDrawable* CCL_API getDrawable () const override;
	void CCL_API move (RectRef size) override;
};

//************************************************************************************************
// MultiDrawable
//************************************************************************************************

class MultiDrawable: public CCL::Object,
					 public CCL::AbstractDrawable
{
public:
	struct DrawItem: public IntrusiveLink<DrawItem>
	{
		PROPERTY_AUTO_POINTER (IDrawable, drawable, Drawable)
		PROPERTY_OBJECT (Rect, size, Size)

		DrawItem (IDrawable* drawable, RectRef size)
		: drawable (drawable), size (size)
		{}
	};

	~MultiDrawable ();

	void addItem (IDrawable* drawable, RectRef size);
	void moveItem (DrawItem* item, RectRef rect);
	Rect getTotalSize ();

	IntrusiveLinkedList<DrawItem>& getItems ();

	// IDrawable
	void CCL_API draw (const DrawArgs& args) override;

	CLASS_INTERFACE (IDrawable, Object)

private:
	IntrusiveLinkedList<DrawItem> drawItems;
	Rect size;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void MultiDrawable::addItem (IDrawable* drawable, RectRef size)
{
	drawItems.append (NEW DrawItem (drawable, size));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void MultiDrawable::moveItem (DrawItem* item, RectRef size)
{
	item->setSize (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline IntrusiveLinkedList<MultiDrawable::DrawItem>& MultiDrawable::getItems ()
{
	return drawItems;
}

} // namespace CCL

#endif // _ccl_multisprite_h
