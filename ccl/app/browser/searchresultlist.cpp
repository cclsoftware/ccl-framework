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
// Filename    : ccl/app/browser/searchresultlist.cpp
// Description : Search Result List
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/browser/searchresultlist.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/iscrollview.h"
#include "ccl/public/gui/framework/themeelements.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/systemservices.h"

#define SORTED_RESULTS 1

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Search")
	XSTRING (ShowInContext, "Show in Context")
	XSTRING (FolderCategory, "Folders")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace CCL {
namespace Browsable {

//************************************************************************************************
// ResultCategoryNode
//************************************************************************************************

class ResultCategoryNode: public ListViewItem
{
public:
	DECLARE_CLASS (ResultCategoryNode, ListViewItem)

	ResultCategoryNode ();

	PROPERTY_BOOL (expanded, Expanded)

	ObjectArray resultItems;
	int numResults;

	int compareWithResultNode (const SearchResultNode& resultNode) const;

	// Object
	int compare (const Object& obj) const override;
};

} // namespace Browsable
} // namespace CCL

using namespace CCL;
using namespace Browsable;

//************************************************************************************************
// SearchResultList
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SearchResultList, ListViewModel)

//////////////////////////////////////////////////////////////////////////////////////////////////

SearchResultList::SearchResultList ()
: fileTypeClassifier (&System::GetFileTypeRegistry ()),
  listStyle (0, Styles::kItemViewBehaviorSelection|
				Styles::kItemViewBehaviorSwallowAlphaChars|
				Styles::kItemViewAppearanceThumbnails|
				Styles::kListViewAppearanceAutoCenterIcons),
  expandSize (9),
  showCategories (true)
{
	getColumns ().addColumn (20, nullptr, kIconID);	// kIcon
	getColumns ().addColumn (300, nullptr, kTitleID);	// kTitle

	categoryNodes.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchResultList::setListStyle (StyleRef style)
{
	if(style != listStyle)
	{
		listStyle = style;

		if(getItemView ())
		{
			ViewBox (getItemView ()).setStyle (listStyle);
			signal (Message (kChanged)); // trigger ItemView::updateSize (invalidate is not enough when changing thumbnail flag)
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SearchResultList::isViewVisible ()
{
	if(UnknownPtr<IView> resultView = getItemView ())
		return ViewBox (resultView).isAttached ();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* SearchResultList::createView (const Rect& bounds)
{
	StyleFlags scrollStyle (0, Styles::kScrollViewBehaviorAutoHideBoth);
	ViewBox listControl (ClassID::ListControl, bounds, scrollStyle);
	listControl.setSizeMode (IView::kAttachAll);

	IItemView* listView = listControl.as<IItemView> ();
	listView->setModel (this);

	ViewBox (listView).setStyle (listStyle);
	setListViewType (getListViewType ()); // apply to view

	return listControl;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SearchResultList::isShowingResultList () const
{
	ViewBox listView (ccl_const_cast (this)->getItemView ());
	return listView && listView.isAttached ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchResultList::onSearchStart (ISearchDescription& description, ISearchProvider* provider)
{
	removeAll ();
	categoryNodes.removeAll ();

	signal (Message (kChanged));

	searchProvider = provider;

	// keep first search term for rating search results
	searchTerms = description.getSearchTerms ();
	int index = searchTerms.index (" ");
	if(index > 0)
		searchTerms.truncate (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchResultList::onSearchEnd (bool canceled)
{
	if(!canceled)
	{
		// select first result if none selected yet
		IItemView* itemView = getItemView ();
		if(itemView && itemView->getSelection ().isEmpty ())
			selectNextResult ();
	}

	searchProvider = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchResultList::onResultItemsAdded (const IUnknownList& resultItems)
{
	ObjectList selectedItems;
	getSelectedItems (selectedItems);

	ForEachUnknown (resultItems, unknown)
		UnknownPtr<IUrl> url (unknown);
		if(url)
		{
			if(searchProvider)
				if(IUrlFilter* urlFilter = searchProvider->getSearchResultFilter ())
					if(!urlFilter->matches (*url))
						continue;

			#if !SORTED_RESULTS
			// filter duplicates
			if(items.findIf ([&] (const Object* obj)
				{
					SearchResultNode* node = ccl_cast<SearchResultNode> (obj);
					return node && *node->getPath () == *url;
				}))
				continue;
			#endif
			SearchResultNode* node = createSearchResultNode (url);
			ASSERT (node)
			if(!node)
				continue;

			if(searchProvider)
			{
				ISearchProvider::CustomizeArgs args (*node, node->getCategory (), node->getSortString ());
				AutoPtr<IUnknown> dragObject = searchProvider->customizeSearchResult (args, unknown);
				node->setDragObject (dragObject);

				// prefer results starting with the first search term
				node->getSortString ().prepend (node->getTitle ().startsWith (searchTerms, false) ? "a" : "b");

				if(node->getCategory ().isEmpty ())
				{
					if(url->isFolder ())
						node->setCategory (XSTR (FolderCategory));
					else if(!fileTypeClassifier || !fileTypeClassifier->getFileTypeCategory (node->getCategory (), url->getFileType ()))
						node->setCategory (url->getFileType ().getDescription ());
				}
			}

			// disable file commands, they wouldn't work here anyway
			node->setFileCommandMask (0);
			if(url->isNativePath ())
			{
				node->canShowInShellBrowser (true);
				node->canOpenWithExternalShell (true);
			}

			ResultCategoryNode* categoryNode = (ResultCategoryNode*)categoryNodes.findIf ([&] (const Object* obj)
			{
				return ((ResultCategoryNode*)obj)->getTitle () == node->getCategory ();
			});

			if(!categoryNode)
			{
				categoryNode = NEW ResultCategoryNode;
				categoryNode->setTitle (node->getCategory ());
				categoryNode->setExpanded (!collapsedCategories.contains (categoryNode->getTitle ()));
				categoryNodes.addSorted (categoryNode);

				if(showCategories)
				{
					#if SORTED_RESULTS
					categoryNode->retain ();
					int insertIndex = items.getInsertIndex (categoryNode);
					insertItem (insertIndex, categoryNode);
					#else
					int categoryIndex = categoryNodes.index (categoryNode);
					ASSERT (categoryIndex >= 0)
					ResultCategoryNode* nextCategoryNode = (ResultCategoryNode*)categoryNodes.at (categoryIndex + 1);
					int insertIndex = nextCategoryNode ? items.index (nextCategoryNode) : -1;
					insertItem (insertIndex, categoryNode);
					#endif
				}
			}

			categoryNode->numResults++;
			if(!showCategories || categoryNode->isExpanded ())
			{
				// insert directly into list of visible items
				#if SORTED_RESULTS
				int index = items.getInsertIndex (node);
				Object* existing = items.at (index);
				if(existing && node->compare (*existing) == 0)
				{
					node->release ();
					continue; // duplicate!
				}
				else
					items.insertAt (index, node);
				#else
				// insert result before next categoryNode, or append
				int categoryIndex = categoryNodes.index (categoryNode);
				ASSERT (categoryIndex >= 0)
				ResultCategoryNode* nextCategoryNode = (ResultCategoryNode*)categoryNodes.at (categoryIndex + 1);
				int insertIndex = items.index (nextCategoryNode);
				insertItem (insertIndex, node);
				#endif
			}
			else
			{
				// insert into collapsed category node
				#if SORTED_RESULTS
				int index = categoryNode->resultItems.getInsertIndex (node);
				Object* existing = categoryNode->resultItems.at (index);
				if(existing && node->compare (*existing) == 0)
				{
					node->release ();
					continue; // duplicate!
				}
				else
					categoryNode->resultItems.insertAt (index, node);
				#else
				categoryNode->resultItems.add (node);
				#endif
			}
		}
	EndFor


	IItemView* itemView = getItemView ();
	if(itemView && !selectedItems.isEmpty ())
	{
		// select previously selected items again (with new indices)
		itemView->selectAll (false);
		ForEach (selectedItems, ListViewItem, item)
			ItemIndex index;
			getIndex (index, item);
			itemView->selectItem (index, true);
		EndFor
	}
	signal (Message (IItemModel::kItemAdded, 0));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchResultList::setShowCategories (bool state)
{
	if(state != showCategories)
	{
		showCategories = state;

		if(showCategories)
		{
			// insert category nodes into list
			for(auto categoryNode : iterate_as<ResultCategoryNode> (categoryNodes))
			{
				ASSERT (!items.contains (categoryNode))

				categoryNode->retain ();
				int insertIndex = items.getInsertIndex (categoryNode);
				insertItem (insertIndex, categoryNode);
			}
		}
		else
		{
			// remove category nodes from list
			for(auto categoryNode : iterate_as<ResultCategoryNode> (categoryNodes))
			{
				expandCategory (*categoryNode, true); // expand to put contained items into list

				bool removed = items.remove (categoryNode);
				ASSERT (removed)
				if(removed)
					categoryNode->release ();
			}
		}
		invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchResultList::expandCategory (ResultCategoryNode& categoryNode, bool state)
{
	if(state != categoryNode.isExpanded ())
	{
		categoryNode.setExpanded (state);

		// determine index of first result from this category
		int index = items.index (&categoryNode);
		ASSERT (index >= 0)
		index++;

		if(state)
		{
			// expand: move cached results back to ListModel
			ForEach (categoryNode.resultItems, BrowserNode, node)
				insertItem (index++, return_shared (node));
			EndFor
			categoryNode.resultItems.removeAll ();
			collapsedCategories.remove (categoryNode.getTitle ());
		}
		else
		{
			// collapse: move results from ListModel to categoryNode
			ASSERT (categoryNode.resultItems.isEmpty ())

			SearchResultNode* node = nullptr;
			while((node = ccl_cast<SearchResultNode> (getItem (index))) && node->getCategory () == categoryNode.getTitle ())
			{
				categoryNode.resultItems.add (node);
				items.removeAt (index);
			}
			collapsedCategories.addOnce (categoryNode.getTitle ());
		}
		signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* SearchResultList::findResultNode (UrlRef path) const
{
	return findResultNodeInternal (path, items);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* SearchResultList::findResultNodeInternal (UrlRef path, const ObjectArray& items) const
{
	// insert category nodes into list
	for(auto item : items)
	{
		if(SearchResultNode* resultNode = ccl_cast<SearchResultNode> (item))
		{
			if(resultNode->getFilePath ().isEqualUrl (path))
				return resultNode;
		}
		else if(ResultCategoryNode* categoryNode = ccl_cast<ResultCategoryNode> (item))
		{
			// search in items of collapsed category
			if(BrowserNode* resultNode = findResultNodeInternal (path, categoryNode->resultItems))
				return resultNode;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SearchResultList::showResultInContext (UrlRef url, bool checkOnly)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SearchResultList::appendResultContextMenu (IContextMenu& menu, UrlRef url)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool SearchResultList::interpretResultCommand (const CommandMsg& msg, UrlRef url)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SearchResultList::canSelectItem (ItemIndexRef index)
{
	if(ResultCategoryNode* categoryNode = ccl_cast<ResultCategoryNode> (resolve (index)))
		return false;
	else
		return SuperClass::canSelectItem (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SearchResultList::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	if(ResultCategoryNode* categoryNode = ccl_cast<ResultCategoryNode> (resolve (index)))
	{
		if(column == kTitle)
			drawTitle (info, String (categoryNode->getTitle ()) << " (" << categoryNode->numResults << ")", true, Font::kBold);
		else if(column == kIcon)
		{
			Rect rect (0, 0, expandSize, expandSize);
			rect.center (info.rect);
			ViewBox (getItemView ()).getTheme ().getPainter ().drawElement (info.graphics, rect, ThemeElements::kTreeViewExpandButton, 
				categoryNode->isExpanded () ? ThemeElements::kTreeItemExpanded : ThemeElements::kTreeItemCollapsed);
		}
		return true;
	}

	return SuperClass::drawCell (index, column, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SearchResultList::editCell (ItemIndexRef index, int column, const EditInfo& info)
{
	if(ResultCategoryNode* categoryNode = ccl_cast<ResultCategoryNode> (resolve (index)))
	{
		expandCategory (*categoryNode, !categoryNode->isExpanded ());
		return true;
	}
	return SuperClass::editCell (index, column, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SearchResultList::openItem (ItemIndexRef index, int column, const EditInfo& info)
{
	 // try to open files via systemshell, show in context as fallback (e.g. for folders)
	if(SearchResultNode* node = ccl_cast<SearchResultNode> (resolve (index)))
		return node->onOpen (false) || onShowResultInContext (index, false);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API SearchResultList::getItemBackground (ItemIndexRef index)
{
	if(ResultCategoryNode* categoryNode = ccl_cast<ResultCategoryNode> (resolve (index)))
		return CSTR ("category");
	else
		return SuperClass::getItemBackground (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SearchResultList::onSearchResultFocused (UrlRef url, IImage* icon, StringRef title)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SearchResultList::onItemFocused (ItemIndexRef index)
{
	if(SearchResultNode* node = ccl_cast<SearchResultNode> (resolve (index)))
		return onSearchResultFocused (*node->getPath (), node->getIcon (), node->getTitle ());

	return SuperClass::onItemFocused (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SearchResultList::appendItemMenu (IContextMenu& menu, ItemIndexRef item, const IItemSelection& selection)
{
	SearchResultNode* node = ccl_cast<SearchResultNode> (resolve (item));
	if(!node)
		return false;

	if(onShowResultInContext (item, true))
		menu.addCommandItem (XSTR (ShowInContext), CSTR ("Search"), CSTR ("Show Result in Context"), nullptr);

	menu.setContextID (CSTR ("SearchResult"));
	if(appendResultContextMenu (menu, node->getFilePath ()))
		return true;

	ObjectArray selectedNodes;
	selectedNodes.add (node);
	node->appendContextMenu (menu, &selectedNodes);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool SearchResultList::onShowResultInContext (ItemIndexRef item, bool checkOnly)
{
	SearchResultNode* node = ccl_cast<SearchResultNode> (resolve (item));
	if(!node)
		return false;

	bool result = showResultInContext (node->getFilePath (), checkOnly);
	if(result && !checkOnly)
		signal (Message (ISearchResultViewer::kCloseViewer));
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SearchResultList::interpretCommand (const CommandMsg& msg, ItemIndexRef item, const IItemSelection& selection)
{
	if(msg.category == "Search")
		if(msg.name == "Show Result in Context")
			return onShowResultInContext (item, msg.checkOnly ());

	if(SearchResultNode* node = ccl_cast<SearchResultNode> (resolve (item)))
		return interpretResultCommand (msg, node->getFilePath ());

	return SuperClass::interpretCommand (msg, item, selection);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SearchResultList::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IItemView::kViewAttached)
	{
		if(IItemView* itemView = getItemView ())
			expandSize = ViewBox (itemView).getVisualStyle ().getMetric ("expandSize", expandSize);
	}
	return SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchResultList::selectNextResult ()
{
	if(IItemView* itemView = getItemView ())
	{
		ItemIndex focusIndex (0);
		if(itemView->getFocusItem (focusIndex))
		{
			if(!itemView->getSelection ().isEmpty ()) // select first if nothing selected, next otherwise
				focusIndex = ItemIndex (focusIndex.getIndex () + 1);
		}

		// avoid a category node as focus node
		while(ccl_cast<ResultCategoryNode> (resolve (focusIndex)))
			focusIndex = ItemIndex (focusIndex.getIndex () + 1);

		itemView->setFocusItem (focusIndex);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SearchResultList::onEditNavigation (const KeyEvent& event, IView* view)
{
	// transfer focus to result ListView when "Arrow Down" pressed in search edit box
	if(event.vKey == VKey::kDown)
	{
		ViewBox vb (view);
		if(view && (vb.getName () == "searchTerms" || vb.getName () == "editString"))
			if(UnknownPtr<IView> resultView = getItemView ())
			{
				ViewBox (resultView).takeFocus ();

				// select first or next result
				selectNextResult ();
				return true;
			}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SearchResultList::showSelectedResultInContext ()
{
	if(IItemView* itemView = getItemView ())
	{
		ItemIndex focusIndex;
		if(itemView->getFocusItem (focusIndex))
			if(ccl_cast<SearchResultNode> (resolve (focusIndex)))
				return onShowResultInContext (focusIndex, false) != 0;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SearchResultNode* SearchResultList::createSearchResultNode (IUrl* url)
{
	AutoPtr<Url> u (NEW Url (*url));
	return NEW SearchResultNode (u);
}

//************************************************************************************************
// ResultCategoryNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ResultCategoryNode, ListViewItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

ResultCategoryNode::ResultCategoryNode ()
: expanded (true),
  numResults (0)
{
	resultItems.objectCleanup ();
}	

//////////////////////////////////////////////////////////////////////////////////////////////////

int ResultCategoryNode::compare (const Object& obj) const
{
	if(const ResultCategoryNode* node = ccl_cast<ResultCategoryNode> (&obj))
		return compareTitle (*node);

	ASSERT (ccl_cast<SearchResultNode> (&obj))
	return compareWithResultNode (static_cast<const SearchResultNode&> (obj));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ResultCategoryNode::compareWithResultNode (const SearchResultNode& resultNode) const
{
	int c = title.compare (resultNode.getCategory ());
	return c == 0 ? -1 : c; // category node before its own result nodes
}

//************************************************************************************************
// SearchResultNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SearchResultNode, FileNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

SearchResultNode::SearchResultNode (Url* path)
: FileNode (path)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef SearchResultNode::getCategory () const
{
	return category;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& SearchResultNode::getCategory ()
{
	return category;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchResultNode::setCategory (StringRef string)
{
	category = string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef SearchResultNode::getSortString () const
{
	return sortString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& SearchResultNode::getSortString ()
{
	return sortString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* SearchResultNode::createDragObject ()
{
	if(dragObject)
		return return_shared<IUnknown> (dragObject);
	else
		return SuperClass::createDragObject ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SearchResultNode::appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes)
{
	return SuperClass::appendContextMenu (contextMenu, selectedNodes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SearchResultNode::compare (const Object& obj) const
{
	static auto compareUrl = [] (const Url& u1, const Url& u2)
	{
		int cmp = u1.getHostName ().compare (u2.getHostName (), false);
		if(cmp != 0)
			return cmp;

		cmp = u1.getPath ().compareWithOptions (u2.getPath (), Text::kIgnoreCase|Text::kCompareNumerically);
		if(cmp != 0)
			return cmp;
		
		return u1.getProtocol ().compare (u2.getProtocol (), false);
	};

	if(const SearchResultNode* node = ccl_cast<SearchResultNode> (&obj))
	{
		// order by 1.) category, 2.) custom sort string, 3.) url
		int c = category.compare (node->getCategory ());
		if(c == 0)
			c = sortString.compare (node->getSortString ());

		return c != 0 ? c : compareUrl (*path, *node->path);
	}

	ASSERT (ccl_cast<ResultCategoryNode> (&obj))
	return - static_cast<const ResultCategoryNode&> (obj).compareWithResultNode (*this);
}
