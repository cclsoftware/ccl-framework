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
// Filename    : ccl/app/controls/spritebuilder.h
// Description : Sprite Builder
//
//************************************************************************************************

#ifndef _ccl_spritebuilder_h
#define _ccl_spritebuilder_h

#include "ccl/public/gui/graphics/types.h"

#include "ccl/public/collections/unknownlist.h"

namespace CCL {

interface ISprite;
interface IView;
interface IVisualStyle;
class StandardSpriteDrawable;

//************************************************************************************************
// SpriteBuilder
/** Helps building a sprite consisting of items that can have an icon and a text. */
//************************************************************************************************

class SpriteBuilder
{
public:
	SpriteBuilder (IView* view = nullptr);
	~SpriteBuilder ();

	void addHeader (StringRef text, int group = 0);
	void addHeader (IImage* icon, StringRef text, int group = 0);

	bool addItem (StringRef text, int group = 0, Font* font = nullptr, SolidBrush* textBrush = nullptr);
	bool addItem (IImage* icon, StringRef text, int group = 0, Font* font = nullptr, SolidBrush* textBrush = nullptr);

	int getLastGroup ();
	bool hasGroup (int group) const;

	enum Options { kForceSmallIcons = 1<<0 };

	ISprite* createSprite (int options = 0);
	Point getDefaultOffset () const; 	///< recommended offset from mouse cursor
	Point getTouchOffset () const; 		///< recommended offset from fingertip
	IImage* getWarningIcon ();

	void replaceItemText (ISprite& sprite, int index, StringRef text);
	String getItemText (ISprite& sprite, int index) const;

	// override standard style
	void setBackgroundBrush (BrushRef brush);
	void setBorderPen (PenRef pen);
	void setBackgroundImage (IImage* image);
	void setMargin (Coord margin);
	void setSpacing (Coord spacing);
	void setRadius (Coord radius);
	void setMinWidth (Coord width);

	PROPERTY_OBJECT (Font, font, Font)
	PROPERTY_OBJECT (SolidBrush, textBrush, TextBrush)
	PROPERTY_BOOL (createSpriteSuspended, CreateSpriteSuspended)

protected:
	StandardSpriteDrawable* drawable;
	IView* view;
	int numItems;

	enum { kMaxItems = 100 };

	class Item;
	const IVisualStyle& getVisualStyle ();
	StandardSpriteDrawable* getDrawable ();
};

} // namespace CCL

#endif // _ccl_spritebuilder_h
