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
// Filename    : core/public/gui/coreuiproperties.h
// Description : GUI Property Definitions
//
//************************************************************************************************

#ifndef _coreuiproperties_h
#define _coreuiproperties_h

#include "core/public/coreproperty.h"
#include "core/public/gui/corerect.h"
#include "core/public/gui/corecolor.h"

namespace Core {

//************************************************************************************************
// Property Type Enumeration
//************************************************************************************************

enum UIPropertyTypes
{
	kViewSizeProperty	= FOUR_CHAR_ID ('V','s','i','z'),	///< @see ViewSizeProperty
	kViewNameProperty	= FOUR_CHAR_ID ('V','n','a','m'),	///< @see ViewNameProperty
	kViewClassProperty	= FOUR_CHAR_ID ('V','c','l','s'),	///< @see ViewClassProperty
	kViewSourceProperty	= FOUR_CHAR_ID ('V','s','r','c'),	///< @see ViewSourceProperty
	kColorProperty		= FOUR_CHAR_ID ('C','o','l','r')	///< @see ColorProperty
};

//************************************************************************************************
// ViewSizeProperty
/** \ingroup core_gui */
//************************************************************************************************

struct ViewSizeProperty: Property
{
	Rect size;	///< view size

	ViewSizeProperty (RectRef size = Rect ())
	: Property (kViewSizeProperty, sizeof(ViewSizeProperty)),
	  size (size)
	{}
};

//************************************************************************************************
// ViewNameProperty
/** \ingroup core_gui */
//************************************************************************************************

struct ViewNameProperty: Property
{
	static const int kMaxNameLength = 32;
	char name[kMaxNameLength];

	ViewNameProperty ()
	: Property (kViewNameProperty, sizeof(ViewNameProperty))
	{
		name[0] = '\0';
	}
};

//************************************************************************************************
// ViewClassProperty
/** \ingroup core_gui */
//************************************************************************************************

struct ViewClassProperty: ViewNameProperty
{
	ViewClassProperty ()
	{
		type = kViewClassProperty;
	}
};

//************************************************************************************************
// ViewSourceProperty
/** \ingroup core_gui */
//************************************************************************************************

struct ViewSourceProperty: Property
{
	static const int kMaxSourceFileLength = 64;
	char sourceFile[kMaxSourceFileLength];

	ViewSourceProperty ()
	: Property (kViewSourceProperty, sizeof(ViewSourceProperty))
	{
		sourceFile[0] = '\0';
	}
};

//************************************************************************************************
// ColorProperty
/** \ingroup core_gui */
//************************************************************************************************

struct ColorProperty: Property
{
	enum ColorID
	{
		kBackColor = FOUR_CHAR_ID ('B','a','c','k'),
		kForeColor = FOUR_CHAR_ID ('F','o','r','e')
	};

	FourCharID colorId;	///< color identifier
	Color color;		///< color value

	ColorProperty (int colorId = kBackColor)
	: Property (kColorProperty, sizeof(ColorProperty)),
	  colorId (colorId)
	{}
};

} // namespace Core

#endif // _coreuiproperties_h
