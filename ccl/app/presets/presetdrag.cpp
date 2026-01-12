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
// Filename    : ccl/app/presets/presetdrag.cpp
// Description : Preset Drag handler
//
//************************************************************************************************

#include "ccl/app/presets/presetdrag.h"
#include "ccl/app/presets/presetcomponent.h"
#include "ccl/app/presets/presetfileprimitives.h"
#include "ccl/app/presets/presetsystem.h"

#include "ccl/app/utilities/pluginclass.h"

#include "ccl/base/objectconverter.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/message.h"
#include "ccl/base/storage/packageinfo.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/app/signals.h"
#include "ccl/public/app/presetmetainfo.h"
#include "ccl/public/gui/framework/ihelpmanager.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/base/irecognizer.h"
#include "ccl/public/plugservices.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Presets")
	XSTRING (EnterName, "Enter Name")
	XSTRING (NoMatchingPreset, "No matching Preset")
END_XSTRINGS

using namespace CCL;

//************************************************************************************************
// PresetCategoryFilter
//************************************************************************************************

void PresetCategoryFilter::excludeClass (UIDRef classID)
{
	excludedClasses.append (classID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool PresetCategoryFilter::check (IAttributeList& metaData) const
{
	if(excludedClasses.isEmpty () == false)
	{
		CCL::UID cid;
		if(PresetMetaAttributes (metaData).getClassID (cid))
		{
			ListForEach (excludedClasses, UID, excludedCID)
				if(cid == excludedCID)
					return false;
			EndFor
		}
	}

	if(exclusiveSubCategories.isEmpty () == false)
	{
		String presetCategory (PresetMetaAttributes (metaData).getCategory ());
		String presetSubCategory (PresetMetaAttributes (metaData).getSubCategory ());
		bool isExlusiveCategory = false;
		for(auto& exlusiveSubCategory : exclusiveSubCategories)
		{
			if(exlusiveSubCategory.category == presetCategory)
			{
				isExlusiveCategory = true;
				if(presetSubCategory.contains (exlusiveSubCategory.subCategory, false))
					return true;
			}
		}	
		if(isExlusiveCategory)
			return false;
	}

	if(categories.isEmpty () == false)
	{
		String presetCategory (PresetMetaAttributes (metaData).getCategory ());
		if(categories.contains (presetCategory) == false)
			return false;

		if(excludedSubCategories.isEmpty () == false)
		{
			String presetSubCategory (PresetMetaAttributes (metaData).getSubCategory ());
			if(presetSubCategory.isEmpty () == false && excludedSubCategories.containsSubStringOf (presetSubCategory, false))
				return false;
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetCategoryFilter::matches (IUnknown* object) const
{
	UnknownPtr<IPreset> preset (object);
	if(preset)
	{
		if(IAttributeList* metaInfo = preset->getMetaInfo ())
			return check (*metaInfo);
	}
	else if(UnknownPtr<IPresetMediator> presetMediator = object)
	{
		Attributes metaInfo;
		if(presetMediator->getPresetMetaInfo (metaInfo))
			return check (metaInfo);
	}
	else if(UnknownPtr<IAttributeList> attributes = object)
	{
		return check (*attributes);
	}
	return false;
}

//************************************************************************************************
// PresetFileTypeFilter
//************************************************************************************************

tbool CCL_API PresetFileTypeFilter::matches (IUnknown* object) const
{
	UnknownPtr<IPreset> preset (object);
	if(preset)
	{
		Url presetUrl;
		preset->getUrl (presetUrl);
		return FileTypeFilter::matches (presetUrl);
	}
	else
	{
		UnknownPtr<IPresetMediator> presetMediator (object);
		if(presetMediator)
		{
			Url url;
			url.setFileType (PresetFilePrimitives::getDefaultHandler (presetMediator).getFileType ());
			return FileTypeFilter::matches (url);
		}
	}
	return false;
}

//************************************************************************************************
// PresetDragHandler
//************************************************************************************************

bool PresetDragHandler::extractClass (PlugInClass& plugClass, IPreset& preset)
{
	IAttributeList* metaInfo = preset.getMetaInfo ();
	if(!metaInfo)
		return false;

	return extractClass (plugClass, *metaInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetDragHandler::extractClass (PlugInClass& plugClass, IPresetMediator& presetMediator)
{
	Attributes metaInfo;
	if(!presetMediator.getPresetMetaInfo (metaInfo))
		return false;

	return extractClass (plugClass, metaInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetDragHandler::extractClass (PlugInClass& plugClass, IAttributeList& metaInfo)
{
	PresetMetaAttributes attribs (metaInfo);
	UID cid;
	attribs.getClassID (cid);

	const IClassDescription* description = System::GetPlugInManager ().getClassDescription (cid);
	if(description)
		plugClass.assign (*description);
	else
	{
		plugClass.setClassID (cid);
		plugClass.setName (attribs.getClassName ());
		plugClass.setCategory (attribs.getCategory ());
		plugClass.setSubCategory (attribs.getSubCategory ());
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (PresetDragHandler, DragHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetDragHandler::PresetDragHandler (IView* view)
: DragHandler (view),
  presetFilter (nullptr)
{
	categoryHeaders.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetDragHandler::PresetDragHandler (UserControl& control)
: DragHandler (control),
  presetFilter (nullptr)
{
	categoryHeaders.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetDragHandler::addCategoryHeader (StringRef category, StringRef header)
{
	categoryHeaders.add (NEW Boxed::VariantWithName (category, header));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetDragHandler::getHelp (IHelpInfoBuilder& helpInfo)
{
	DragHandler* childHandler = unknown_cast<DragHandler> (childDragHandler);
	return childHandler ? childHandler->getHelp (helpInfo) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* PresetDragHandler::preparePreset (IUnknown& item)
{
	IPreset* preset = ObjectConverter::toInterface<IPreset> (&item);
	if(preset)
	{
		if(presetFilter && !presetFilter->matches (preset))
		{
			Url presetUrl;
			preset->getUrl (presetUrl);

			safe_release (preset);

			// try other preset handlers for that file type (note: bypasses PresetManager / PresetStore)
			IPresetFileRegistry& registry = System::GetPresetFileRegistry ();
			for(int i = 1, num = registry.countHandlers (); i < num; i++)
				if(IPresetFileHandler* handler = registry.getHandler (i))
				{
					FileType fileType;
					int index = 0;
					while((fileType = handler->getFileType (index++)).isValid ())
						if(fileType == presetUrl.getFileType ())
							if(preset = handler->openPreset (presetUrl))
							{
								if(presetFilter->matches (preset))
									break;
								else
									safe_release (preset);
							}
				}

			if(!preset)
				return nullptr;
		}

		String text;
		IImage* icon = nullptr;

		PlugInClass plugClass;
		if(extractClass (plugClass, *preset))
		{
			if(displayClassName ())
				text = plugClass.getName ();
			StringRef presetName = preset->getPresetName ();
			if(!presetName.isEmpty () && presetName != text)
			{
				if(!text.isEmpty ())
					text << ": ";
				text << presetName;
			}
			icon = plugClass.getIcon ();
		}
		else
			text = preset->getPresetName ();

		int group = 0;
		if(!categoryHeaders.isEmpty() && preset->getMetaInfo ())
		{
			group = categoryHeaders.index (Boxed::Variant (PresetMetaAttributes (*preset->getMetaInfo ()).getCategory ()));
			if(group >= 0)
				group = 2 * group + 1; // reserve even groups for headers, odd groups for data items
		}
		spriteBuilder.addItem (icon, text, group);
	}
	return preset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* PresetDragHandler::prepareDataItem (IUnknown& item, IUnknown* context)
{
	IPreset* preset = preparePreset (item);

	if(!preset)
		prepareFolderContent (item, context, 20);

	return preset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetDragHandler::prepare (const IUnknownList& items, const IObjectFilter* filter, StringRef title)
{
	if(!title.isEmpty ())
		spriteBuilder.addHeader (title);

	presetFilter = filter;
	spriteBuilder.setCreateSpriteSuspended (true);

	bool result = SuperClass::prepare (items);

	for(int g = 0, numGroups = categoryHeaders.count (); g < numGroups; g++)
		if(spriteBuilder.hasGroup (g * 2 + 1))
			spriteBuilder.addHeader (((Boxed::VariantWithName*)categoryHeaders.at (g))->getName (), 2 * g);

	presetFilter = nullptr;
	spriteBuilder.setCreateSpriteSuspended (false);

	buildSprite ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* PresetDragHandler::getFirstPreset () const
{
	UnknownPtr<IPreset> preset (getData ().getFirst ());
	return preset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetDragHandler::hasAcceptedPreset () const
{
	// only try first preset for now
	IPreset* preset = getFirstPreset ();
	return preset && preset->getMetaInfo () && acceptPreset (PresetMetaAttributes (*preset->getMetaInfo ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetDragHandler::acceptPreset (const PresetMetaAttributes& metaInfo) const
{
	return true;
}

//************************************************************************************************
// StorePresetDragHandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (StorePresetDragHandler, DragHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

StorePresetDragHandler::StorePresetDragHandler (IView* view)
: DragHandler (view),
  targetItemType (kTargetNone),
  flags (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StorePresetDragHandler::StorePresetDragHandler (UserControl& control)
: DragHandler (control),
  targetItemType (kTargetNone),
  flags (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StorePresetDragHandler::getPresetSpriteInfo (String& text, AutoPtr<IImage>& icon, IPresetMediator& presetMediator, StringRef presetName)
{
	text = presetName;

	PlugInClass plugClass;
	if(PresetDragHandler::extractClass (plugClass, presetMediator))
	{
		text = plugClass.getName ();
		if(!presetName.isEmpty () && presetName != text)
		{
			if(!text.isEmpty ())
				text << ": ";
			text << presetName;
		}
		icon.share (plugClass.getIcon (true));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* StorePresetDragHandler::prepareDataItem (IUnknown& item, IUnknown* context)
{
	IPresetMediator* presetMediator = ObjectConverter::toInterface<IPresetMediator> (&item);
	if(presetMediator)
	{
		if(presetMediatorFilter && !presetMediatorFilter->matches (presetMediator))
		{
			presetMediator->release ();
			return nullptr;
		}

		String text;
		AutoPtr<IImage> icon;
		getPresetSpriteInfo (text, icon, *presetMediator, presetMediator->makePresetName (false));

		spriteBuilder.addItem (icon, text);

		if(!firstMediatorInfo)
		{
			firstMediatorInfo = NEW Attributes;
			presetMediator->getPresetMetaInfo (*firstMediatorInfo);
		}
	}
	return presetMediator;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StorePresetDragHandler::finishPrepare ()
{
	if(!data.isEmpty ())
		spriteBuilder.addHeader (PresetComponent::getStorePresetTitle (), -1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StorePresetDragHandler::setTargetPreset (IPreset* preset, ItemIndex* highlightItem)
{
	targetItemType = preset ? kTargetPreset : kTargetNone;

	targetPreset = preset;
	targetFolderPath = String::kEmpty;
	targetFolderInfo = nullptr;
	this->highlightItem = highlightItem ? *highlightItem : ItemIndex ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StorePresetDragHandler::setTargetFolder (StringRef path, IAttributeList* metaInfo)
{
	targetItemType = metaInfo ? kTargetPresetFolder : kTargetNone;

	targetFolderPath = path;
	targetFolderInfo = metaInfo;
	targetPreset = nullptr;
	highlightItem = ItemIndex ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* StorePresetDragHandler::getReplaceTarget () const
{
	// check if mouseover preset should & can be replaced
	return replacePreset () ? getMatchingTargetPreset () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* StorePresetDragHandler::getMatchingTargetPreset () const
{
	// check if target preset info matches first dragged preset
	if(targetPreset
		&& targetPreset->getMetaInfo ()
		&& firstMediatorInfo
		&& PresetMetaAttributes (*targetPreset->getMetaInfo ()).isSimilar (*firstMediatorInfo))
		return targetPreset;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StorePresetDragHandler::isMatchingTargetFolder () const
{
	return targetItemType == kTargetPresetFolder
		&& targetFolderInfo
		&& firstMediatorInfo
		&& PresetMetaAttributes (*targetFolderInfo).isSimilar (*firstMediatorInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StorePresetDragHandler::getTargetFolder (String& folderPath) const
{
	switch(targetItemType)
	{
	case kTargetPresetFolder:
		if(isMatchingTargetFolder ())
		{
			// explicitly given target folder
			folderPath = targetFolderPath;
			return true;
		}
		break;

	case kTargetPreset:
		if(IPreset* preset = getMatchingTargetPreset ())
		{
			// folder of target preset
			folderPath = PresetMetaAttributes (*targetPreset->getMetaInfo ()).getSubFolder ();
			return true;
		}
		break;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StorePresetDragHandler::verifyTargetItem (ItemIndex& item, int& relation)
{
	if(replacePreset ())
	{
		if(getMatchingTargetPreset ())
			relation = IItemViewDragHandler::kOnItem;
		else
			return false;
	}
	else
	{
		if(isMatchingTargetFolder () || highlightItem.isValid ())
		{
			if(highlightItem.isValid ())
				item = highlightItem;
			relation = IItemViewDragHandler::kOnItem;
		}
		else
		{
			// no target folder selection, presets are sorted automatically
			item = ItemIndex ();
			relation = IItemViewDragHandler::kFullView;
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StorePresetDragHandler::getHelp (IHelpInfoBuilder& helpInfo)
{
	SuperClass::getHelp (helpInfo);
	
	helpInfo.addOption (0, nullptr, PresetComponent::getStorePresetTitle ());
	helpInfo.addOption (KeyState::kCommand, nullptr, XSTR (EnterName));
	helpInfo.addOption (KeyState::kOption, nullptr, PresetComponent::getUpdatePresetTitle ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StorePresetDragHandler::dragOver (const DragEvent& event)
{
	SuperClass::dragOver (event); // derived class might set a targetPreset or folder here (via child ItemDragHandler -> verifyTargetItem)

	replacePreset (event.keys.isSet (KeyState::kOption));
	showStoreDialog (event.keys.isSet (KeyState::kCommand) && !replacePreset ());

	IPreset* presetToReplace = getReplaceTarget ();
	int result = IDragSession::kDropCopyReal;

	if(sprite)
	{
		String header (replacePreset () ? PresetComponent::getUpdatePresetTitle () : PresetComponent::getStorePresetTitle ());
		spriteBuilder.replaceItemText (*sprite, 0, header);

		UnknownPtr<IPresetMediator> firstMediator (getData ().getFirst ());
		if(firstMediator)
		{
			// update first preset sprite text (name changes e.g. when toggling modes)
			String presetName (firstMediator->makePresetName (false));
			if(replacePreset ())
			{
				if(presetToReplace)
					presetName = presetToReplace->getPresetName ();
				else
				{
					presetName = XSTR (NoMatchingPreset);
					result = IDragSession::kDropNone;
				 }
			}
			else if(showStoreDialog ())
				presetName = String (" (") << XSTR (EnterName) << ")";

			String text;
			AutoPtr<IImage> icon;
			getPresetSpriteInfo (text, icon, *firstMediator, presetName);
			spriteBuilder.replaceItemText (*sprite, 1, text);
		}
	}

	event.session.setResult (result);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StorePresetDragHandler::afterDrop (const DragEvent& event)
{
	AutoPtr<IUrl> revealUrl;
	SharedPtr<IAttributeList> revealMetaInfo;

	IPreset* presetToReplace = getReplaceTarget ();

	ForEachUnknown (data, unk)
		UnknownPtr<IPresetMediator> presetMediator (unk);
		if(presetMediator)
		{
			AutoPtr<PackageInfo> metaInfo (NEW PackageInfo);
			presetMediator->getPresetMetaInfo (*metaInfo);

			IPresetFileHandler& handler = PresetFilePrimitives::getDefaultHandler (presetMediator);

			Url presetUrl;
			if(presetToReplace)
			{
				presetToReplace->getUrl (presetUrl);

				if(IAttributeList* existingMetaInfo = presetToReplace->getMetaInfo ())
					metaInfo->copyFrom (*existingMetaInfo);

				if(presetToReplace->isReadOnly ())
				{
					// read only: store in write location instead
					PresetFilePrimitives::makeRelativePresetUrl (presetUrl, metaInfo);

					Url location;
					if(handler.getWriteLocation (location, metaInfo))
						presetUrl.makeAbsolute (location);
				}
				presetToReplace = nullptr; // only replace first in case of multiple presets
			}
			else
			{
				Url location;
				if(handler.getWriteLocation (location, metaInfo))
				{
					// preset name
					PresetMetaAttributes metaAttributes (*metaInfo);
					String presetName = metaAttributes.getTitle ();
					if(presetName.isEmpty ())
					{
						presetName = presetMediator->makePresetName (false);
						if(presetName.isEmpty ())
						{
							PlugInClass plugClass;
							if(PresetDragHandler::extractClass (plugClass, *presetMediator))
								presetName = plugClass.getName ();
						}
					}

					String subFolder;
					if(getTargetFolder (subFolder))
						metaAttributes.setSubFolder (subFolder);

					auto makePresetUrl = [&] (StringRef name)
					{
						String presetName = PresetFilePrimitives::makeUniquePresetName (name, metaInfo, &handler.getFileType ());
						metaAttributes.setTitle (presetName);

						Url url (location);
						PresetFilePrimitives::descendSubFolder (url, *metaInfo);
						PresetFilePrimitives::descendPresetName (url, presetName, handler, true);
						return url;
					};
				
					presetUrl = makePresetUrl (presetName);

					if(showStoreDialog ())
					{
						if(PresetComponent::askPresetInfo (metaAttributes, metaInfo))
						{
							presetName = metaAttributes.getTitle ();
							presetUrl = makePresetUrl (presetName);
						}
						else
							return SuperClass::afterDrop (event);
					}
				}
			}

			if(PresetFilePrimitives::writePreset (presetUrl, *metaInfo, handler, *presetMediator, IPresetNotificationSink::kStorePreset))
			{
				if(!revealUrl)
				{
					revealUrl = NEW Url (presetUrl);
					revealMetaInfo = metaInfo;
				}
			}
		}
	EndFor

	if(revealUrl)
		SignalSource (Signals::kPresetManager).signal (Message (Signals::kRevealPreset, (IUrl*)revealUrl, (IAttributeList*)revealMetaInfo));

	return SuperClass::afterDrop (event);
}
