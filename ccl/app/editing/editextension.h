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
// Filename    : ccl/app/editing/editextension.h
// Description : Editing Extension
//
//************************************************************************************************

#ifndef _ccl_editextension_h
#define _ccl_editextension_h

#include "ccl/app/editing/editmodel.h"

#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/public/text/cstring.h"

namespace CCL {

class UserControl;

//************************************************************************************************
// EditExtension
//************************************************************************************************

class EditExtension: public Object
{
public:
	DECLARE_CLASS (EditExtension, Object)

	PROPERTY_MUTABLE_CSTRING (name, Name)

	/** Called after the model was created. The extension may add edit layers. */
	virtual void extendModel (EditModel& model, EditorComponent& editor);

	/** Create edit handler for given object. */
	virtual EditHandler* createEditHandler (Object* object, EditView& editView, const MouseEvent& event);
	
	/** Create drag handler for given event. */
	virtual IDragHandler* createDragHandler (UserControl& control, const DragEvent& event);
	
	/** Collect all supported edit task categories. */
	virtual void collectTaskCategories (StringList& taskCategories);

	/** Check for task candidates in given edit view. */
	virtual bool canPerformTask (EditView& editView, const EditTaskDescription& task);
	
	/** Create task candidates for given edit view. */
	virtual bool collectTaskCandidates (Container& resultList, EditView& editView, const EditTaskDescription& task);
};

//************************************************************************************************
// EditExtensionRegistry
//************************************************************************************************

class EditExtensionRegistry: public EditExtension,
							 public Singleton<EditExtensionRegistry>
{
public:
	DECLARE_CLASS (EditExtensionRegistry, EditExtension)

	EditExtensionRegistry ();

	/** Add extension (takes ownership). */
	void addExtension (EditExtension* extension);

	// EditExtension
	void extendModel (EditModel& model, EditorComponent& editor) override;
	EditHandler* createEditHandler (Object* object, EditView& editView, const MouseEvent& event) override;
	IDragHandler* createDragHandler (UserControl& control, const DragEvent& event) override;
	void collectTaskCategories (StringList& taskCategories) override;
	bool canPerformTask (EditView& editView, const EditTaskDescription& task) override;
	bool collectTaskCandidates (Container& resultList, EditView& editView, const EditTaskDescription& task) override;

	// Object
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

protected:
	ObjectList extensions;

	EditExtension* getExtension (StringID name) const;
};

} // namespace CCL

#endif // _ccl_editextension_h
