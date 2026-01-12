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
// Filename    : ccl/gui/skin/skinlayouts.h
// Description : Skin Layout Elements
//
//************************************************************************************************

#ifndef _ccl_skinlayouts_h
#define _ccl_skinlayouts_h

#include "ccl/gui/skin/skinmodel.h"

namespace CCL {

class Layout;

namespace SkinElements {

//************************************************************************************************
// LayoutElement
/** Layout container base class. */
//************************************************************************************************

class LayoutElement: public ViewElement
{
public:
	DECLARE_SKIN_ELEMENT_ABSTRACT (LayoutElement, ViewElement)

	LayoutElement ();
	~LayoutElement ();

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;
	void viewAdded (View* parent, View* child, ViewElement* childElement, SkinWizard& wizard) override;

protected:
	virtual Layout* createLayout () const = 0;
	Layout* getLayout () const;

private:
	mutable Layout* layout;
};

//************************************************************************************************
// AnchorLayoutElement
/** Base class for anchor layout elements. Currently the following layout classes are available:
 "box" (<Horizontal> or <Vertical>), "clipper", "sizevariant" (<SizeVariant>) or "table" (<Table>)
 
 The layout class can be specified via the attribute "layout.class".
 
\see HorizontalElement \see VerticalElement \see SizeVariantElement \see TableElement
*/
//************************************************************************************************

class AnchorLayoutElement: public LayoutElement
{
public:
	DECLARE_SKIN_ELEMENT (AnchorLayoutElement, LayoutElement)

	PROPERTY_MUTABLE_CSTRING (layoutClass, LayoutClass)
	PROPERTY_MUTABLE_CSTRING (persistenceID, PersistenceID)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;
	
protected:
	Layout* createLayout () const override;
};

//************************************************************************************************
// HorizontalElement
/** Arranges child views horizontally, trying to fill the width of the Horizontal element.
The intitial widths of the child views are considered their preferred widths. For a given container width,
the total preferred with of all child views can result in either some remaining or missing width.

The horizontal layout tries to distribute this remaining or missing width equally among the child views, by
either enlarging or shrinking the childs. But this is limited by their attachments and their size limits:
Like in the basic layout of the View class, a view only get's sized horizontally, if it's attached left and right to it's parent (attach="left right").
*/
//************************************************************************************************

class HorizontalElement: public AnchorLayoutElement
{
public:
	DECLARE_SKIN_ELEMENT (HorizontalElement, AnchorLayoutElement)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	View* createView (const CreateArgs& args, View* view) override;

	// LayoutElement
	Layout* createLayout () const override;
};

//************************************************************************************************
// VerticalElement
/** Arranges child views vertically.
Behaves the same as <Horizontal>, but in the vertical direction. \see HorizontalElement */
//************************************************************************************************

class VerticalElement: public AnchorLayoutElement
{
public:
	DECLARE_SKIN_ELEMENT (VerticalElement, AnchorLayoutElement)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	View* createView (const CreateArgs& args, View* view) override;

	// LayoutElement
	Layout* createLayout () const override;
};

//************************************************************************************************
// RowElement
/** Deprecated. Use <Vertical> instead. \see VerticalElement */
//************************************************************************************************

class RowElement: public VerticalElement
{
public:
	DECLARE_SKIN_ELEMENT (RowElement, VerticalElement)
};

//************************************************************************************************
// ColumnElement
/** Deprecated. Use <Horizontal> instead. \see HorizontalElement */
//************************************************************************************************

class ColumnElement: public HorizontalElement
{
public:
	DECLARE_SKIN_ELEMENT (ColumnElement, HorizontalElement)
};

//************************************************************************************************
// TableElement
/** Arranges child views in a table grid.
To define the number of row and columns, either the "rows" or "columns" attribute must be specified.
(Then other one is calculated from the specified one and the number of views).

The rows as a whole are arranged vertically by the same algorithm used by <Vertical>,
while columns are arranged horizontally as in <Horizontal>.

The optional attribute "cellratio" sizes childs with the given aspect ratio (width/height, e.g. 1 for square views).
*/
//************************************************************************************************

class TableElement: public AnchorLayoutElement
{
public:
	DECLARE_SKIN_ELEMENT (TableElement, AnchorLayoutElement)

	// LayoutElement
	Layout* createLayout () const override;
};

//************************************************************************************************
// SizeVariantElement
/** Dynamically selects one of it's child elements, depending on the container width or height.
Child elements must specify their minimum width or height in the "data.minsize" attribute.
The best matching child view gets selected every time the container size changes.

\code{.xml}
<SizeVariant options="vertical" height="100" attach="all">
	<Label title="tiny"/> <!-- used when parent height < 10 -->
	<Label title="small" data.minsize="10"/> <!-- used when parent height >= 10 and < 20 -->
	<Label title="large" data.minsize="20"/> <!-- used when parent height >= 20 -->
</SizeVariant>
\endcode
*/
//************************************************************************************************

class SizeVariantElement: public AnchorLayoutElement
{
public:
	DECLARE_SKIN_ELEMENT (SizeVariantElement, AnchorLayoutElement)

	// LayoutElement
	Layout* createLayout () const override;
	void viewAdded (View* parent, View* child, ViewElement* childElement, SkinWizard& wizard) override;
};

//************************************************************************************************
// FlexboxElement
/** Arranges its children according to the CSS Flexbox specification: https://www\.w3\.org/TR/css-flexbox-1/ */
//************************************************************************************************

class FlexboxElement: public LayoutElement
{
public:
	DECLARE_SKIN_ELEMENT (FlexboxElement, LayoutElement)

	// LayoutElement
	void viewAdded (View* parent, View* child, ViewElement* childElement, SkinWizard& wizard) override;
	Layout* createLayout () const override;
};

} // namespace SkinElements
} // namespace CCL

#endif // _ccl_skinlayouts_h
