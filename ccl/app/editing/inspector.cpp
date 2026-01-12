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
// Filename    : ccl/app/editing/inspector.cpp
// Description : Inspector Component
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/editing/inspector.h"
#include "ccl/app/controls/usercontrol.h"

#include "ccl/base/storage/storage.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/iskinmodel.h"
#include "ccl/public/gui/framework/iwindowmanager.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

namespace CCL {

//************************************************************************************************
// InspectorDelegate
//************************************************************************************************

class InspectorDelegate: public UserControl
{
public:
	InspectorDelegate (const Rect& size, InspectorComponent* inspector, StringRef workspaceFrame)
	: UserControl (size),
	  inspector (inspector),
	  workspaceFrame (workspaceFrame)
	{}

	PROPERTY_STRING (workspaceFrame, WorkspaceFrame)
	
	void attached (IView* parent) override
	{		
		UserControl::attached (parent);

		inspector->setWorkspaceFrame (workspaceFrame);
		inspector->setView (*this);
		inspector->createEditor ();
	}
	
	void removed (IView* parent) override
	{
		UserControl::removed (parent);

		inspector->setView (nullptr);
		inspector->updateEditorView ();
		inspector->setWorkspaceFrame (String::kEmpty);
		inspector->setEditor (nullptr);
	}

protected:
	InspectorComponent* inspector;
};

//************************************************************************************************
// InspectorComponent::SimpleEditor
//************************************************************************************************

class InspectorComponent::SimpleEditor: public CCL::Component
{
public:
	SimpleEditor (StringRef name, StringID formName);

	// Component
	bool save (const CCL::Storage& storage) const override;
	CCL::IView* CCL_API createView (StringID name, VariantRef data, const CCL::Rect& bounds) override;

private:
	MutableCString formName;
};

} // namespace CCL

//************************************************************************************************
// InspectorComponent::SimpleEditor
//************************************************************************************************

InspectorComponent::SimpleEditor::SimpleEditor (StringRef name, StringID formName)
: Component (name),
  formName (formName)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool InspectorComponent::SimpleEditor::save (const Storage& storage) const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API InspectorComponent::SimpleEditor::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "EditView")
	{
		ITheme* theme = getTheme ();
		ASSERT (theme != nullptr)
		UnknownPtr<IAttributeList> arguments (data.asUnknown ());
		return theme ? theme->createView (formName, this->asUnknown (), arguments) : nullptr; 
	}
	return Component::createView (name, data, bounds);
}

//************************************************************************************************
// InspectorComponent
//************************************************************************************************

DEFINE_CLASS (InspectorComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

InspectorComponent::InspectorComponent (StringRef inspectorName, StringID windowClass)
: Component (inspectorName),
  windowClass (windowClass),
  delegateView (nullptr),
  alwaysCreateEditor (false),
  opening (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

InspectorComponent::~InspectorComponent ()
{
	if(editor)
		removeChild (editor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InspectorComponent::onTargetChanged ()
{
	if(delegateView || isAlwaysCreateEditor ())
		createEditor ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool InspectorComponent::load (const Storage& storage)
{
	storage.getAttributes ().get (editorStates, "editors");
	return SuperClass::load (storage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool InspectorComponent::save (const Storage& storage) const
{
	if(editor)
		const_cast<InspectorComponent*> (this)->storeEditorState (editor);

	if(!editorStates.isEmpty ())
		storage.getAttributes ().set ("editors", editorStates);
	return SuperClass::save (storage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes* InspectorComponent::getEditorState (StringID name) const
{
	return editorStates.getAttributes (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InspectorComponent::setEditorState (StringID name, Attributes* state)
{
	editorStates.set (name, state, Attributes::kShare);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool InspectorComponent::storeEditorState (Component* editor)
{
	AutoPtr<Attributes> a = NEW Attributes;
	bool result = editor->save (Storage (*a));
	ASSERT (result)
	if(result && !a->isEmpty ())
	{
		MutableCString name (editor->getName ());
		ASSERT (!name.isEmpty ())
		CCL_PRINTF ("Inspector: storing editor %s\n", name.str ())
		setEditorState (name, a);
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool InspectorComponent::restoreEditorState (Component* editor)
{
	bool result = false;
	MutableCString name (editor->getName ());
	ASSERT (!name.isEmpty ())
	Attributes* a = editorStates.getAttributes (name);
	if(a)
	{
		CCL_PRINTF ("Inspector: restoring editor %s\n", name.str ())
		bool result = editor->load (Storage (*a));
		ASSERT (result)
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Component* InspectorComponent::createSimpleEditor (StringRef name, StringID formName)
{
	return NEW SimpleEditor (name, formName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Component* InspectorComponent::getEditor () const
{
	return editor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InspectorComponent::setEditor (Component* editor)
{
	if(this->editor != editor)
	{
		if(this->editor)
		{
			storeEditorState (this->editor);
			this->editor->terminate ();

			removeChild (this->editor);
		}

		this->editor = editor;
	
		if(editor)
		{
			addChild (editor);

			editor->initialize (nullptr);
			restoreEditorState (editor);
		}
	}
	signalHasChild (CCLSTR ("Editor"));
	updateEditorView ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InspectorComponent::setView (IView* view)
{
	this->delegateView = view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InspectorComponent::showEditor (bool state)
{
	ScopedVar<bool> scope (opening, state && !isEditorVisible ());

	if(state)
		System::GetWindowManager ().openWindow (windowClass, false);
	else
		System::GetWindowManager ().closeWindow (windowClass);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool InspectorComponent::isEditorVisible () const
{
	return delegateView != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InspectorComponent::updateEditorView ()
{
	if(delegateView)
	{
		ViewBox delegateBox (delegateView);

		Rect bounds;
		delegateBox.getClientRect (bounds);

		Rect delegateSize (delegateBox.getSize ());
		//bool isFirstContent = delegateBox.getChildren ().isEmpty ();
		delegateBox.getChildren ().removeAll ();

		CCL_PRINTLN ("InspectorComponent::updateEditorView called!")
	
		ccl_forceGC (); 

		if(editor)
		{
			Attributes arguments;
			ASSERT (!workspaceFrame.isEmpty ())
			arguments.set ("frame", workspaceFrame);
			IView* editView = editor->createView ("EditView", static_cast<IAttributeList*> (&arguments), bounds);
			if(editView)
			{
				// fit edit view into our delegate according to it's attachment
				Rect editorSize (editView->getSize ());
				ViewBox editViewBox (editView);

				if(delegateBox.getSizeMode () & IView::kHFitSize)
					delegateSize.setWidth (editorSize.getWidth ());
				else if((editViewBox.getSizeMode () & (IView::kAttachLeft|IView::kAttachRight)) == (IView::kAttachLeft|IView::kAttachRight))
					editorSize.setWidth (delegateSize.getWidth ());

				if(delegateBox.getSizeMode () & IView::kVFitSize)
					delegateSize.setHeight (editorSize.getHeight ());
				if((editViewBox.getSizeMode () & (IView::kAttachTop|IView::kAttachBottom)) == (IView::kAttachTop|IView::kAttachBottom))
					editorSize.setHeight (delegateSize.getHeight ());

				delegateView->setSize (delegateSize);
				editView->setSize (editorSize);

				delegateBox.getChildren ().add (editView);

				//if(!isFirstContent)
					if(UserControl* uc = UserControl::cast_IView<UserControl> (delegateView))
						uc->resetSizeLimits ();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool InspectorComponent::focusEditorView ()
{
	if(delegateView)
	{
		IView* toFocus = delegateView;

		// focus "EditView" (created content) if desired, otherwise delegateView (container frame)
		IView* editView = delegateView->getChildren ().getFirstView ();
		if(editView && ViewBox (editView).wantsFocus ())
			toFocus = editView;

		toFocus->takeFocus ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* CCL_API InspectorComponent::findChild (StringRef id) const
{
	if(id == CCLSTR ("Editor"))
		return editor;

	return SuperClass::findChild (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API InspectorComponent::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "Delegate")
	{
		String workspaceFrame;
		UnknownPtr<ISkinCreateArgs> args (data.asUnknown ());
		if(args)
		{
			Variant var;
			args->getVariable (var, "frame");
			workspaceFrame = var.asString ();
		}

		ASSERT (delegateView == nullptr)
		InspectorDelegate* id = NEW InspectorDelegate (bounds, this, workspaceFrame);
		ViewBox v (*id);
		return v;
	}
	return SuperClass::createView (name, data, bounds);
}
