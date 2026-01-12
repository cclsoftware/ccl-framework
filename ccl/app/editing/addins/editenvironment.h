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
// Filename    : ccl/app/editing/addins/editenvironment.h
// Description : Edit Environment
//
//************************************************************************************************

#ifndef _ccl_editenvironment_h
#define _ccl_editenvironment_h

#include "ccl/app/component.h"

#include "ccl/base/signalsource.h"

#include "ccl/public/app/ieditenvironment.h"

namespace CCL {

class EditView;
class EditorComponent;
class EditAddInCollection;
interface IActionContext;

//************************************************************************************************
// EditEnvironment
//************************************************************************************************

class EditEnvironment: public Component,
					   public IEditEnvironment
{
public:
	DECLARE_CLASS (EditEnvironment, Component)
	DECLARE_METHOD_NAMES (EditEnvironment)
	DECLARE_PROPERTY_NAMES (EditEnvironment)

	EditEnvironment (StringRef name = nullptr);
	~EditEnvironment ();

	static EditEnvironment* getInstance (EditView* editView);
	static EditEnvironment* getInstance (const Component& component);

	PROPERTY_POINTER (IActionContext, actionContext, ActionContext)
	void setActiveEditor (EditorComponent* editor);
	EditAddInCollection& getAddIns ();

	// IEditEnvironment
	IObject* CCL_API getMainEditor () override;
	IObject* CCL_API getActiveEditor () override;
	ISelection* CCL_API getActiveSelection () override;
	IUnknown* CCL_API getFocusItem () override;
	IObject* CCL_API getFocusItemPropertyEditor () override;
	IUnknown* CCL_API getAddInInstance (StringRef name) override;
	IActionJournal* CCL_API getActionJournal () override;
	tbool CCL_API canRunEditTask (UIDRef cid) override;
	tbool CCL_API runEditTask (UIDRef cid, IAttributeList* arguments = nullptr) override;

	bool runEditTaskWithClassName (StringRef className, IAttributeList* arguments);

	// Component
	IObjectNode* CCL_API findChild (StringRef id) const override;
	tresult CCL_API terminate () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (IEditEnvironment, Component)

protected:
	EditorComponent* activeEditor;
	AutoSignalSink signalSink;
	EditAddInCollection* addIns;

	void signalEditEvent (StringID name);
	bool runTask (UIDRef cid, bool checkOnly, IAttributeList* arguments = nullptr);
	void onSelectionChanged (MessageRef msg);

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

} // namespace CCL

#endif // _ccl_editenvironment_h
