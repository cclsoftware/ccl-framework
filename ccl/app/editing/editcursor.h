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
// Filename    : ccl/app/editing/editcursor.h
// Description : Edit Cursor
//
//************************************************************************************************

#ifndef _ccl_editcursor_h
#define _ccl_editcursor_h

#include "ccl/base/object.h"

#include "ccl/public/gui/graphics/types.h"

namespace CCL {

class UserControl;
interface ISprite;
interface IDrawable;

//************************************************************************************************
// AbstractEditCursor
//************************************************************************************************

class AbstractEditCursor: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (AbstractEditCursor, Object)

	AbstractEditCursor (UserControl* editView = nullptr);

	UserControl* getEditView () const;

	virtual void attached () = 0;			///< called by UserControl::attached()
	virtual void removed  () = 0;			///< called by UserControl::removed()
	virtual void updateSize () = 0;			///< called by UserControl::onSize() and UserControl::onMove()
	virtual void move (PointRef position) = 0;

	virtual bool isVisible () const = 0;
	virtual void setVisible (bool state) = 0;

protected:
	UserControl* editView;

	static UIDRef getSpriteClass ();
};

//************************************************************************************************
// BasicEditCursor
/** Base classs for EditCursor with one sprite. */
//************************************************************************************************

class BasicEditCursor: public AbstractEditCursor
{
public:
	DECLARE_CLASS_ABSTRACT (BasicEditCursor, AbstractEditCursor)

	BasicEditCursor (UserControl* editView = nullptr);
	~BasicEditCursor ();

	// AbstractEditCursor
	void attached () override;
	void removed  () override;
	void updateSize () override;
	bool isVisible () const override;
	void setVisible (bool state) override;

protected:
	ISprite* sprite;

	virtual IDrawable* createDrawable () = 0;
	virtual Rect& getSpriteRect (Rect& rect) const = 0;
};

//************************************************************************************************
// EditCursor
/** An edit cursor marks the current insert position in a graphical editor. */
//************************************************************************************************

class EditCursor: public BasicEditCursor
{
public:
	DECLARE_CLASS (EditCursor, BasicEditCursor)

	EditCursor (UserControl* editView = nullptr, int width = 1);

	PROPERTY_VARIABLE (int, width, Width)

	void setColor (ColorRef color);

	Coord getPosition () const;
	void scrolled (Coord delta);

	// BasicEditCursor
	void move (PointRef position) override;
	IDrawable* createDrawable () override;
	Rect& getSpriteRect (Rect& rect) const override;

protected:
	Coord position;
};

//************************************************************************************************
// CrossHairCursor
//************************************************************************************************

class CrossHairCursor: public AbstractEditCursor
{
public:
	DECLARE_CLASS (CrossHairCursor, AbstractEditCursor)

	CrossHairCursor (UserControl* editView = nullptr, int width = 1);
	~CrossHairCursor ();

	PROPERTY_VARIABLE (int, width, Width)

	// AbstractEditCursor
	void attached () override;
	void removed  () override;
	void updateSize () override;
	void move (PointRef position) override;
	bool isVisible () const override;
	void setVisible (bool state) override;

protected:
	ISprite* hSprite;
	ISprite* vSprite;
	bool visible;
	Point position;

	void showSprite ();
	void hideSprite ();
};

} // namespace CCL

#endif // _ccl_editcursor_h
