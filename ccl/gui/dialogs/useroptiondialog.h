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
// Filename    : ccl/gui/dialogs/useroptiondialog.h
// Description : User Option Dialog
//
//************************************************************************************************

#ifndef _ccl_useroptiondialog_h
#define _ccl_useroptiondialog_h

#include "ccl/gui/views/view.h"

#include "ccl/public/gui/paramlist.h"
#include "ccl/public/gui/iuseroption.h"
#include "ccl/public/gui/icontroller.h"
#include "ccl/public/gui/iviewfactory.h"
#include "ccl/public/gui/iparamobserver.h"
#include "ccl/public/gui/framework/idialogbuilder.h"
#include "ccl/public/gui/framework/iitemmodel.h"

namespace CCL {

class OptionRoot;
class OptionCategory;
class ListParam;

//************************************************************************************************
// UserOptionDialog
//************************************************************************************************

class UserOptionDialog: public Object,
						public IUserOptionDialog,
						public AbstractController,
						public IParamObserver,
						public IViewFactory,
						public IDialogButtonInterest,
						public ItemViewObserver<AbstractItemModel>
{
public:
	DECLARE_CLASS (UserOptionDialog, Object)

	UserOptionDialog ();
	~UserOptionDialog ();

	// IUserOptionDialog
	tresult CCL_API run (IUserOptionList& optionList) override;
	tresult CCL_API run (IUserOptionList* lists[], int count, int index) override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACES (Object)

protected:
	OptionRoot& optionRoot;
	IImage* defaultIcon;
	ParamList paramList;
	IParameter* applyButton;
	IParameter* optionHeader;
	ListParam* listParam;
	IParameter* nextListParam;
	ObservedPtr<View> nextListButton;
	Vector<IUserOptionList*> optionLists;
	IUserOptionList* visibleList;

	void showList (IUserOptionList* list);
	IUserOptionList* getNextList () const;
	
	void updateApply ();
	void updateWindow ();
	void updateNextButton ();

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

	// IController
	DECLARE_PARAMETER_LOOKUP (paramList)
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;

	// IParamObserver
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API paramEdit (IParameter* param, tbool begin) override {}

	// IViewFactory
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

	// IDialogButtonInterest
	void CCL_API setDialogButton (IParameter* button, int which) override;
	tbool CCL_API onDialogButtonHit (int which) override;

	// IItemModel
	int CCL_API countFlatItems () override;
	tbool CCL_API getItemTitle (String& title, ItemIndexRef index) override;
	IImage* CCL_API getItemIcon (ItemIndexRef index) override;
	tbool CCL_API onItemFocused (ItemIndexRef index) override;

	OptionCategory* getCategory (ItemIndexRef index);
};

} // namespace CCL

#endif // _ccl_useroptiondialog_h
