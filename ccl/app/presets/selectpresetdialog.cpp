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
// Filename    : ccl/app/presets/selectpresetdialog.cpp
// Description : Select Preset Dialog
//
//************************************************************************************************

#include "ccl/app/presets/selectpresetdialog.h"
#include "ccl/app/presets/presetnode.h"

#include "ccl/app/fileinfo/fileinfocomponent.h"
#include "ccl/app/fileinfo/filepreviewcomponent.h"

#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/itheme.h"

using namespace CCL;

//************************************************************************************************
// SelectPresetDialog::PresetInfoComponent
//************************************************************************************************

class SelectPresetDialog::PresetInfoComponent: public FilePreviewComponent
{
public:	
	DECLARE_CLASS_ABSTRACT (PresetInfoComponent, FilePreviewComponent)

	PresetInfoComponent (StringID skinNameSpace, StringID infoFormName);

	PROPERTY_MUTABLE_CSTRING (formName, FormName)

protected:
	// FilePreviewComponent
	IFileInfoComponent* createInfoComponent () override;
};

//************************************************************************************************
// SelectPresetDialog::PresetInfoComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SelectPresetDialog::PresetInfoComponent, FilePreviewComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

SelectPresetDialog::PresetInfoComponent::PresetInfoComponent (StringID skinNameSpace, StringID infoFormName)
: FilePreviewComponent ("Preview", skinNameSpace),
  formName (infoFormName)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileInfoComponent* SelectPresetDialog::PresetInfoComponent::createInfoComponent ()
{
	IFileInfoComponent* component = SuperClass::createInfoComponent ();

	auto* fileInfoComponent = unknown_cast<FileInfoComponent> (component);
	if(fileInfoComponent && fileInfoComponent->getFormName () == "PresetFileInfo")
	{
		fileInfoComponent->setSkinNamespace (getSkinNamespace ());
		fileInfoComponent->setFormName (getFormName ());

		if(auto* selector = getParentNode<SelectPresetDialog> ())
			selector->extendInfoComponent (*fileInfoComponent);
	}

	return component;
}

//************************************************************************************************
// SelectPresetDialog
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SelectPresetDialog, Browser)

//////////////////////////////////////////////////////////////////////////////////////////////////

SelectPresetDialog::SelectPresetDialog (IAttributeList* metaInfo, StringRef name, StringRef title)
: Browser (name.isEmpty () ? "SelectPreset" : name),  
  filePreviewComponent (nullptr),
  dialogBox (nullptr)
{	
	StyleFlags treeStyle (0, Styles::kItemViewBehaviorSelectExclusive|
							 Styles::kTreeViewAppearanceNoRoot|
							 Styles::kTreeViewBehaviorAutoExpand|
							 Styles::kItemViewBehaviorSelectFullWidth|
							 Styles::kItemViewBehaviorNoDrag);
	setTreeStyle (treeStyle);
	displayTreeLeafs (true);
	showListView (false); 
	canRefresh (false);
	persistentStates (true);

	Browsable::PresetContainerNode* presetsNode = NEW Browsable::PresetContainerNode (metaInfo, title);
	presetsNode->getBuilder ().setForceAlways (); // needed, otherwise the tree is empty (PresetNodesBuilder::cancelPresets called in hasPresets + refresh missing)
	addBrowserNode (presetsNode);
	setTreeRoot (presetsNode, false, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectPresetDialog::setPreviewForm (StringID skinNameSpace, StringID infoFormName)
{
	addFilePreviewComponent (NEW PresetInfoComponent (skinNameSpace, infoFormName));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectPresetDialog::addFilePreviewComponent (FilePreviewComponent* component)
{
	ASSERT (!filePreviewComponent)
	addComponent (filePreviewComponent = component);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* SelectPresetDialog::getSelectedPreset () const
{
	return selected;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectPresetDialog::updateFileInfo ()
{
	if(filePreviewComponent)
	{
		IPreset* selected = getSelectedPreset ();
		if(selected)
		{
			Url presetUrl;
			selected->getUrl (presetUrl);
			filePreviewComponent->setFile (presetUrl, nullptr, selected->getPresetName ());
		}
		else
			filePreviewComponent->setFile (Url::kEmpty, nullptr, String::kEmpty);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* SelectPresetDialog::runDialog (StringRef title)
{
	ASSERT (!getFormName ().isEmpty ())

	SharedPtr<IPreset> result;

	ITheme* theme = getTheme ();
	IView* dialogView = theme ? theme->createView (getFormName (), asUnknown ()) : nullptr;
	ASSERT (dialogView != nullptr)
	if(dialogView)
	{
		dialogView->setViewAttribute (IView::kTitle, title);
		DialogBox box;
		ScopedVar<DialogBox*> dialogScope (dialogBox, &box);
		int answer = box->runDialog (dialogView, Styles::kWindowCombinedStyleDialog | Styles::kWindowBehaviorSizable | Styles::kWindowBehaviorRestoreSize, Styles::kDialogOkCancel);
		if(answer == DialogResult::kOkay)
			result = getSelectedPreset ();
	}

	saveSettings ();
	return result.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectPresetDialog::onNodeFocused (BrowserNode* node, bool inList)
{
	if(Browsable::PresetNode* presetNode = ccl_cast<Browsable::PresetNode> (node))
		selected = presetNode->getPreset ();
	else
		selected.release ();
	updateFileInfo ();

	SuperClass::onNodeFocused (node, inList);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SelectPresetDialog::openNode (BrowserNode* node)
{
	if(ccl_cast<Browsable::PresetNode> (node))
	{
		if(dialogBox)
		{
			(*dialogBox)->setDialogResult (DialogResult::kOkay);
			(*dialogBox)->close ();
		}
		return true;
	}
	else
		return SuperClass::openNode (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SelectPresetDialog::appendContextMenu (IContextMenu& contextMenu)
{
	// e.g. suppress adding "Refresh" (SuperClass)
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SelectPresetDialog::restoreCurrentState ()
{
	SuperClass::restoreCurrentState ();

	if(!selected)
	{
		AutoPtr<IRecognizer> presetNodeRecognizer = Recognizer::create ([](IUnknown* unk) 
		{ 
			return unknown_cast<Browsable::PresetNode> (unk) != nullptr;}	
		);

		if(Browsable::PresetNode* presetNode = findNode<Browsable::PresetNode> (presetNodeRecognizer))
			setFocusNode (presetNode);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SelectPresetDialog::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "selectedPreset")
	{
		var = selected;
		var.share ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}
