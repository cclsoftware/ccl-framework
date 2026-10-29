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
// Filename    : ccl/gui/skin/skinlayouts.cpp
// Description : Skin Layout Elements
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/skin/skinlayouts.h"

#include "ccl/base/message.h"

#include "ccl/gui/layout/alignview.h"
#include "ccl/gui/layout/anchorlayout.h"
#include "ccl/gui/layout/boxlayout.h"
#include "ccl/gui/layout/divider.h"
#include "ccl/gui/layout/flexboxlayout.h"

#include "ccl/gui/skin/skinattributes.h"
#include "ccl/gui/skin/skinwizard.h"

#include "ccl/public/gui/framework/skinxmldefs.h"

namespace CCL {
namespace SkinElements {

//////////////////////////////////////////////////////////////////////////////////////////////////

void linkSkinLayouts ()
{} // force linkage of this file

CCL_KERNEL_INIT_LEVEL (LayoutElement, kFrameworkLevelFirst)
{
	// register layout class enumeration
	getTypeLibrary ().addEnum (&LayoutFactory::instance (), true);
	return true;
}

//************************************************************************************************
// LayoutElement
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (LayoutElement, ViewElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (LayoutElement, ViewElement, TAG_LAYOUT, SKIN_TYPELIB)
DEFINE_TYPELIB_ABSTRACT (LayoutElement)
DEFINE_TYPELIB_DOCGROUP (LayoutElement, DOC_GROUP_LAYOUT)
DEFINE_TYPELIB_RELATED_CLASS (LayoutElement, LayoutView)

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutElement::LayoutElement ()
: layout (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutElement::~LayoutElement ()
{
	if(layout)
		layout->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Layout* LayoutElement::getLayout () const
{
	if(!layout)
		layout = createLayout ();
	return layout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LayoutElement::setAttributes (const SkinAttributes& a)
{
	Layout* layout = getLayout ();
	if(layout)
		layout->setAttributes (a);

	return ViewElement::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LayoutElement::getAttributes (SkinAttributes& a) const
{
	Layout* layout = getLayout ();
	if(layout)
		layout->getAttributes (a);

	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* LayoutElement::createView (const CreateArgs& args, View* view)
{
	LayoutView* layoutView = ccl_cast<LayoutView> (view);
	if(layoutView == nullptr)
		layoutView = NEW LayoutView (size, options);

	if(Layout* layout = getLayout ())
		layoutView->setLayout (layout);

	View* result = SuperClass::createView (args, layoutView);
	layoutView->onViewCreated ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LayoutElement::viewAdded (View* parent, View* child, ViewElement* childElement, SkinWizard& wizard)
{
	if(const SkinAttributes* a = childElement->getDataAttributes ())
	{
		LayoutView* layoutView = ccl_cast<LayoutView> (parent);
		LayoutItem* item = layoutView ? layoutView->findLayoutItem (child) : nullptr;
		ASSERT (item != nullptr)
		if(item != nullptr)
			item->setAttributes (*a);
	}
}

//************************************************************************************************
// AnchorLayoutElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AnchorLayoutElement, LayoutElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (AnchorLayoutElement, LayoutElement, TAG_BASICLAYOUT, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (AnchorLayoutElement, DOC_GROUP_LAYOUT)
DEFINE_TYPELIB_RELATED_CLASS (AnchorLayoutElement, AnchorLayoutView)
BEGIN_TYPELIB_MEMBERS (AnchorLayoutElement)
	ADD_TYPELIB_MEMBER (ATTR_SPACING, TypeNames::kMetric) ///< spacing between views (in pixels)
	ADD_TYPELIB_MEMBER (ATTR_MARGIN, TypeNames::kMetric) ///< outer margin (in pixels)
	ADD_TYPELIB_MEMBER_STRING (ATTR_LAYOUTCLASS) ///< name of the layout class to be used
	ADD_TYPELIB_MEMBER_STRING (ATTR_PERSISTENCE_ID) ///< storage id used to store and restore the layout state
	ADD_TYPELIB_MEMBER (ATTR_OPTIONS, TypeNames::kEnum) ///< options specific to the selected layout class
END_TYPELIB_MEMBERS (AnchorLayoutElement)
DEFINE_TYPELIB_ENUMERATION (TAG_BASICLAYOUT, ATTR_OPTIONS, BoxLayout::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

Layout* AnchorLayoutElement::createLayout () const
{
	MutableCString layoutName (layoutClass);
	if(layoutName.isEmpty ())
		layoutName = LAYOUTCLASS_BOX;

	Layout* layout = LayoutFactory::instance ().createLayout (layoutName);
	if(!layout)
		SKIN_WARNING (this, "Layout class not found: '%s'", layoutName.str ())
	return layout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnchorLayoutElement::setAttributes (const SkinAttributes& a)
{
	layoutClass = a.getString (ATTR_LAYOUTCLASS);
	persistenceID = a.getString (ATTR_PERSISTENCE_ID);

	const StyleDef* customStyleDef = nullptr;
	Layout* layout = getLayout ();
	if(layout)
		customStyleDef = ccl_cast<AnchorLayout> (layout)->getCustomStyles ();

	a.getOptions (options, ATTR_OPTIONS, customStyleDef);
	if(options.common == 0)
		options.common = Styles::kHorizontal;

	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AnchorLayoutElement::getAttributes (SkinAttributes& a) const
{
	Layout* layout = getLayout ();

	const StyleDef* customStyleDef = nullptr;
	if(layout)
	{
		customStyleDef = ccl_cast<AnchorLayout> (layout)->getCustomStyles ();
		layout->getAttributes (a);
	}
	a.setString (ATTR_LAYOUTCLASS, layoutClass);
	a.setString (ATTR_PERSISTENCE_ID, persistenceID);
	a.setOptions (ATTR_OPTIONS, options, customStyleDef);

	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* AnchorLayoutElement::createView (const CreateArgs& args, View* view)
{
	AnchorLayoutView* layoutView = ccl_cast<AnchorLayoutView> (view);
	if(layoutView == nullptr)
		layoutView = NEW AnchorLayoutView (size, options);

	layoutView->setPersistenceID (persistenceID);

	return SuperClass::createView (args, layoutView);
}

//************************************************************************************************
// HorizontalElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (HorizontalElement, AnchorLayoutElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (HorizontalElement, AnchorLayoutElement, TAG_HORIZONTAL, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (HorizontalElement, DOC_GROUP_LAYOUT)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HorizontalElement::setAttributes (const SkinAttributes& a)
{
	bool result = SuperClass::setAttributes (a);

	if(options.isCustomStyle (Styles::kLayoutHidePriority) && (sizeMode & View::kHFitSize))
		SKIN_WARNING (this, "hidepriority conflicts with hfit", 0)

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* HorizontalElement::createView (const CreateArgs& args, View* view)
{
	if(view == nullptr)
	{
		options.setCommonStyle (Styles::kHorizontal, true);
		options.setCommonStyle (Styles::kVertical, false);
		view = NEW AnchorLayoutView (size, options);
	}
	return AnchorLayoutElement::createView (args, view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Layout* HorizontalElement::createLayout () const
{
	return LayoutFactory::instance ().createLayout (LAYOUTCLASS_BOX);
}

//************************************************************************************************
// VerticalElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (VerticalElement, AnchorLayoutElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (VerticalElement, AnchorLayoutElement, TAG_VERTICAL, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (VerticalElement, DOC_GROUP_LAYOUT)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VerticalElement::setAttributes (const SkinAttributes& a)
{
	bool result = SuperClass::setAttributes (a);

	if(options.isCustomStyle (Styles::kLayoutHidePriority) && (sizeMode & View::kVFitSize))
		SKIN_WARNING (this, "hidepriority conflicts with vfit", 0)

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* VerticalElement::createView (const CreateArgs& args, View* view)
{
	if(view == nullptr)
	{
		options.setCommonStyle (Styles::kHorizontal, false);
		options.setCommonStyle (Styles::kVertical, true);
		view = NEW AnchorLayoutView (size, options);
	}
	return AnchorLayoutElement::createView (args, view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Layout* VerticalElement::createLayout () const
{
	return LayoutFactory::instance ().createLayout (LAYOUTCLASS_BOX);
}

//************************************************************************************************
// TableElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TableElement, AnchorLayoutElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (TableElement, AnchorLayoutElement, TAG_TABLE, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (TableElement, DOC_GROUP_LAYOUT)
BEGIN_TYPELIB_MEMBERS (TableElement)
	ADD_TYPELIB_MEMBER (ATTR_ROWS, TypeNames::kInt) ///< number of rows (uses as many columns as required)
	ADD_TYPELIB_MEMBER (ATTR_COLUMNS, TypeNames::kInt) ///< number of columns (uses as many rows as required)
	ADD_TYPELIB_MEMBER (ATTR_CELLRATIO, TypeNames::kFloat) ///< aspect ratio for cell views (width/height, e\.g\. 1 for square views)
	ADD_TYPELIB_MEMBER (ATTR_MINCELLRATIO, TypeNames::kFloat) ///< optional minimum aspect ratio for cell views (width/height, e\.g\. 1 for square views)
END_TYPELIB_MEMBERS (TableElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

Layout* TableElement::createLayout () const
{
	return LayoutFactory::instance ().createLayout (LAYOUTCLASS_TABLE);
}

//************************************************************************************************
// SizeVariantElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SizeVariantElement, AnchorLayoutElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (SizeVariantElement, AnchorLayoutElement, TAG_SIZEVARIANT, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (SizeVariantElement, DOC_GROUP_LAYOUT)

//////////////////////////////////////////////////////////////////////////////////////////////////

Layout* SizeVariantElement::createLayout () const
{
	return LayoutFactory::instance ().createLayout (LAYOUTCLASS_SIZEVARIANT);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SizeVariantElement::viewAdded (View* parent, View* child, ViewElement* childElement, SkinWizard& wizard)
{
	if(const SkinAttributes* a = childElement->getDataAttributes ())
	{
		AnchorLayoutView* layoutView = ccl_cast<AnchorLayoutView> (parent);
		LayoutItem* item = layoutView ? layoutView->findLayoutItem (child) : nullptr;
		ASSERT (item != nullptr)
		if(item)
		{
			Coord minSize = 0;
			String sizeString = a->getString (ATTR_MINSIZE);
			if(sizeString.contains (SkinVariable::prefix))
				sizeString = wizard.resolveTitle (sizeString);
			sizeString.getIntValue (minSize);

			// abusing the priority member for minSize
			MutableSkinAttributes skinAttributes;
			skinAttributes.setInt (ATTR_LAYOUTPRIORITY, minSize);
			item->setAttributes (skinAttributes);
		}
	}
}

//************************************************************************************************
// FlexboxElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (FlexboxElement, LayoutElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (FlexboxElement, LayoutElement, TAG_FLEXBOX, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (FlexboxElement, DOC_GROUP_LAYOUT)
BEGIN_TYPELIB_MEMBERS (FlexboxElement)
	ADD_TYPELIB_MEMBER (ATTR_FLEXDIRECTION, TypeNames::kEnum) 		///< Defines the direction of the main axis, in which the children are layed out
	ADD_TYPELIB_MEMBER (ATTR_FLEXWRAP, TypeNames::kEnum)			///< If children should wrap automatically if there is not enough room on the main axis
	ADD_TYPELIB_MEMBER (ATTR_FLEXJUSTIFY, TypeNames::kEnum)			///< Justification of the children on the main axis
	ADD_TYPELIB_MEMBER (ATTR_FLEXALIGN, TypeNames::kEnum)			///< Alignment of the children on the cross axis
	ADD_TYPELIB_MEMBER_STRING (ATTR_FLEXPADDING)					///< Shorthand for individual padding, enter between one and four values which are interpreted as follows: "left=top=right=bottom", "left=right, top=bottom", "left, top, right, bottom=0", "left, top, right, bottom"
	ADD_TYPELIB_MEMBER (ATTR_FLEXPADDINGTOP, TypeNames::kMetric)	///< Space added to the top edge on the inside of the container
	ADD_TYPELIB_MEMBER (ATTR_FLEXPADDINGRIGHT, TypeNames::kMetric)	///< Space added to the right edge on the inside of the container
	ADD_TYPELIB_MEMBER (ATTR_FLEXPADDINGBOTTOM, TypeNames::kMetric)	///< Space added to the bottom edge on the inside of the container
	ADD_TYPELIB_MEMBER (ATTR_FLEXPADDINGLEFT, TypeNames::kMetric)	///< Space added to the left edge on the inside of the container
	ADD_TYPELIB_MEMBER_STRING (ATTR_FLEXGAP)						///< Shorthand for individual gaps, enter one or two values which are interpreted as follows: "row=column", "row, column"
	ADD_TYPELIB_MEMBER (ATTR_FLEXGAPROW, TypeNames::kMetric)		///< Space added between elements horizontally
	ADD_TYPELIB_MEMBER (ATTR_FLEXGAPCOLUMN, TypeNames::kMetric)		///< Space added between elements vertically
END_TYPELIB_MEMBERS (FlexboxElement)

DEFINE_TYPELIB_ENUMERATION (TAG_FLEXBOX, ATTR_FLEXDIRECTION, FlexShared::flexDirection, SKIN_TYPELIB)
DEFINE_TYPELIB_ENUMERATION (TAG_FLEXBOX, ATTR_FLEXWRAP, FlexShared::flexWrap, SKIN_TYPELIB)
DEFINE_TYPELIB_ENUMERATION (TAG_FLEXBOX, ATTR_FLEXJUSTIFY, FlexShared::flexJustify, SKIN_TYPELIB)
DEFINE_TYPELIB_ENUMERATION (TAG_FLEXBOX, ATTR_FLEXALIGN, FlexShared::flexAlign, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

void FlexboxElement::viewAdded (View* parent, View* child, ViewElement* childElement, SkinWizard& wizard)
{
	LayoutView* layoutView = ccl_cast<LayoutView> (parent);
	if(layoutView == nullptr)
		return;

	FlexLayoutItem* item = ccl_cast<FlexLayoutItem> (layoutView->findLayoutItem (child));
	ASSERT (item != nullptr)
	if(item == nullptr)
		return;

	const SkinAttributes* flexAttributes = childElement->getFlexAttributes ();
	if(flexAttributes != nullptr)
	{
		ResolvedSkinAttributes resolvedSkinAttributes (*flexAttributes, wizard);
		item->setAttributes (resolvedSkinAttributes);
	}

	item->initialize (childElement->getDesignSize ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Layout* FlexboxElement::createLayout () const
{
	return LayoutFactory::instance ().createLayout (LAYOUTCLASS_FLEXBOX);
}

} // namespace SkinElements
} // namespace CCL
