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
// Filename    : ccl/app/browser/pluginselector.cpp
// Description : Plug-in Selector
//
//************************************************************************************************

#include "ccl/app/browser/pluginselector.h"
#include "ccl/app/browser/pluginnodes.h"
#include "ccl/app/browser/browser.h"
#include "ccl/app/browser/searchresultlist.h"

#include "ccl/app/components/searchcomponent.h"

#include "ccl/base/storage/settings.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/boxedtypes.h"

#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/ithememanager.h"
#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/iparametermenu.h"
#include "ccl/public/gui/framework/popupselectorclient.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/ipackagemetainfo.h"
#include "ccl/public/plugservices.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("PlugInSelector")
	XSTRING (Flat, "Flat")
	XSTRING (Folder, "Folder")
	XSTRING (Vendor, "Vendor")
	XSTRING (SubCategory, "Category")
	XSTRING (Type, "Type")
	XSTRING (UnknownVendor, "(Unknown Vendor)")
	XSTRING (NoPlugInClass, "None")
END_XSTRINGS

namespace CCL {

//************************************************************************************************
// PlugInSelectorBrowser
//************************************************************************************************

class PlugInSelectorBrowser: public Browser
{
public:
	DECLARE_CLASS_ABSTRACT (PlugInSelectorBrowser, Browser)

	// one shared instance per plugin category
	static PlugInSelectorBrowser& instance (const PlugInSelector& selector);

	PROPERTY_STRING (category, Category)
	PROPERTY_STRING (selectorId, SelectorId)
	PROPERTY_OBJECT (UID, selectedClassID, SelectedClassID)
	PROPERTY_BOOL (popupResultConfirmed, PopupResultConfirmed)

	SearchComponent* getSearchComponent ();

	// Browser
	void onNodeFocused (BrowserNode* node, bool inList) override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;

	static void cleanupInstances ();

private:
	static ObjectList instances;

	PlugInSelectorBrowser (const PlugInSelector& selector);

	class SearchResultList;
};

//************************************************************************************************
// PlugInSelectorPopup
//************************************************************************************************

class PlugInSelectorPopup: public Component,
						   public PopupSelectorClient
{
public:
	PlugInSelectorPopup (PlugInSelector& selector);

	// PopupSelectorClient
	IView* CCL_API createPopupView (SizeLimit& limits) override;
	bool hasPopupResult () override;
	void CCL_API onPopupClosed (Result result) override;
	Result CCL_API onMouseDown (const MouseEvent& event, IWindow& popupWindow) override;
	Result CCL_API onKeyDown (const KeyEvent& event) override;

	CLASS_INTERFACE (IPopupSelectorClient, Component)

private:
	PlugInSelector& selector;
	PlugInSelectorBrowser& browser;
};

//************************************************************************************************
// PlugInSelectorBrowser::SearchResultList
//************************************************************************************************

class PlugInSelectorBrowser::SearchResultList: public CCL::SearchResultList
{
public:
	SearchResultList (PlugInSelectorBrowser& browser)
	: browser (browser)
	{
		setListStyle (StyleFlags (0, Styles::kItemViewBehaviorSelection|
									 Styles::kItemViewBehaviorAutoSelect|
									 Styles::kItemViewBehaviorSwallowAlphaChars));
	}

	tbool CCL_API onItemFocused (ItemIndexRef index) override
	{
		if(BrowserNode* node = ccl_cast<BrowserNode> (resolve (index)))
			browser.onNodeFocused (node, true);

		return CCL::SearchResultList::onItemFocused (index);
	}

	tbool CCL_API onEditNavigation (const KeyEvent& event, IView* view) override
	{
		// close popup when Escape pressed in search edit box
		if(event.vKey == VKey::kEscape)
			return tryCloseWindowFromSearchBox (view);

		if(isShowingResultList ())
		{
			// close popup when Return / Enter pressed in search edit box and we have a result (otherwise the edit box would just give up focus)
			if(event.vKey == VKey::kReturn || event.vKey == VKey::kEnter)
				if(browser.getSelectedClassID ().isValid () && tryCloseWindowFromSearchBox (view))
				{
					browser.setPopupResultConfirmed (true); // force accepting the result, even though there was no formal IPopupSelectorClient::kOkay via mouse/key event in PopupSelector
					return true;
				}
		}
		else
		{
			if(event.vKey == VKey::kDown)
			{
				if(browser.getTreeView ())
				{
					// arrow down with no search result list: transfer focus to browser tree
					ViewBox (browser.getTreeView ()).takeFocus ();

					// use the arrow key to navigate to first or next item
					BrowserNode* startNode = browser.isAnyNodeSelected () ? browser.getFocusNode () : nullptr;
					if(!startNode)
						startNode = browser.getTreeRoot ();

					if(startNode)
						if(BrowserNode* firstNode = browser.navigate (*startNode, 1))
							browser.setFocusNode (firstNode, true);
				}
			}
		}
		return CCL::SearchResultList::onEditNavigation (event, view);
	}

private:
	PlugInSelectorBrowser& browser;

	bool tryCloseWindowFromSearchBox (IView* view)
	{
		ViewBox vb (view);
		if(view && (vb.getName () == "searchTerms" || vb.getName () == "editString"))
			if(IWindow* window = view->getIWindow ())
			{
				window->close ();
				return true; // event handled
			}
		return false;
	}
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// PlugInMenuParam
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (PlugInMenuParam, kListChanged, "listChanged")
DEFINE_CLASS (PlugInMenuParam, MenuParam)
DEFINE_CLASS_UID (PlugInMenuParam, 0xf969e8ba, 0xcbf6, 0x4b0a, 0xbf, 0x79, 0x17, 0xba, 0x8a, 0xcb, 0xb7, 0x7c)
DEFINE_CLASS_NAMESPACE (PlugInMenuParam, "Host")

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInMenuParam::PlugInMenuParam (StringID name, StringRef category, StringRef subCategory, bool autoRebuildEnabled)
: MenuParam (name),
  category1 (category, subCategory),
  displayStyle (0),
  noPlugInLabel (XSTR (NoPlugInClass))
{
	setSignalAlways (true);
	setAutoRebuild (autoRebuildEnabled);

	SignalSource::addObserver (Signals::kPlugIns, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInMenuParam::~PlugInMenuParam ()
{
	SignalSource::removeObserver (Signals::kPlugIns, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInMenuParam::setAutoRebuild (bool state)
{
	if(state != autoRebuild ())
	{
		autoRebuild (state);
		setOutOfRange (state); // no indicator for current value
		checkRebuild ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInMenuParam::checkRebuild ()
{
	if(autoRebuild ())
		removeAll ();
	else
		updateList ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInMenuParam::updateList ()
{
	SharedPtr<PlugInClass> selectedClass = const_cast<PlugInClass*> (getSelectedClass ());

	removeAll ();

	for(int i = 0; i < 2; i++)
	{
		const PlugInCategory& category = i == 0 ? category1 : category2;
		if(category.getCategory ().isEmpty ())
			break;

		ForEachPlugInClass (category.getCategory (), description)
			if(!category.getSubCategory ().isEmpty ())
			{
				if(description.getSubCategory ().contains (category.getSubCategory (), false) == false)
					continue;
			}

			if(System::GetPluginPresentation ().isHidden (description.getClassID ()))
				continue;

			PlugInClass* plugClass = NEW PlugInClass (description);
			if(isDisplayVendor ())
				plugClass->setTitle (plugClass->makeTitleWithVendor ());

			if(filter && !filter->matches (plugClass->asUnknown ()))
			{
				plugClass->release ();
				continue;
			}

			// list category1 before category2
			plugClass->setMenuPriority (i); 

			appendObject (plugClass);
		EndFor
	}

	if(isDisplaySorted ())
		list.sort ();

	if(isDisplayUnselectItem ())
		appendString (noPlugInLabel, 0);

	if(selectedClass)
		selectClass (selectedClass->getClassID ());

	Object::signal (Message (kListChanged)); // do not defer!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInMenuParam::setNoPlugInLabel (StringRef label)
{
	noPlugInLabel = label;
	checkRebuild ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInMenuParam::prepareStructure ()
{
	if(autoRebuild ())
		updateList ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInMenuParam::cleanupStructure ()
{
	if(autoRebuild ())
		removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInMenuParam::setDisplayVendor (bool state)
{
	if(state != displayVendor ())
	{
		displayVendor (state);
		checkRebuild ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInMenuParam::setDisplaySorted (bool state)
{
	if(state != displaySorted ())
	{
		displaySorted (state);
		checkRebuild ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInMenuParam::setDisplayUnselectItem (bool state)
{
	if(state != displayUnselect ())
	{
		displayUnselect (state);
		checkRebuild ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInMenuParam::setCategory1 (const PlugInCategory& _category)
{
	if(!category1.equals (_category))
	{
		category1 = _category;
		checkRebuild ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PlugInCategory& PlugInMenuParam::getCategory1 () const
{
	return category1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInMenuParam::setCategory2 (const PlugInCategory& _category)
{
	if(!category2.equals (_category))
	{
		category2 = _category;
		checkRebuild ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PlugInCategory& PlugInMenuParam::getCategory2 () const
{
	return category2;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInMenuParam::setFilter (IObjectFilter* f)
{
	if(filter != f)
	{
		filter = f;
		checkRebuild ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectFilter* PlugInMenuParam::getFilter () const
{
	return filter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInMenuParam::canIncrement () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* PlugInMenuParam::getIcon (const PlugInClass& plugClass) const
{
	IImage* icon = nullptr;
	if(isDisplayExactIcon ())
	{
		icon = plugClass.getExactIcon (true);
		if(icon == nullptr && canIgnoreSubCategory ()) // second try without subcategory
			icon = plugClass.getExactIcon (false);
		if(icon == nullptr)
			icon = plugClass.getCategoryIcon ();
	}
	else
		icon = plugClass.getIcon (false);
	return icon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInMenuParam::extendMenu (IMenu& menu, StringID name)
{
	// set large menu variant
	if(isDisplayLargeMenu ())
		menu.setMenuAttribute (IMenu::kMenuVariant, IMenu::strLargeVariant);

	if(isStructuredMenu ())
		return;

	// set menu icons for all plug-in classes
	int numItems = menu.countItems ();
	for(int i = 0; i < numItems; i++)
	{
		IMenuItem* item = menu.getItem (i);
		PlugInClass* plugClass = getObject<PlugInClass> (i);
		if(plugClass)
		{
			IImage* icon = getIcon (*plugClass);
			item->setItemAttribute (IMenuItem::kItemIcon, Variant (icon));
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API PlugInMenuParam::getMenuType () const
{
	return isStructuredMenu () ? MenuPresentation::kExtended : MenuPresentation::kTree;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInMenuParam::onMenuKeyDown (const KeyEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInMenuParam::buildStructuredMenu (IMenu& menu, FolderNode& folderNode, IParameterMenuBuilder& builder)
{
	auto addClassItem = [&] (IMenu& menu, Browsable::PlugInClassNode* classNode)
	{
		PlugInClass plugClass (classNode->getClassDescription ());
		int index = getObjectIndex (plugClass);
		if(index >= 0)
		{
			IMenuItem* menuItem = builder.addValueItem (menu, *this, index);
			IImage* icon = getIcon (plugClass);
			menuItem->setItemAttribute (IMenuItem::kItemIcon, Variant (icon));
		}
	};

	for(auto node : iterate_as<BrowserNode> (folderNode.getContent ()))
	{
		if(Browsable::PlugInClassNode* classNode = ccl_cast<Browsable::PlugInClassNode> (node))
		{
			addClassItem (menu, classNode);
		}
		else if(FolderNode* subFolderNode = ccl_cast<FolderNode> (node))
		{
			IMenuItem* subMenuItem = builder.addSubMenu (menu, *this, node->getTitle ());
			buildStructuredMenu (*subMenuItem->getItemMenu (), *subFolderNode, builder);
		}
		else if(ccl_cast<Browsable::PlugInFavoritesNode> (node) || ccl_cast<Browsable::RecentPlugInsNode> (node))
		{
			ObjectList subNodes;
			subNodes.objectCleanup (true);
			node->getSubNodes (subNodes);

			IMenuItem* subMenuItem = builder.addSubMenu (menu, *this, node->getTitle ());
			subMenuItem->setItemAttribute (IMenuItem::kItemIcon, Variant (node->getIcon ()));
			IMenu* subMenu = subMenuItem->getItemMenu ();

			for(auto subNode: subNodes)
				if(Browsable::PlugInClassNode* classNode = ccl_cast<Browsable::PlugInClassNode> (subNode))
					addClassItem (*subMenu, classNode);
		}
		else if(ccl_cast<SeparatorNode> (node))
			menu.addSeparatorItem ();
		else
		{
			ASSERT (0)
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInMenuParam::buildMenu (IMenu& menu, IParameterMenuBuilder& builder)
{
	if(isStructuredMenu ())
	{
		AutoPtr<Browsable::PlugInCategoryNode> categoryNode (NEW Browsable::PlugInCategoryNode (category1.getCategory (), category1.getSubCategory ()));
		categoryNode->setCategory2 (category2.getCategory ());
		categoryNode->hasFavoritesFolder (true);
		categoryNode->hasRecentFolder (true);
		//was: categoryNode->setSorter (AutoPtr<NodeSorter> (PlugInSorterComponent::createVendorSorter ()));
		categoryNode->setSorter (AutoPtr<NodeSorter> (PlugInSorterComponent::createUserFolderSorter ()));
		categoryNode->build ();

		buildStructuredMenu (menu, *categoryNode, builder);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PlugInClass* PlugInMenuParam::getSelectedClass () const
{
	return getObject<PlugInClass> (getValue ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInMenuParam::selectClass (UIDRef classID, bool update)
{
	int index = getObjectIndex (PlugInClass (classID));
	if(index == -1)
	{
		if(classID == kNullUID && isDisplayUnselectItem ())
			index = 0;
		else
			return false;
	}

	setValue (index, update);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInMenuParam::notify (ISubject* s, MessageRef msg)
{
	if(msg == Signals::kClassCategoryChanged)
	{
		String category (msg[0].asString ());
		if(category == category1.getCategory () || category == category2.getCategory ())
			checkRebuild ();
	}
	SuperClass::notify (s, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInMenuParam::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == MenuPopupSelectorBehavior::kMustCloseMenuOnSelect)
	{
		var = true;
		return true;
	}

	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (PlugInMenuParam)
	DEFINE_METHOD_ARGS ("setCategory", "category: string")
	DEFINE_METHOD_ARGS ("setCategory2", "category: string")
	DEFINE_METHOD_ARGS ("setDisplaySorted", "state: bool")
	DEFINE_METHOD_ARGS ("setDisplayUnselectItem", "state: bool")
	DEFINE_METHOD_ARGR ("getSelectedClass", "", "string")
	DEFINE_METHOD_ARGR ("selectClass", "cid: UID | string", "bool")
END_METHOD_NAMES (PlugInMenuParam)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInMenuParam::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "setCategory")
	{
		setCategory1 (msg[0].asString ());
		return true;
	}
	if(msg == "setCategory2")
	{
		setCategory2 (msg[0].asString ());
		return true;
	}
	if(msg == "setDisplaySorted")
	{
		setDisplaySorted (msg[0].asBool ());
		return true;
	}
	if(msg == "setDisplayUnselectItem")
	{
		setDisplayUnselectItem (msg[0].asBool ());
		return true;
	}
	if(msg == "getSelectedClass")
	{
		String cidString;
		if(const PlugInClass* plugClass = getSelectedClass ())
			plugClass->getClassID ().toString (cidString);
		returnValue = cidString;
		returnValue.share ();
		return true;
	}
	if(msg == "selectClass")
	{
		UIDBytes cid = Boxed::UID::fromVariant (msg[0]);
		returnValue = selectClass (cid);
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// PlugInCategoryParam
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PlugInCategoryParam, MenuParam)

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInCategoryParam::PlugInCategoryParam (StringID name)
: MenuParam (name)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInCategoryParam::addCategory (const PlugInCategory& category)
{
	appendObject (NEW PlugInCategory (category));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PlugInCategory* PlugInCategoryParam::getSelectedCategory () const
{
	return getObject<PlugInCategory> (getValue ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCategoryParam::selectCategory (const PlugInCategory& category, bool update)
{
	int index = getObjectIndex (category);
	if(index == -1)
		return false;

	setValue (index, update);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInCategoryParam::extendMenu (IMenu& menu, StringID name)
{
	// set the icons for all plug-in categories
	int numItems = menu.countItems ();
	for(int i = 0; i < numItems; i++)
	{
		IMenuItem* item = menu.getItem (i);
		PlugInCategory* category = getObject<PlugInCategory> (i);
		if(category)
			item->setItemAttribute (IMenuItem::kItemIcon, Variant (category->getIcon ()));
	}
}

//************************************************************************************************
// PlugInSelector
//************************************************************************************************

DEFINE_CLASS (PlugInSelector, Component)
DEFINE_CLASS_UID (PlugInSelector, 0xf7447d54, 0x73fa, 0x4930, 0x8e, 0x83, 0x96, 0x0e, 0x8f, 0xba, 0x8c, 0x2f)
DEFINE_CLASS_NAMESPACE (PlugInSelector, "Host")

//////////////////////////////////////////////////////////////////////////////////////////////////

SharedPtr<IParameter> PlugInSelector::globalPopupModeParam;

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInSelector::setPopupModeParam (IParameter* param)
{
	globalPopupModeParam = param;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInSelector::PlugInSelector (StringRef classCategory1, StringRef classCategory2)
: Component (CCLSTR ("PlugInSelector")),
  plugInSelectorPopup (nullptr),
  runningModal (false)
{
	PlugInMenuParam* plugList = NEW PlugInMenuParam (CSTR ("plugList"), classCategory1, String::kEmpty, true); // auto-rebuild
	if(!classCategory2.isEmpty ())
		plugList->setCategory2 (PlugInCategory (classCategory2));
	plugList->setDisplaySorted (true);
	paramList.add (plugList, kPlugList);

	IAliasParameter* modeParam = paramList.addAlias ("selectorMode", Tag::kSelectorMode);
	if(globalPopupModeParam)
		modeParam->setOriginal (globalPopupModeParam);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API PlugInSelector::getObject (StringID name, UIDRef classID)
{
	if(name == "PlugInBrowser")
	{
		if(!plugInSelectorPopup)
		{
			plugInSelectorPopup = NEW PlugInSelectorPopup (*this);
			addComponent (plugInSelectorPopup);
		}
		return ccl_as_unknown (plugInSelectorPopup);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInSelector::isEnabled () const
{
	if(IParameter* list = paramList.byTag (kPlugList))
		return list->isEnabled () != 0;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInSelector::enable (bool state)
{
	if(IParameter* list = getPlugList ())
		list->enable (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInSelector::setStructuredMenu (bool state)
{
	paramList.byTag<PlugInMenuParam> (kPlugList)->isStructuredMenu (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInSelector::setFilter (IObjectFilter* filter)
{
	paramList.byTag<PlugInMenuParam> (kPlugList)->setFilter (filter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectFilter* PlugInSelector::getFilter () const
{
	return paramList.byTag<PlugInMenuParam> (kPlugList)->getFilter ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* PlugInSelector::getPlugList ()
{
	return paramList.byTag (kPlugList);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInSelector::setCategories (StringRef classCategory1, StringRef classCategory2)
{
	if(!classCategory1.isEmpty ())
		paramList.byTag<PlugInMenuParam> (kPlugList)->setCategory1 (PlugInCategory (classCategory1));
	if(!classCategory2.isEmpty ())
		paramList.byTag<PlugInMenuParam> (kPlugList)->setCategory2 (PlugInCategory (classCategory2));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PlugInCategory& PlugInSelector::getCategory1 () const
{
	return paramList.byTag<PlugInMenuParam> (kPlugList)->getCategory1 ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PlugInCategory& PlugInSelector::getCategory2 () const
{
	return paramList.byTag<PlugInMenuParam> (kPlugList)->getCategory2 ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInSelector::showMenu ()
{
	CString messageId = System::GetDesktop ().isInMode (IDesktop::kModalMode) ? IParameter::kReleaseFocus : IParameter::kRequestFocus;
	UnknownPtr<ISubject> (paramList.byTag (kPlugList))->signal (Message (messageId));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInSelector::runDialog (StringRef title)
{
	ITheme* theme = getTheme ();
	int result = DialogResult::kCancel;

	IView* view = theme ? theme->createView ("CCL/PlugInSelector", this->asUnknown ()) : nullptr;
	if(view)
	{
		if(!title.isEmpty ())
			view->setViewAttribute (IView::kTitle, title);

		runningModal = true;
		result = DialogBox ()->runDialog (view, Styles::kWindowCombinedStyleDialog, Styles::kDialogOkCancel);
		runningModal = false;
	}

	bool success = result == DialogResult::kOkay;
	if(success)
		signal (Message (kChanged));
	return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PlugInClass* PlugInSelector::getSelected () const
{
	return paramList.byTag<PlugInMenuParam> (kPlugList)->getSelectedClass ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInSelector::setSelected (UIDRef classID)
{
	PlugInMenuParam* plugList = paramList.byTag<PlugInMenuParam> (kPlugList);

	if(!plugList->selectClass (classID))
	{
		plugList->appendObject (NEW PlugInClass (classID));
		return plugList->selectClass (classID);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInSelector::paramChanged (IParameter* param)
{
	if(param && param->getTag () == kPlugList)
	{
		if(!runningModal)
		{
			signal (Message (kChanged));
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (PlugInSelector)
	DEFINE_METHOD_ARGS ("setCategories", "classCategory1: string, classCategory2: string")
	DEFINE_METHOD_ARGR ("getSelected", "", "string")
END_METHOD_NAMES (PlugInSelector)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInSelector::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "setCategories")
	{
		setCategories (msg[0].asString (), msg[1].asString ());
		return true;
	}
	if(msg == "getSelected")
	{
		String cidString;
		if(const PlugInClass* plugClass = getSelected ())
			plugClass->getClassID ().toString (cidString);
		returnValue = cidString;
		returnValue.share ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// PlugInSelectorBrowser
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PlugInSelectorBrowser, Browser)

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectList PlugInSelectorBrowser::instances;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_TERM (PlugInSelectorBrowser)
{
	PlugInSelectorBrowser::cleanupInstances ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInSelectorBrowser::cleanupInstances ()
{
	instances.objectCleanup (true);
	instances.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInSelectorBrowser& PlugInSelectorBrowser::instance (const PlugInSelector& selector)
{
	StringRef selectorId = selector.getSelectorID ();

	ListForEachObject (instances, PlugInSelectorBrowser, browser)
		if(browser->getSelectorId () == selectorId)
			return *browser;
	EndFor

	PlugInSelectorBrowser* browser = NEW PlugInSelectorBrowser (selector);
	browser->load (Storage (Settings::instance ().getAttributes (String ("PlugInSelectorBrowser/") << selectorId)));
	instances.add (browser);
	return *browser;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInSelectorBrowser::PlugInSelectorBrowser (const PlugInSelector& selector)
: Browser ("PlugInBrowser"),
  category (selector.getCategory1 ().getCategory ()),
  selectorId (selector.getSelectorID ()),
  popupResultConfirmed (false)
{
	StringRef category2 (selector.getCategory2 ().getCategory ());

	// configure browser
	setTreeStyle (StyleFlags (0, Styles::kItemViewBehaviorAutoSelect|
								 Styles::kItemViewBehaviorSelectExclusive|
								 Styles::kTreeViewAppearanceNoRoot|
								 Styles::kTreeViewBehaviorAutoExpand|
								 Styles::kItemViewBehaviorSelectFullWidth|
								 Styles::kItemViewBehaviorNoDoubleClick));
	displayTreeLeafs (true);
	showListView (false);

	// favorite column
	AutoPtr<IColumnHeaderList> columns (ccl_new<IColumnHeaderList> (ClassID::ColumnHeaderList));
	columns->addColumn (200, nullptr, nullptr, 0, 0);
	columns->addColumn (20, nullptr, Browsable::PlugInClassNode::kFavorite, 0, 0);
	columns->moveColumn (Browsable::PlugInClassNode::kFavorite, 0);
	setDefaultColumns (columns);
	hideColumnHeaders (true);

	// add categoryNode root node
	Browsable::PlugInCategoryNode* categoryNode = selector.createBrowserNode ();
	if(categoryNode == nullptr)
	{
		categoryNode = NEW Browsable::PlugInCategoryNode (category, "plugins", false);
		categoryNode->hasFavoritesFolder (true);
		categoryNode->hasRecentFolder (true);
		if(category2.isEmpty () == false)
			categoryNode->setCategory2 (category2);

		if(IObjectFilter* filter = selector.getBrowserFilter ())
			categoryNode->setClassFilter (filter);
	}
	addBrowserNode (categoryNode);
	setTreeRoot (categoryNode, false, false);

	// sorter
	PlugInSorterComponent* sorter = NEW PlugInSorterComponent;
	categoryNode->setSorterProvider (&sorter->getSorterProvider ());
	addComponent (sorter);

	// search
	AutoPtr<MultiSearchProvider> searchProvider (NEW MultiSearchProvider); // use MultiSearchProvider to hide startPoint of PluginSearchProvider (-> "hasLocation")
	PluginSearchProvider* plugSearchProvider = NEW PluginSearchProvider (category, categoryNode->getClassFilter ());
	searchProvider->addSearchProvider (plugSearchProvider);
	searchProvider->setUrlFilter (plugSearchProvider->getSearchResultFilter ());
	if(category2.isEmpty () == false)
	{
		plugSearchProvider = NEW PluginSearchProvider (category2, categoryNode->getClassFilter ());
		searchProvider->addSearchProvider (plugSearchProvider);
	}
	AutoPtr<CCL::SearchResultList> resultList (NEW SearchResultList (*this));

	SearchComponent* search = NEW SearchComponent;
	search->setResultViewer (resultList);
	search->setSearchProvider (searchProvider);
	search->setTypingTimeOutInitial (100); // shorter timeouts, as we steal the return key (that usually triggers search before timeout) for closing popup (it's an in-memory search anyway)
	search->setTypingTimeOutAgain (100);
	addComponent (search);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SearchComponent* PlugInSelectorBrowser::getSearchComponent ()
{
	return getComponent<SearchComponent> ("Search");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInSelectorBrowser::setProperty (MemberID propertyId, const Variant& var)
{
	#if 0 // not used anymore
	if(propertyId == "sorter")
	{
		CCL_PRINTF ("PlugInSelectorBrowser sorter: %s\n", MutableCString (var.asString ()).str ())

		// find NodeSorterComponent with given path and connect our category node to it's sorter provider
		if(NodeSorterComponent* sorter = unknown_cast<NodeSorterComponent> (RootComponent::instance ().lookupChild (var.asString ())))
			if(SortedNode* sortedNode = ccl_cast<SortedNode> (getRootNode ()->getContent ().first ()))
				sortedNode->setSorterProvider (&sorter->getSorterProvider ());

		return true;
	}
	#endif
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInSelectorBrowser::onNodeFocused (BrowserNode* node, bool inList)
{
	UID cid;

	if(Browsable::PlugInClassNode* classNode = ccl_cast<Browsable::PlugInClassNode> (node))
		cid = classNode->getClassDescription ().getClassID ();
	else if(Browsable::FileNode* fileNode = ccl_cast<Browsable::FileNode> (node))
		cid.fromCString (MutableCString (fileNode->getPath ()->getHostName ())); // search result node: class id in hostname

	setSelectedClassID (cid);

	SuperClass::onNodeFocused (node, inList);
}

//************************************************************************************************
// PlugInSelectorPopup
//************************************************************************************************

PlugInSelectorPopup::PlugInSelectorPopup (PlugInSelector& selector)
: Component ("PlugInSelectorPopup"),
  selector (selector),
  browser (PlugInSelectorBrowser::instance (selector))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API PlugInSelectorPopup::createPopupView (SizeLimit& limits)
{
	if(selector.isEnabled () == false)
		return nullptr;

	ITheme* theme = getTheme ();
	ASSERT (theme != nullptr)
	if(theme)
	{
		browser.setSelectedClassID (UID ());
		browser.setPopupResultConfirmed (false);
		browser.resetScrollState ();

		if(SearchComponent* search = browser.getSearchComponent ())
			search->clearSearchTerms ();

		static const CString formName ("CCL/PlugInBrowserPopup");

		IView* view = theme->createView (formName, browser.asUnknown ());
		if(view == nullptr)
		{
			ITheme* theme2 = System::GetThemeManager ().getApplicationTheme ();
			if(theme2 && theme2 != theme)
				view = theme2->createView (formName, browser.asUnknown ());
		}
		checkPopupLimits (view, limits);

		acceptOnMouseDown (true);
		acceptOnMouseUp (true); // for using the "drag" gesture as in a menu
		return view;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInSelectorPopup::hasPopupResult ()
{
	return browser.getSelectedClassID ().isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInSelectorPopup::onPopupClosed (Result result)
{
	if(result == IPopupSelectorClient::kOkay || browser.isPopupResultConfirmed ())
	{
		selector.setSelected (browser.getSelectedClassID ());
		selector.signal (Message (kChanged));
	}

	browser.save (Storage (Settings::instance ().getAttributes (String ("PlugInSelectorBrowser/") << browser.getSelectorId ())));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient::Result CCL_API PlugInSelectorPopup::onMouseDown (const MouseEvent& event, IWindow& popupWindow)
{
	// if we receive a mouse down, it means that the user did not "drag" into the menu; in this case we want the browser to stay open
	acceptOnMouseUp (false);

	// find mouse view, ignore if not on browser treeview
	IView* view = ViewBox (&popupWindow).getChildren ().findChildView (event.where, true);
	if(!UnknownPtr<IItemView> (view).isValid ())
		return kIgnore;

	return PopupSelectorClient::onMouseDown (event, popupWindow);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient::Result CCL_API PlugInSelectorPopup::onKeyDown (const KeyEvent& event)
{
	Result result = PopupSelectorClient::onKeyDown (event);
	if(result == kIgnore)
	{
		// We must prevent that key presses in the popup window (e.g. letters) are interpreted as key commands
		// This can happen if there is no focus view or the focus view doesn't handle the key (e.g. button)

		// find tree view and window (can be browser tree or search result, check which one is attached to window)
		IView* treeView = ViewBox (browser.getTreeView ());
		IWindow* window = treeView ? treeView->getIWindow () : nullptr;
		if(!window)
		{
			// try search result view
			SearchComponent* search = browser.getSearchComponent ();
			SearchResultList* resultList = search ? unknown_cast<SearchResultList> (search->getResultViewer ()) : nullptr;
			treeView = resultList ? ViewBox (resultList->getItemView ()).as<IView> () : nullptr;
			window = treeView ? treeView->getIWindow () : nullptr;
		}

		if(window)
		{
			// accept focus in tree view or search field - in all other cases give focus to tree view
			IView* focusView = window->getFocusIView ();
			if(!focusView || (focusView != treeView && ViewBox (focusView).getName () != "searchTerms" && ViewBox (focusView).getName () != "editString"))
			{
				treeView->takeFocus ();
				result = kIgnore;
			}
		}
	}
	return result;
}

//************************************************************************************************
// PlugInSortMethods
//************************************************************************************************

String PlugInSortMethods::getVendor (const IClassDescription& description)
{
	String vendorString;
	Variant classVendor;
	if(description.getClassAttribute (classVendor, Meta::kClassVendor))
		vendorString = classVendor.asString ();
	else
		vendorString = description.getModuleVersion ().getVendor ();

	if(vendorString.isEmpty ())
		vendorString = XSTR (UnknownVendor);
	return vendorString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PlugInSortMethods::getType (const IClassDescription& description)
{
	// type is first segment of subCategory path
	String typeString = description.getSubCategory ();
	int index = typeString.index (Url::strPathChar);
	if(index >= 0)
		typeString.truncate (index);
	return typeString;
}

//************************************************************************************************
// PlugInSorterByFolder
//************************************************************************************************

class PlugInSorterByFolder: public CCL::NodeSorter
{
public:
	bool getSortPath (String& path, const CCL::BrowserNode* node) override
	{
		if(const Browsable::PlugInClassNode* classNode = ccl_cast<Browsable::PlugInClassNode> (node))
		{
			Variant folder;
			if(classNode->getClassDescription ().getClassAttribute (folder, Meta::kClassFolder))
			{
				path = folder;
				return true;
			}
		}
		return false;
	}
};

//************************************************************************************************
// PlugInSorterByVendor
//************************************************************************************************

class PlugInSorterByVendor: public CCL::NodeSorter
{
public:
	bool getSortPath (String& path, const CCL::BrowserNode* node) override
	{
		if(const Browsable::PlugInClassNode* classNode = ccl_cast<Browsable::PlugInClassNode> (node))
		{
			path = PlugInSortMethods::getVendor (classNode->getClassDescription ());
			return true;
		}
		return false;
	}
};

//************************************************************************************************
// PlugInSorterBySubCategory
//************************************************************************************************

class PlugInSorterBySubCategory: public CCL::NodeSorter
{
public:
	bool getSortPath (String& path, const CCL::BrowserNode* node) override
	{
		if(const Browsable::PlugInClassNode* classNode = ccl_cast<Browsable::PlugInClassNode> (node))
		{
			// prefer localized subcategory if available
			classNode->getClassDescription ().getLocalizedSubCategory (path);
			return true;
		}
		return false;
	}
};

//************************************************************************************************
// PlugInSorterByUserFolder
//************************************************************************************************

class PlugInSorterByUserFolder: public CCL::NodeSorter
{
public:
	bool getSortPath (String& path, const CCL::BrowserNode* node) override
	{
		if(const Browsable::PlugInClassNode* classNode = ccl_cast<Browsable::PlugInClassNode> (node))
		{
			path = System::GetPluginPresentation ().getSortPath (classNode->getClassDescription ().getClassID ());
			return true;
		}
		return false;
	}
};

//************************************************************************************************
// PlugInSorterByType
//************************************************************************************************

class PlugInSorterByType: public CCL::NodeSorter
{
public:
	bool getSortPath (String& path, const CCL::BrowserNode* node) override
	{
		if(const Browsable::PlugInClassNode* classNode = ccl_cast<Browsable::PlugInClassNode> (node))
		{
			path = PlugInSortMethods::getType (classNode->getClassDescription ());
			return true;
		}
		return false;
	}
};

//************************************************************************************************
// PlugInSorterComponent
//************************************************************************************************

CCL::NodeSorter* PlugInSorterComponent::createUserFolderSorter ()
{
	return NEW PlugInSorterByUserFolder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::NodeSorter* PlugInSorterComponent::createVendorSorter ()
{
	return NEW PlugInSorterByVendor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::NodeSorter* PlugInSorterComponent::createSubCategorySorter ()
{
	return NEW PlugInSorterBySubCategory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInSorterComponent::PlugInSorterComponent ()
{
	enum SortBy
	{
		kFlat,
		kFolder,
		kVendor,
		kType,
		kSubCategory,
		kUserFolder = Browsable::PlugInCategoryNode::kUserFolderSorterTag
	};

	addSorter (NEW NodeSorterFlat, XSTR (Flat));
	addSorter (NEW PlugInSorterByUserFolder, XSTR (Folder), kUserFolder);
	addSorter (NEW PlugInSorterByVendor, XSTR (Vendor), kVendor);
	addSorter (NEW PlugInSorterByType, XSTR (Type), kType);
	selectSorterByTag (kVendor); // default is sort by vendor
}

//************************************************************************************************
// PluginMenu
//************************************************************************************************

void PluginMenu::popup (IParameter* menuParam, StringRef title, StringRef text)
{
	if(menuParam)
	{
		static bool menuActive = false;

		if(menuActive)
			return;

		ScopedVar<bool> scope (menuActive, true);

		// temporarily suppress the "StructuredMenu" option for this popup
		PlugInMenuParam* plugParam = unknown_cast<PlugInMenuParam> (menuParam);
		bool wasStructuredMenu = false;
		if(plugParam)
		{
			wasStructuredMenu = plugParam->isStructuredMenu ();
			plugParam->isStructuredMenu (false);
		}

		AutoPtr<IParameterMenuBuilder> builder = ccl_new<IParameterMenuBuilder> (ClassID::ParameterMenuBuilder);
		builder->construct (menuParam);
		AutoPtr<IMenu> menu = builder->buildIMenu ();
		DialogBox ()->runWithMenu (menu, title, text);

		if(plugParam)
			plugParam->isStructuredMenu (wasStructuredMenu);
	}
}

