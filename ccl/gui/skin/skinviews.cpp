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
// Filename    : ccl/gui/skin/skinviews.cpp
// Description : Skin View Elements
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/skin/skinviews.h"

#include "ccl/gui/controls/label.h"
#include "ccl/gui/controls/pictureviewer.h"
#include "ccl/gui/controls/popupbox.h"
#include "ccl/gui/controls/variantview.h"
#include "ccl/gui/controls/commandbar/commandbarview.h"

#include "ccl/gui/dialogs/dialogbuilder.h"

#include "ccl/gui/itemviews/dropbox.h"
#include "ccl/gui/itemviews/listview.h"
#include "ccl/gui/itemviews/treeview.h"

#include "ccl/gui/popup/popupslider.h"

#include "ccl/gui/skin/form.h"
#include "ccl/gui/skin/skinattributes.h"
#include "ccl/gui/skin/skincontrols.h"
#include "ccl/gui/skin/skinwizard.h"

#include "ccl/gui/system/webbrowserview.h"

#include "ccl/gui/views/dialoggroup.h"
#include "ccl/gui/views/viewanimation.h"

#include "ccl/public/gui/framework/skinxmldefs.h"

namespace CCL {
namespace SkinElements {

void linkSkinViews () {} // force linkage of this file

//************************************************************************************************
// VariantElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (VariantElement, ViewElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (VariantElement, ViewElement, TAG_VARIANT, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (VariantElement, DOC_GROUP_LAYOUT)
DEFINE_TYPELIB_RELATED_CLASS (VariantElement, VariantView)
BEGIN_TYPELIB_MEMBERS (VariantElement)
	ADD_TYPELIB_MEMBER_STRING (ATTR_PROPERTY) ///< property that selects the current view
	ADD_TYPELIB_MEMBER_STRING (ATTR_CONTROLLER) ///< controller that has the property
	ADD_TYPELIB_MEMBER (ATTR_TRANSITION, TypeNames::kEnum) ///< transition that happens when view is replaced.
END_TYPELIB_MEMBERS (VariantElement)
DEFINE_TYPELIB_ENUMERATION (TAG_VARIANT, ATTR_OPTIONS, VariantView::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

VariantElement::VariantElement ()
: transitionType (Styles::kTransitionNone)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VariantElement::setAttributes (const SkinAttributes& a)
{
	propertyId = a.getString (ATTR_PROPERTY);
	controller = a.getString (ATTR_CONTROLLER);
	a.getOptions (options, ATTR_OPTIONS, VariantView::customStyles);
	transitionType = a.getOptions (ATTR_TRANSITION, ViewAnimator::transitionTypes, true, Styles::kTransitionNone);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VariantElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_PROPERTY, propertyId);
	a.setString (ATTR_CONTROLLER, getController ());
	a.setOptions (ATTR_TRANSITION, transitionType, ViewAnimator::transitionTypes, true);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VariantElement::appendOptions (String& string) const
{
	SkinAttributes::makeOptionsString (string, options.custom, VariantView::customStyles);
	return SuperClass::appendOptions (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* VariantElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		if(propertyId.isEmpty ())
		{
			IParameter* param = ControlElement::getParameter (args, getName (), this);
			view = NEW VariantView (args.controller, size, param, options);
		}
		else
		{
			// lookup controller for property (optional)
			IUnknown* propertyController = args.controller;
			if(!getController ().isEmpty ())
			{
				SkinWizard::ResolvedName controllerName (args.wizard, getController ());
				propertyController = args.wizard.lookupController (args.controller, controllerName.string ());
				if(!propertyController)
				{
					SKIN_WARNING (this, "Controller not found for Variant: '%s'", MutableCString (controllerName.string ()).str ())
					CCL_DEBUGGER ("Controller not found for Variant.\n");
				}
			}

			SkinWizard::ResolvedName resolvedPropertyId (args.wizard, propertyId);
			view = NEW VariantView (propertyController, size, resolvedPropertyId.string (), options);
		}

		((VariantView*)view)->setTransitionType (transitionType);
	}
	return SuperClass::createView (args, view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VariantElement::viewCreated (View* view)
{
	((VariantView*)view)->onChildsAdded ();

	SuperClass::viewCreated (view);
}

//************************************************************************************************
// LabelElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (LabelElement, ViewElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (LabelElement, ViewElement, TAG_LABEL, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (LabelElement, DOC_GROUP_VIEWS)
DEFINE_TYPELIB_RELATED_CLASS (LabelElement, Label)
DEFINE_TYPELIB_ENUMERATION (TAG_LABEL, ATTR_OPTIONS, Label::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LabelElement::setAttributes (const SkinAttributes& a)
{
	a.getOptions (options, ATTR_OPTIONS, Label::customStyles);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LabelElement::appendOptions (String& string) const
{
	SkinAttributes::makeOptionsString (string, options.custom, Label::customStyles);
	return SuperClass::appendOptions (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* LabelElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
		view = NEW Label (size, options, title);
	return SuperClass::createView (args, view);
}

//************************************************************************************************
// HeadingElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (HeadingElement, LabelElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (HeadingElement, LabelElement, TAG_HEADING, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (HeadingElement, DOC_GROUP_VIEWS)
DEFINE_TYPELIB_RELATED_CLASS (HeadingElement, Heading)
BEGIN_TYPELIB_MEMBERS (HeadingElement)
	ADD_TYPELIB_MEMBER (ATTR_LEVEL, TypeNames::kInt) ///< heading level (1 to 6)
END_TYPELIB_MEMBERS (HeadingElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

HeadingElement::HeadingElement ()
: level (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HeadingElement::setAttributes (const SkinAttributes& a)
{
	level = a.getInt (ATTR_LEVEL);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* HeadingElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		view = NEW Heading (size, options, title);

		// assign standard heading style based on level
		bool warned = false;
		int styleIndex = level > 0 ? ThemePainter::kHeading1Style + level - 1 : ThemePainter::kHeading1Style;
		if(styleIndex > ThemePainter::kLastHeadingStyle)
		{
			SKIN_WARNING (this, "Heading level %d not supported.", level)
			warned = true;
		}

		ccl_upper_limit<int> (styleIndex, ThemePainter::kLastHeadingStyle);

		VisualStyle* headingStyle = args.wizard.getTheme ()->getStandardStyle (styleIndex);
		if(!headingStyle && !warned)
			SKIN_WARNING (this, "Heading style %d not found.", level)

		view->setVisualStyle (headingStyle);
	}
	return SuperClass::createView (args, view);
}

//************************************************************************************************
// PictureElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PictureElement, ImageViewElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (PictureElement, ImageViewElement, TAG_PICTURE, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (PictureElement, DOC_GROUP_VIEWS)
DEFINE_TYPELIB_RELATED_CLASS (PictureElement, PictureViewer)

//////////////////////////////////////////////////////////////////////////////////////////////////

View* PictureElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
		view = NEW PictureViewer (nullptr, size, imageStyle);

	return SuperClass::createView (args, view);
}

//************************************************************************************************
// PopupBoxElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PopupBoxElement, ViewElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (PopupBoxElement, ViewElement, TAG_POPUPBOX, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (PopupBoxElement, DOC_GROUP_VIEWS)
DEFINE_TYPELIB_RELATED_CLASS (PopupBoxElement, PopupBox)
BEGIN_TYPELIB_MEMBERS (PopupBoxElement)
	ADD_TYPELIB_MEMBER (ATTR_POPUP, TypeNames::kEnum) ///< Specifies the alignment of the popup relative to the PopupBox.
	ADD_TYPELIB_MEMBER_STRING (ATTR_POPUPSTYLE) ///< Specifies the style that is used for the popup
	ADD_TYPELIB_MEMBER_STRING (ATTR_FORMNAME) ///< The name of the skin form that defines the view that pops up.
END_TYPELIB_MEMBERS (PopupBoxElement)
DEFINE_TYPELIB_ENUMERATION (TAG_POPUPBOX, ATTR_POPUP, PopupSelector::popupStyles, SKIN_TYPELIB)
DEFINE_TYPELIB_ENUMERATION (TAG_POPUPBOX, ATTR_OPTIONS, PopupBox::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupBoxElement::PopupBoxElement ()
: popupOptions (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PopupBoxElement::setAttributes (const SkinAttributes& a)
{
	a.getOptions (options, ATTR_OPTIONS, PopupBox::customStyles);
	popupOptions = a.getOptions (ATTR_POPUP, PopupSelector::popupStyles);
	popupStyleName = a.getString (ATTR_POPUPSTYLE);
	formName = a.getString (ATTR_FORMNAME);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PopupBoxElement::getAttributes (SkinAttributes& a) const
{
	a.setOptions (ATTR_OPTIONS, options, PopupBox::customStyles);
	a.setOptions (ATTR_POPUP, popupOptions, PopupSelector::popupStyles);
	a.setString (ATTR_POPUPSTYLE, popupStyleName);
	a.setString (ATTR_FORMNAME, formName);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* PopupBoxElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		IParameter* param = nullptr;
		if(options.isCustomStyle (Styles::kPopupBoxBehaviorSlider | Styles::kPopupBoxBehaviorHasTriggerParameter))
			param = ControlElement::getParameter (args, getName (), this);

		// try to find a popup client ...
		UnknownPtr<IPopupSelectorClient> client;

		// 1) special client for popup slider
		if(options.isCustomStyle (Styles::kPopupBoxBehaviorSlider))
		{
			if(param)
			{
				PopupSlider* sliderClient = NEW PopupSlider (param, options);
				UnknownPtr<IObjectNode> controller (args.controller);
				sliderClient->setSourceController (controller);
				client = sliderClient->asUnknown ();
				client->release ();
			}
		}

		// 2) try IController::getObject
		if(!client)
		{
			UnknownPtr<IController> controller (args.controller);
			if(controller)
				client = controller->getObject (getName (), ccl_iid<IPopupSelectorClient> ());
		}

		// 3) try a child (eg. subComponent) of the controller
		if(!client && !getName ().isEmpty ())
		{
			UnknownPtr<IObjectNode> iNode (args.controller);
			if(iNode)
				client = iNode->lookupChild (String (getName ()));
		}

		// 4) default client implementation
		if(!client)
		{
			SimplePopupSelectorClient* simpleClient = NEW SimplePopupSelectorClient;
			UnknownPtr<IObjectNode> controller (args.controller);
			simpleClient->setSourceController (controller);
			client = (IPopupSelectorClient*)simpleClient;
			client->release ();
		}

		PopupBox* popupBox = NEW PopupBox (client, formName, size, param, options);
		args.wizard.getVariables (popupBox->getFormVariables ());

		if(!popupStyleName.isEmpty ())
		{
			SkinWizard::ResolvedName resolvedName (args.wizard, popupStyleName);
			CString resolvedPopupStyle = resolvedName.string ();
			if(VisualStyle* popupStyle = args.wizard.getModel ().getStyle (resolvedPopupStyle, this))
				popupBox->setPopupVisualStyle (popupStyle);
		}

		view = popupBox;
	}

	if(popupOptions)
		((PopupBox*)view)->setPopupOptions (popupOptions);

	return SuperClass::createView (args, view);
}

//************************************************************************************************
// DialogGroupElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DialogGroupElement, ViewElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (DialogGroupElement, ViewElement, TAG_DIALOGGROUP, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (DialogGroupElement, DOC_GROUP_LAYOUT)
DEFINE_TYPELIB_RELATED_CLASS (DialogGroupElement, DialogGroup)
DEFINE_TYPELIB_ENUMERATION (TAG_DIALOGGROUP, ATTR_OPTIONS, DialogGroup::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DialogGroupElement::setAttributes (const SkinAttributes& a)
{
	a.getOptions (options, ATTR_OPTIONS, DialogGroup::customStyles);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DialogGroupElement::getAttributes (SkinAttributes& a) const
{
	a.setOptions (ATTR_OPTIONS, options, DialogGroup::customStyles);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* DialogGroupElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
		view = NEW DialogGroup (size, options);

	return SuperClass::createView (args, view);
}

//************************************************************************************************
// TargetElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TargetElement, ViewElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (TargetElement, ViewElement, TAG_TARGET, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (TargetElement, DOC_GROUP_VIEWS)
BEGIN_TYPELIB_ATTRIBUTES (TargetElement)
	ADD_TYPELIB_SCHEMAGROUP_ATTRIBUTE ("") // remove inherited schema groups
END_TYPELIB_ATTRIBUTES (TargetElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

TargetElement::TargetElement (ViewElement* parent)
{
	setParent (parent);
	setName (parent->getName ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TargetElement::TargetElement ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TargetElement::setAttributes (const SkinAttributes& a)
{
	a.getOptions (options, ATTR_OPTIONS);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* TargetElement::createView (const CreateArgs& args, View* view)
{
	View* v = SuperClass::createView (args, view);
	{
		// apply options from Target element (to a view possibly created via IViewFactory)
		View::StyleModifier style (*v);
		style.common |= options.common;
		style.custom |= options.custom;
	}
	return v;
}

//************************************************************************************************
// ScrollHeaderElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ScrollHeaderElement, TargetElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (ScrollHeaderElement, TargetElement, TAG_SCROLLHEADER, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (ScrollHeaderElement, DOC_GROUP_VIEWS)

//////////////////////////////////////////////////////////////////////////////////////////////////

ScrollHeaderElement::ScrollHeaderElement (ViewElement* parent)
: TargetElement (parent)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScrollHeaderElement::ScrollHeaderElement ()
{}

//************************************************************************************************
// ScrollViewElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ScrollViewElement, ViewElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (ScrollViewElement, ViewElement, TAG_SCROLLVIEW, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (ScrollViewElement, DOC_GROUP_VIEWS)
DEFINE_TYPELIB_RELATED_CLASS (ScrollViewElement, ScrollView)
BEGIN_TYPELIB_MEMBERS (ScrollViewElement)
	ADD_TYPELIB_MEMBER_STRING (ATTR_PERSISTENCE_ID) ///< storage id used to store and restore the scroll state
	ADD_TYPELIB_MEMBER_STRING (ATTR_HSCROLLSTYLE) ///< name of a visual style for the horizontal scrollbar
	ADD_TYPELIB_MEMBER_STRING (ATTR_VSCROLLSTYLE) ///< name of a visual style for the vertical scrollbar
	ADD_TYPELIB_MEMBER_STRING (ATTR_HSCROLLNAME) ///< name of the horizontal scroll parameter
	ADD_TYPELIB_MEMBER_STRING (ATTR_VSCROLLNAME) ///< name of the vertical scroll parameter
END_TYPELIB_MEMBERS (ScrollViewElement)
BEGIN_TYPELIB_ATTRIBUTES (ScrollViewElement)
	ADD_TYPELIB_CHILDGROUP_ATTRIBUTE (TAG_TARGET)
END_TYPELIB_ATTRIBUTES (ScrollViewElement)

DEFINE_TYPELIB_ENUMERATION (TAG_SCROLLVIEW, ATTR_OPTIONS, ScrollView::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

ScrollViewElement::ScrollViewElement ()
: targetElement (nullptr),
  headerElement (nullptr)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScrollViewElement::~ScrollViewElement ()
{
	if(targetElement)
		targetElement->release ();
	if(headerElement)
		headerElement->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollViewElement::setAttributes (const SkinAttributes& a)
{
	a.getOptions (options, ATTR_OPTIONS, ScrollView::customStyles);
	persistenceID = a.getString (ATTR_PERSISTENCE_ID);

	horizontalScrollBarStyle = a.getString (ATTR_HSCROLLSTYLE);
	verticalScrollBarStyle = a.getString (ATTR_VSCROLLSTYLE);
	horizontalScrollName = a.getString (ATTR_HSCROLLNAME);
	verticalScrollName = a.getString (ATTR_VSCROLLNAME);

	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScrollViewElement::getAttributes (SkinAttributes& a) const
{
	a.setOptions (ATTR_OPTIONS, options, ScrollView::customStyles);
	a.setString (ATTR_PERSISTENCE_ID, persistenceID);

	if(!horizontalScrollBarStyle.isEmpty ())
		a.setString (ATTR_HSCROLLSTYLE, horizontalScrollBarStyle);
	if(!verticalScrollBarStyle.isEmpty ())
		a.setString (ATTR_VSCROLLSTYLE, verticalScrollBarStyle);
	if(!horizontalScrollName.isEmpty ())
		a.setString (ATTR_HSCROLLNAME, horizontalScrollName);
	if(!verticalScrollName.isEmpty ())
		a.setString (ATTR_VSCROLLNAME, verticalScrollName);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ScrollViewElement::addChild (Element* e, int index)
{
	if(TargetElement* _targetElement = ccl_cast<TargetElement> (e))
	{
		if(ScrollHeaderElement* _headerElement = ccl_cast<ScrollHeaderElement> (_targetElement))
		{
			ASSERT (headerElement == nullptr)
			headerElement = _headerElement;
		}
		else
		{
			ASSERT (targetElement == nullptr)
			targetElement = _targetElement;
		}
		_targetElement->setParent (this);
	}
	else
	{
		ASSERT (0)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ScrollViewElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		// we use a dummy element to create the target view
		if(targetElement == nullptr)
			targetElement = NEW TargetElement (this);

		View* target = targetElement->createView (args);
		ASSERT (target != nullptr)

		// this is a rare case where the sizeMode of a <Form> must not be overriden on usage
		if(Form* form = ccl_cast<Form> (target))
			form->setSizeMode (form->getSkinElement ()->getSizeMode ());

		if(!styleClass.isEmpty ())
			visualStyle = args.wizard.getModel ().getStyle (styleClass, this);

		view = NEW ScrollView (size, target, options, visualStyle, args.wizard.getZoomFactor ());

		if(headerElement)
		{
			View* headerView = headerElement->createView (args);
			ASSERT (headerView != nullptr)
			if(headerView)
				((ScrollView*)view)->setHeader (headerView);
		}
	}
	((ScrollView*)view)->setPersistenceID (persistenceID);

	// individual scrollbar styles
	if(!horizontalScrollBarStyle.isEmpty ())
		if(VisualStyle* visualStyle = args.wizard.getModel ().getStyle (horizontalScrollBarStyle, this))
			((ScrollView*)view)->setHScrollBarStyle (visualStyle);

	if(!verticalScrollBarStyle.isEmpty ())
		if(VisualStyle* visualStyle = args.wizard.getModel ().getStyle (verticalScrollBarStyle, this))
			((ScrollView*)view)->setVScrollBarStyle (visualStyle);

	// scroll parameters
	if(!horizontalScrollName.isEmpty ())
	{
		IParameter* scrollParam = ControlElement::getParameter (args, horizontalScrollName, this);
		if(scrollParam)
			((ScrollView*)view)->setHScrollParam (scrollParam);
	}
	if(!verticalScrollName.isEmpty ())
	{
		IParameter* scrollParam = ControlElement::getParameter (args, verticalScrollName, this);
		if(scrollParam)
			((ScrollView*)view)->setVScrollParam (scrollParam);
	}

	return SuperClass::createView (args, view);
}

//************************************************************************************************
// ItemViewElement
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ItemViewElement, ScrollViewElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (ItemViewElement, ScrollViewElement, TAG_ITEMVIEW, SKIN_TYPELIB)
DEFINE_TYPELIB_ABSTRACT (ItemViewElement)
DEFINE_TYPELIB_DOCGROUP (ItemViewElement, DOC_GROUP_VIEWS)
BEGIN_TYPELIB_MEMBERS (ItemViewElement)
	ADD_TYPELIB_MEMBER (ATTR_SCROLLOPTIONS, TypeNames::kEnum) ///< Options for the surrounding scrollview
	ADD_TYPELIB_MEMBER_STRING (ATTR_HEADERSTYLE) ///< Name of a visual style that will be assigned to the column header view
END_TYPELIB_MEMBERS (ItemViewElement)
BEGIN_TYPELIB_ATTRIBUTES (ItemViewElement)
	ADD_TYPELIB_CHILDGROUP_ATTRIBUTE (SCHEMA_GROUP_VIEWSSTATEMENTS)
END_TYPELIB_ATTRIBUTES (ItemViewElement)

DEFINE_TYPELIB_ENUMERATION_WITH_PARENT (TAG_ITEMVIEW, ATTR_SCROLLOPTIONS, nullptr, TAG_SCROLLVIEW, ATTR_OPTIONS, SKIN_TYPELIB)
DEFINE_TYPELIB_ENUMERATION (TAG_ITEMVIEW, ATTR_OPTIONS, ItemView::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemViewElement::setAttributes (const SkinAttributes& a)
{
	bool result = SuperClass::setAttributes (a);
	a.getOptions (options, ATTR_OPTIONS, ItemView::customStyles);
	if(getCustomDef ())
		options.custom |= a.getOptions (ATTR_OPTIONS, getCustomDef ());

	a.getOptions (scrollOptions, ATTR_SCROLLOPTIONS, ScrollView::customStyles);
	headerStyleName = a.getString (ATTR_HEADERSTYLE);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemViewElement::getAttributes (SkinAttributes& a) const
{
	bool result = SuperClass::getAttributes (a);
	a.setOptions (ATTR_SCROLLOPTIONS, scrollOptions, ScrollView::customStyles);
	a.setString (ATTR_HEADERSTYLE, headerStyleName);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemViewElement::appendOptions (String& string) const
{
	SkinAttributes::makeOptionsString (string, options.custom, ItemView::customStyles);
	if(getCustomDef ())
		SkinAttributes::makeOptionsString (string, options.custom, getCustomDef ());

	return SuperClass::appendOptions (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IItemModel* ItemViewElement::getModel (const CreateArgs& args)
{
	UnknownPtr<IItemModel> itemModel;
	UnknownPtr<IController> controller (args.controller);
	if(controller)
	{
		SkinWizard::ResolvedName resolvedName (args.wizard, getName ());
		itemModel = controller->getObject (resolvedName.string (), ccl_iid<IItemModel> ());
	}
	return itemModel;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ItemViewElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
		view = createControl (args);

	if(!headerStyleName.isEmpty ())
		if(VisualStyle* headerStyle = args.wizard.getModel ().getStyle (headerStyleName, this))
			if(ItemControl* itemControl = ccl_cast<ItemControl> (view))
				itemControl->setHeaderViewStyle (headerStyle);

	SuperClass::createView (args, view);

	// Note: model must be assigned *after* name attribute has been set!
	ItemControlBase* itemControl = ccl_cast<ItemControlBase> (view);
	if(itemControl)
	{
		IItemModel* model = getModel (args);
		AutoPtr<IItemModel> modelToRelease;
		if(model == nullptr)
		{
			IParameter* param = ControlElement::getParameter (args, getName (), this);
			if(param)
				modelToRelease = model = NEW ParamItemModel (getName (), param);
		}

		if(model)
			itemControl->getItemView ()->setModel (model);
	}
	return view;
}

//************************************************************************************************
// ListViewElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ListViewElement, ItemViewElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (ListViewElement, ItemViewElement, TAG_LISTVIEW, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (ListViewElement, DOC_GROUP_VIEWS)
DEFINE_TYPELIB_RELATED_CLASS (ListViewElement, ListControl)
BEGIN_TYPELIB_MEMBERS (ListViewElement)
	ADD_TYPELIB_MEMBER (ATTR_VIEWTYPE, TypeNames::kEnum)
	ADD_TYPELIB_MEMBER (ATTR_TEXTTRIMMODE, TypeNames::kEnum)
END_TYPELIB_MEMBERS (ListViewElement)
DEFINE_TYPELIB_ENUMERATION (TAG_LISTVIEW, ATTR_VIEWTYPE, ListView::viewTypeNames, SKIN_TYPELIB)
DEFINE_TYPELIB_ENUMERATION (TAG_LISTVIEW, ATTR_OPTIONS, ListView::customStyles, SKIN_TYPELIB)
DEFINE_TYPELIB_ENUMERATION_WITH_PARENT (TAG_LISTVIEW, ATTR_TEXTTRIMMODE, nullptr, TAG_TEXTBOX, ATTR_TEXTTRIMMODE, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

ListViewElement::ListViewElement ()
: viewType (Styles::kListViewList),
  textTrimMode (Font::kTrimModeDefault)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewElement::setAttributes (const SkinAttributes& a)
{
	viewType = a.getOptions (ATTR_VIEWTYPE, ListView::viewTypeNames, true, Styles::kListViewList);
	textTrimMode = a.getOptions (ATTR_TEXTTRIMMODE, FontElement::textTrimModes, true, Font::kTrimModeDefault);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewElement::getAttributes (SkinAttributes& a) const
{
	a.setOptions (ATTR_VIEWTYPE, viewType, ListView::viewTypeNames, true);
	a.setOptions (ATTR_TEXTTRIMMODE, textTrimMode, FontElement::textTrimModes, true);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const StyleDef* ListViewElement::getCustomDef () const
{
	return ListView::customStyles;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemControlBase* ListViewElement::createControl (const CreateArgs& args)
{
	ListControl* listControl = NEW ListControl (size, nullptr, options, scrollOptions);
	ListView* listView = ccl_cast<ListView> (listControl->getItemView ());
	listView->setViewType (viewType);
	listView->setTextTrimMode (textTrimMode);
	return listControl;
}

//************************************************************************************************
// TreeViewElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TreeViewElement, ItemViewElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (TreeViewElement, ItemViewElement, TAG_TREEVIEW, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (TreeViewElement, DOC_GROUP_VIEWS)
DEFINE_TYPELIB_RELATED_CLASS (TreeViewElement, TreeControl)
DEFINE_TYPELIB_ENUMERATION (TAG_TREEVIEW, ATTR_OPTIONS, TreeView::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

const StyleDef* TreeViewElement::getCustomDef () const
{
	return TreeView::customStyles;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemControlBase* TreeViewElement::createControl (const CreateArgs& args)
{
	return NEW TreeControl (size, nullptr, options, scrollOptions);
}

//************************************************************************************************
// DropBoxElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DropBoxElement, ItemViewElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (DropBoxElement, ItemViewElement, TAG_DROPBOX, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (DropBoxElement, DOC_GROUP_VIEWS)
DEFINE_TYPELIB_RELATED_CLASS (DropBoxElement, DropBoxControl)
DEFINE_TYPELIB_ENUMERATION (TAG_DROPBOX, ATTR_OPTIONS, DropBox::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

const StyleDef* DropBoxElement::getCustomDef () const
{
	return DropBox::customStyles;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemControlBase* DropBoxElement::createControl (const CreateArgs& args)
{
	auto* dropBoxControl = NEW DropBoxControl (size, options, scrollOptions);
	args.wizard.getVariables (dropBoxControl->getDropBoxArguments ());
	if(DropBox* dropBox = ccl_cast<DropBox> (dropBoxControl->getItemView ()))
		dropBox->setScopeName (args.wizard.getCurrentScope ());

	return dropBoxControl;
}

//************************************************************************************************
// WebViewElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WebViewElement, ViewElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (WebViewElement, ViewElement, TAG_WEBVIEW, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (WebViewElement, DOC_GROUP_VIEWS)
DEFINE_TYPELIB_RELATED_CLASS (WebViewElement, WebBrowserView)
DEFINE_TYPELIB_ENUMERATION (TAG_WEBVIEW, ATTR_OPTIONS, WebBrowserView::customStyles, SKIN_TYPELIB)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebViewElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);

	StyleFlags webViewOptions;
	a.getOptions (webViewOptions, ATTR_OPTIONS, WebBrowserView::customStyles);
	options.custom |= webViewOptions.custom;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* WebViewElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
		view = NEW WebBrowserView (args.controller, size, options);

	return SuperClass::createView (args, view);
}

//************************************************************************************************
// CommandBarViewElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CommandBarViewElement, ViewElement)
DEFINE_TYPELIB_CLASS_WITH_PARENT (CommandBarViewElement, ViewElement, TAG_COMMANDBARVIEW, SKIN_TYPELIB)
DEFINE_TYPELIB_DOCGROUP (CommandBarViewElement, DOC_GROUP_CONTROLS)
DEFINE_TYPELIB_RELATED_CLASS (CommandBarViewElement, CommandBarView)
BEGIN_TYPELIB_MEMBERS (CommandBarViewElement)
	ADD_TYPELIB_MEMBER_STRING (ATTR_FORMNAME) ///< name of a form for items
	ADD_TYPELIB_MEMBER_STRING (ATTR_MENUFORMNAME) ///< name of a from for a context menu
END_TYPELIB_MEMBERS (CommandBarViewElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandBarViewElement::setAttributes (const SkinAttributes& a)
{
	a.getOptions (options, ATTR_OPTIONS);

	itemFormName = a.getString (ATTR_FORMNAME);
	contextMenuFormName = a.getString (ATTR_MENUFORMNAME);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* CommandBarViewElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		view = NEW CommandBarView (size);
		((CommandBarView*)view)->setController (args.controller);
		if(!itemFormName.isEmpty ())
			((CommandBarView*)view)->setItemFormName (itemFormName);
		if(!contextMenuFormName.isEmpty ())
			((CommandBarView*)view)->setContextMenuFormName (contextMenuFormName);
		view->setStyle (options);

		CommandBarModel* model = nullptr;
		if(UnknownPtr<IController> controller = args.controller)
			model = unknown_cast<CommandBarModel> (controller->getObject (getName (), ccl_typeid<CommandBarModel> ().getClassID ()));

		((CommandBarView*)view)->setModel (model);
	}

	return SuperClass::createView (args, view);
}

} // namespace SkinElements
} // namespace CCL
