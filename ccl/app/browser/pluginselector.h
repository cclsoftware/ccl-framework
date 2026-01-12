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
// Filename    : ccl/app/browser/pluginselector.h
// Description : Plug-in Selector
//
//************************************************************************************************

#ifndef _ccl_pluginselector_h
#define _ccl_pluginselector_h

#include "ccl/app/params.h"
#include "ccl/app/utilities/pluginclass.h"
#include "ccl/app/browser/nodesorter.h"

#include "ccl/public/gui/framework/iparametermenu.h"
#include "ccl/public/base/irecognizer.h"

namespace CCL {

class NodeSorter;
class FolderNode;
class PlugInSelectorPopup;

namespace Browsable {
class PlugInCategoryNode; }

//************************************************************************************************
// PlugInMenuParam
//************************************************************************************************

class PlugInMenuParam: public MenuParam,
					   public StructuredParameter,
					   public IParameterMenuCustomize
{
public:
	DECLARE_CLASS (PlugInMenuParam, MenuParam)
	DECLARE_METHOD_NAMES (PlugInMenuParam)

	PlugInMenuParam (StringID name = nullptr, 
					 StringRef category = nullptr,
					 StringRef subCategory = nullptr,
					 bool autoRebuildEnabled = false);
	~PlugInMenuParam ();

	DECLARE_STRINGID_MEMBER (kListChanged)

	void setDisplayVendor (bool state);
	PROPERTY_READONLY_FLAG (displayStyle, kDisplayVendor, isDisplayVendor)

	void setDisplaySorted (bool state);
	PROPERTY_READONLY_FLAG (displayStyle, kDisplaySorted, isDisplaySorted)

	void setDisplayUnselectItem (bool state);
	PROPERTY_READONLY_FLAG (displayStyle, kDisplayUnselect, isDisplayUnselectItem)

	PROPERTY_FLAG (displayStyle, kDisplayExactIcon, isDisplayExactIcon)
	PROPERTY_FLAG (displayStyle, kCanIgnoreSubCategory, canIgnoreSubCategory)		
	PROPERTY_FLAG (displayStyle, kDisplayLargeMenu, isDisplayLargeMenu)
	PROPERTY_FLAG (displayStyle, kStructuredMenu, isStructuredMenu)

	const PlugInCategory& getCategory1 () const;
	void setCategory1 (const PlugInCategory& category);

	const PlugInCategory& getCategory2 () const;
	void setCategory2 (const PlugInCategory& category);

	IObjectFilter* getFilter () const;
	void setFilter (IObjectFilter* filter);

	const PlugInClass* getSelectedClass () const;
	bool selectClass (UIDRef classID, bool update = false);
	
	void setAutoRebuild (bool state);
	PROPERTY_READONLY_FLAG (displayStyle, kAutoRebuild, isAutoRebuild)
	
	void checkRebuild ();

	void setNoPlugInLabel (StringRef label);

	// MenuParam
	tbool CCL_API canIncrement () const override;
	void CCL_API extendMenu (IMenu& menu, StringID name) override;

	// StructuredParameter
	void CCL_API prepareStructure () override;
	void CCL_API cleanupStructure () override;

	// Object
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	void CCL_API notify (ISubject* s, MessageRef msg) override;

	CLASS_INTERFACE2 (IStructuredParameter, IParameterMenuCustomize, MenuParam)

protected:
	enum DisplayStyles 
	{ 
		kDisplayVendor    = 1<<0,
		kDisplaySorted    = 1<<1,
		kDisplayExactIcon = 1<<2,
		kDisplayLargeMenu = 1<<3,
		kAutoRebuild      = 1<<4,
		kDisplayUnselect  = 1<<5,
		kCanIgnoreSubCategory = 1<<6,
		kStructuredMenu	  = 1<<7
	};

	PlugInCategory category1;
	PlugInCategory category2;
	int displayStyle;
	SharedPtr<IObjectFilter> filter;
	String noPlugInLabel;

	PROPERTY_FLAG (displayStyle, kAutoRebuild, autoRebuild)
	PROPERTY_FLAG (displayStyle, kDisplayVendor, displayVendor)
	PROPERTY_FLAG (displayStyle, kDisplaySorted, displaySorted)
	PROPERTY_FLAG (displayStyle, kDisplayUnselect, displayUnselect)

	IImage* getIcon (const PlugInClass& plugClass) const;
	void updateList ();
	void buildStructuredMenu (IMenu& menu, FolderNode& folderNode, IParameterMenuBuilder& builder);

	// IParameterMenuCustomize
	StringID CCL_API getMenuType () const override;
	tbool CCL_API buildMenu (IMenu& menu, IParameterMenuBuilder& builder) override;
	tbool CCL_API onMenuKeyDown (const KeyEvent& event) override;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// PluginMenu
//************************************************************************************************

class PluginMenu
{
public:
	static void popup (IParameter* menuParam, StringRef title, StringRef text);
};

//************************************************************************************************
// PlugInCategoryParam
//************************************************************************************************

class PlugInCategoryParam: public MenuParam
{
public:
	DECLARE_CLASS (PlugInCategoryParam, MenuParam)

	PlugInCategoryParam (StringID name = nullptr);

	void addCategory (const PlugInCategory& category);

	const PlugInCategory* getSelectedCategory () const;
	bool selectCategory (const PlugInCategory& category, bool update = false);

	// MenuParam
	void CCL_API extendMenu (IMenu& menu, StringID name) override;
};

//************************************************************************************************
// PlugInSelector
//************************************************************************************************

class PlugInSelector: public Component
{
public:
	DECLARE_CLASS (PlugInSelector, Component)
	DECLARE_METHOD_NAMES (PlugInSelector)

	PlugInSelector (StringRef classCategory1 = nullptr, StringRef classCategory2 = nullptr);

	void setFilter (IObjectFilter* filter);
	IObjectFilter* getFilter () const;

	PROPERTY_SHARED_AUTO (IObjectFilter, browserFilter, BrowserFilter) // separate filter for PlugInSelectorBrowser

	void setStructuredMenu (bool state);

	void setCategories (StringRef classCategory1 = nullptr, StringRef classCategory2 = nullptr);
	const PlugInCategory& getCategory1 () const;
	const PlugInCategory& getCategory2 () const;
	IParameter* getPlugList ();
	void showMenu ();

	bool runDialog (StringRef title = nullptr);

	const PlugInClass* getSelected () const;
	bool setSelected (UIDRef classID);

	bool isEnabled () const;
	void enable (bool state);

	virtual Browsable::PlugInCategoryNode* createBrowserNode () const {return nullptr;}
	virtual String getSelectorID () const { return getCategory1 ().getCategory (); }

	// Component
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;
	tbool CCL_API paramChanged (IParameter* param) override;

	static void setPopupModeParam (IParameter* param);

protected:
	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;

private:
	PlugInSelectorPopup* plugInSelectorPopup;
	bool runningModal;
	static SharedPtr<IParameter> globalPopupModeParam;

	enum Tag { kPlugList = 100, kSelectorMode };
};

//************************************************************************************************
// PlugInSortMethods
//************************************************************************************************

struct PlugInSortMethods
{
	static String getVendor (const IClassDescription& description);
	static String getType (const IClassDescription& description);
};

//************************************************************************************************
// PlugInSorterComponent
//************************************************************************************************

class PlugInSorterComponent: public NodeSorterComponent
{
public:
	PlugInSorterComponent ();

	static CCL::NodeSorter* createUserFolderSorter ();
	static CCL::NodeSorter* createVendorSorter ();
	static CCL::NodeSorter* createSubCategorySorter ();
};

} // namespace CCL

#endif // _ccl_pluginselector_h
