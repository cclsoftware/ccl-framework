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
// Filename    : ccl/gui/dialogs/dialogbuilder.cpp
// Description : Dialog Builder
//
//************************************************************************************************

#include "ccl/gui/dialogs/dialogbuilder.h"
#include "ccl/gui/dialogs/alert.h"
#include "ccl/gui/popup/menu.h"

#include "ccl/gui/theme/thememanager.h"

#include "ccl/gui/controls/label.h"
#include "ccl/gui/controls/button.h"
#include "ccl/gui/controls/selectbox.h"
#include "ccl/gui/controls/valuebox.h"
#include "ccl/gui/controls/colorbox.h"
#include "ccl/gui/controls/segmentbox.h"
#include "ccl/gui/views/dialoggroup.h"
#include "ccl/gui/views/viewdecorator.h"
#include "ccl/gui/layout/anchorlayout.h"
#include "ccl/gui/windows/desktop.h"

#include "ccl/gui/skin/form.h"

#include "ccl/public/gui/icontroller.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/iparamobserver.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/ivisualstyle.h"
#include "ccl/public/text/itranslationtable.h"
#include "ccl/public/text/translation.h"

#include "ccl/app/paramcontainer.h"

#include "ccl/base/asyncoperation.h"

namespace CCL {

class StandardButton;

//************************************************************************************************
// StandardDialog
//************************************************************************************************

class StandardDialog: public Dialog,
					  public IParamObserver
{
public:
	DECLARE_CLASS (StandardDialog, Dialog)

	StandardDialog (const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);
	~StandardDialog ();

	void registerButton (StandardButton* button);
	void unregisterButton (StandardButton* button);
	void pushButton (int buttonIndex);

	void prepareButtons (int standardButtons, const ObjectArray& customButtonItems);
	void setButtonOrder (Styles::ButtonOrder newOrder);

	class ButtonItem;
	ObjectArray& getButtonItems () { return buttonItems; }
	ButtonItem* getButtonItemForResult (int dialogResult) const;
	ButtonItem* getButtonItemAt (int index) const;

	// IParamObserver
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API paramEdit (IParameter* param, tbool begin) override {}

	// Dialog
	void attached (View* parent) override;
	void removed (View* parent) override;
	bool onKeyDown (const KeyEvent& event) override;
	bool onClose () override;

	CLASS_INTERFACE (IParamObserver, Dialog)

protected:
	ObjectArray standardButtons;
	ObjectArray buttonItems;
	Styles::ButtonOrder buttonOrder;

	class Information: public DialogInformation
	{
	public:
		Information (StandardDialog& dialog)
		: DialogInformation (kStandardDialog, nullptr, dialog.getTitle ()),
		  dialog (dialog)
		{}

		// DialogInformation
		void CCL_API close (int buttonIndex) override
		{
			dialog.pushButton (buttonIndex);
		}

	protected:
		StandardDialog& dialog;
	};

	AutoPtr<Information> dialogInformation;
};

//************************************************************************************************
// StandardDialog::ButtonItem
//************************************************************************************************

class StandardDialog::ButtonItem: public Object
{
public:
	ButtonItem  ()
	: dialogResult (DialogResult::kNone),
	  buttonRole (DialogResult::kNone)
	{}

	PROPERTY_VARIABLE (int, dialogResult, DialogResult)
	PROPERTY_VARIABLE (int, buttonRole, ButtonRole)
		
	PROPERTY_STRING (customTitle, CustomTitle)
	PROPERTY_SHARED_AUTO (IParameter, customParameter, CustomParameter)
};

//************************************************************************************************
// StandardButton
/** A button with a predefined standard behavior.
Usually used in dialogs.

\code{.xml}
<DialogButton result="cancel"/>
\endcode
*/
//************************************************************************************************

class StandardButton: public Button
{
public:
	DECLARE_CLASS (StandardButton, Button)

	StandardButton (const Rect& size = Rect (), int dialogResult = DialogResult::kOkay, StringRef title = nullptr);

	int getResult () const { return dialogResult; }

	// Button
	void attached (View* parent) override;
	void removed (View* parent) override;

protected:
	int dialogResult;
};

//************************************************************************************************
// MenuDialogInformation
//************************************************************************************************

class MenuDialogInformation: public DialogInformation
{
public:
	MenuDialogInformation (IMenu* menu, StringRef text, StringRef title)
	: DialogInformation (kMenuDialog, text, title),
	  popupSelector (nullptr)
	{
		setMenu (menu);
	}

	~MenuDialogInformation ()
	{
		setPopupSelector (nullptr);
	}

	void begin (PopupSelector* ps)
	{
		retain ();
		AlertService::instance ().beginDialog (*this);
		setPopupSelector (ps);
	}

	// DialogInformation
	void CCL_API close (int buttonIndex) override
	{
		if(popupSelector)
			popupSelector->close ();
	}

	void CCL_API notify (ISubject* subject, MessageRef msg) override
	{
		if(msg == PopupSelector::kPopupClosed)
		{
			// notify command handler if menu has been canceled
			bool canceled = popupSelector->getPopupResult () == IPopupSelectorClient::kCancel;
			if(canceled == true)
			{
				if(Menu* menu = unknown_cast<Menu> (getMenu ()))
					if(menu->getHandler ())
						menu->getHandler ()->interpretCommand (CommandMsg ("Navigation", "Cancel"));
			}

			AlertService::instance ().endDialog (*this);
			release ();
		}
	}

protected:
	PopupSelector* popupSelector;

	void setPopupSelector (PopupSelector* ps)
	{
		share_and_observe (this, popupSelector, ps);
	}
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Dialog")
	XSTRING (Cancel, "Cancel")
	XSTRING (Okay,   "OK")
	XSTRING (Close,  "Close")
	XSTRING (Apply,  "Apply")
END_XSTRINGS

//************************************************************************************************
// DialogBuilder::Decorator
//************************************************************************************************

class DialogBuilder::Decorator: public ViewDecorator
{
public:
	Decorator (StandardDialog& dialog, View* contentView)
	: ViewDecorator (contentView, "Standard.DialogFrame"),
	  dialog (dialog)
	{
		// make TitleBarImage from contentView style accessible to skin
		IImage* titleBarImage = contentView ? contentView->getVisualStyle ().getImage ("Dialog.TitleBarImage") : nullptr;
		if(titleBarImage)
			getParamList ().addImage ("titleBarImage")->setImage (titleBarImage);

		getDecorArguments ().setAttribute ("hasTitleBarImage", titleBarImage != nullptr);
	}

	// ViewDecorator
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override
	{
		auto getButtonItem = [&] (StringID arrayKey) -> StandardDialog::ButtonItem*
		{
			int buttonIndex = -1;
			if(arrayKey.getIntValue (buttonIndex))
				return dialog.getButtonItemAt (buttonIndex);
			else
				return nullptr;
		};

		MutableCString arrayKey;
		if(propertyId.getBetween (arrayKey, "hasButton[", "]"))
		{
			var = getButtonItem (arrayKey) != nullptr;
			return true;
		}
		else if(propertyId.getBetween (arrayKey, "buttonResult[", "]"))
		{
			StandardDialog::ButtonItem* item = getButtonItem (arrayKey);
			var = item ? item->getDialogResult () : Styles::kOkayButton;
			return true;
		}
		else if(propertyId.getBetween (arrayKey, "buttonTitle[", "]"))
		{
			StandardDialog::ButtonItem* item = getButtonItem (arrayKey);
			var = item ? item->getCustomTitle () : String::kEmpty;
			return true;
		}
		return SuperClass::getProperty (var, propertyId);
	}

	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override
	{
		UnknownPtr<ISkinCreateArgs> args (data);
		if(args && args->getElement ())
		{
			// the default (platform) button order can be overriden by a view in the dialog template (containing the buttons) via attribute "data.buttonOrder"
			String data;
			if(args->getElement ()->getDataDefinition (data, "buttonOrder"))
			{
				if(data == UserInterface::kAffirmativeButtonLeftID)
					dialog.setButtonOrder (Styles::kAffirmativeButtonLeft);
				else if(data == UserInterface::kAffirmativeButtonRightID)
					dialog.setButtonOrder (Styles::kAffirmativeButtonRight);
			}
		}
		return SuperClass::createView (name, data, bounds);
	}

private:
	StandardDialog& dialog;

	using SuperClass = ViewDecorator;
};

//************************************************************************************************
// DialogBuilder
//************************************************************************************************

View* DialogBuilder::createStandardButton (RectRef rect, int dialogResult, StringRef title)
{
	return NEW StandardButton (rect, dialogResult, title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef DialogBuilder::getStandardButtonTitle (int dialogResult)
{
	switch(dialogResult)
	{
	case DialogResult::kCancel : return XSTR (Cancel);
	case DialogResult::kOkay   : return XSTR (Okay);
	case DialogResult::kClose  : return XSTR (Close);
	case DialogResult::kApply  : return XSTR (Apply);
	default:
		return String::kEmpty;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (DialogBuilder, Object)
DEFINE_CLASS_UID (DialogBuilder, 0x352f4422, 0x89bc, 0x437c, 0x99, 0x77, 0x82, 0xf9, 0xfc, 0xb0, 0x63, 0x5)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_VISUALSTYLE_CLASS (DialogBuilder, VisualStyle, "DialogBuilderStyle")
	ADD_VISUALSTYLE_METRIC  ("noDialogFrame")				///< overrides the global configuration variable GUI.StandardDialog.UseDialogFrame for a dialog
END_VISUALSTYLE_CLASS (DialogBuilder)

//////////////////////////////////////////////////////////////////////////////////////////////////

const Configuration::BoolValue DialogBuilder::useDialogFrame ("GUI.StandardDialog", "UseDialogFrame", false);

//////////////////////////////////////////////////////////////////////////////////////////////////

DialogBuilder::DialogBuilder ()
: theme (nullptr),
  stringTable (nullptr),
  firstFocus (nullptr),
  currentDialog (nullptr)
{
	customButtonItems.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DialogBuilder::~DialogBuilder ()
{
	ASSERT (currentDialog == nullptr)

	if(theme)
		theme->release ();
	if(stringTable)
		stringTable->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DialogBuilder::setTheme (Theme* _theme)
{
	take_shared<Theme> (theme, _theme);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DialogBuilder::setTheme (Theme& theme)
{
	setTheme (&theme);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DialogBuilder::setTheme (ITheme* theme)
{
	setTheme (unknown_cast<Theme> (theme));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Theme& DialogBuilder::getTheme () const
{
	if(theme)
		return *theme;

	CCL_PRINTLN ("Warning: No theme assigned to DialogBuilder!!")
	return ThemeManager::instance ().getDefaultTheme ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DialogBuilder::setStrings (ITranslationTable* table)
{
	take_shared (stringTable, table);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DialogBuilder::addCustomButton (IParameter* parameter, StringRef title, int buttonRole)
{
	auto* buttonItem = NEW StandardDialog::ButtonItem;
	buttonItem->setDialogResult (DialogResult::kFirstCustomDialogResult + customButtonItems.count ());
	buttonItem->setButtonRole (Styles::toDialogResult (buttonRole));
	buttonItem->setCustomParameter (parameter);
	buttonItem->setCustomTitle (title);

	customButtonItems.add (buttonItem);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DialogBuilder::setDialogResult (int dialogResult)
{
	if(currentDialog)
		currentDialog->setDialogResult (dialogResult);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DialogBuilder::close ()
{
	if(currentDialog)
		currentDialog->close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DialogBuilder::excludeStyleFlags (StyleRef flags)
{
	excludedStyleFlags = flags;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API DialogBuilder::runDialog (IView* view, int dialogStyle, int buttons, IWindow* parentWindow)
{
	return runDialog (unknown_cast<View> (view), StyleFlags (0, dialogStyle), buttons, parentWindow);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API DialogBuilder::runDialogAsync (IView* view, int dialogStyle, int buttons, IWindow* parentWindow)
{
	return runDialogAsync (unknown_cast<View> (view), StyleFlags (0, dialogStyle), buttons, parentWindow);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DialogBuilder::runDialog (View* view, StyleRef _style, int buttons, IWindow* parentWindow)
{
	StandardDialog dialog;
	ScopedVar<Dialog*> dialogScope (currentDialog, &dialog);
	prepareStandardDialog (dialog, view, _style, buttons, parentWindow);

	return dialog.showModal (parentWindow);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* DialogBuilder::runDialogAsync (View* view, StyleRef _style, int buttons, IWindow* parentWindow)
{
	ASSERT (currentDialog == nullptr)
	if(currentDialog != nullptr)
		return nullptr;

	StandardDialog* dialog = NEW StandardDialog;
	prepareStandardDialog (*dialog, view, _style, buttons, parentWindow);
	currentDialog = dialog;

	retain (); // stay alive while dialog is open, so that onDialogCompleted can be called safely

	Promise dialogPromise (dialog->showDialog ());
	return return_shared<IAsyncOperation> (dialogPromise.then (this, &DialogBuilder::onDialogCompleted));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DialogBuilder::onDialogCompleted (IAsyncOperation&)
{
	safe_release (currentDialog);

	release (); // release refCount from runDialogAsync
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DialogBuilder::prepareStandardDialog (StandardDialog& dialog, View* view, StyleRef _style, int buttons, IWindow* parentWindow)
{
	ThemeSelector themeSelector (getTheme ());

	ASSERT (view != nullptr)
	StyleFlags style (_style);

	auto takeWindowStyle = [&] (Form& form)
	{
		StyleFlags formStyle (form.getWindowStyle ());

		// reset the default when it should be custom (no titlebar)
		if(formStyle.isCustomStyle (Styles::kWindowAppearanceCustomFrame))
			style.setCustomStyle (Styles::kWindowAppearanceTitleBar, false);

		style.common |= formStyle.common;
		style.custom |= formStyle.custom;
	};

	Form* form = ccl_cast<Form> (view);
	if(form)
	{
		takeWindowStyle (*form);

		if(!firstFocus)
			firstFocus = form->findFirstFocusView ();
	}

	bool sizable = style.isCustomStyle (Styles::kWindowBehaviorSizable);

	Point dialogPosition (view->getSize ().getLeftTop ());
	view->setPosition (Point ());

	dialog.setTitle (view->getTitle ());
	dialog.setController (view->getController ());

	String name (view->getName ());
	if(name.isEmpty ())
		name = CCLSTR ("StandardDialog");
	dialog.setName (name);

	String helpid (view->getHelpIdentifier ());
	if(helpid.isEmpty ())
		helpid = name;
	dialog.setHelpIdentifier (helpid);

	View* contentView = nullptr;

	if(buttons == 0)
	{
		buttons |= view->getStyle ().isCustomStyle (Styles::kCancelButton) ? Styles::kCancelButton : 0;
		buttons |= view->getStyle ().isCustomStyle (Styles::kOkayButton) ? Styles::kOkayButton : 0;
		buttons |= view->getStyle ().isCustomStyle (Styles::kCloseButton) ? Styles::kCloseButton : 0;
		buttons |= view->getStyle ().isCustomStyle (Styles::kApplyButton) ? Styles::kApplyButton : 0;
	}
	dialog.prepareButtons (buttons, customButtonItems);

	if(useDialogFrame && theme)
	{
		// try to decorate the view (with a form from skin that can add margins, buttons, ...)
		// except when its visualstyle vetos it
		if(!view->getVisualStyle ().getMetric ("noDialogFrame", false))
		{
			AutoPtr<Decorator> decorator (NEW Decorator (dialog, view));
			decorator->getDecorArguments ().setAttribute ("title", dialog.getTitle ());
			contentView = decorator->decorateView (*theme);

			if(contentView == view) // no decor applied
				contentView = nullptr;
			else
			{
				if(auto* form = ccl_cast<Form> (contentView))
					takeWindowStyle (*form);
			}
		}
	}

	if(!contentView)
	{
		if(buttons != 0)
		{
			// create standard frame with buttons
			Rect buttonRect;
			buttonRect.setWidth (view->getWidth ());
			buttonRect.setHeight (view->getTheme ().getThemeMetric (ThemeElements::kButtonHeight));

			StyleFlags layoutStyle (Styles::kVertical, Styles::kLayoutUnifySizes);
			contentView = NEW BoxLayoutView (Rect (), layoutStyle);
			contentView->addView (view);
			contentView->setSizeMode (View::kAttachAll|View::kFitSize);

			View* buttonView = createStandardButtons (buttonRect, buttons, dialog);
			buttonView->setSizeMode (View::kAttachLeft|View::kAttachRight|View::kAttachBottom);
			contentView->addView (buttonView);

			if(sizable)
				contentView->setSizeMode (View::kAttachAll);
		}
		else
			contentView = view;
	}

	style.common &= ~excludedStyleFlags.common;
	style.custom &= ~excludedStyleFlags.custom;

	dialog.setStyle (style);

	Rect dialogSize (contentView->getSize ());
	dialogSize.offset (dialogPosition);
	dialog.setSize (dialogSize);
	dialog.addView (contentView);
	dialog.setSizeMode (View::kAttachAll);
#if 0
	if(sizable && !contentView->hasExplicitSizeLimits ())
	{
		const SizeLimit& contentLimits (contentView->getSizeLimits ());
		SizeLimit limits (contentView->getWidth (), contentView->getHeight (), contentLimits.maxWidth, contentLimits.maxHeight);

		ccl_lower_limit (limits.maxWidth, limits.minWidth);
		ccl_lower_limit (limits.maxHeight, limits.maxHeight);
		dialog.setSizeLimits (limits);
	}
#endif
	if(form && form->hasVisualStyle ())
		dialog.setVisualStyle (unknown_cast<VisualStyle> (&form->getVisualStyle ()));
	else
		dialog.setVisualStyle (theme->getStandardStyle (ThemePainter::kBackgroundRenderer));

	dialog.setFirstFocusView (firstFocus);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API DialogBuilder::runWithParameters (StringRef name, IController& paramList, StringRef title,
											  int dialogStyle, int buttons, IWindow* parentWindow)
{
	ThemeSelector themeSelector (getTheme ());

	View* view = createParameterListView (paramList);
	view->setName (name);
	view->setTitle (title);

	return runDialog (view, StyleFlags (0, dialogStyle), buttons, parentWindow);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API DialogBuilder::runWithParametersAsync (StringRef name, IController& paramList, StringRef title,
																int dialogStyle, int buttons, IWindow* parentWindow)
{
	ThemeSelector themeSelector (getTheme ());

	View* view = createParameterListView (paramList);
	view->setName (name);
	view->setTitle (title);

	return runDialogAsync (view, StyleFlags (0, dialogStyle), buttons, parentWindow);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DialogBuilder::askForString (String& string, StringID label, StringRef title, StringRef dialogName)
{
	ParamContainer params;
	IParameter* param = params.addString (label);
	param->fromString (string);

	StringRef dlgName = dialogName.isEmpty () ? CCLSTR ("AskForString") : dialogName;

	if(runWithParameters (dlgName, params, title) == DialogResult::kOkay)
	{
		param->toString (string);
		return true;
	}
	return false;

}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API DialogBuilder::askForStringAsync (StringRef string, StringID label, StringRef title, StringRef dialogName)
{
	ParamContainer params;
	IParameter* param = params.addString (label);
	param->setValue (string);
	StringRef dlgName = dialogName.isEmpty () ? CCLSTR ("AskForString") : dialogName;

	auto operation = Promise (runWithParametersAsync (dlgName, params, title)).modifyState ([] (const IAsyncOperation& op)
	{
		switch(op.getResult ().asInt ())
		{
		case DialogResult::kCancel:
			return IAsyncInfo::kCanceled;
		case DialogResult::kOkay:
			return IAsyncInfo::kCompleted;
		default:
			return IAsyncInfo::kFailed;
		}
	}).then ([param] (IAsyncOperation& op)
	{
		op.setResult (Variant (param->getValue ().asString (), true));
	});
	return return_shared<IAsyncOperation> (operation);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* DialogBuilder::runWithMenuInternal (IMenu* menu, StringRef title, StringRef text, bool async)
{
	ASSERT (menu != nullptr)

	AutoPtr<PopupSelector> popupSelector = NEW PopupSelector;
	popupSelector->setTheme (&FrameworkTheme::instance ());

	AutoPtr<ParamContainer> decorController = NEW ParamContainer;
	decorController->addString ("title")->fromString (title);
	if(text.isEmpty () == false)
		decorController->addString ("text")->fromString (text);
	popupSelector->setDecor ("CCL/MenuDialog", decorController->asUnknown ());

	UnknownPtr<IView> view = Desktop.getApplicationWindow ();
	if(view == nullptr)
		view = Desktop.getDialogParentWindow ();

	AutoPtr<MenuDialogInformation> information = NEW MenuDialogInformation (menu, text, title);
	information->begin (popupSelector);
	PopupSizeInfo sizeInfo (view, PopupSizeInfo::kHCenter|PopupSizeInfo::kVCenter);
	
	if(async)
	{
		return popupSelector->popupAsync (menu, sizeInfo, MenuPresentation::kTree);
	}
	else
	{
		popupSelector->popup (menu, sizeInfo, MenuPresentation::kTree);
		return nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DialogBuilder::runWithMenu (IMenu* menu, StringRef title, StringRef text)
{
	runWithMenuInternal (menu, title, text, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API DialogBuilder::runWithMenuAsync (IMenu* menu, StringRef title, StringRef text)
{
	return runWithMenuInternal (menu, title, text, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& DialogBuilder::getButtonRect (Rect& rect) const
{
	Coord buttonW = getTheme ().getThemeMetric (ThemeElements::kButtonWidth);
	Coord buttonH = getTheme ().getThemeMetric (ThemeElements::kButtonHeight);
	rect (0, 0, buttonW, buttonH);
	return rect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* DialogBuilder::createStandardButtons (Rect size, int buttons, StandardDialog& dialog)
{
	Rect rect;
	getButtonRect (rect);

	BoxLayoutView* frame = NEW BoxLayoutView (size, StyleFlags (Styles::kHorizontal, Styles::kLayoutReverse));
	frame->setMargin (0);

	for(auto* buttonItem : iterate_as<StandardDialog::ButtonItem> (dialog.getButtonItems ()))
	{
		Button* button = NEW StandardButton (rect, buttonItem->getDialogResult (), buttonItem->getCustomTitle ());
		frame->addView (button);
	}

	SizeLimit limits (frame->getSizeLimits ());
	limits.maxWidth = kMaxCoord;
	frame->setSizeLimits (limits);
	frame->autoSize (true, false);
	return frame;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* DialogBuilder::createParameterListView (IController& paramList)
{
	StyleFlags hStyle (Styles::kHorizontal);
	StyleFlags vStyle (Styles::kVertical);

	BoxLayoutView* frame = NEW BoxLayoutView (Rect (), hStyle);
	frame->setSizeMode (View::kFitSize);
	frame->setMargin (0);

	BoxLayoutView* leftColumn = NEW BoxLayoutView (Rect (), vStyle);
	leftColumn->setSizeMode (View::kFitSize);
	frame->setMargin (0);
	frame->addView (leftColumn);

	BoxLayoutView* rightColumn = NEW BoxLayoutView (Rect (), vStyle);
	rightColumn->setSizeMode (View::kFitSize);
	frame->setMargin (0);
	frame->addView (rightColumn);

	for(int i = 0; i < paramList.countParameters (); i++)
	{
		IParameter* p = paramList.getParameterAt (i);

		View* control = createParameterControl (p);
		ASSERT (control != nullptr)
		if(!control)
			continue;

		if(firstFocus == nullptr)
			firstFocus = control;

		rightColumn->addView (control);

		// translate parameter name
		String title;
		if(stringTable)
			stringTable->getString (title, nullptr, p->getName ());
		else
			title = p->getName ().str ();

		Label* label = NEW Label (Rect (0, 0, 0, control->getHeight ()), 0, title);
		label->autoSize (true, false);
		leftColumn->addView (label);
	}

	Rect size (frame->getSize ());
	size.bottom += 16;

	DialogGroup* group = NEW DialogGroup (size);
	size = frame->getSize ();
	size.offset (0, 16);
	frame->setSize (size);
	group->addView (frame);
	return group;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* DialogBuilder::createParameterControl (IParameter* param)
{
	View* control = nullptr;
	Rect rect;

	switch(param->getType ())
	{
	case IParameter::kToggle :
		rect.setWidth (20);
		rect.setHeight (getTheme ().getThemeMetric (ThemeElements::kCheckBoxSize));
		control = NEW CheckBox (rect, param);
		break;

	case IParameter::kString :
	case IParameter::kFloat :
	case IParameter::kInteger :
		rect.setWidth (param->getType () == IParameter::kString ? 150 : 100);
		rect.setHeight (getTheme ().getThemeMetric (ThemeElements::kTextBoxHeight));
		if(param->isEnabled ())
		{
			if(param->getType () == IParameter::kInteger)
				control = NEW ValueBox (rect, param, StyleFlags (Styles::kBorder, Styles::kEditBoxBehaviorDialogEdit));
			else
				control = NEW EditBox (rect, param, StyleFlags (Styles::kBorder, Styles::kEditBoxBehaviorDialogEdit));
		}
		else
		{
			String title;
			param->toString (title);
			control = NEW Label (rect, StyleFlags (), title);
		}
		break;

	case IParameter::kList :
		rect.setWidth (100);
		rect.setHeight (getTheme ().getThemeMetric (ThemeElements::kTextBoxHeight));
		control = NEW SelectBox (rect, param, Styles::kBorder);
		break;

	case IParameter::kColor :
		rect.setWidth (100);
		rect.setHeight (getTheme ().getThemeMetric (ThemeElements::kTextBoxHeight));
		control = NEW ColorBox (rect, param);
		control->addView (NEW SelectBox (rect, param, StyleFlags (Styles::kTransparent|Styles::kBorder, Styles::kSelectBoxAppearanceHideText|Styles::kSelectBoxAppearanceHideFocus)));
		break;

	case IParameter::kImage :
		rect.setWidth (22);
		rect.setHeight (22);
		control = NEW DialogGroup (rect, StyleFlags (0, Styles::kDialogGroupAppearanceSecondary));
		{
			rect.contract (2);
			ImageView* imageView = NEW ImageView (nullptr, rect, StyleFlags (0, Styles::kImageViewAppearanceFitImage));
			imageView->setImageProvider (UnknownPtr<IImageProvider> (param));
			rect.moveTo (Point ());
			imageView->addView (NEW SelectBox (rect, param, StyleFlags (Styles::kTransparent|Styles::kBorder, Styles::kSelectBoxAppearanceHideText|Styles::kSelectBoxAppearanceHideFocus|Styles::kSelectBoxAppearanceHideButton|Styles::kSelectBoxAppearanceHideImage)));
			control->addView (imageView);
		}
		break;

	case IParameter::kSegments :
		rect.setWidth (120);
		rect.setHeight (getTheme ().getThemeMetric (ThemeElements::kTextBoxHeight));
		control = NEW SegmentBox (rect, param);
		break;

	default :
		CCL_DEBUGGER ("Parameter type not supported!")
		break;
	}
	return control;
}

//************************************************************************************************
// StandardDialog
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StandardDialog, Dialog)

//////////////////////////////////////////////////////////////////////////////////////////////////

StandardDialog::StandardDialog (const Rect& size, StyleRef style, StringRef title)
: Dialog (size, style, title),
  buttonOrder (Styles::kAffirmativeButtonLeft)
{
	buttonItems.objectCleanup (true);

	Variant platformButtonOrder;
	if(GUI.getPlatformStyle (platformButtonOrder, Styles::kButtonOrder))
		buttonOrder = platformButtonOrder.asInt ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StandardDialog::~StandardDialog ()
{
	removeAll (); // need this on Mac to detach StandardButtons
	ASSERT (standardButtons.isEmpty ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StandardDialog::ButtonItem* StandardDialog::getButtonItemAt (int index) const
{
	return static_cast<ButtonItem*> (buttonItems.at (index));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StandardDialog::ButtonItem* StandardDialog::getButtonItemForResult (int dialogResult) const
{
	return static_cast<ButtonItem*> (buttonItems.findIf ([&] (Object* obj)
		{ return static_cast<ButtonItem*> (obj)->getDialogResult () == dialogResult; }));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StandardDialog::prepareButtons (int standardButtons, const ObjectArray& customButtonItems)
{
	// Button order in dialogs, right to left
	static int orderedResults[] = {DialogResult::kCancel, DialogResult::kOkay, DialogResult::kClose, DialogResult::kApply};
	if(buttonOrder == Styles::kAffirmativeButtonRight)
	{
		orderedResults[0] = DialogResult::kOkay;
		orderedResults[1] = DialogResult::kCancel;
	}

	// create items for requested standard buttons
	for(int i = 0; i < ARRAY_COUNT (orderedResults); i++)
	{
		int buttonResult = orderedResults[i];
		if((standardButtons & Styles::toDialogButton (buttonResult)) != 0)
		{
			auto* buttonItem = NEW StandardDialog::ButtonItem;
			buttonItem->setDialogResult (buttonResult);
			buttonItem->setButtonRole (buttonResult);
			buttonItems.add (buttonItem);
		}
	}

	auto getSortOrder = [&] (int dialogResult)
	{
		for(int i = 0; i < ARRAY_COUNT (orderedResults); i++)
			if(orderedResults[i] == dialogResult)
				return i;

		return -1;
	};

	auto getInsertIndex = [&] (ButtonItem* newItem) -> int
	{
		int index = 0;
		for(auto otherItem : iterate_as<ButtonItem> (buttonItems))
		{
			if(getSortOrder (newItem->getButtonRole ()) < getSortOrder (otherItem->getButtonRole ()))
				break;
			index++;
		}
		return index;
	};

	// add custom button items
	for(auto customItem : iterate_as<ButtonItem> (customButtonItems))
	{
		int index = getInsertIndex (customItem);
		buttonItems.insertAt (index, return_shared (customItem));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StandardDialog::setButtonOrder (Styles::ButtonOrder order)
{
	if(buttonOrder != order)
	{
		buttonOrder = order;

		// collect buttons that should be rightmost
		int rightButtonRole = buttonOrder == Styles::kAffirmativeButtonRight ? DialogResult::kOkay : DialogResult::kCancel;
		ObjectList rightButtons;
		for(auto* buttonItem : iterate_as<ButtonItem> (buttonItems))
			if(buttonItem->getButtonRole () == rightButtonRole)
				rightButtons.add (buttonItem);

		// move them to front (array order is right to left)
		for(auto* buttonItem : iterate_as<ButtonItem> (rightButtons))
		{
			bool removed = buttonItems.remove (buttonItem);
			ASSERT (removed)
			buttonItems.insertAt (0, buttonItem);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StandardDialog::registerButton (StandardButton* button)
{
	if(!DialogResult::isCustomResult (button->getResult ()))
	{
		button->connect (this, button->getResult ());

		if(UnknownPtr<IDialogButtonInterest> interest = getController ())
			interest->setDialogButton (button->getParameter (), button->getResult ());
	}
	else
	{
		// connect to custom parameter
		if(ButtonItem* item = getButtonItemForResult (button->getResult ()))
			button->setParameter (item->getCustomParameter ());
	}
	standardButtons.add (button);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StandardDialog::unregisterButton (StandardButton* button)
{
	if(!DialogResult::isCustomResult (button->getResult ()))
	{
		button->connect (nullptr, 0);

		if(UnknownPtr<IDialogButtonInterest> interest = getController ())
			interest->setDialogButton (nullptr, button->getResult ());
	}
	standardButtons.remove (button);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StandardDialog::pushButton (int buttonIndex)
{
	if(standardButtons.isEmpty ())
		close ();
	else if(StandardButton* button = (StandardButton*)standardButtons.at (buttonIndex))
		if(button->getParameter ()->isEnabled ())
			button->getParameter ()->setValue (1, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StandardDialog::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case DialogResult::kApply :
	case DialogResult::kCancel :
	case DialogResult::kOkay :
	case DialogResult::kClose :
		{
			if(UnknownPtr<IDialogButtonInterest> interest = getController ())
				if(interest->onDialogButtonHit (param->getTag ()))
					return true;

			if(param->getTag () != DialogResult::kApply)
			{
				setDialogResult (param->getTag ());
				close ();
			}
		}
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StandardDialog::onKeyDown (const KeyEvent& event)
{
	if(event.vKey == VKey::kReturn
	|| event.vKey == VKey::kEnter
	|| event.vKey == VKey::kEscape)
	{
		int dialogResult = (event.vKey == VKey::kEscape) ? DialogResult::kCancel : DialogResult::kOkay;

		// trigger parameter of a custom button with the matching role
		for(auto item : iterate_as<ButtonItem> (buttonItems))
			if(DialogResult::isCustomResult (item->getDialogResult ()) && dialogResult == item->getButtonRole ())
			{
				IParameter* param = item->getCustomParameter ();
				if(param && param->isEnabled ())
				{
					param->setValue (param->getMax (), true);
					param->setValue (param->getMin (), false);
					return true; // don't close
				}
			}

		// allow controller to intercept as if the corresponding button was pressed, e.g. to suppress closing
		if(UnknownPtr<IDialogButtonInterest> interest = getController ())
		{
			int buttonCode = event.vKey == VKey::kEscape ? DialogResult::kCancel : DialogResult::kOkay;
			if(interest->onDialogButtonHit (buttonCode))
				return true; // don't close
		}
	}
	return SuperClass::onKeyDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StandardDialog::onClose ()
{
	if(dialogResult == DialogResult::kNone) // not caused by StandardButton
	{
		if(UnknownPtr<IDialogButtonInterest> interest = getController ())
			if(interest->onDialogButtonHit (DialogResult::kCancel))
				return false;
	}

	return SuperClass::onClose ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StandardDialog::attached (View* parent)
{
	SuperClass::attached (parent);
	standardButtons.reverse ();

	dialogInformation = NEW Information (*this);
	if(!standardButtons.isEmpty ())
	{
		int i = 0;
		ForEach (standardButtons, StandardButton, button)
			dialogInformation->setButtonTitle (i, button->getTitle ());
			if(++i >= 3)
				break;
		EndFor
	}
	else
		dialogInformation->setButtonTitle (0, XSTR (Close));

	AlertService::instance ().beginDialog (*dialogInformation);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StandardDialog::removed (View* parent)
{
	SuperClass::removed (parent);

	if(dialogInformation)
	{
		AlertService::instance ().endDialog (*dialogInformation);
		dialogInformation.release ();
	}
}

//************************************************************************************************
// StandardButton
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StandardButton, Button)

//////////////////////////////////////////////////////////////////////////////////////////////////

StandardButton::StandardButton (const Rect& size, int dialogResult, StringRef title)
: Button (size, nullptr, 0, title),
  dialogResult (dialogResult)
{
	if(title.isEmpty ())
		setTitle (DialogBuilder::getStandardButtonTitle (dialogResult));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StandardButton::attached (View* parent)
{
	SuperClass::attached (parent);

	if(StandardDialog* dialog = ccl_cast<StandardDialog> (getWindow ()))
		dialog->registerButton (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StandardButton::removed (View* parent)
{
	if(StandardDialog* dialog = ccl_cast<StandardDialog> (getWindow ()))
		dialog->unregisterButton (this);

	SuperClass::removed (parent);
}
