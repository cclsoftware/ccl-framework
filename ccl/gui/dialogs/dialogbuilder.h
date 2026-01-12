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
// Filename    : ccl/gui/dialogs/dialogbuilder.h
// Description : Dialog Builder
//
//************************************************************************************************

#ifndef _ccl_dialogbuilder_h
#define _ccl_dialogbuilder_h

#include "ccl/gui/windows/dialog.h"
#include "ccl/gui/theme/visualstyleclass.h"
#include "ccl/base/collections/objectarray.h"
#include "ccl/base/storage/configuration.h"

#include "ccl/public/gui/framework/idialogbuilder.h"

namespace CCL {

interface IParameter;
interface IParamObserver;
interface IController;
class StandardDialog;
class PopupSelector;

//************************************************************************************************
// DialogBuilder
//************************************************************************************************

class DialogBuilder: public Object,
					 public IDialogBuilder
{
public:
	DECLARE_CLASS (DialogBuilder, Object)

	DialogBuilder ();
	~DialogBuilder ();

	void setTheme (Theme* theme);
	void setTheme (Theme& theme);
	Theme& getTheme () const;

	int runDialog (View* view, 
				   StyleRef style = Styles::dialogWindowStyle,
				   int buttons = 0,
				   IWindow* parentWindow = nullptr);

	IAsyncOperation* runDialogAsync (View* view, 
				   StyleRef style = Styles::dialogWindowStyle,
				   int buttons = 0,
				   IWindow* parentWindow = nullptr);				   

	// IDialogBuilder
	void CCL_API setTheme (ITheme* theme) override;
	void CCL_API setStrings (ITranslationTable* table) override;
	void CCL_API addCustomButton (IParameter* param, StringRef title, int buttonRole = Styles::kOkayButton) override;

	int CCL_API runDialog (IView* view, 
						   int dialogStyle = Styles::kWindowCombinedStyleDialog,
						   int buttons = 0,
						   IWindow* parentWindow = nullptr) override;

	IAsyncOperation* CCL_API runDialogAsync (IView* view, 
						   int dialogStyle = Styles::kWindowCombinedStyleDialog,
						   int buttons = 0,
						   IWindow* parentWindow = nullptr) override;

	int CCL_API runWithParameters (StringRef name,
								   IController& paramList,
								   StringRef title,
								   int dialogStyle = Styles::kWindowCombinedStyleDialog,
								   int buttons = Styles::kDialogOkCancel, 
								   IWindow* parentWindow = nullptr) override;

	IAsyncOperation* CCL_API runWithParametersAsync (StringRef name,
								   IController& paramList,
								   StringRef title,
								   int dialogStyle = Styles::kWindowCombinedStyleDialog,
								   int buttons = Styles::kDialogOkCancel, 
								   IWindow* parentWindow = nullptr) override;

	tbool CCL_API askForString (String& string,
							  StringID label,
							  StringRef title,
							  StringRef dialogName = nullptr) override;

	IAsyncOperation* CCL_API askForStringAsync (StringRef string,
	                                            StringID label,
	                                            StringRef title,
	                                            StringRef dialogName = nullptr) override;

	void CCL_API runWithMenu (IMenu* menu, 
							  StringRef title,
							  StringRef text) override;

	IAsyncOperation* CCL_API runWithMenuAsync (IMenu* menu,
							  StringRef title,
							  StringRef text) override;

	void CCL_API setDialogResult (int dialogResult) override;
	void CCL_API close () override;
	void CCL_API excludeStyleFlags (StyleRef flags) override;

	CLASS_INTERFACE (IDialogBuilder, Object)

	// create standard dialog button
	static View* createStandardButton (RectRef rect, int dialogResult, StringRef title);

	static StringRef getStandardButtonTitle (int dialogResult);

protected:
	Theme* theme;
	ITranslationTable* stringTable;
	View* firstFocus;
	Dialog* currentDialog;
	ObjectArray customButtonItems;
	StyleFlags excludedStyleFlags;

	static const Configuration::BoolValue useDialogFrame;

	class Decorator;

	Rect& getButtonRect (Rect& rect) const;
	View* createStandardButtons (Rect size, int buttons, StandardDialog& dialog);
	View* createParameterListView (IController& paramList);
	View* createParameterControl (IParameter* param);
	IAsyncOperation* runWithMenuInternal (IMenu* menu, StringRef title, StringRef text, bool async);
	
	void prepareStandardDialog (StandardDialog& dialog, View* view, StyleRef style, int buttons, IWindow* parentWindow);
	void onDialogCompleted (IAsyncOperation&);
};

DECLARE_VISUALSTYLE_CLASS (DialogBuilder)

} // namespace CCL

#endif // _ccl_dialogbuilder_h
