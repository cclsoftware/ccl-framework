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
// Filename    : ccl/app/editing/inspector.h
// Description : Inspector Component
//
//************************************************************************************************

#ifndef _inspector_h
#define _inspector_h

#include "ccl/app/component.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/public/gui/framework/iview.h"

namespace CCL {

//************************************************************************************************
// InspectorComponent
/** A base class for components with inspector-like behavior. */
//************************************************************************************************

class InspectorComponent: public Component
{
public:
	DECLARE_CLASS_ABSTRACT (InspectorComponent, Component)

	InspectorComponent (StringRef inspectorName = "Inspector", StringID windowClass = "Editor");
	~InspectorComponent ();
	
	virtual void createEditor () = 0;

	PROPERTY_BOOL (alwaysCreateEditor, AlwaysCreateEditor)	///< create editor even if no view attached
	PROPERTY_STRING (workspaceFrame, WorkspaceFrame)		///< workspace frame URL (repeated for delegate view)

	void onTargetChanged ();

	Component* getEditor () const;
	virtual void setEditor (Component* editor);
	
	void setView (IView* view);
	virtual void updateEditorView ();

	void showEditor (bool state);
	bool focusEditorView ();
	bool isEditorVisible () const;
	bool isOpening () const;
	
	StringID getWindowClass () const;

	Attributes* getEditorState (StringID name) const;
	void setEditorState (StringID name, Attributes* state); // shared

	// Component
	IObjectNode* CCL_API findChild (StringRef id) const override;
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	bool save (const Storage& storage) const override;
	bool load (const Storage& storage) override;

protected:
	SharedPtr<Component> editor;
	IView* delegateView;
	MutableCString windowClass;
	Attributes editorStates;
	bool opening;

	virtual bool storeEditorState (Component* editor);
	virtual bool restoreEditorState (Component* editor);

	class SimpleEditor;
	Component* createSimpleEditor (StringRef name, StringID formName); ///< creates an editor component that only displays a form
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline StringID InspectorComponent::getWindowClass () const
{ return windowClass; }

inline bool InspectorComponent::isOpening () const
{ return opening; } 

} // namespace CCL

#endif // _inspector_h
