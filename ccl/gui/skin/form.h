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
// Filename    : ccl/gui/skin/form.h
// Description : Form class
//
//************************************************************************************************

#ifndef _ccl_form_h
#define _ccl_form_h

#include "ccl/gui/views/imageview.h"
#include "ccl/gui/skin/skinmodel.h"
#include "ccl/public/gui/framework/iform.h"

#include "ccl/base/storage/attributes.h"

namespace CCL {

//************************************************************************************************
// Form
/** Skin-based view class. */
//************************************************************************************************

class Form: public ImageView,
			public IForm
{
public:
	DECLARE_CLASS (Form, ImageView)

	Form (SkinWizard* wizard = nullptr, const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);
	~Form ();

	SkinElements::FormElement* getSkinElement () const;
	void setSkinElement (SkinElements::FormElement* e);

	PROPERTY_STRING (firstFocus, FirstFocus)
	View* findFirstFocusView ();

	Window* open (Window* parentWindow = nullptr);
	bool close ();

	// IForm
	StringID CCL_API getFormName () const override;
	StyleRef CCL_API getWindowStyle () const override;
	void CCL_API setWindowStyle (StyleRef style) override;
	IUnknown* CCL_API getController () const override;
	tbool CCL_API setController (IUnknown* controller) override;
	IWindow* CCL_API openWindow (IWindow* parentWindow = nullptr) override;
	void CCL_API closeWindow () override;
	void CCL_API reload () override;
	ISkinElement* CCL_API getISkinElement () const override;

	// View
	void calcSizeLimits () override;
	StringRef getHelpIdentifier () const override;

	CLASS_INTERFACE (IForm, ImageView)

protected:
	SharedPtr<SkinElements::FormElement> skinElement;
	SharedPtr<IUnknown> controller;
	SkinWizard* wizard;
	StyleFlags windowStyle;

	static View* findFirstFocusView (View* parent, StringRef firstFocus);
};

//************************************************************************************************
// FormDelegateView styles
//************************************************************************************************

namespace Styles
{
	enum FormDelegateViewStyles
	{
		kFormDelegateViewBehaviorDeferredRemove = 1<<0,	///< don't remove child in removed (), but just before creating a new one in attached
		kFormDelegateViewBehaviorKeepView = 1<<1		///< never remove child in removed (), the view created in attached is kept as child forever
	};
};

//************************************************************************************************
// FormDelegateView
//************************************************************************************************

class FormDelegateView: public View
{
public:
	DECLARE_CLASS (FormDelegateView, View)

	FormDelegateView (SkinWizard* wizard = nullptr, const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);

	DECLARE_STYLEDEF (customStyles)

	PROPERTY_MUTABLE_CSTRING (formName, FormName)
	PROPERTY_SHARED_AUTO (IUnknown, formController, FormController)
   	PROPERTY_MUTABLE_CSTRING (subControllerName, SubControllerName)   

	Attributes& getFormArguments () { return formArguments; }

	// View
	void attached (View* parent) override;
	void removed (View* parent) override;
	void onSize (const Point& delta) override;
	void calcSizeLimits () override;

protected:
	SkinWizard* wizard;
	Attributes formArguments;

	enum PrivateFlags
	{
		kAttachedInternal = 1<<(kLastPrivateFlag + 1)
	};

	PROPERTY_FLAG (privateFlags, kAttachedInternal, isAttachedInternal)

	void sizeChild (View* view);
};

} // namespace CCL

#endif // _ccl_form_h
