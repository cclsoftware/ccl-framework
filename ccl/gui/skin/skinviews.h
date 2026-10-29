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
// Filename    : ccl/gui/skin/skinviews.h
// Description : Skin View Elements
//
//************************************************************************************************

#ifndef _ccl_skinviews_h
#define _ccl_skinviews_h

#include "ccl/gui/skin/skinmodel.h"

namespace CCL {

class ItemControlBase;
interface IItemModel;

namespace SkinElements {

//************************************************************************************************
// VariantElement
/** Dynamically selects one of its child elements.
A Variant has always only one (or none) of its child elements attached as a child view.
The active child can selected via a numeric parameter or a property of the controller.

\code{.xml}
<!-- Example 1: A numeric parameter selects a view by index -->
<Variant name="indexValue" attach="fitsize">
	<Label title="Variant 0"/>    <!-- shown when indexValue is 0 -->
	<Label title="Variant 1"/>    <!-- shown when indexValue is 1 -->
</Variant>

<!-- Example 2: A property of the controller selects a view by property value. -->
<Variant property="indexValue" attach="fitsize">
	<Label title="Variant 0"/>    <!-- shown when indexValue is 0 -->
	<Label title="Variant 1"/>    <!-- shown when indexValue is 1 -->
</Variant>

<!-- Example 3: A boolean parameter switches a view on/off -->
<Variant name="boolValue" attach="fitsize">
	<Label title="Variant"/>    <!-- shown when boolValue is true -->
</Variant>
\endcode
*/
//************************************************************************************************

class VariantElement: public ViewElement
{
public:
	DECLARE_CLASS (VariantElement, ViewElement)
	DECLARE_TYPELIB_CLASS (VariantElement)

	VariantElement ();

	PROPERTY_MUTABLE_CSTRING (propertyId, PropertyId)
	PROPERTY_MUTABLE_CSTRING (controller, Controller)
	PROPERTY_VARIABLE (TransitionType, transitionType, TransitionType)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	bool appendOptions (String& string) const override;
	View* createView (const CreateArgs& args, View* view = nullptr) override;
	void viewCreated (View* view) override;
};

//************************************************************************************************
// LabelElement
//************************************************************************************************

class LabelElement: public ViewElement
{
public:
	DECLARE_CLASS (LabelElement, ViewElement)
	DECLARE_TYPELIB_CLASS (LabelElement)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool appendOptions (String& string) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// HeadingElement
//************************************************************************************************

class HeadingElement: public LabelElement
{
public:
	DECLARE_CLASS (HeadingElement, LabelElement)
	DECLARE_TYPELIB_CLASS (HeadingElement)

	HeadingElement ();

	PROPERTY_VARIABLE (int, level, Level)

	// LabelElement
	bool setAttributes (const SkinAttributes& a) override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// PictureElement
//************************************************************************************************

class PictureElement: public ImageViewElement
{
public:
	DECLARE_CLASS (PictureElement, ImageViewElement)
	DECLARE_TYPELIB_CLASS (PictureElement)

	// ViewElement
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// PopupBoxElement
//************************************************************************************************

class PopupBoxElement: public ViewElement
{
public:
	DECLARE_CLASS (PopupBoxElement, ViewElement)
	DECLARE_TYPELIB_CLASS (PopupBoxElement)

	PopupBoxElement ();

	PROPERTY_MUTABLE_CSTRING (formName, FormName)
	PROPERTY_MUTABLE_CSTRING (popupStyleName, PopupStyleName)
	PROPERTY_VARIABLE (int, popupOptions, PopupOptions)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// DialogGroupElement
//************************************************************************************************

class DialogGroupElement: public ViewElement
{
public:
	DECLARE_CLASS (DialogGroupElement, ViewElement)
	DECLARE_TYPELIB_CLASS (DialogGroupElement)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// TargetElement
/** Specifies the target view of a scroll view.
The <Target> tag can only appear as child of a <ScrollView> tag.
The name attribute specifies the name of a form that is used to create the target view. */
//************************************************************************************************

class TargetElement: public ViewElement
{
public:
	DECLARE_CLASS (TargetElement, ViewElement)
	DECLARE_TYPELIB_CLASS (TargetElement)

	TargetElement (ViewElement* parent);
	TargetElement ();

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	View* createView (const CreateArgs& args, View* view = nullptr) override;
};

//************************************************************************************************
// ScrollHeaderElement
/** Specifies the header view of a scroll view.
The <ScrollHeader> tag can only appear as child of a <ScrollView> tag.
The name attribute specifies the name of a form that is used to create the header view. */
//************************************************************************************************

class ScrollHeaderElement: public TargetElement
{
public:
	DECLARE_CLASS (ScrollHeaderElement, TargetElement)
	DECLARE_TYPELIB_CLASS (ScrollHeaderElement)

	ScrollHeaderElement (ViewElement* parent);
	ScrollHeaderElement ();
};

//************************************************************************************************
// ScrollViewElement
//************************************************************************************************

class ScrollViewElement: public ViewElement
{
public:
	DECLARE_CLASS (ScrollViewElement, ViewElement)
	DECLARE_TYPELIB_CLASS (ScrollViewElement)

	ScrollViewElement ();
	~ScrollViewElement ();

	PROPERTY_MUTABLE_CSTRING (persistenceID, PersistenceID)

	// ViewElement
	void addChild (Element* e, int index = -1) override;
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;

private:
	MutableCString horizontalScrollBarStyle;
	MutableCString verticalScrollBarStyle;
	MutableCString horizontalScrollName;
	MutableCString verticalScrollName;
	TargetElement* targetElement;
	ScrollHeaderElement* headerElement;
};

//************************************************************************************************
// ItemViewElement
//************************************************************************************************

class ItemViewElement: public ScrollViewElement
{
public:
	DECLARE_CLASS_ABSTRACT (ItemViewElement, ScrollViewElement)
	DECLARE_TYPELIB_CLASS (ItemViewElement)

	PROPERTY_OBJECT (StyleFlags, scrollOptions, ScrollOptions)
	PROPERTY_MUTABLE_CSTRING (headerStyleName, HeaderStyleName)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	bool appendOptions (String& string) const override;
	View* createView (const CreateArgs& args, View* view) override;

protected:
	virtual const StyleDef* getCustomDef () const = 0;
	virtual ItemControlBase* createControl (const CreateArgs& args) = 0;

	IItemModel* getModel (const CreateArgs& args);
};

//************************************************************************************************
// ListViewElement
//************************************************************************************************

class ListViewElement: public ItemViewElement
{
public:
	DECLARE_CLASS (ListViewElement, ItemViewElement)
	DECLARE_TYPELIB_CLASS (ListViewElement)

	ListViewElement ();

	PROPERTY_VARIABLE (int, viewType, ViewType)
	PROPERTY_VARIABLE (int, textTrimMode, TextTrimMode)

	// ItemViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;

protected:
	// ItemViewElement
	const StyleDef* getCustomDef () const override;
	ItemControlBase* createControl (const CreateArgs& args) override;
};

//************************************************************************************************
// TreeViewElement
//************************************************************************************************

class TreeViewElement: public ItemViewElement
{
public:
	DECLARE_CLASS (TreeViewElement, ItemViewElement)
	DECLARE_TYPELIB_CLASS (TreeViewElement)

protected:
	// ItemViewElement
	const StyleDef* getCustomDef () const override;
	ItemControlBase* createControl (const CreateArgs& args) override;
};

//************************************************************************************************
// DropBoxElement
//************************************************************************************************

class DropBoxElement: public ItemViewElement
{
public:
	DECLARE_CLASS (DropBoxElement, ItemViewElement)
	DECLARE_TYPELIB_CLASS (DropBoxElement)

protected:
	// ItemViewElement
	const StyleDef* getCustomDef () const override;
	ItemControlBase* createControl (const CreateArgs& args) override;
};

//************************************************************************************************
// WebViewElement
//************************************************************************************************

class WebViewElement: public ViewElement
{
public:
	DECLARE_CLASS (WebViewElement, ViewElement)
	DECLARE_TYPELIB_CLASS (WebViewElement)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// CommandBarViewElement
//************************************************************************************************

class CommandBarViewElement: public ViewElement
{
public:
	DECLARE_CLASS (CommandBarViewElement, ViewElement)
	DECLARE_TYPELIB_CLASS (CommandBarViewElement)

	PROPERTY_MUTABLE_CSTRING (itemFormName, ItemFormName)
	PROPERTY_MUTABLE_CSTRING (contextMenuFormName, ContextMenuFormName)

	// ViewElement
	View* createView (const CreateArgs& args, View* view) override;
	bool setAttributes (const SkinAttributes& a) override;
};

} // namespace SkinElements
} // namespace CCL

#endif // _ccl_skincontrols_h
