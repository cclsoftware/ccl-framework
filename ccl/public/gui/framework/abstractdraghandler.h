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
// Filename    : ccl/public/gui/framework/abstractdraghandler.h
// Description : Abstract drag handler base class
//
//************************************************************************************************

#ifndef _ccl_abstractdraghandler_h
#define _ccl_abstractdraghandler_h

#include "ccl/public/gui/framework/idragndrop.h"
#include "ccl/public/gui/framework/isprite.h"
#include "ccl/public/gui/framework/guievent.h"

namespace CCL {

//************************************************************************************************
// AbstractDragHandler
/** Base class for implementing a drag handler.	Optionally uses a sprite for visual feedback.
\ingroup gui_data  */
//************************************************************************************************

class AbstractDragHandler: public IDragHandler
{
public:
	AbstractDragHandler ();
	virtual ~AbstractDragHandler ();

	// Sprite (optional)
	PROPERTY_POINTER (ISprite, sprite, Sprite)			///< setSprite takes ownership
	PROPERTY_OBJECT (Point, spriteOffset, SpriteOffset)	///< offset from drag position
	void moveSprite (PointRef where);
	void moveSprite (RectRef rect);
	virtual void moveSprite (const DragEvent& event);	///< moves sprite with mouse
	void hideSprite ();
	void deleteSprite ();

	// IDragHandler 
	tbool CCL_API dragEnter (const DragEvent& event) override;
	tbool CCL_API dragOver (const DragEvent& event) override;
	tbool CCL_API dragLeave (const DragEvent& event) override;
	tbool CCL_API drop (const DragEvent& event) override;
	tbool CCL_API afterDrop (const DragEvent& event) override;
	tbool CCL_API hasVisualFeedback () const override;
	tbool CCL_API isNullHandler () const override;
	tbool CCL_API wantsAutoScroll () const override;
};

//************************************************************************************************
// AbstractDragHandler inline
//************************************************************************************************

inline AbstractDragHandler::AbstractDragHandler ()
: sprite (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline AbstractDragHandler::~AbstractDragHandler ()
{
	deleteSprite ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void AbstractDragHandler::deleteSprite ()
{
	if(sprite)
	{
		sprite->hide ();
		sprite->release ();
		sprite = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////

inline void AbstractDragHandler::moveSprite (PointRef where)
{
	if(sprite)
	{
		sprite->moveTo (where);
		sprite->show ();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////

inline void AbstractDragHandler::moveSprite (RectRef rect)
{
	if(sprite)
	{
		sprite->move (rect);
		sprite->show ();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////

inline void AbstractDragHandler::moveSprite (const DragEvent& event)
{
	moveSprite (event.where + spriteOffset);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

inline void AbstractDragHandler::hideSprite ()
{
	if(sprite)
		sprite->hide ();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

inline tbool CCL_API AbstractDragHandler::dragEnter (const DragEvent& event)
{
	moveSprite (event);
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

inline tbool CCL_API AbstractDragHandler::dragOver (const DragEvent& event)
{
	moveSprite (event);
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

inline tbool CCL_API AbstractDragHandler::dragLeave (const DragEvent& event)
{
	hideSprite ();
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

inline tbool CCL_API AbstractDragHandler::drop (const DragEvent& event)
{
	hideSprite ();
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

inline tbool CCL_API AbstractDragHandler::afterDrop (const DragEvent& event)
{
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

inline tbool CCL_API AbstractDragHandler::hasVisualFeedback () const
{
	return sprite != nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

inline tbool CCL_API AbstractDragHandler::isNullHandler () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline tbool CCL_API AbstractDragHandler::wantsAutoScroll () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_abstractdraghandler_h
