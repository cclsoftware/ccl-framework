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
// Filename    : ccl/app/browser/pluginmanagement.cpp
// Description : Plug-in Management Component
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/browser/pluginmanagement.h"
#include "ccl/app/browser/pluginselector.h"
#include "ccl/app/utilities/shellcommand.h"

#include "ccl/app/presets/objectpreset.h"
#include "ccl/app/controls/listviewmodel.h"
#include "ccl/app/options/useroption.h"
#include "ccl/app/utilities/appdiagnostic.h"

#include "ccl/base/storage/settings.h"

#include "ccl/public/app/presetmetainfo.h"
#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/gui/framework/idragndrop.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/framework/icommandtable.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/text/itextstreamer.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/plugins/versionnumber.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"

namespace CCL {
	
DEFINE_IID_ (IPlugInVersionProvider, 0x5039efba, 0x503f, 0x4ac0, 0xa3, 0x9a, 0x72, 0xac, 0x7b, 0xb7, 0x7b, 0x9d)
DEFINE_IID_ (IPlugInManagementExtension, 0xbf1b77d8, 0x1eec, 0x4742, 0xb0, 0x33, 0x66, 0x94, 0xa2, 0xab, 0x81, 0xb1)

//************************************************************************************************
// PlugInListViewModel
//************************************************************************************************

class PlugInListViewModel: public ListViewModel
{
public:
	DECLARE_CLASS_ABSTRACT (PlugInListViewModel, ListViewModel)

	PlugInListViewModel (PlugInManagementComponent& component);

	PROPERTY_BOOL (visible, Visible)

	// ListViewModel
	IUnknown* CCL_API createDragSessionData (ItemIndexRef index) override;
	void onVisibleChanged (bool state) override;
	void onSelectionChanged () override;
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override;
	tbool CCL_API editCell (ItemIndexRef index, int column, const EditInfo& info) override;
	tbool CCL_API appendItemMenu (IContextMenu& menu, ItemIndexRef itemIndex, const IItemSelection& selection) override;

protected:
	PlugInManagementComponent& component;
	
	bool showFileInSystem (CmdArgs args, VariantRef data);
};

//************************************************************************************************
// BlockListViewModel
//************************************************************************************************

class BlockListViewModel: public ListViewModel
{
public:
	DECLARE_CLASS_ABSTRACT (BlockListViewModel, ListViewModel)

	BlockListViewModel (PlugInManagementComponent& component);

	// ListViewModel
	tbool CCL_API canRemoveItem (ItemIndexRef index) override { return true; }
	tbool CCL_API canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView = nullptr) override;
	tbool CCL_API insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session) override;
	bool removeItems (ItemIndexRef index, const IItemSelection& selection) override;
	void onSelectionChanged () override;

protected:
	PlugInManagementComponent& component;

	static bool getModulePath (Url& path, const ObjectPreset& preset);
};

//************************************************************************************************
// PriorityListModel
//************************************************************************************************

class PriorityListModel: public ListViewModel,
						 public IItemDragVerifier
{
public:
	DECLARE_CLASS_ABSTRACT (PriorityListModel, ListViewModel)

	PriorityListModel (PlugInManagementComponent& component);
	
	PROPERTY_VARIABLE (int, minDragIndex, MinDragIndex)

	DECLARE_STRINGID_MEMBER (kPriorityColumn)
	DECLARE_STRINGID_MEMBER (kMoveIndicatorColumn)

	int index (StringRef title) const;

	// ListViewModel
	IUnknown* CCL_API createDragSessionData (ItemIndexRef index) override;
	tbool CCL_API canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView = nullptr) override;
	tbool CCL_API insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session) override;
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override;

	// IItemDragVerifier
	tbool CCL_API verifyTargetItem (ItemIndex& item, int& relation) override;

	CLASS_INTERFACE (IItemDragVerifier, ListViewModel)
};

//************************************************************************************************
// PlugInListItem
//************************************************************************************************

class PlugInListItem: public ListViewItem
{
public:
	PlugInListItem ()
	: lastModified (0),
	  lastUsed (0),
	  loadDuration (0),
	  saveDuration (0),
	  saveSize (0)
	{}

	PROPERTY_OBJECT (UID, cid, ClassID)

	PROPERTY_VARIABLE (int64, lastModified, LastModified)
	PROPERTY_VARIABLE (int64, lastUsed, LastUsed)
	PROPERTY_VARIABLE (double, loadDuration, LoadDuration)
	PROPERTY_VARIABLE (double, saveDuration, SaveDuration)
	PROPERTY_VARIABLE (double, saveSize, SaveSize)

	DECLARE_STRINGID_MEMBER (kTypeID)
	DECLARE_STRINGID_MEMBER (kVendorID)
	DECLARE_STRINGID_MEMBER (kFolderID)
	DECLARE_STRINGID_MEMBER (kVersionID)
	DECLARE_STRINGID_MEMBER (kLastModifiedID)
	DECLARE_STRINGID_MEMBER (kLastUsedID)
	DECLARE_STRINGID_MEMBER (kLoadDurationID)
	DECLARE_STRINGID_MEMBER (kSaveDurationID)
	DECLARE_STRINGID_MEMBER (kSaveSizeID)
	DECLARE_STRINGID_MEMBER (kBlocklistURL)
};

//************************************************************************************************
// PlugInListSorter
//************************************************************************************************

namespace PlugInListSorter
{
	DEFINE_ARRAY_COMPARE (sortByName, ListViewItem, item1, item2)
		return item1->getTitle ().compareWithOptions (item2->getTitle (), Text::kIgnoreCase|Text::kIgnoreDiacritic);
	}

	DEFINE_ARRAY_COMPARE (sortByType, ListViewItem, item1, item2)
		String type1 = item1->getDetails ().getString (PlugInListItem::kTypeID);
		String type2 = item2->getDetails ().getString (PlugInListItem::kTypeID);
		return type1.compareWithOptions (type2, Text::kIgnoreCase|Text::kIgnoreDiacritic);
	}

	DEFINE_ARRAY_COMPARE (sortByVendor, ListViewItem, item1, item2)
		String vendor1 = item1->getDetails ().getString (PlugInListItem::kVendorID);
		String vendor2 = item2->getDetails ().getString (PlugInListItem::kVendorID);
		return vendor1.compareWithOptions (vendor2, Text::kIgnoreCase|Text::kIgnoreDiacritic);
	}

	DEFINE_ARRAY_COMPARE (sortByFolder, ListViewItem, item1, item2)
		String folder1 = item1->getDetails ().getString (PlugInListItem::kFolderID);
		String folder2 = item2->getDetails ().getString (PlugInListItem::kFolderID);
		return folder1.compareWithOptions (folder2, Text::kIgnoreCase|Text::kIgnoreDiacritic);
	}

	DEFINE_ARRAY_COMPARE (sortByVersion, ListViewItem, item1, item2)
		VersionNumber v1;
		VersionNumber v2;
		v1.scan (item1->getDetails ().getString (PlugInListItem::kVersionID));
		v2.scan (item2->getDetails ().getString (PlugInListItem::kVersionID));
		return v1.compare (v2);
	}
	
	DEFINE_ARRAY_COMPARE (sortByLastModified, PlugInListItem, item1, item2)
		return ccl_compare (item1->getLastModified (), item2->getLastModified ());
	}

	DEFINE_ARRAY_COMPARE (sortByLastUsed, PlugInListItem, item1, item2)
		return ccl_compare (item1->getLastUsed (), item2->getLastUsed ());
	}

	DEFINE_ARRAY_COMPARE (sortByLoadDuration, PlugInListItem, item1, item2)
		return ccl_compare (item1->getLoadDuration (), item2->getLoadDuration ());
	}

	DEFINE_ARRAY_COMPARE (sortBySaveDuration, PlugInListItem, item1, item2)
		return ccl_compare (item1->getSaveDuration (), item2->getSaveDuration ());
	}

	DEFINE_ARRAY_COMPARE (sortBySaveSize, PlugInListItem, item1, item2)
		return ccl_compare (item1->getSaveSize (), item2->getSaveSize ());
	}
}

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("PlugInManagement")
	XSTRING (PlugInManager, "Plug-In Manager")
	XSTRING (Name, "Name")
	XSTRING (Type, "Type")
	XSTRING (Vendor, "Vendor")
	XSTRING (Folder, "Folder")
	XSTRING (Version, "Version")
	XSTRING (LastUsed, "Last Used")
	XSTRING (LastModified, "Last Modified")
	XSTRING (LoadDuration, "Avg. Load Time")
	XSTRING (SaveDuration, "Avg. Save Time")
	XSTRING (SaveSize, "Avg. Preset Size")
	XSTRING (RemoveSettings, "Remove Plug-in Settings")
	XSTRING (DuplicatesHidden, "Plug-in duplicates were hidden")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum PlugInManagementTags
	{
		kSearchTerms = 100,
		kDeselectAllFilter,
		kResetFilter,
		kShowAll,
		kHideAll,
		kRemoveFromBlocklist,
		kHideDuplicates,
		kAutoHideDuplicates,
		kLinkedOptionFirst,
		kLinkedOptionLast = kLinkedOptionFirst + 100,
		kMissingInformation
	};
}

//************************************************************************************************
// PlugInListItem
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (PlugInListItem, kTypeID, "type")
DEFINE_STRINGID_MEMBER_ (PlugInListItem, kVendorID, "vendor")
DEFINE_STRINGID_MEMBER_ (PlugInListItem, kFolderID, "folder")
DEFINE_STRINGID_MEMBER_ (PlugInListItem, kVersionID, "version")
DEFINE_STRINGID_MEMBER_ (PlugInListItem, kLastModifiedID, "lastModified")
DEFINE_STRINGID_MEMBER_ (PlugInListItem, kLastUsedID, "lastUsed")
DEFINE_STRINGID_MEMBER_ (PlugInListItem, kLoadDurationID, "loadDuration")
DEFINE_STRINGID_MEMBER_ (PlugInListItem, kSaveDurationID, "saveDuration")
DEFINE_STRINGID_MEMBER_ (PlugInListItem, kSaveSizeID, "saveSize")
DEFINE_STRINGID_MEMBER_ (PlugInListItem, kBlocklistURL, "blocklistURL")

//************************************************************************************************
// PlugInManagementComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PlugInManagementComponent, Component)
DEFINE_COMPONENT_SINGLETON (PlugInManagementComponent)
static const CString kPluginsChanged ("pluginsChanged");
static const CString kApplyVisibility ("applyVisibility");
static const CString kHiddenTypes ("hiddenTypes");
static const CString kHiddenVendors ("hiddenVendors");
static const CString kTypes ("types");
static const CString kAutoHideDuplicates ("autoHideDuplicates");
static const CString kShowPlugInManagerAction ("showPlugInManager");

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInManagementComponent::PlugInManagementComponent ()
: Component ("PlugInManagement"),
  pluginSignalSink (Signals::kPlugIns),
  typeList (nullptr),
  typePriorityList (nullptr),
  vendorList (nullptr),
  pluginList (nullptr),
  diagnosticList (nullptr),
  blockList (nullptr)
{
	pluginSignalSink.setObserver (this);

	typeList = NEW ListViewModel;
	typeList->getColumns ().addColumn (20, nullptr, ListViewModel::kCheckBoxID);
	typeList->getColumns ().addColumn (200, XSTR (Name), ListViewModel::kTitleID);
	typeList->addObserver (this);
	addObject ("typeList", typeList);
	
	typePriorityList = NEW PriorityListModel (*this);
	typePriorityList->addObserver (this);
	addObject ("priorityList", typePriorityList);

	vendorList = NEW ListViewModel;
	vendorList->getColumns ().addColumn (20, nullptr, ListViewModel::kCheckBoxID);
	vendorList->getColumns ().addColumn (200, XSTR (Name), ListViewModel::kTitleID);
	vendorList->addObserver (this);
	addObject ("vendorList", vendorList);

	pluginList = NEW PlugInListViewModel (*this);
	pluginList->getColumns ().addColumn (20, nullptr, ListViewModel::kCheckBoxID);
	pluginList->getColumns ().addColumn (20, nullptr, ListViewModel::kIconID);
	pluginList->getColumns ().addColumn (200, XSTR (Name), ListViewModel::kTitleID, 50, IColumnHeaderList::kSortable | IColumnHeaderList::kSizable | IColumnHeaderList::kCanFit);
	pluginList->getColumns ().addColumn (100, XSTR (Type), PlugInListItem::kTypeID, 50, IColumnHeaderList::kSortable | IColumnHeaderList::kSizable | IColumnHeaderList::kCanFit);
	pluginList->getColumns ().addColumn (100, XSTR (Vendor), PlugInListItem::kVendorID, 50, IColumnHeaderList::kSortable | IColumnHeaderList::kSizable | IColumnHeaderList::kCanFit);
	pluginList->getColumns ().addColumn (100, XSTR (Folder), PlugInListItem::kFolderID, 50, IColumnHeaderList::kSortable | IColumnHeaderList::kSizable | IColumnHeaderList::kCanFit);
	pluginList->getColumns ().addColumn (100, XSTR (Version), PlugInListItem::kVersionID, 50, IColumnHeaderList::kSortable | IColumnHeaderList::kSizable | IColumnHeaderList::kCanFit);
	pluginList->getColumns ().addColumn (100, XSTR (LastModified), PlugInListItem::kLastModifiedID, 50, IColumnHeaderList::kSortable | IColumnHeaderList::kSizable | IColumnHeaderList::kCanFit);
	pluginList->addObserver (this);
	addObject ("pluginList", pluginList);

	diagnosticList = NEW PlugInListViewModel (*this);
	diagnosticList->getColumns ().addColumn (20, nullptr, ListViewModel::kCheckBoxID);
	diagnosticList->getColumns ().addColumn (20, nullptr, ListViewModel::kIconID);
	diagnosticList->getColumns ().addColumn (200, XSTR (Name), ListViewModel::kTitleID, 50, IColumnHeaderList::kSortable | IColumnHeaderList::kSizable | IColumnHeaderList::kCanFit);
	diagnosticList->getColumns ().addColumn (100, XSTR (LastUsed), PlugInListItem::kLastUsedID, 50, IColumnHeaderList::kSortable | IColumnHeaderList::kSizable | IColumnHeaderList::kCanFit);
	diagnosticList->getColumns ().addColumn (100, XSTR (LoadDuration), PlugInListItem::kLoadDurationID, 50, IColumnHeaderList::kSortable | IColumnHeaderList::kSizable | IColumnHeaderList::kCanFit);
	diagnosticList->getColumns ().addColumn (100, XSTR (SaveDuration), PlugInListItem::kSaveDurationID, 50, IColumnHeaderList::kSortable | IColumnHeaderList::kSizable | IColumnHeaderList::kCanFit);
	diagnosticList->getColumns ().addColumn (100, XSTR (SaveSize), PlugInListItem::kSaveSizeID, 50, IColumnHeaderList::kSortable | IColumnHeaderList::kSizable | IColumnHeaderList::kCanFit);
	diagnosticList->addObserver (this);
	addObject ("diagnosticList", diagnosticList);

	ListViewSorter* nameSorter = NEW ListViewSorter (ListViewModel::kTitleID, XSTR (Name), &PlugInListSorter::sortByName);
	pluginList->addSorter (nameSorter);
	pluginList->sortBy (nameSorter);
	pluginList->addSorter (NEW ListViewSorter (PlugInListItem::kTypeID, XSTR (Type), &PlugInListSorter::sortByType));
	pluginList->addSorter (NEW ListViewSorter (PlugInListItem::kVendorID, XSTR (Vendor), &PlugInListSorter::sortByVendor));
	pluginList->addSorter (NEW ListViewSorter (PlugInListItem::kFolderID, XSTR (Folder), &PlugInListSorter::sortByFolder));
	pluginList->addSorter (NEW ListViewSorter (PlugInListItem::kVersionID, XSTR (Version), &PlugInListSorter::sortByVersion));
	pluginList->addSorter (NEW ListViewSorter (PlugInListItem::kLastModifiedID, XSTR (LastModified), &PlugInListSorter::sortByLastModified));
	
	nameSorter = NEW ListViewSorter (ListViewModel::kTitleID, XSTR (Name), &PlugInListSorter::sortByName);
	diagnosticList->addSorter (nameSorter);
	diagnosticList->sortBy (nameSorter);
	diagnosticList->addSorter (NEW ListViewSorter (PlugInListItem::kLastUsedID, XSTR (LastUsed), &PlugInListSorter::sortByLastUsed));
	diagnosticList->addSorter (NEW ListViewSorter (PlugInListItem::kSaveDurationID, XSTR (SaveDuration), &PlugInListSorter::sortBySaveDuration));
	diagnosticList->addSorter (NEW ListViewSorter (PlugInListItem::kLoadDurationID, XSTR (LoadDuration), &PlugInListSorter::sortByLoadDuration));
	diagnosticList->addSorter (NEW ListViewSorter (PlugInListItem::kSaveSizeID, XSTR (SaveSize), &PlugInListSorter::sortBySaveSize));

	blockList = NEW BlockListViewModel (*this);
	addObject ("blockList", blockList);

	paramList.addString ("searchTerms", Tag::kSearchTerms);	
	paramList.addParam ("deselectAllFilter", Tag::kDeselectAllFilter);
	paramList.addParam ("resetFilter", Tag::kResetFilter);
	paramList.addParam ("showAll", Tag::kShowAll);
	paramList.addParam ("hideAll", Tag::kHideAll);
	paramList.addParam ("removeFromBlocklist", Tag::kRemoveFromBlocklist);
	paramList.addParam ("missingInformation", Tag::kMissingInformation);
	paramList.addParam ("hideDuplicates", Tag::kHideDuplicates);
	IParameter* autoHideParam = paramList.addParam ("autoHideDuplicates", Tag::kAutoHideDuplicates);
	autoHideParam->setDefaultValue (true);
	autoHideParam->setValue (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInManagementComponent::~PlugInManagementComponent ()
{
	ASSERT (linkedOptions.isEmpty ())
	cancelSignals ();
	typeList->removeObserver (this);
	typeList->release ();
	typePriorityList->removeObserver (this);
	typePriorityList->release ();
	vendorList->removeObserver (this);
	vendorList->release ();
	diagnosticList->removeObserver (this);
	diagnosticList->release ();
	pluginList->removeObserver (this);
	pluginList->release ();
	blockList->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManagementComponent::addCategory (StringRef category)
{
	categories.add (category);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManagementComponent::addType (StringRef type, int priority, bool fixed)
{
	ASSERT (!types.contains (type))
	types.addSorted ({ type, priority, false, fixed });
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManagementComponent::addLinkedOption (IUserOption* userOption)
{
	MutableCString paramName ("linkedOption:");
	paramName += userOption->getName ();
	paramList.addParam (paramName, Tag::kLinkedOptionFirst + linkedOptions.count ());
	linkedOptions.add (userOption);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManagementComponent::addVersionProvider (IPlugInVersionProvider* provider)
{
	versionProviders.add (provider);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManagementComponent::addManagementExtension (IPlugInManagementExtension* extension)
{
	managementExtensions.add (extension);
	extension->addPlugInListColumns (pluginList->getColumns ());	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManagementComponent::onViewVisible (bool state)
{
	pluginSignalSink.enable (state);
	Attributes& settings = Settings::instance ().getAttributes (getName ());

	if(state)
	{
		restorePrioritySettings (settings);
		updateFilters ();
		restoreFilters (settings);
		updateResultList ();
		updateBlockList ();
	}
	else
	{
		settings.removeAll ();
		storeFilters (settings);
		storePrioritySettings (settings);

		typeList->removeAll ();
		typePriorityList->removeAll ();
		vendorList->removeAll ();
		pluginList->removeAll ();
		blockList->removeAll ();

		searchDescription.release ();
		paramList.byTag (Tag::kSearchTerms)->fromString (String::kEmpty);

		setInformationMissing (false);
	}

	updateEnabledStates ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManagementComponent::onListSelectionChanged (ListViewModel* listModel)
{
	updateEnabledStates ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInManagementComponent::initialize (IUnknown* context)
{
	System::GetNotificationCenter ().registerHandler (this);
	
	Attributes& settings = Settings::instance ().getAttributes (getName ());
	restorePrioritySettings (settings);

	if(paramList.byTag (Tag::kAutoHideDuplicates)->getValue ().asBool ())
		autoHideDuplicates ();

	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInManagementComponent::terminate ()
{
	System::GetNotificationCenter ().unregisterHandler (this);

	for(auto provider : versionProviders)
		provider->release ();
	versionProviders.removeAll ();

	for(auto extension : managementExtensions)
		extension->release ();
	managementExtensions.removeAll ();

	linkedOptions.removeAll ();
	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInManagementComponent::notify (ISubject* subject, MessageRef msg)
{
	if(msg == ListViewModel::kItemChecked)
	{
		if(subject == typeList || subject == vendorList)
			updateResultList ();
		else if(subject == pluginList || subject == diagnosticList)
			(NEW Message (kApplyVisibility))->post (this, -1);
	}
	else if(msg == Signals::kResetBlocklistDone)
	{
		updateBlockList ();
	}
	else if(msg == Signals::kClassCategoryChanged)
	{
		String category (msg[0].asString ());
		if(categories.contains (category))
			(NEW Message (kPluginsChanged))->post (this, -1);
	}
	else if(msg == Signals::kPluginPresentationChanged)
	{
		IUnknown* sender = msg.getArgCount () > 0 ? msg[0].asUnknown () : nullptr;
		if(sender != this->asUnknown ())
			updatePresentationDetails ();
	}
	else if(msg == kPluginsChanged)
	{
		updateFilters ();
		updateResultList ();
		updateBlockList ();
	}
	else if(msg == kApplyVisibility)
	{
		applyVisibility ();
	}
	else if(msg == kChanged && isEqualUnknown (subject, ccl_as_unknown (typePriorityList)))
	{
		updatePriorities ();
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInManagementComponent::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kSearchTerms :
		{
			String searchTerms;
			param->toString (searchTerms);
			searchTerms.trimWhitespace ();
			if(searchTerms.isEmpty ())
				searchDescription.release ();
			else
				searchDescription = SearchDescription::create (Url (), searchTerms);
			updateResultList ();
		}
		break;

	case Tag::kDeselectAllFilter :
	case Tag::kResetFilter :
		resetFilters (param->getTag () == Tag::kDeselectAllFilter);
		break;

	case Tag::kShowAll :
	case Tag::kHideAll :
		pluginList->checkAll (param->getTag () == Tag::kShowAll);
		diagnosticList->signal (Message (kChanged));
		break;

	case Tag::kRemoveFromBlocklist :
		blockList->removeSelectedItems ();
		break;

	case Tag::kHideDuplicates :
		hideDuplicates (*pluginList);
		break;

	default :
		if(param->getTag () >= Tag::kLinkedOptionFirst && param->getTag () < Tag::kLinkedOptionFirst + linkedOptions.count ())
		{
			int index = param->getTag () - Tag::kLinkedOptionFirst;
			IUserOption* option = linkedOptions.at (index);
			ASSERT (option != nullptr)
			UserOptionManager::instance ().runDialog (nullptr, option);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManagementComponent::updateEnabledStates ()
{
	paramList.byTag (Tag::kHideAll)->enable (!pluginList->isEmpty ());
	paramList.byTag (Tag::kShowAll)->enable (!pluginList->isEmpty ());
	paramList.byTag (Tag::kHideDuplicates)->enable (!pluginList->isEmpty ());

	paramList.byTag (Tag::kRemoveFromBlocklist)->enable (blockList->canRemoveSelectedItems ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManagementComponent::updateFilters ()
{
	Vector<String> vendorNames;

	for(const auto& category : categories)
	{
		ForEachPlugInClass (category, description)
			String typeName = PlugInSortMethods::getType (description);
			String vendor = PlugInSortMethods::getVendor (description);

			if(!typeName.isEmpty ())
			{
				PlugInType* existingType = types.findIf ([&] (const PlugInType& type) { return type.name == typeName; });
				if(existingType)
					existingType->found = true;
				else
					types.add ({ typeName, types.count (), true });
			}
			if(!vendor.isEmpty ())
				vendorNames.addOnce (vendor);
		EndFor
	}

	typeList->removeAll ();
	typePriorityList->removeAll ();
	int i = 0;
	int lastFixedItem = 0;
	for(int i = 0; i < types.count (); i++)
	{
		if(!types[i].found)
			continue;

		if(types[i].fixed)
			lastFixedItem = i;

		ListViewItem* item = NEW ListViewItem (types[i].name);
		item->setChecked (true);
		typeList->addSorted (item);

		ListViewItem* priorityItem = NEW ListViewItem (types[i].name);
		typePriorityList->addItem (priorityItem);
	}
	typePriorityList->setMinDragIndex (lastFixedItem + 1);

	vendorList->removeAll ();
	for(const auto& vendorName : vendorNames)
	{
		ListViewItem* item = NEW ListViewItem (vendorName);
		item->setChecked (true);
		vendorList->addSorted (item);
	}

	typeList->signal (Message (kChanged));
	typePriorityList->signal (Message (kChanged));
	vendorList->signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManagementComponent::resetFilters (bool deselectAll)
{
	searchDescription.release ();
	paramList.byTag (Tag::kSearchTerms)->fromString (String::kEmpty);

	ForEach (*typeList, ListViewItem, item)
		item->setChecked (!deselectAll);
	EndFor
	typeList->invalidate ();

	ForEach (*vendorList, ListViewItem, item)
		item->setChecked (!deselectAll);
	EndFor
	vendorList->invalidate ();

	updateResultList ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManagementComponent::storeFilters (Attributes& a) const
{
	// Note: We ignore the fact that some strings might be translated.
	ForEach (*typeList, ListViewItem, item)
		if(!item->isChecked ())
			a.queue (kHiddenTypes, NEW Boxed::String (item->getTitle ()), Attributes::kOwns);
	EndFor

	ForEach (*vendorList, ListViewItem, item)
		if(!item->isChecked ())
			a.queue (kHiddenVendors, NEW Boxed::String (item->getTitle ()), Attributes::kOwns);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManagementComponent::restoreFilters (const Attributes& a)
{
	auto isHidden = [&] (StringID queueId, StringRef string)
	{
		IterForEach (a.newQueueIterator (queueId, ccl_typeid<Boxed::String> ()), Boxed::String, savedString)
			if(savedString && *savedString == string)
				return true;
		EndFor
		return false;
	};

	ForEach (*typeList, ListViewItem, item)
		item->setChecked (!isHidden (kHiddenTypes, item->getTitle ()));
	EndFor

	ForEach (*vendorList, ListViewItem, item)
		item->setChecked (!isHidden (kHiddenVendors, item->getTitle ()));
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManagementComponent::storePrioritySettings (Attributes& a) const
{
	for(const PlugInType& type : types)
		a.queue (kTypes, NEW Boxed::String (type.name), Attributes::kOwns);

	a.set (kAutoHideDuplicates, paramList.byTag (Tag::kAutoHideDuplicates)->getValue ().asBool ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManagementComponent::restorePrioritySettings (const Attributes& a)
{
	int i = 0;
	IterForEach (a.newQueueIterator (kTypes, ccl_typeid<Boxed::String> ()), Boxed::String, savedType)
		if(savedType == nullptr)
			continue;

		PlugInType* type = types.findIf ([&] (const PlugInType& type) { return type.name == *savedType; });
		if(type)
		{
			if(!type->fixed)
				type->priority = i;
		}
		else
			types.add ({ *savedType, i });
		i++;
	EndFor
	types.sort ();
	
	if(a.contains (kAutoHideDuplicates))
		paramList.byTag (Tag::kAutoHideDuplicates)->setValue (a.getBool (kAutoHideDuplicates), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManagementComponent::updatePriorities ()
{
	for(PlugInType& type : types)
	{
		int priority = typePriorityList->index (type.name);
		if(priority >= 0 && !type.fixed)
			type.priority = priority;
	}
	types.sort ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInManagementComponent::matchesType (StringRef type) const
{
	ForEach (*typeList, ListViewItem, item)
		if(item->getTitle () == type)
			return item->isChecked ();
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInManagementComponent::matchesVendor (StringRef vendor) const
{
	ForEach (*vendorList, ListViewItem, item)
		if(item->getTitle () == vendor)
			return item->isChecked ();
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInManagementComponent::matchesName (StringRef name) const
{
	if(searchDescription)
		return searchDescription->matchesName (name) != 0;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManagementComponent::setInformationMissing (bool state)
{
	paramList.getParameterByTag (Tag::kMissingInformation)->setValue (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManagementComponent::updateResultList ()
{
	WaitCursor wc (System::GetGUI ());

	pluginList->removeAll ();
	diagnosticList->removeAll ();
	for(const auto& category : categories)
	{
		ForEachPlugInClass (category, description)
			String type = PlugInSortMethods::getType (description);
			if(!matchesType (type))
				continue;

			String vendor = PlugInSortMethods::getVendor (description);
			if(!matchesVendor (vendor))
				continue;

			String name = description.getName ();
			if(!matchesName (name))
				continue;

			PlugInListItem* item = NEW PlugInListItem;
			item->setTitle (name);
			item->setClassID (description.getClassID ());
			item->setIcon (PlugInClass (description).getIcon ());
			item->getDetails ().set (PlugInListItem::kTypeID, type);
			item->getDetails ().set (PlugInListItem::kVendorID, vendor);

			if(pluginList->isVisible ())
			{
				String version;
				if(!getPlugInVersion (version, description))
					setInformationMissing ();

				item->getDetails ().set (PlugInListItem::kVersionID, version);

				for(const IPlugInVersionProvider* provider : versionProviders)
				{
					FileTime lastModified; // local time
					if(provider->getLastModifiedTime (lastModified, description) == kResultOk)
					{
						item->setLastModified (UnixTime::fromLocal (lastModified)); // store as unix timestamp (UTC)
						item->getDetails ().set (PlugInListItem::kLastModifiedID, Format::TimeAgo::print (lastModified));
						break;
					}
				}

				for(const IPlugInManagementExtension* extension : managementExtensions)
					extension->setPlugInListColumnData (*item, description);
			}

			if(diagnosticList->isVisible ())
			{
				int64 unixTime = System::GetPluginPresentation ().getLastUsage (description.getClassID ());
				if(unixTime != 0)
				{
					item->setLastUsed (unixTime);
					DateTime lastUsed = UnixTime::toLocal (unixTime);
					item->getDetails ().set (PlugInListItem::kLastUsedID, Format::TimeAgo::print (lastUsed));
				}

				MutableCString context (DiagnosticID::kClassIDPrefix);
				context.append (UIDCString (description.getClassID ()));
				Vector<CString> keys { DiagnosticID::kLoadDuration, DiagnosticID::kSaveDuration,  DiagnosticID::kSaveSize };
				AutoPtr<IDiagnosticResultSet> statistics = System::GetDiagnosticStore ().queryMultipleResults (context, keys, keys.count ());
				if(statistics)
				{
					if(IDiagnosticResult* loadDuration = statistics->at (0))
					{
						item->setLoadDuration (loadDuration->getAverage ());
						String durationString = DiagnosticPresentation::printDuration (loadDuration->getAverage ());
						item->getDetails ().set (PlugInListItem::kLoadDurationID, durationString);
					}

					if(IDiagnosticResult* saveDuration = statistics->at (1))
					{
						item->setSaveDuration (saveDuration->getAverage ());
						String durationString = DiagnosticPresentation::printDuration (saveDuration->getAverage ());
						item->getDetails ().set (PlugInListItem::kSaveDurationID, durationString);
					}
					
					if(IDiagnosticResult* saveSize = statistics->at (2))
					{
						item->setSaveSize (saveSize->getAverage ());
						String sizeString = DiagnosticPresentation::printSize (saveSize->getAverage ());
						item->getDetails ().set (PlugInListItem::kSaveSizeID, sizeString);
					}
				}
			}

			pluginList->addSorted (item);
			diagnosticList->addSorted (return_shared (item));
		EndFor
	}

	updatePresentationDetails ();
	updateEnabledStates ();

	pluginList->signal (Message (kChanged));
	diagnosticList->signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManagementComponent::updatePresentationDetails ()
{
	auto& plugPresentation = System::GetPluginPresentation ();

	ForEach (*pluginList, PlugInListItem, item)
		UIDRef cid = item->getClassID ();
		String folder = plugPresentation.getSortPath (cid);
		bool hidden = plugPresentation.isHidden (cid);

		item->setChecked (!hidden);
		item->getDetails ().set (PlugInListItem::kFolderID, folder);

		if(managementExtensions.isEmpty () == false)
		{
			if(auto description = System::GetPlugInManager ().getClassDescription (cid))
				for(const IPlugInManagementExtension* extension : managementExtensions)
					extension->setPlugInListColumnData (*item, *description);
		}
	EndFor

	pluginList->invalidate ();
	diagnosticList->invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManagementComponent::applyVisibility ()
{
	auto& plugPresentation = System::GetPluginPresentation ();

	bool visibilityChanged = false;
	ForEach (*pluginList, PlugInListItem, item)
		UIDRef cid = item->getClassID ();
		tbool hidden = !item->isChecked ();
		if(plugPresentation.isHidden (cid) != hidden)
		{
			plugPresentation.setHidden (cid, hidden);
			visibilityChanged = true;
		}		
	EndFor

	if(visibilityChanged)
	{
		plugPresentation.saveSettings ();
		SignalSource (Signals::kPlugIns).signal (Message (Signals::kPluginPresentationChanged, asUnknown ()));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
void PlugInManagementComponent::hideDuplicates (T& list, bool onlyUnused)
{
	auto& plugPresentation = System::GetPluginPresentation ();

	bool visibilityChanged = false;

	auto getPriority = [&] (UIDRef cid) 
	{
		const IClassDescription* description = System::GetPlugInManager ().getClassDescription (cid);
		if(description == nullptr)
			return 0;
		return -types.index (PlugInSortMethods::getType (*description));
	};
	
	auto getVersion = [&] (UIDRef cid) 
	{
		VersionNumber version;
		const IClassDescription* description = System::GetPlugInManager ().getClassDescription (cid);
		if(description)
		{
			String versionString;
			getPlugInVersion (versionString, *description);
			version.scan (versionString);
		}
		return version;
	};

	Vector<UID> hidden;
	ForEach (list, PlugInListItem, item)
		UIDRef cid = item->getClassID ();
		if(plugPresentation.isHidden (cid))
			continue;

		Vector<UID> duplicates;
		PlugIn::findDuplicates (duplicates, cid);

		int priority = getPriority (cid);
		VersionNumber version = getVersion (cid);

		for(UIDRef duplicate : duplicates)
		{
			if(plugPresentation.isHidden (duplicate))
				continue;
		
			int duplicatePriority = getPriority (duplicate);
			VersionNumber duplicateVersion = getVersion (duplicate);

			if(duplicatePriority > priority || (duplicatePriority == priority && duplicateVersion > version))
			{
				if(onlyUnused && System::GetPluginPresentation ().getLastUsage (cid) != 0)
					continue;

				plugPresentation.setHidden (cid, true);
				visibilityChanged = true;
				hidden.addOnce (cid);
			}
			else
			{
				if(onlyUnused && System::GetPluginPresentation ().getLastUsage (duplicate) != 0)
					continue;

				plugPresentation.setHidden (duplicate, true);
				visibilityChanged = true;
				hidden.addOnce (duplicate);
			}
		}
	EndFor

	if(visibilityChanged)
	{
		plugPresentation.saveSettings ();
		SignalSource (Signals::kPlugIns).signal (Message (Signals::kPluginPresentationChanged, asUnknown ()));
		updatePresentationDetails ();
	}

	if(!hidden.isEmpty ())
	{
		bool useAlert = pluginList->isVisible () || diagnosticList->isVisible ();

		String message;
		if(useAlert)
		{
			message = XSTR (DuplicatesHidden);
			message.append (":\n");
		}
		for(UIDRef cid : hidden)
		{
			const IClassDescription* description = System::GetPlugInManager ().getClassDescription (cid);
			if(description == nullptr)
				continue;
			message.appendFormat ("\t%(1)\n", description->getName ());
		}
		if(useAlert)
			Alert::info (message);
		else
		{
			NotificationActionProperties actionProperties {kShowPlugInManagerAction, XSTR (PlugInManager)};
			System::GetNotificationCenter ().sendInAppNotification (XSTR (DuplicatesHidden), message, nullptr, &actionProperties, 1);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManagementComponent::autoHideDuplicates ()
{
	ObjectArray plugIns;
	plugIns.objectCleanup ();
	for(const auto& category : categories)
	{
		ForEachPlugInClass (category, description)
			PlugInListItem* item = NEW PlugInListItem;
			item->setClassID (description.getClassID ());
			plugIns.add (item);
		EndFor
	}
	hideDuplicates (plugIns, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManagementComponent::updateBlockList ()
{
	UnknownList blocklistContent;
	System::GetPlugInManager ().getBlocklistContent (blocklistContent);

	blockList->removeAll ();
	ForEachUnknown (blocklistContent, unk)
		if(UnknownPtr<IUrl> path = unk)
		{
			String name;
			path->getName (name, false);
			ListViewItem* item = NEW ListViewItem (name);
			item->getDetails ().set (PlugInListItem::kBlocklistURL, path, Attributes::kShare);

			blockList->addSorted (item);
		}
	EndFor

	blockList->signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInManagementComponent::getPlugInVersion (String& result, const IClassDescription& description) const
{
	Variant version;
	bool versionValid = false;
	if(description.getClassAttribute (version, Meta::kClassVersion))
		versionValid = true;
	if(version.asString ().isEmpty ())
	{
		for(const IPlugInVersionProvider* provider : versionProviders)
		{
			String versionString;
			if(provider->getVersionString (versionString, description) == kResultOk)
			{
				versionValid = true;
				version.fromString (versionString);
				if(!version.asString ().isEmpty ())
					break;
			}
		}
	}
	if(version.asString ().isEmpty ())
	{
		version.fromString (description.getModuleVersion ().getVersion ());
		if(!version.asString ().isEmpty ())
			versionValid = true;
	}
	result = version.asString ();
	return versionValid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API PlugInManagementComponent::countDiagnosticData () const
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInManagementComponent::getDiagnosticDescription (DiagnosticDescription& description, int index) const
{
	if(index == 0)
	{
		description.categoryFlags = DiagnosticDescription::kPlugInInformation;
		description.fileName = getName ();
		description.fileType = FileTypes::Csv ();
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API PlugInManagementComponent::createDiagnosticData (int index)
{
	if(index == 0)
	{
		AutoPtr<IStream> stream = NEW MemoryStream;
		AutoPtr<ITextStreamer> streamer = System::CreateTextStreamer (*stream, {Text::kUTF8, Text::kSystemLineFormat});
		if(streamer == nullptr)
			return nullptr;
		
		String header;
		header.appendFormat ("%(1),%(2),%(3),%(4),", XSTR (Vendor), XSTR (Name), XSTR (Type), XSTR (Version));
		header.append (XSTR (LastModified));
		streamer->writeLine (header);

		struct Item
		{
			String type;
			String vendor;
			String name;
			String version;
			String modified;

			bool operator > (const Item& other) const
			{
				int result = vendor.compareWithOptions (other.vendor, Text::kIgnoreCase|Text::kIgnoreDiacritic);
				if(result == 0)
					result = type.compareWithOptions (other.type, Text::kIgnoreCase|Text::kIgnoreDiacritic);
				if(result == 0)
					result = name.compareWithOptions (other.name, Text::kIgnoreCase|Text::kIgnoreDiacritic);
				if(result == 0)
					result = version.compare (other.version);
				if(result == 0)
					result = modified.compare (other.modified);
				return result > 0;
			}
		};
		Vector<Item> items;
		
		for(const auto& category : categories)
		{
			ForEachPlugInClass (category, description)
				String type = PlugInSortMethods::getType (description);
				String vendor = PlugInSortMethods::getVendor (description);
				String name = description.getName ();

				String version;
				getPlugInVersion (version, description);
				
				FileTime lastModified;
				for(const IPlugInVersionProvider* provider : versionProviders)
				{
					if(provider->getLastModifiedTime (lastModified, description) == kResultOk)
						break;
				}
				
				vendor.replace (",", ".");
				name.replace (",", ".");

				items.addSorted ({ type, vendor, name, version, Format::TimeAgo::print (lastModified) });
			EndFor
		}

		for(const Item& item : items)
		{
			String line;
			line.appendFormat ("%(1),%(2),%(3),%(4),", item.vendor, item.name, item.type, item.version);
			line.append (item.modified);
			streamer->writeLine (line);
		}

		return stream.detach ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInManagementComponent::canExecute (StringID actionId, const INotification& n) const
{
	return actionId == kShowPlugInManagerAction && n.getCategory () == INotificationCenter::kInAppNotificationCategory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInManagementComponent::execute (StringID actionId, INotification& n)
{
	bool succeeded = false;
	if(actionId == kShowPlugInManagerAction)
		succeeded = System::GetCommandTable ().performCommand (CommandMsg ("View", "Plug-In Manager"), true);
	return succeeded ? kResultOk : kResultFailed;
}

//************************************************************************************************
// PlugInListViewModel
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PlugInListViewModel, ListViewModel)

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInListViewModel::PlugInListViewModel (PlugInManagementComponent& component)
: component (component),
  visible (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInListViewModel::onVisibleChanged (bool state)
{
	setVisible (state);
	component.onViewVisible (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInListViewModel::onSelectionChanged ()
{
	component.onListSelectionChanged (this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInListViewModel::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	ListViewItem* item = resolve (index);
	CString columnID;
	ColumnType columnType = getColumnType (columnID, column);
	if(item && columnType == kCheckBoxColumn)
	{
		if(IImage* icon = info.view->getVisualStyle ().getImage ("VisibleIcon"))
		{
			drawButtonImage (info, icon, item->isChecked ());
			return true;
		}
	}

	return SuperClass::drawCell (index, column, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInListViewModel::editCell (ItemIndexRef index, int column, const EditInfo& info)
{
	CCL_PRINTF ("PlugInListViewModel edit cell index = %d column = %d\n", index.getIndex (), column)

	CString columnID;
	if(getColumnType (columnID, column) == kDetailColumn)
		if(PlugInListItem* item = static_cast<PlugInListItem*> (resolve (index)))
		{
			if(component.managementExtensions.isEmpty () == false)
				if(auto description = System::GetPlugInManager ().getClassDescription (item->getClassID ()))
				{
					for(const IPlugInManagementExtension* extension : component.managementExtensions)
						if(extension->editPlugInListColumn (*item, *description, columnID, info))
						{
							if(auto itemView = getItemView ())
								itemView->invalidateItem (index);
							return true;
						}
				}
		}

	return SuperClass::editCell (index, column, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInListViewModel::appendItemMenu (IContextMenu& menu, ItemIndexRef itemIndex, const IItemSelection& selection)
{
	PlugInListItem* focusItem = static_cast<PlugInListItem*> (resolve (itemIndex));
	if(focusItem == nullptr)
		return false;

	AutoPtr<Url> modulePath = NEW Url;
	PlugIn::getModulePath (*modulePath, focusItem->getClassID (), PlugIn::kCheckKnownLocation);
	if(!modulePath->isEmpty ())
		menu.addCommandItem (ShellCommand::getShowFileInSystemTitle (), "Plug-In Manager", "Show in Explorer/Finder", 
			CommandDelegate<PlugInListViewModel>::make (this, &PlugInListViewModel::showFileInSystem, modulePath->asUnknown ()));

	if(component.managementExtensions.isEmpty () == false)
	{		
		if(auto focusClass = System::GetPlugInManager ().getClassDescription (focusItem->getClassID ()))
		{
			AutoPtr<UnknownList> selectedClasses = NEW UnknownList;
			selectedClasses->add (const_cast<IClassDescription*> (focusClass), true);
			ForEachItem (selection, selectedIndex)
				if(auto selectedItem = static_cast<PlugInListItem*> (resolve (selectedIndex)))
					if(auto selectedClass = System::GetPlugInManager ().getClassDescription (selectedItem->getClassID ()))
						if(selectedClass != focusClass)
							selectedClasses->add (const_cast<IClassDescription*> (selectedClass), true);
			EndFor
			
			for(const IPlugInManagementExtension* extension : component.managementExtensions)
				extension->appendPlugInListItemMenu (menu, *focusClass, *selectedClasses);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInListViewModel::showFileInSystem (CmdArgs args, VariantRef data)
{
	const Url* path = unknown_cast<Url> (data.asUnknown ());
	if(path == nullptr)
		return false;

	return ShellCommand::showFileInSystem (*path, args.checkOnly ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API PlugInListViewModel::createDragSessionData (ItemIndexRef index)
{
	if(auto item = static_cast<PlugInListItem*> (resolve (index)))
	{
		auto description = System::GetPlugInManager ().getClassDescription (item->getClassID ());
		return ccl_as_unknown (NEW ObjectPreset (description));
	}
	else
		return nullptr;
}

//************************************************************************************************
// BlockListViewModel
//************************************************************************************************

DEFINE_CLASS_HIDDEN (BlockListViewModel, ListViewModel)

//////////////////////////////////////////////////////////////////////////////////////////////////

BlockListViewModel::BlockListViewModel (PlugInManagementComponent& component)
: component (component)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BlockListViewModel::onSelectionChanged ()
{
	component.onListSelectionChanged (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BlockListViewModel::getModulePath (Url& modulePath, const ObjectPreset& preset)
{
	IAttributeList* metaInfo = preset.getMetaInfo ();
	if(metaInfo == nullptr)
		return false;

	PresetMetaAttributes metaAttributes (*metaInfo);
	UID cid;
	if(!metaAttributes.getClassID (cid))
		return false;

	return PlugIn::getModulePath (modulePath, cid, PlugIn::kCheckKnownLocation);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BlockListViewModel::canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView)
{
	if(session)
	{
		ObjectPreset* preset = unknown_cast<ObjectPreset> (session->getItems ().getFirst ());
		if(preset == nullptr)
			return false;

		Url modulePath;
		return getModulePath (modulePath, *preset);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BlockListViewModel::insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session)
{
	bool succeeded = false;
	if(session)
	{
		ForEachUnknown (session->getItems (), unk)
			ObjectPreset* preset = unknown_cast<ObjectPreset> (unk);
			if(preset)
			{
				Url modulePath;
				if(getModulePath (modulePath, *preset))
				{
					auto& plugManager = System::GetPlugInManager ();
					tbool oldState = plugManager.enableBlocklist (true);
					plugManager.addToBlocklist (modulePath);
					plugManager.enableBlocklist (oldState);

					component.updateBlockList ();
					component.updateResultList ();

					succeeded = true;
				}
			}
		EndFor
	}
	if(succeeded)
		SignalSource (Signals::kPlugIns).deferSignal (NEW Message (Signals::kPluginPresentationChanged));
	return succeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BlockListViewModel::removeItems (ItemIndexRef index, const IItemSelection& selection)
{
	InterfaceList<IUrl> urlList;
	ForEachItem (selection, i)
		if(auto item = resolve (i))
			if(UnknownPtr<IUrl> url = item->getDetails ().getUnknown (PlugInListItem::kBlocklistURL))
				urlList.append (url.detach ());
	EndFor
	
	if(!urlList.isEmpty ())
	{
		auto& plugManager = System::GetPlugInManager ();
		tbool oldState = plugManager.enableBlocklist (true);
		for(auto url : urlList)
			plugManager.removeFromBlocklist (*url);
		plugManager.enableBlocklist (oldState);

		component.updateBlockList ();
	}
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_STRINGID_MEMBER_ (PriorityListModel, kPriorityColumn, "priority")
DEFINE_STRINGID_MEMBER_ (PriorityListModel, kMoveIndicatorColumn, "mover")

//************************************************************************************************
// PriorityListModel
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PriorityListModel, ListViewModel)

//////////////////////////////////////////////////////////////////////////////////////////////////

PriorityListModel::PriorityListModel (PlugInManagementComponent& component)
: minDragIndex (0)
{
	IColumnHeaderList& columns = getColumns ();
	columns.addColumn (20, String::kEmpty, kPriorityColumn);
	columns.addColumn (100, String::kEmpty, kTitleID, 0, IColumnHeaderList::kFill);
	columns.addColumn (20, String::kEmpty, kMoveIndicatorColumn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PriorityListModel::index (StringRef title) const
{
	for(int i = 0; i < items.count (); i++)
	{
		const ListViewItem* item = ccl_cast<ListViewItem> (items.at (i));
		ASSERT (item != nullptr)
		if(item->getTitle () == title)
			return i;
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API PriorityListModel::createDragSessionData (ItemIndexRef index) 
{
	if(index.getIndex () >= minDragIndex)
		return SuperClass::createDragSessionData (index);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PriorityListModel::canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView)
{
	UnknownPtr<IItemDragTarget> dragTarget (targetView);
	ListViewItem* item = unknown_cast<ListViewItem> (data.getFirst ());
	if(dragTarget && item && items.contains (item) && index.getIndex () >= minDragIndex)
	{
		// reorder items
		AutoPtr<IDragHandler> handler (dragTarget->createDragHandler (IItemView::kCanDragBetweenItems|IItemView::kDropInsertsData|IItemView::kCanDragPrePostItems, this));
		session->setDragHandler (handler);
		session->setResult (IDragSession::kDropMove);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PriorityListModel::insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session)
{
	int insertIndex = index.getIndex ();
	if(insertIndex >= 0 && insertIndex < minDragIndex)
		return false;

	ListViewItem* item = unknown_cast<ListViewItem> (data.getFirst ());
	if(item)
	{
		UnknownPtr<IItemViewDragHandler> indicator (session ? session->getDragHandler () : nullptr);
		if(indicator)
		{
			ItemIndex targetIndex;
			int relation = 0;
			if(indicator && indicator->getTarget (targetIndex, relation))
			{
				insertIndex = targetIndex.getIndex ();
				if(relation == IItemViewDragHandler::kAfterItem && insertIndex >= 0)
					insertIndex++;
			}
		}
		
		if(insertIndex < 0)
			insertIndex = items.count ();

		// remove items from their old positons first
		int oldIndex = items.index (item);
		items.remove (item);
		if(oldIndex < insertIndex)
			insertIndex--;

		// insert at target index
		ccl_lower_limit (insertIndex, minDragIndex);
		items.insertAt (insertIndex++, item);

		signal (Message (kChanged));

		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PriorityListModel::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	if(index.getIndex () >= minDragIndex)
	{
		StringID columnID = getColumns ().getColumnID (column);
		if(columnID == kPriorityColumn)
		{
			String numberStr;
			numberStr << index.getIndex ();
			Rect r (info.rect);
			r.right -= 3;

			info.graphics.drawString (r, numberStr, info.style.font, info.style.getTextBrush (true), Alignment::kRight|Alignment::kVCenter);
		}
		else if(columnID == kMoveIndicatorColumn)
		{
			Coord mid = info.rect.top + info.rect.getHeight () / 2;

			Rect lineRect (0, info.rect.top, info.rect.getWidth (), info.rect.bottom);
			lineRect.contract (4);
			lineRect.setHeight (2);
			lineRect.moveTo (Point (info.rect.left, mid - lineRect.getHeight () - 1));

			IGraphics& graphics = info.graphics;
			SolidBrush handleBrush (info.view->getVisualStyle ().getColor ("dragHandleColor", Colors::kGray));
			graphics.fillRoundRect (lineRect, 1, 1, handleBrush);

			lineRect.moveTo (Point (info.rect.left, mid + 1));
			graphics.fillRoundRect (lineRect, 1, 1, handleBrush);
		}
	}

	return SuperClass::drawCell (index, column, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PriorityListModel::verifyTargetItem (ItemIndex& index, int& relation)
{
	return index.getIndex () >= minDragIndex;
}
