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
// Filename    : ccl/app/editing/editor.h
// Description : Editor Component
//
//************************************************************************************************

#ifndef _ccl_editor_h
#define _ccl_editor_h

#include "ccl/base/singleton.h"
#include "ccl/base/storage/configuration.h"

#include "ccl/app/component.h"
#include "ccl/app/editing/selection.h"

#include "ccl/public/gui/commanddispatch.h"

namespace CCL {

class EditView;
class EditModel;
class Selection;
class EditTool;
class ToolCollection;
class ToolBar;
class EditEnvironment;

//************************************************************************************************
// EditorComponent
//************************************************************************************************

class EditorComponent: public Component,
					   public CommandDispatcher<EditorComponent>,
					   public ISelectionViewer
{
public:
	DECLARE_CLASS (EditorComponent, Component)

	EditorComponent (StringRef name = nullptr, StringRef title = nullptr);
	~EditorComponent ();

	PROPERTY_POINTER (EditEnvironment, editEnvironment, EditEnvironment)

	// Tools
	void setToolBar (ToolBar* toolBar);
	ToolBar* getToolBar () const;
	ToolCollection& getTools ();
	EditTool* getDefaultTool () const;
	EditTool* getActiveTool () const;
	bool activateTool (EditTool* tool);

	// Model
	EditModel& getModel ();

	// Views
	virtual void addEditView (EditView* editView);
	virtual void removeEditView (EditView* editView);
	void setActiveEditView (EditView* editView, bool onAttach = false);
	EditView* getActiveEditView () const;
	Iterator* getEditViews () const;
	EditView* findEditView (MetaClassRef viewClass) const;
	template<class T> T* findEditView () const;
	bool hasEditView (EditView* editView) const;
	bool hasEditViews () const;

	// Commands
	virtual bool onEditCut (CmdArgs);
	virtual bool onEditCopy (CmdArgs);
	virtual bool onEditPaste (CmdArgs);
	virtual bool onEditDelete (CmdArgs);
	virtual bool onSelectAll (CmdArgs);
	virtual bool onDeselect (CmdArgs);
	virtual bool onInvertSelection (CmdArgs);
	virtual bool onNavigation (CmdArgs);
	void registerNavigationCommand (StringID commandName, int direction, int mode);

	// Component
	DECLARE_COMMANDS (EditorComponent)
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	class DocumentDirtyGuard;
	class ContextMenuBuilder;

	static Configuration::BoolValue contextMenuSelectAll;
	static Configuration::BoolValue contextMenuUndo;

protected:
	ObjectList editViews;
	ObjectArray navigationCommands;
	EditView* activeEditView;
	EditModel* model;
	ToolCollection* toolList;
	int configuration;

	PROPERTY_FLAG (configuration, 1<<0, canPasteOnItem)

	class NavigationCommand;

	virtual EditModel* createModel ();
	virtual void onActiveEditViewChanged (EditView* editView);

	// ISelectionViewer
	void showSelection (bool redraw = true) override;
	void hideSelection (bool redraw = true) override;
	void makeSelectedItemsVisible (bool relaxed) override;
};

//************************************************************************************************
// EditorComponent::DocumentDirtyGuard
/** Sets document dirty if a parameter changes during it's lifetime. */
//************************************************************************************************

class EditorComponent::DocumentDirtyGuard
{
public:
	DocumentDirtyGuard (IParameter* parameter);
	DocumentDirtyGuard (IParameter* parameter, bool checkOnly);
	~DocumentDirtyGuard ();

private:
	IParameter* parameter;
	Variant oldValue;

	void init (IParameter* param);
};

//************************************************************************************************
// EditorComponent::ContextMenuBuilder
//************************************************************************************************

class EditorComponent::ContextMenuBuilder
{
public:
	ContextMenuBuilder (EditorComponent& component, IContextMenu& contextMenu);

	void build ();

	void appendToolCommands ();
	void appendEditCommands (bool includingPaste);
	void appendSelectionCommands ();
	void appendUndoCommands ();

	void appendEditCut ();
	void appendEditCopy ();
	void appendEditPaste ();
	void appendEditDelete ();

protected:
	EditorComponent& component;
	IContextMenu& contextMenu;
};

//************************************************************************************************
// EditorRegistry
//************************************************************************************************

class EditorRegistry: public Object,
					  public Singleton<EditorRegistry>
{
public:
	EditorRegistry ();
	~EditorRegistry ();

	/// add / remove editor components
	void addEditor (EditorComponent* editor);
	void removeEditor (EditorComponent* editor);

	// currently active editor (global)
	EditorComponent* getActiveEditor () const;
	void setActiveEditor (EditorComponent* editor);

	// lock changes to active editor
	struct ActiveEditorGuard;

	// iterate all editors
	Iterator* getEditors () const;

	/// find component by class
	EditorComponent* findEditor (MetaClassRef type, bool needsViews = true) const;
	template <class T> T* findEditor (bool needsViews = true) const;

private:
	ObjectList editors;
	EditorComponent* activeEditor;
	bool activeEditorLocked;
};

//************************************************************************************************
// EditorRegistry::ActiveEditorGuard - lock changes to active editor
//************************************************************************************************

struct EditorRegistry::ActiveEditorGuard
{
	EditorRegistry& registry;
	bool wasLocked;

	ActiveEditorGuard (EditorRegistry& registry)
	: registry (registry),
	  wasLocked (registry.activeEditorLocked)
	{
		registry.activeEditorLocked = true;
	}

	~ActiveEditorGuard ()
	{
		if(wasLocked == false)
			registry.activeEditorLocked = false;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool EditorComponent::hasEditViews () const
{ return !editViews.isEmpty (); }

template <class T> T* EditorComponent::findEditView () const
{ return static_cast<T*> (findEditView (ccl_typeid<T> ())); }

template <class T> T* EditorRegistry::findEditor (bool needsViews) const
{ return findEditor (ccl_typeid<T> (), needsViews); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_editor_h
