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
// Filename    : ccl/gui/theme/visualstyleclass.cpp
// Description : VisualStyle MetaClass
//
//************************************************************************************************

#include "ccl/gui/theme/visualstyleclass.h"
#include "ccl/gui/theme/theme.h"
#include "ccl/gui/graphics/graphicshelper.h"

#include "ccl/base/singleton.h"

#include "ccl/public/cclversion.h"

namespace CCL {

//************************************************************************************************
// VisualStyleClass::Library
//************************************************************************************************

class VisualStyleClass::Library: public TypeLibrary,
								 public Singleton<Library>
{
public:
	Library ()
	: TypeLibrary (CCL_STYLES_TYPELIB_NAME),
	  metrics ("ThemeMetrics", Theme::metricNames, ThemeElements::kNumMetrics),
	  colors ("ThemeColors", Theme::colorNames, ThemeElements::kNumColors),
	  cursors ("ThemeCursors", Theme::cursorNames, ThemeElements::kNumCursors),
	  fonts ("ThemeFonts", Theme::fontNames, ThemeElements::kNumFonts),
	  standardStyles ("StandardStyles", ThemePainter::standardStyleNames, ThemePainter::kNumStandardStyles),
	  elementImages ("ThemeElementImages", ThemePainter::uniqueImageNames, ThemePainter::kUniqueImageCount),
	  elementStates ("ThemeElementStates", ThemePainter::stateNames, ThemeElements::kNumElementStates),
	  elementStatesOn ("ThemeElementStatesOn", ThemePainter::stateNamesOn, ThemeElements::kNumElementStates)
	{
		addEnum (&metrics);
		addEnum (&colors);
		addEnum (&cursors);
		addEnum (&fonts);
		addEnum (&standardStyles);
		addEnum (&elementImages);
		addEnum (&elementStates);
		addEnum (&elementStatesOn);
		addEnum (&GraphicsHelper::instance ().getDefaultColors ());
	}

protected:
	CStringEnumTypeInfo metrics;
	CStringEnumTypeInfo colors;
	CStringEnumTypeInfo cursors;
	CStringEnumTypeInfo fonts;
	CStringEnumTypeInfo standardStyles;
	CStringEnumTypeInfo elementImages;
	CStringEnumTypeInfo elementStates;
	CStringEnumTypeInfo elementStatesOn;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// VisualStyleClass::MemberDescriptionModifier
//************************************************************************************************

VisualStyleClass::MemberDescriptionModifier::MemberDescriptionModifier (VisualStyleClass& This, Model::MemberDescription members[])
{
	This.members = members;
}

//************************************************************************************************
// VisualStyleClass::Library
//************************************************************************************************

DEFINE_SINGLETON (VisualStyleClass::Library)

//************************************************************************************************
// VisualStyleClass
/** Basic visual style attributes inherited by all other styles. */
//************************************************************************************************

BEGIN_VISUALSTYLE_BASE (VisualStyle, "BasicStyle") //  see also: StyleID
	ADD_VISUALSTYLE_COLOR	("forecolor")	///< color used for foreground elements
	ADD_VISUALSTYLE_COLOR	("hilitecolor")	///< color used for a hilite state
	ADD_VISUALSTYLE_COLOR	("backcolor")	///< backgound color
	ADD_VISUALSTYLE_COLOR	("textcolor")	///< color used for drawing text
	ADD_VISUALSTYLE_METRIC	("strokewidth")	///< width in pixels used when lines are drawn
	ADD_VISUALSTYLE_FONT	("textfont")	///< font used for drawing text
	ADD_VISUALSTYLE_ALIGN	("textalign")	///< alignment of text
	ADD_VISUALSTYLE_OPTIONS	("textoptions")	///< additional options for drawing text. Supported option: "wordbreak" (for multiline text)
	ADD_VISUALSTYLE_IMAGE	("background")	///< background image
END_VISUALSTYLE_CLASS (VisualStyle)

//////////////////////////////////////////////////////////////////////////////////////////////////

TypeLibrary& VisualStyleClass::getTypeLibrary ()
{
	return Library::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyleClass::VisualStyleClass (const char* name, const VisualStyleClass* parentClass)
: TypeInfo (name, parentClass),
  members (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VisualStyleClass::getDetails (ITypeInfoDetails& details) const
{
	if(members)
		for(Model::MemberDescription* m = members; !m->name.isEmpty (); m++)
			details.addMember (*m);

	return true;
}
