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
// Filename    : ccl/app/editing/tasks/edittaskpreview.h
// Description : Edit Task with Preview
//
//************************************************************************************************

#ifndef _ccl_edittaskpreview_h
#define _ccl_edittaskpreview_h

#include "ccl/app/component.h"

#include "ccl/app/editing/tasks/edittask.h"

#include "ccl/public/storage/ipersistattributes.h"
#include "ccl/public/gui/framework/idialogbuilder.h"

namespace CCL {

class Action;
class EditTaskWithPreview;

//************************************************************************************************
// EditTaskComponent
//************************************************************************************************

class EditTaskComponent: public Component,
						 public IDialogButtonInterest,
						 public IPersistAttributes
{
public:
	DECLARE_CLASS (EditTaskComponent, Component)

	EditTaskComponent (StringRef name = nullptr);

	PROPERTY_MUTABLE_CSTRING (formName, FormName)
	PROPERTY_POINTER (EditTaskWithPreview, outerTask, OuterTask)

	bool runDialog ();

	bool isApplyNeeded () const;
	void setApplyNeeded (bool state);

	// IPersistAttributes
	tresult CCL_API storeValues (IAttributeList& values) const override;
	tresult CCL_API restoreValues (const IAttributeList& values) override;

	// Component
	tbool CCL_API paramChanged (IParameter* param) override;

	CLASS_INTERFACE2 (IDialogButtonInterest, IPersistAttributes, Component)

protected:
	bool applyNeeded;
	IParameter* applyButton;
	IParameter* defaultButton;

	virtual void onDefaultButtonHit ();

	// IDialogButtonInterest
	void CCL_API setDialogButton (IParameter* button, int which) override;
	tbool CCL_API onDialogButtonHit (int which) override;
};

//************************************************************************************************
// EditTaskWithPreview
//************************************************************************************************

class EditTaskWithPreview: public EditTask,
						   public IPersistAttributes

{
public:
	DECLARE_CLASS_ABSTRACT (EditTaskWithPreview, EditTask)

	EditTaskWithPreview ();

	virtual EditTask* createInnerTask () = 0;
	virtual EditTaskComponent* createComponent () = 0;

	bool apply ();
	void revert ();

	// EditTask
	tresult CCL_API prepareEdit (IObject& context) override;
	tresult CCL_API performEdit (IObject& context) override;

	// IPersistAttributes
	tresult CCL_API storeValues (IAttributeList& values) const override;
	tresult CCL_API restoreValues (const IAttributeList& values) override;

	CLASS_INTERFACE (IPersistAttributes, EditTask)

protected:
	AutoPtr<EditTaskComponent> component;
	IObject* currentContext;
	Action* currentAction;
	int candidateCounter;
};

} // namespace CCL

#endif // _ccl_edittaskpreview_h
