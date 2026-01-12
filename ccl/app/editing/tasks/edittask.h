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
// Filename    : ccl/app/editing/tasks/edittask.h
// Description : Edit Task
//
//************************************************************************************************

#ifndef _ccl_edittask_h
#define _ccl_edittask_h

#include "ccl/app/paramcontainer.h"

#include "ccl/public/app/iedittask.h"
#include "ccl/public/gui/iparamobserver.h"

namespace CCL {

class Iterator;
class Container;
class EditView;
class EditModel;
class EditorComponent;
class EditEnvironment;
class ActionExecuter;

interface IActionContext;
interface IAttributeList;
interface IObjectFilter;

//************************************************************************************************
// EditTask
/** Base class for native edit tasks. */
//************************************************************************************************

class EditTask: public Object,
				public IEditTask,
				public IParamObserver
{
public:
	DECLARE_CLASS (EditTask, Object)

	virtual String getTitle () const; ///< get title of this task from class description
	PROPERTY_VARIABLE (MutableCString, formName, FormName) ///< dialog form name (optional)
	CString getThemeID () const;

	// IEditTask
	tresult CCL_API prepareEdit (IObject& context) override;
	tresult CCL_API performEdit (IObject& context) override;

	// IParamObserver
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API paramEdit (IParameter* param, tbool begin) override;

	CLASS_INTERFACES (Object)

protected:
	SharedPtr<ParamContainer> paramList;

	virtual void construct (IObject& context); ///< overwrite to build parameters, etc.

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Access context properties
	// LATER TODO: this could be moved to a Context wrapper class!
	//////////////////////////////////////////////////////////////////////////////////////////////

	bool isSilentMode (IObject& context) const;
	IAttributeList* getArguments (IObject& context) const;
	void restore (IObject& context);
	tresult runDialog (IObject& context, StringID formName, StringID themeID = nullptr);

	EditView* getEditView (IObject& context) const;
	EditModel* getEditModel (IObject& context) const;
	EditorComponent* getEditor (IObject& context) const;
	EditEnvironment* getEditEnvironment (IObject& context) const;
	Iterator* getIterator (IObject& context) const;
	ActionExecuter* getFunctions (IObject& context) const;
	IActionContext* getActionContext (IObject& context) const;
	bool collectEditItems (Container& items, IObject& context, MetaClassRef type, IObjectFilter* filter = nullptr) const; ///< uses iterator to collect items of given class

	template <class T> T* getEditView (IObject& context) const { return ccl_cast<T> (getEditView  (context)); }
	template <class T> T* getEditModel (IObject& context) const { return ccl_cast<T> (getEditModel (context)); }
	template <class T> T* getEditor (IObject& context) const { return ccl_cast<T> (getEditor (context)); }
	template <class T> T* getFunctions (IObject& context) const { return ccl_cast<T> (getFunctions (context)); }
	template <class T> bool collectEditItems (Container& items, IObject& context, IObjectFilter* filter = nullptr) const { return collectEditItems (items, context, ccl_typeid<T> (), filter); }
};

} // namespace CCL

#endif // _ccl_edittask_h
