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
// Filename    : ccl/app/presets/presetbrowser.cpp
// Description : Preset Browser
//
//************************************************************************************************

#include "ccl/app/presets/presetbrowser.h"
#include "ccl/app/presets/presetcomponent.h"
#include "ccl/app/presets/presetfileprimitives.h"
#include "ccl/app/presets/presetnode.h"
#include "ccl/app/presets/presetsystem.h"
#include "ccl/app/presets/presetfile.h"

#include "ccl/app/browser/browser.h"
#include "ccl/app/components/searchcomponent.h"

#include "ccl/base/asyncoperation.h"
#include "ccl/base/storage/packageinfo.h"

#include "ccl/public/app/presetmetainfo.h"
#include "ccl/public/app/signals.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/controlstyles.h"
#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/gui/framework/ithememanager.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/plugservices.h"

using namespace CCL;
using namespace Browsable;

//************************************************************************************************
// PresetBrowser::PresetFilter
//************************************************************************************************

class PresetBrowser::PresetFilter: public Unknown,
								   public IObjectFilter,
								   public IUrlFilter
{
public:
	PresetFilter (int type, PresetComponent& presetComponent);
	
	// IObjectFilter
	tbool CCL_API matches (IUnknown* object) const override;
	
	// IUrlFilter
	tbool CCL_API matches (UrlRef url) const override;
	
	CLASS_INTERFACE (IObjectFilter, Unknown)
private:
	PresetComponent& presetComponent;
	int type;
	
	bool matches (const IPreset& preset) const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("PresetBrowser")
	XSTRING (NoPresets, "(no presets)")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (PresetBrowser)
	DEFINE_COMMAND ("Presets", "Next",		PresetBrowser::onNextPreset)
	DEFINE_COMMAND ("Presets", "Previous",	PresetBrowser::onPrevPreset)
END_COMMANDS (PresetBrowser)

namespace CCL {
namespace Browsable {

//************************************************************************************************
// PresetRootNode
//************************************************************************************************

class PresetRootNode: public PresetContainerNode
{
public:
	typedef PresetContainerNode SuperClass;

	PresetRootNode (IAttributeList* metaInfo);

	// PresetContainerNode
	void build () override;
	bool handlesPreset (const PresetMetaAttributes& presetAttribs) const override;
	void onPresetCreated (IPreset& preset) override;
	void onPresetRemoved (IPreset& preset) override;

private:
	UID classID;
	String category;
};

} // namespace Browsable
} // namespace CCL

//************************************************************************************************
// PresetRootNode
//************************************************************************************************

PresetRootNode::PresetRootNode (IAttributeList* metaInfo)
: PresetContainerNode (metaInfo, CCLSTR ("Presets"))
{
	PresetMetaAttributes metaAttribs (*metaInfo);
	metaAttribs.getClassID (classID);

	// presets with no classID are identified by category
	if(!classID.isValid ())
		category = metaAttribs.getCategory ();

	builder.setForceAlways (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetRootNode::build ()
{
	SuperClass::build ();

	if(getContent ().isEmpty ())
		addSorted (NEW BrowserNode (XSTR (NoPresets)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetRootNode::handlesPreset (const PresetMetaAttributes& presetAttribs) const
{
	UID classID;
	bool hasClassID = presetAttribs.getClassID (classID);
	return hasClassID ? classID == this->classID : category == presetAttribs.getCategory ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetRootNode::onPresetCreated (IPreset& preset)
{
	SuperClass::onPresetCreated (preset);

	// remove the "No Presets" dummy if necessary
	if(content.count () == 2)
	{
		BrowserNode* firstNode = getNodeAt (0);
		if(firstNode->getTitle () == XSTR (NoPresets))
			SortedNode::removeNode (this, firstNode, getBrowser ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetRootNode::onPresetRemoved (IPreset& preset)
{
	bool wasEmpty = content.isEmpty ();

	SuperClass::onPresetRemoved (preset);

	// add the "No Presets" dummy if necessary
	if(!wasEmpty && content.isEmpty ())
		SortedNode::insertNode (this, NEW BrowserNode (XSTR (NoPresets)), getBrowser ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum PresetTags
	{
		kPresetName = 200,
		kClassName,
		kCreator,
		kEditMode,
		kPresetFilter
	};
};

//************************************************************************************************
// PresetBrowser
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PresetBrowser, Browser)
IMPLEMENT_COMMANDS (PresetBrowser, Browser)

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetBrowser::PresetBrowser (PresetComponent& presetComponent)
: Browser ("PresetBrowser"),
  presetComponent (presetComponent),
  selectedPreset (nullptr),
  loadedPreset (nullptr),
  fileTypes (nullptr),
  commandsDisabled (false)
{
	// configure browser
	setTreeStyle (StyleFlags (0, Styles::kItemViewBehaviorAutoSelect|
								 Styles::kItemViewBehaviorSelectExclusive|
								 Styles::kTreeViewAppearanceNoRoot|
								 Styles::kTreeViewBehaviorAutoExpand|
								 Styles::kItemViewBehaviorSelectFullWidth));
	displayTreeLeafs (true);
	showListView (false);

	// params
	IParameter* editModeParam = paramList.addParam (CSTR ("editMode"), Tag::kEditMode);
	editModeParam->enable (false); // Editing is enabled when filtering for user presets only
	
	// metainfo params
	paramList.addString (CSTR ("presetName"), Tag::kPresetName);
	paramList.addString (CSTR ("className"), Tag::kClassName);
	paramList.addString (CSTR ("creator"), Tag::kCreator);

	// add preset root node
	AutoPtr<IAttributeList> metaInfo (createMetaInfo ());
	if(metaInfo->isEmpty () == false)
	{
		auto presetRoot = NEW PresetRootNode (metaInfo);
		presetRoot->hasFavoritesFolder (presetComponent.hasPresetFavorites ());
		addBrowserNode (presetRoot);

		setTreeRoot (presetRoot, false, false);

		if(presetComponent.hasPresetFavorites ())
		{
			// favorite column
			AutoPtr<IColumnHeaderList> columns (ccl_new<IColumnHeaderList> (ClassID::ColumnHeaderList));
			columns->addColumn (200, nullptr, nullptr, 0, 0);
			columns->addColumn (20, nullptr, PresetNode::kFavorite, 0, 0);
			columns->moveColumn (PresetNode::kFavorite, 0);
			setDefaultColumns (columns);
			hideColumnHeaders (true);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetBrowser::~PresetBrowser ()
{
	if(fileTypes)
		fileTypes->release ();

	if(selectedPreset)
		selectedPreset->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetBrowser::getCheckedPresets (Vector<IPreset*>& checkedPresets) const
{
	UnknownList nodes;
	getRootItem ()->getContent (nodes);
	ForEachUnknown (nodes, node)
		auto* presetNode = unknown_cast<PresetNode> (node);
		if(presetNode != nullptr && presetNode->isChecked ())
			checkedPresets.add (presetNode->getPreset ());
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetBrowser::filterPresets (FilterType type)
{
	if(IParameter* filterParam = paramList.byTag (Tag::kPresetFilter))
		filterParam->setValue (int(type), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* PresetBrowser::getPresetRootNode ()
{
	return findNode (CSTR ("root/Presets"), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* PresetBrowser::createMetaInfo ()
{
	IAttributeList* metaInfo = NEW PackageInfo;
	presetComponent.getPresetMediator ()->getPresetMetaInfo (*metaInfo);
	return metaInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetBrowser::updateMetaInfo (IPreset* preset)
{
	String presetName;
	AutoPtr<IAttributeList> metaInfo;
	if(preset)
	{
		presetName = preset->getPresetName ();
		metaInfo.share (preset->getMetaInfo ());
	}
	if(!metaInfo)
		metaInfo = NEW Attributes;

	PresetMetaAttributes metaAttribs (*metaInfo);
	paramList.byTag (Tag::kPresetName)->setValue (presetName);
	paramList.byTag (Tag::kClassName)->setValue (metaAttribs.getClassName ());
	paramList.byTag (Tag::kCreator)->setValue (metaAttribs.getCreator ());
	// ...
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetBrowser::selectCurrentPreset ()
{
	if(BrowserNode* presetRoot = getPresetRootNode ())
	{
		AutoPtr<IAttributeList> metaInfo (createMetaInfo ());
		PresetNode* node = PresetNodeSorter::findPresetNode (*presetRoot, presetComponent.getCurrentPresetUrl (), metaInfo, true);
		if(!node && presetComponent.getCurrentPresetUrl ().getProtocol () == MemoryUrl::Protocol)
		{
			// try to find source preset by name if a memory preset is loaded
			String name = presetComponent.getCurrentPresetName ();
			if(name.isEmpty () == false)
			{
				AutoPtr<IUnknownList> presets = System::GetPresetManager ().getPresets (metaInfo);
				if(presets)
				{
					ForEachUnknown (*presets, p)
						UnknownPtr<IPreset> preset (p);
						if(preset && preset->getPresetName () == name)
						{
							Url presetUrl;
							preset->getUrl (presetUrl);
							node = PresetNodeSorter::findPresetNode (*presetRoot, presetUrl, metaInfo, true);
							if(node)
								break;
						}
					EndFor			
				}
			}			
		}
		if(node)
		{
			setFocusNode (node, true);
			loadedPreset = selectedPreset; // set loaded preset to selected preset to avoid that loadSelectedPreset loads it again

			// select according filter
			if(nodeFilter && nodeFilter->matches (node->asUnknown ()) == false)
			{
				if(IParameter* filterParam = paramList.byTag (Tag::kPresetFilter))
				{
					for(int i = filterParam->getMin ().asInt (), max = filterParam->getMax ().asInt (); i <= max; i++)
					{										
						if(i != filterParam->getValue ().asInt ())
						{
							if(PresetFilter (i, presetComponent).matches (node->asUnknown ()))
							{
								filterParam->setValue (i, true);
								break;
							}
						}
					}					
				}	
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetBrowser::loadSelectedPreset (bool force)
{
	if(selectedPreset)
		if(force || loadedPreset != selectedPreset)
		{
			PresetComponent::GUIActionScope guiActionScope (true);
			if(presetComponent.restorePreset (selectedPreset))
				loadedPreset = selectedPreset;
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API PresetBrowser::createPopupView (SizeLimit& limits)
{
	if(presetComponent.isEnabled () == false)
		return nullptr;

	ITheme* theme = getTheme ();
	ASSERT (theme != nullptr)
	if(theme)
	{
		loadedPreset = nullptr;

		StyleFlags treeStyle (getTreeStyle ());
		treeStyle.setCustomStyle (Styles::kItemViewBehaviorNoDoubleClick);
		setTreeStyle (treeStyle);
		static const CString formName ("CCL/PresetBrowserPopup"); 

		IView* view = theme->createView (formName, presetComponent.asUnknown ());
		if(view == nullptr)
		{
			ITheme* theme2 = System::GetThemeManager ().getApplicationTheme ();
			if(theme2 && theme2 != theme)
				view = theme2->createView (formName, presetComponent.asUnknown ());
		}
		checkPopupLimits (view, limits);

		acceptOnMouseUp (true); // for using the "drag" gesture as in a menu
		return view;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetBrowser::hasPopupResult ()
{
	return selectedPreset != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetBrowser::onPopupClosed (Result result)
{
	if(result == IPopupSelectorClient::kOkay)
		loadSelectedPreset (false);
	
	if(auto* searchComponent = findChildNode<SearchComponent> ())
		searchComponent->setVisible (false);
		
	paramList.byTag (Tag::kEditMode)->setValue (false, true);
	presetComponent.onPresetBrowserClosed (result == IPopupSelectorClient::kOkay);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient::Result CCL_API PresetBrowser::onMouseDown (const MouseEvent& event, IWindow& popupWindow)
{
	// if we receive a mouse down, it means that the user did not "drag" into the menu; in this case we want the browser to stay open
	acceptOnMouseUp (false);

	UnknownPtr<IView> view (&popupWindow);
	if(view)
	{
		IViewChildren& children = ViewBox (view).getChildren ();
		IView* deepest = children.findChildView (event.where, true);
		if(deepest)
		{
			// accept on a double click on our tree view, but not elsewhere (avoid doubleclick detection delay when not required)
			acceptOnDoubleClick (UnknownPtr<IItemView> (deepest).isValid ());

			// if a mouse down on a scrollbar occurs: ignore
			if(ViewBox (deepest).getClassID () == CCL::ClassID::ScrollBar)
				return kIgnore;
		}
	}
	
	return PopupSelectorClient::onMouseDown (event, popupWindow);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient::Result CCL_API PresetBrowser::onKeyDown (const KeyEvent& event)
{
	if(event.vKey == VKey::kSpace && !event.isRepeat ())
	{
		loadSelectedPreset (true);
		return kSwallow; // stay open, event consumed
	}
	return PopupSelectorClient::onKeyDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetBrowser::paramChanged (IParameter* param)
{
	if(param->getTag () == Tag::kPresetFilter)
	{
		int filterType = param->getValue ().asInt ();
		
		IParameter* editModeParam = paramList.byTag (Tag::kEditMode);
		if(filterType == kFilterUser)
			editModeParam->enable (true);
		else
		{
			editModeParam->enable (false);
			editModeParam->setValue (false, true);
		}
		
		AutoPtr<PresetFilter> presetFilter = NEW PresetFilter (filterType, presetComponent);
		setNodeFilter (presetFilter);
		if(auto* searchProvider = unknown_cast<PresetSearchProvider> (getSearchProvider ()))
			searchProvider->setUrlFilter (presetFilter);
		
		if(auto* searchComponent = findChildNode<SearchComponent> ())
			searchComponent->setVisible (false);
	}
	else if(param->getTag () == Tag::kEditMode)
	{
		int isEditMode = param->getValue ().asBool ();

		// Uncheck all nodes when leaving edit mode
		if(!isEditMode)
			forEachPresetNode ([] (PresetNode* node) {
				node->setChecked (false);
			});
		
		// Update style while editing
		if(IItemView* view = getTreeView ())
		{
			ViewBox vb (view);
			ViewBox::StyleModifier (vb).setCustomStyle (Styles::kItemViewBehaviorSelectExclusive|Styles::kItemViewBehaviorNoUnselect, !isEditMode);
		}
		
		// Add/Remove edit selection column
		AutoPtr<IColumnHeaderList> columns (ccl_new<IColumnHeaderList> (ClassID::ColumnHeaderList));
		columns->addColumn (100, nullptr, nullptr, 0, IColumnHeaderList::kFill);
		if(isEditMode)
		{
			columns->addColumn (40, nullptr, PresetNode::kEditSelection, 0, 0);
			columns->moveColumn (PresetNode::kEditSelection, 0);
		}
		setDefaultColumns (columns);
		hideColumnHeaders (true);

		propertyChanged ("canUpdatePreset");
	}
	
	return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetBrowser::onViewAttached (IItemView* itemView)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetBrowser::attached (IWindow& popupWindow)
{
	PopupSelectorClient::attached (popupWindow);
	presetComponent.onPresetBrowserOpened ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetBrowser::restoreCurrentState ()
{
	SuperClass::restoreCurrentState ();

	selectAll (false); // In case the current preset does not exist as a node, orphaned selections should be removed
	safe_release (selectedPreset); // reset also selectedPreset, in case a previously used preset was restored in restoreCurrentState
	loadedPreset = nullptr;
	selectCurrentPreset (); // after browser has restored expand state
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetBrowser::addSearch ()
{
	if(AutoPtr<IAttributeList> metaAttributes = createMetaInfo ())
	{
		CCL::UID classID;
		PresetMetaAttributes (*metaAttributes).getClassID (classID);
		setSearchProvider (NEW PresetSearchProvider (classID));
		resultListHideCategories (true);
	}
	Browser::addSearch ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetBrowser::addSourceFilter ()
{
	if(paramList.byTag (Tag::kPresetFilter) == nullptr)
	{
		IParameter* presetFilterParam = paramList.addInteger (0, kFilterTypeCount , "presetFilter", Tag::kPresetFilter);
		presetFilterParam->setSignalAlways (true);
		presetFilterParam->setValue (int(kFilterFactory), true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetBrowser::onNodeRemoved (BrowserNode* node)
{
	if(auto* presetNode = ccl_cast<PresetNode> (node))
	{
		if(selectedPreset != nullptr && presetNode->getPreset () == selectedPreset)
			safe_release (selectedPreset);
	}
	
	SuperClass::onNodeRemoved (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetBrowser::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "canUpdatePreset")
	{
		if(selectedPreset != nullptr)
		{
			Url selectedPresetUrl;
			selectedPreset->getUrl (selectedPresetUrl);

			bool isFactoryPreset = presetComponent.isFactoryPreset (selectedPresetUrl);

			IParameter* editModeParam = paramList.byTag (Tag::kEditMode);
			bool isEditing = editModeParam->isEnabled () && editModeParam->getValue ().asBool ();

			var = !isFactoryPreset && isAnyNodeSelected () && !isEditing;
		}
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetBrowser::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IItemView::kSelectionChanged)
		propertyChanged ("canUpdatePreset");

	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetBrowser::mouseWheelOnSource (const MouseWheelEvent& event, IView* source)
{
	selectNextPreset (event.delta < 0 ? +1 : -1, false);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetBrowser::onNodeFocused (BrowserNode* node, bool inList)
{
	PresetNode* presetNode = ccl_cast<PresetNode> (node);
	if(presetNode == nullptr)
	{
		// Could be a SearchResultNode
		if(SearchResultNode* searchResultNode = ccl_cast<SearchResultNode> (node))
		{
			const Url* url = searchResultNode->getPath ();
			if(url != nullptr)
				presetNode = findPresetNodeWithUrl (*url);
		}
	}
	
	IPreset* preset = nullptr;
	if(presetNode != nullptr && !presetNode->isFolder ())
		preset = presetNode->getPreset ();
	
	take_shared (selectedPreset, preset);
	updateMetaInfo (preset);

	SuperClass::onNodeFocused (node, inList);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetBrowser::onEditNode (BrowserNode& node, StringID columnID, const IItemModel::EditInfo& info)
{
	auto editPresetName = [this, &node, &info] ()
	{
		auto* presetNode = ccl_cast<PresetNode> (&node);
		if(!presetNode)
			return;
		
		ListViewModelBase* treeModel = getTreeModel ();
		IPreset* preset = presetNode->getPreset ();
		if(treeModel == nullptr || preset == nullptr)
			return;

		Promise (treeModel->editString (preset->getPresetName (), info.rect, info)).then ([preset] (IAsyncOperation& operation)
		{
			if(operation.getState () != IAsyncInfo::kCompleted)
				return;

			String presetName = PresetFilePrimitives::makeUniquePresetName (operation.getResult (), preset->getMetaInfo (), nullptr);
			if(operation.getState () == IAsyncInfo::kCompleted && preset != nullptr)
				System::GetPresetManager ().renamePreset (*preset, presetName);
		});
	};
	
	bool isEditing = paramList.getParameterByTag (Tag::kEditMode)->getValue ().asBool ();
	if(isEditing)
	{
		if(columnID == PresetNode::kEditSelection)
		{
			node.setChecked (!node.isChecked ());
			redrawNode (&node);
		}
		else
			editPresetName ();
	}
	else
	{
		bool isRightClick = false;
		if(auto mouseEvent = info.editEvent.as<MouseEvent> ())
			isRightClick = mouseEvent->keys.isSet (KeyState::kRButton);
		
		if(!isRightClick)
			loadSelectedPreset (true);
	}
	
	SuperClass::onEditNode (node, columnID, info);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

struct PresetSelectFilter: public ObjectFilter
{
	tbool CCL_API matches (IUnknown* object) const override
	{
		// skip folders (including collections)
		// todo (if target loads complete collections): if current preset was a collection, allow (force?) nextNode to be a collection
		BrowserNode* node = unknown_cast<BrowserNode> (object);
		return node && !node->isFolder ();
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetBrowser::selectNextPreset (int increment, bool checkOnly)
{	
	if(checkOnly)
		return true; // would be too expensive...

	if(BrowserNode* presetRoot = getPresetRootNode ())
	{
		UrlRef currentUrl (presetComponent.getCurrentPresetUrl ());
		AutoPtr<IAttributeList> metaInfo (createMetaInfo ());

		BrowserNode* currentNode = PresetNodeSorter::findPresetNode (*presetRoot, currentUrl, metaInfo, true);
		if(!currentNode)
			currentNode = presetRoot;

		PresetSelectFilter filter;
		if(BrowserNode* nextNode = navigate (*currentNode, increment, &filter))
		{
			setFocusNode (nextNode);
			loadSelectedPreset (false);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetNode* PresetBrowser::findPresetNodeWithUrl (UrlRef url) const
{
	AutoPtr<IRecognizer> presetNodeRecognizer = Recognizer::create ([&url] (IUnknown* object)
	{
		PresetNode* presetNode = unknown_cast<PresetNode> (object);
		if(presetNode == nullptr)
			return false;
		
		IPreset* preset = presetNode->getPreset ();
		if(preset == nullptr)
			return false;
		
		Url presetUrl;
		preset->getUrl (presetUrl);
		if(presetUrl != url)
			return false;
			
		return true;
	});
	
	return findNode<PresetNode> (presetNodeRecognizer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetBrowser::onNextPreset (CmdArgs args)
{
	if(commandsDisabled)
		return false;

	return selectNextPreset (+1, args.checkOnly ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetBrowser::onPrevPreset (CmdArgs args)
{
	if(commandsDisabled)
		return false;

	return selectNextPreset (-1, args.checkOnly ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetBrowser::forEachPresetNode (PresetNodeEditFunc nodeEditFunction)
{
	UnknownList nodes;
	PresetBrowser::getRootItem ()->getContent (nodes);
	ForEachUnknown (nodes, node)
		if(auto* presetNode = unknown_cast<PresetNode> (node))
			nodeEditFunction (presetNode);
	EndFor
}

//************************************************************************************************
// PresetFilter
//************************************************************************************************

PresetBrowser::PresetFilter::PresetFilter (int type, PresetComponent& presetComponent)
: type (type),
  presetComponent (presetComponent)
{}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetBrowser::PresetFilter::matches (IUnknown* object) const
{
	if(unknown_cast<PresetContainerNode> (object) != nullptr)
		return true;
	
	auto* presetNode = unknown_cast<PresetNode> (object);
	if(presetNode == nullptr)
		return false;
	
	IPreset* preset = presetNode->getPreset ();
	if(preset == nullptr)
		return false;
	
	return matches (*preset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetBrowser::PresetFilter::matches (UrlRef url) const
{
	IPreset* preset = System::GetPresetManager ().openPreset (url);
	if(preset == nullptr)
		return false;
	
	return matches (*preset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetBrowser::PresetFilter::matches (const IPreset& preset) const
{
	Url url;
	preset.getUrl (url);
	bool isFactoryPreset = presetComponent.isFactoryPreset (url);
	
	if(type == PresetBrowser::kFilterFactory && isFactoryPreset)
		return true;
	
	if(type == PresetBrowser::kFilterUser && !isFactoryPreset)
		return true;
	
	return false;
}
