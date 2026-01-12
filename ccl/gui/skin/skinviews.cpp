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

//////////////////////////////////////////////////////////////////////////////////////////////////

void linkSkinViews ()
{} // force linkage of this file

//************************************************************************************************
// VariantElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (VariantElement, ViewElement, TAG_VARIANT, DOC_GROUP_LAYOUT, VariantView)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_PROPERTY, TYPE_STRING)   ///< property that selects the current view
	ADD_SKIN_ELEMENT_MEMBER (ATTR_CONTROLLER, TYPE_STRING) ///< controller that has the property
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TRANSITION, TYPE_ENUM)   ///< transition that happens when view is replaced.
END_SKIN_ELEMENT_WITH_MEMBERS (VariantElement)
DEFINE_SKIN_ENUMERATION (TAG_VARIANT, ATTR_OPTIONS, VariantView::customStyles)

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

DEFINE_SKIN_ELEMENT (LabelElement, ViewElement, TAG_LABEL, DOC_GROUP_VIEWS, Label)
DEFINE_SKIN_ENUMERATION (TAG_LABEL, ATTR_OPTIONS, Label::customStyles)

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

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (HeadingElement, LabelElement, TAG_HEADING, DOC_GROUP_VIEWS, Heading)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_LEVEL, TYPE_INT) ///< heading level (1, 2, or 3)
END_SKIN_ELEMENT_WITH_MEMBERS (HeadingElement)

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

DEFINE_SKIN_ELEMENT (PictureElement, ImageViewElement, TAG_PICTURE, DOC_GROUP_VIEWS, PictureViewer)

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

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (PopupBoxElement, ViewElement, TAG_POPUPBOX, DOC_GROUP_VIEWS, PopupBox)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_POPUP, TYPE_ENUM)		   ///< Specifies the alignment of the popup relative to the PopupBox.
	ADD_SKIN_ELEMENT_MEMBER (ATTR_POPUPSTYLE, TYPE_STRING) ///< Specifies the style that is used for the popup
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FORMNAME, TYPE_STRING)   ///< The name of the skin form that defines the view that pops up.
END_SKIN_ELEMENT_WITH_MEMBERS (PopupBoxElement)
DEFINE_SKIN_ENUMERATION (TAG_POPUPBOX, ATTR_POPUP, PopupSelector::popupStyles)
DEFINE_SKIN_ENUMERATION (TAG_POPUPBOX, ATTR_OPTIONS, PopupBox::customStyles)

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

DEFINE_SKIN_ELEMENT (DialogGroupElement, ViewElement, TAG_DIALOGGROUP, DOC_GROUP_LAYOUT, DialogGroup)
DEFINE_SKIN_ENUMERATION (TAG_DIALOGGROUP, ATTR_OPTIONS, DialogGroup::customStyles)

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

DEFINE_SKIN_ELEMENT (TargetElement, ViewElement, TAG_TARGET, DOC_GROUP_VIEWS, 0)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (TargetElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE ("") // remove inherited schema groups
END_SKIN_ELEMENT_ATTRIBUTES (TargetElement)

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

DEFINE_SKIN_ELEMENT (ScrollHeaderElement, TargetElement, TAG_SCROLLHEADER, DOC_GROUP_VIEWS, 0)

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

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (ScrollViewElement, ViewElement, TAG_SCROLLVIEW, DOC_GROUP_VIEWS, ScrollView)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_PERSISTENCE_ID, TYPE_STRING) ///< storage id used to store and restore the scroll state
	ADD_SKIN_ELEMENT_MEMBER (ATTR_HSCROLLSTYLE, TYPE_STRING)   ///< name of a visual style for the horizontal scrollbar
	ADD_SKIN_ELEMENT_MEMBER (ATTR_VSCROLLSTYLE, TYPE_STRING)   ///< name of a visual style for the vertical scrollbar
	ADD_SKIN_ELEMENT_MEMBER (ATTR_HSCROLLNAME, TYPE_STRING)	   ///< name of the horizontal scroll parameter
	ADD_SKIN_ELEMENT_MEMBER (ATTR_VSCROLLNAME, TYPE_STRING)	   ///< name of the vertical scroll parameter
END_SKIN_ELEMENT_WITH_MEMBERS (ScrollViewElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (ScrollViewElement)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (TAG_TARGET)
END_SKIN_ELEMENT_ATTRIBUTES (ScrollViewElement)

DEFINE_SKIN_ENUMERATION (TAG_SCROLLVIEW, ATTR_OPTIONS, ScrollView::customStyles)

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
	horizontalScrollValue = a.getString (ATTR_HSCROLLNAME);
	verticalScrollValue = a.getString (ATTR_VSCROLLNAME);

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
	if(!horizontalScrollValue.isEmpty ())
		a.setString (ATTR_HSCROLLNAME, horizontalScrollValue);
	if(!verticalScrollValue.isEmpty ())
		a.setString (ATTR_VSCROLLNAME, verticalScrollValue);
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
	if(!horizontalScrollValue.isEmpty ())
	{
		IParameter* scrollParam = ControlElement::getParameter (args, horizontalScrollValue, this);
		if(scrollParam)
			((ScrollView*)view)->setHScrollParam (scrollParam);
	}
	if(!verticalScrollValue.isEmpty ())
	{
		IParameter* scrollParam = ControlElement::getParameter (args, verticalScrollValue, this);
		if(scrollParam)
			((ScrollView*)view)->setVScrollParam (scrollParam);
	}

	return SuperClass::createView (args, view);
}

//************************************************************************************************
// ItemViewElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_ABSTRACT_WITH_MEMBERS (ItemViewElement, ScrollViewElement, TAG_ITEMVIEW, DOC_GROUP_VIEWS, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_SCROLLOPTIONS, TYPE_ENUM) ///< Options for the surrounding scrollview
	ADD_SKIN_ELEMENT_MEMBER (ATTR_HEADERSTYLE, TYPE_STRING) ///< Name of a visual style that will be assigned to the column header view
END_SKIN_ELEMENT_WITH_MEMBERS (ItemViewElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (ItemViewElement)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (SCHEMA_GROUP_VIEWSSTATEMENTS)
END_SKIN_ELEMENT_ATTRIBUTES (ItemViewElement)

DEFINE_SKIN_ENUMERATION_PARENT (TAG_ITEMVIEW, ATTR_SCROLLOPTIONS, nullptr, TAG_SCROLLVIEW, ATTR_OPTIONS)
DEFINE_SKIN_ENUMERATION (TAG_ITEMVIEW, ATTR_OPTIONS, ItemView::customStyles)

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

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (ListViewElement, ItemViewElement, TAG_LISTVIEW, DOC_GROUP_VIEWS, ListControl)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_VIEWTYPE, TYPE_ENUM)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TEXTTRIMMODE, TYPE_ENUM)
END_SKIN_ELEMENT_WITH_MEMBERS (ListViewElement)
DEFINE_SKIN_ENUMERATION (TAG_LISTVIEW, ATTR_VIEWTYPE, ListView::viewTypeNames)
DEFINE_SKIN_ENUMERATION (TAG_LISTVIEW, ATTR_OPTIONS, ListView::customStyles)
DEFINE_SKIN_ENUMERATION_PARENT (TAG_LISTVIEW, ATTR_TEXTTRIMMODE, nullptr, TAG_TEXTBOX, ATTR_TEXTTRIMMODE)

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

DEFINE_SKIN_ELEMENT (TreeViewElement, ItemViewElement, TAG_TREEVIEW, DOC_GROUP_VIEWS, TreeControl)
DEFINE_SKIN_ENUMERATION (TAG_TREEVIEW, ATTR_OPTIONS, TreeView::customStyles)

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

DEFINE_SKIN_ELEMENT (DropBoxElement, ItemViewElement, TAG_DROPBOX, DOC_GROUP_VIEWS, DropBoxControl)
DEFINE_SKIN_ENUMERATION (TAG_DROPBOX, ATTR_OPTIONS, DropBox::customStyles)

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
	return dropBoxControl;
}

//************************************************************************************************
// WebViewElement
//************************************************************************************************

DEFINE_SKIN_ELEMENT (WebViewElement, ViewElement, TAG_WEBVIEW, DOC_GROUP_VIEWS, WebBrowserView)
DEFINE_SKIN_ENUMERATION (TAG_WEBVIEW, ATTR_OPTIONS, WebBrowserView::customStyles)

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

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (CommandBarViewElement, ViewElement, TAG_COMMANDBARVIEW, DOC_GROUP_CONTROLS, CommandBarView)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FORMNAME, TYPE_STRING)	 ///< name of a form for items
	ADD_SKIN_ELEMENT_MEMBER (ATTR_MENUFORMNAME, TYPE_STRING) ///< name of a from for a context menu
END_SKIN_ELEMENT_WITH_MEMBERS (CommandBarViewElement)

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
