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
// Filename    : ccl/app/presets/presetdrag.h
// Description : Preset Drag handler
//
//************************************************************************************************

#ifndef _ccl_presetdrag_h
#define _ccl_presetdrag_h

#include "ccl/app/controls/draghandler.h"

#include "ccl/base/collections/stringlist.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/app/ipreset.h"
#include "ccl/public/gui/framework/iitemmodel.h"
#include "ccl/public/base/irecognizer.h"

namespace CCL {

interface IDataTarget;
interface IObjectFilter;
class PlugInClass;
class PresetMetaAttributes;

//************************************************************************************************
// PresetDragHandler
//************************************************************************************************

class PresetDragHandler: public DragHandler
{
public:
	DECLARE_CLASS_ABSTRACT (PresetDragHandler, DragHandler)

	PresetDragHandler (IView* view = nullptr);
	PresetDragHandler (UserControl& control);

	static bool extractClass (PlugInClass& plugClass, IPreset& preset);
	static bool extractClass (PlugInClass& plugClass, IPresetMediator& presetMediator);
	static bool extractClass (PlugInClass& plugClass, IAttributeList& metaInfo);

	void addCategoryHeader (StringRef category, StringRef header); ///< header strings for preset categories
	bool prepare (const IUnknownList& items, const IObjectFilter* presetFilter = nullptr, StringRef title = String::kEmpty);

	IPreset* getFirstPreset () const;
	bool hasAcceptedPreset () const;

protected:
	const IObjectFilter* presetFilter;
	ObjectList categoryHeaders;

	IPreset* preparePreset (IUnknown& item);
	virtual bool displayClassName () const {return true;}
	virtual bool acceptPreset (const PresetMetaAttributes& metaInfo) const;

	// DragHandler
	IUnknown* prepareDataItem (IUnknown& item, IUnknown* context) override;
	bool getHelp (IHelpInfoBuilder& helpInfo) override;
};

//************************************************************************************************
// StorePresetDragHandler
/** Drag a preset mediator somewhere to store a preset. */
//************************************************************************************************

class StorePresetDragHandler: public DragHandler,
							  public IItemDragVerifier,
							  public ISourceDragBlocker
{
public:
	DECLARE_CLASS_ABSTRACT (StorePresetDragHandler, DragHandler)

	StorePresetDragHandler (IView* view = nullptr);
	StorePresetDragHandler (UserControl& control);

	PROPERTY_SHARED_AUTO (IObjectFilter, presetMediatorFilter, PresetMediatorFilter)

	CLASS_INTERFACE2 (IItemDragVerifier, ISourceDragBlocker, DragHandler)

protected:
	enum TargetItemType { kTargetNone, kTargetPreset, kTargetPresetFolder };

	TargetItemType targetItemType;
	SharedPtr<IPreset> targetPreset;
	SharedPtr<IAttributeList> targetFolderInfo;
	String targetFolderPath;
	ItemIndex highlightItem; // optional alternative highlight item (delivered in verifyTargetItem)

	AutoPtr<IAttributeList> firstMediatorInfo;
	int flags;

	PROPERTY_FLAG (flags, 1<<0, showStoreDialog)
	PROPERTY_FLAG (flags, 1<<1, replacePreset)

	void setTargetPreset (IPreset* preset, ItemIndex* highlightItem = nullptr);
	void setTargetFolder (StringRef path, IAttributeList* metaInfo);

	IPreset* getMatchingTargetPreset () const;
	IPreset* getReplaceTarget () const;
	bool isMatchingTargetFolder () const;
	bool getTargetFolder (String& folderPath) const;
	void getPresetSpriteInfo (String& text, AutoPtr<IImage>& icon, IPresetMediator& presetMediator, StringRef presetName);

	// DragHandler
	IUnknown* prepareDataItem (IUnknown& item, IUnknown* context) override;
	void finishPrepare () override;
	bool getHelp (IHelpInfoBuilder& helpInfo) override;
	tbool CCL_API dragOver (const DragEvent& event) override;
	tbool CCL_API afterDrop (const DragEvent& event) override;

	// IItemDragVerifier
	tbool CCL_API verifyTargetItem (ItemIndex& item, int& relation) override;
};

//************************************************************************************************
// PresetCategoryFilter
//************************************************************************************************

struct PresetCategoryFilter: public ObjectFilter
{
	void addCategory (StringRef category);
	void excludeSubCategory (StringRef subCategory);
	void excludeClass (UIDRef classID);
	void addExclusiveSubCategory (StringRef category, StringRef subCategory);

	// ObjectFilter
	tbool CCL_API matches (IUnknown* object) const override;

private:
	StringList categories;
	StringList excludedSubCategories;
	LinkedList<UID> excludedClasses;
	struct ExcusiveSubCategory { String category; String subCategory; };
	LinkedList<ExcusiveSubCategory> exclusiveSubCategories;

	tbool check (IAttributeList& metaData) const;
};

//************************************************************************************************
// PresetFileTypeFilter
//************************************************************************************************

class PresetFileTypeFilter: public FileTypeFilter,
							public IObjectFilter
{
public:
	// IObjectFilter
	CCL::tbool CCL_API matches (IUnknown* object) const override;

	CLASS_INTERFACE (IObjectFilter, FileTypeFilter)
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline 
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void PresetCategoryFilter::addCategory (StringRef category)
{ categories.add (category); }

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void PresetCategoryFilter::excludeSubCategory (StringRef subCategory)
{ excludedSubCategories.add (subCategory); }

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void PresetCategoryFilter::addExclusiveSubCategory (StringRef category, StringRef subCategory)
{ exclusiveSubCategories.append ({category, subCategory}); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_presetdrag_h
