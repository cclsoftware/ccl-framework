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
// Filename    : ccl/extras/modeling/modelbrowser.cpp
// Description : Class Model Browser
//
//************************************************************************************************

#define BROWSE_CLASS_RELATIONS 1 // show super class + derived classes

#include "ccl/extras/modeling/modelbrowser.h"

#include "ccl/app/components/searchcomponent.h"
#include "ccl/app/components/searchprovider.h"
#include "ccl/app/navigation/navigatorbase.h"

#include "ccl/base/message.h"
#include "ccl/base/collections/stringlist.h"
#include "ccl/base/signalsource.h"

#include "ccl/public/app/signals.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/framework/controlstyles.h"
#include "ccl/public/gui/framework/itheme.h"

using namespace CCL;

//************************************************************************************************
// ClassModelSearchProvider
//************************************************************************************************

class ClassModelSearchProvider: public SearchProvider
{
public:
	ClassModelSearchProvider (RepositoryNode* repositoryNode);

	// ISearchProvider
	ISearcher* createSearcher (ISearchDescription& description) override;
	IUnknown* customizeSearchResult (CustomizeArgs& args, IUnknown* resultItem) override;

private:
	SharedPtr<RepositoryNode> repositoryNode;

	StringID getIconName (StringRef protocol);
};

//************************************************************************************************
// ClassModelNavigator
//************************************************************************************************

class ClassModelNavigator: public NavigatorBase2
{
public:
	typedef NavigatorBase2 SuperClass;

	ClassModelNavigator (ClassModelBrowser& classBrowser);

	// NavigatorBase
	tresult CCL_API navigate (UrlRef url) override;
	tresult CCL_API refresh () override;

private:
	ClassModelBrowser& classBrowser;
};

//************************************************************************************************
// ClassModelSearchProvider
//************************************************************************************************

ClassModelSearchProvider::ClassModelSearchProvider (RepositoryNode* repositoryNode)
: repositoryNode (repositoryNode)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearcher* ClassModelSearchProvider::createSearcher (ISearchDescription& description)
{
	return repositoryNode && repositoryNode->getRepository () ? repositoryNode->getRepository ()->createSearcher (description) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* ClassModelSearchProvider::customizeSearchResult (CustomizeArgs& args, IUnknown* resultItem)
{
	UnknownPtr<IUrl> url (resultItem);
	if(url)
	{
		const char* iconName = getIconName (url->getProtocol ());
		if(iconName)
		{
			// replace slashes with dots for qualified names
			String path (url->getPath ());
			path.replace (Url::strPathChar, ".");
			args.presentation.setTitle (path);

			Browser* browser = repositoryNode->getBrowser ();
			ITheme* theme = browser ? browser->getTheme () : nullptr;
			args.presentation.setIcon (theme ? theme->getImage (iconName): nullptr);
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID ClassModelSearchProvider::getIconName (StringRef protocol)
{
	if(protocol == "class")
		return CSTR ("ModelElement:Class");
	if(protocol == "member")
		return CSTR ("ModelElement:Member");
	if(protocol == "method")
		return CSTR ("ModelElement:Method");
	if(protocol == "enum")
		return CSTR ("ModelElement:Enum");
	if(protocol == "enumerator")
		return CSTR ("ModelElement:Enumerator");
	if(protocol == "object")
		return CSTR ("ModelElement:Object");
	
	return CString::kEmpty;
}

//************************************************************************************************
// ClassModelNavigator
//************************************************************************************************

ClassModelNavigator::ClassModelNavigator (ClassModelBrowser& classBrowser)
: NavigatorBase2 ("ClassModelNavigator"),
  classBrowser (classBrowser)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ClassModelNavigator::navigate (UrlRef url)
{
	if(BrowserNode* node = classBrowser.findNodeWithUrl (url))
	{
		classBrowser.setFocusNode (node);
		return kResultOk;
	}
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ClassModelNavigator::refresh ()
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

// XSTRINGS_OFF     hint for xstring tool to skip this section

BEGIN_XSTRINGS ("ClassBrowser")
	XSTRING (Classes, "Classes")
	XSTRING (Objects, "Objects")
	XSTRING (Enumerations, "Enumerations")
	XSTRING (SuperClass, "Base Type")
	XSTRING (DerivedClasses, "Derived Types")
END_XSTRINGS

//************************************************************************************************
// ClassModelBrowser
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ClassModelBrowser, Browser)

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassModelBrowser::ClassModelBrowser ()
: Browser (String ("ClassBrowser"))
{
	scrollStyle.common |= Styles::kBorder;

	StyleFlags treeStyle (0, Styles::kTreeViewAppearanceNoRoot|
							 Styles::kTreeViewBehaviorAutoExpand|
							 Styles::kItemViewBehaviorSelectFullWidth|
							 Styles::kItemViewBehaviorNoDrag);
	setTreeStyle (treeStyle);

	displayTreeLeafs (true);
	showListView (false);
	canRefresh (false);

	addSearch ();
	setSearchProvider (NEW MultiSearchProvider);

	addComponent (NEW ClassModelNavigator (*this));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassModelBrowser::~ClassModelBrowser ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassModelBrowser::addRepository (Model::ClassRepository* repository)
{
	RepositoryNode* repositoryNode = NEW RepositoryNode;
	repositoryNode->setRepository (repository);
	
	addBrowserNode (repositoryNode);

	if(MultiSearchProvider* multiSearch = unknown_cast<MultiSearchProvider> (searchProvider))
		multiSearch->addSearchProvider (NEW ClassModelSearchProvider (repositoryNode));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Model::ClassRepository* ClassModelBrowser::findRepository (StringID name) const
{
	ForEach (rootNode->getContent (), Object, obj)
		RepositoryNode* repositoryNode = ccl_cast<RepositoryNode> (obj);
		Model::ClassRepository* repository = repositoryNode ? repositoryNode->getRepository () : nullptr;
		if(repository && repository->getName () == name)
			return repositoryNode->getRepository () ;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Model::Class* ClassModelBrowser::findClass (StringID name) const
{
	ForEach (rootNode->getContent (), Object, obj)
		RepositoryNode* repositoryNode = ccl_cast<RepositoryNode> (obj);
		Model::ClassRepository* repository = repositoryNode ? repositoryNode->getRepository () : nullptr;
		if(repository)
			if(const Model::Class* c = repository->findClass (name))
				return c;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassModelBrowser::canDisplayAsNode (const Model::Element& element)
{
	return !(ccl_cast<Model::Member> (&element) || ccl_cast<Model::Enumerator> (&element) || ccl_cast<Model::Property> (&element));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassModelBrowser::makeBrowserPath (MutableCString& path, const Model::Element& element)
{
	if(!canDisplayAsNode (element))
		return element.getEnclosure () ? makeBrowserPath (path, *element.getEnclosure ()) : 0;

	MutableCString className;
	MutableCString subFolder;
	if(const Model::Class* c = ccl_cast<Model::Class> (&element))
	{
		className = XSTR (Classes); // TODO: should be untranslated!
		subFolder = c->getGroupName ();
	}
	else if(const Model::Method* m = ccl_cast<Model::Method> (&element))
	{
		if(m->getEnclosure () && makeBrowserPath (path, *m->getEnclosure ()))
		{
			path += "/";
			path += element.getName ();
			return true;
		}
		else
			return false;
	}
	else if(const Model::Enumeration* c = ccl_cast<Model::Enumeration> (&element))
	{
		className = XSTR (Enumerations); // TODO: should be untranslated!
	}
	else if(const Model::ObjectElement* o = ccl_cast<Model::ObjectElement> (&element))
	{
		className = XSTR (Objects); // TODO: should be untranslated!
	}
	else
		return false;

	makePath (path, getTreeRoot ());

	if(Model::ClassRepository* repository = element.findRepository ())
	{
		path += "/";
		path += repository->getName ();
	}
	path += "/";
	path += className;
	if(!subFolder.isEmpty ())
	{
		path += "/";
		path += subFolder;
	}
	path += "/";
	path += element.getName ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* ClassModelBrowser::findNodeWithUrl (UrlRef url)
{
	// used for showing search result in context
	if(Model::ClassRepository* repository = findRepository (MutableCString (url.getHostName ())))
		if(const Model::Element* element = Model::ElementUrl::findElement (*repository, url))
		{
			MutableCString path;
			makeBrowserPath (path, *element);
			return findNode (path, true, false);
		}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassModelBrowser::revealElementNode (const Model::Element& element)
{
	MutableCString path;
	makeBrowserPath (path, element);

	if(BrowserNode* node = findNode (path, true, false))
		setFocusNode (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ClassModelBrowser::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "RevealClass")
	{
		MutableCString className (msg[0].asString ());
		MutableCString repositoryName (msg[1].asString ());
		const Model::Class* c = nullptr;
		if(repositoryName.isEmpty ())
			c = findClass (className);			
		else if(Model::ClassRepository* repository = findRepository (repositoryName))
			c = repository->findClass (className);
		if(c)
			revealElementNode (*c);
	}
	else if(msg == "RevealEnum")
	{
		MutableCString enumName (msg[0].asString ());
		MutableCString repositoryName (msg[1].asString ());
		if(Model::ClassRepository* repository = findRepository (repositoryName))
			if(const Model::Enumeration* e = (repository->findEnum (enumName)))
				revealElementNode (*e);
	}
	else
		SuperClass::notify (subject, msg);
}

//************************************************************************************************
// RepositoryNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (RepositoryNode, BrowserNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

RepositoryNode::RepositoryNode ()
: BrowserNode ("Repository")
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

RepositoryNode::~RepositoryNode ()
{
	if(repository)
		repository->removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RepositoryNode::setRepository (Model::ClassRepository* newRepository)
{
	if(repository != newRepository)
	{
		if(repository)
			repository->removeObserver (this);
		repository = newRepository;
		if(repository)
		{
			repository->addObserver (this);
			setTitle (String (repository->getName ()));
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RepositoryNode::getSubNodes (Container& children, NodeFlags flags)
{
	if(repository == nullptr) // not set
		return true;

	// Classes
	if(!repository->getClasses ().isEmpty ())
	{
		FolderNode* classFolder = NEW FolderNode (XSTR (Classes), this);
		classFolder->setIcon (getThemeIcon ("ModelElement:Folder"));

		StringList groupNames;
		repository->collectGroupNames (groupNames);
		int groupCount = groupNames.count ();
		if(groupCount > 1)
		{
			for(int i = 0; i < groupCount; i++)
			{
				String groupName = groupNames[i];
				FolderNode* parentNode = classFolder;
				if(!groupName.isEmpty ())
				{
					parentNode = NEW FolderNode (groupName);
					classFolder->add (parentNode);
				}

				ObjectArray classes;
				repository->collectGroupClasses (classes, groupName);
				ArrayForEach (classes, Model::Class, c)
					parentNode->add (NEW ClassBrowserNode (c));
				EndFor				
			}
		}
		else
		{
			ForEach (repository->getClasses (), Model::Class, c)
				classFolder->add (NEW ClassBrowserNode (c));
			EndFor
		}

		children.add (classFolder);
	}

	// Enumerations
	if(!repository->getEnumerations ().isEmpty ())
	{
		FolderNode* enumFolder = NEW FolderNode (XSTR (Enumerations), this);
		enumFolder->setIcon (getThemeIcon ("ModelElement:Folder"));
		ForEach (repository->getEnumerations (), Model::Enumeration, e)
			enumFolder->add (NEW EnumBrowserNode (e));
		EndFor
		children.add (enumFolder);
	}

	// Objects
	if(!repository->getObjects ().isEmpty ())
	{
		FolderNode* objectFolder = NEW FolderNode (XSTR (Objects), this);
		objectFolder->setIcon (getThemeIcon ("ModelElement:Folder"));
		ForEach (repository->getObjects (), Model::ObjectElement, o)
			objectFolder->add (NEW ObjectBrowserNode (o));
		EndFor
		children.add (objectFolder);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API RepositoryNode::notify (ISubject* subject, MessageRef msg)
{
	if(repository && subject == repository)
	{
		if(msg == kChanged)
		{
			setTitle (repository->getTitle ());
			if(Browser* browser = getBrowser ())
				browser->refreshNode (this);
			signal (Message (Browser::kNodeFocused, this->asUnknown ()));
		}
		else if(msg == kPropertyChanged)
		{
			setTitle (repository->getTitle ());
			
			if(Browser* browser = getBrowser ())
				if(browser->getFocusNode ())
					browser->signal (Message (Browser::kNodeFocused, browser->getFocusNode ()->asUnknown ()));

			SignalSource (Signals::kDocumentManager).signal (Message (Signals::kDocumentDirty));	
		}
	}
	else
		SuperClass::notify (subject, msg);
}

//************************************************************************************************
// ModelElementBrowserNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ModelElementBrowserNode, BrowserNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

ModelElementBrowserNode::ModelElementBrowserNode (Model::Element* modelElement, BrowserNode* parent)
: BrowserNode (String (), parent),
  modelElement (modelElement),
  hasDocumentation (false)
{
	ASSERT (modelElement)
	modelElement->addObserver (this);
	setTitle (String (modelElement->getName ()));
	hasDocumentation = modelElement->hasDocumentation ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ModelElementBrowserNode::~ModelElementBrowserNode ()
{
	modelElement->removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ModelElementBrowserNode::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged && isEqualUnknown (subject, ccl_as_unknown (modelElement)))
	{
		hasDocumentation = modelElement->hasDocumentation ();

		if(Browser* browser = getBrowser ())
			browser->redrawNode (this);
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ModelElementBrowserNode::getIcon ()
{
	if(!icon)
		icon = getThemeIcon (iconName);
	return icon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ModelElementBrowserNode::drawIconOverlay (const IItemModel::DrawInfo& info)
{
	if(hasDocumentation)
	{
		SolidBrush brush (Color (Colors::kGreen).setAlphaF (.2f));
		info.graphics.fillRect (info.rect, brush);
	}
	else if(modelElement->isNew ())
	{
		SolidBrush brush (Color (Colors::kRed).setAlphaF (.2f));
		info.graphics.fillRect (info.rect, brush);
	}
	return true;
}

//************************************************************************************************
// ClassBrowserNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ClassBrowserNode, ModelElementBrowserNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassBrowserNode::ClassBrowserNode (Model::Class* classElement, BrowserNode* parent)
: ModelElementBrowserNode (classElement, parent),
  relation (false),
  derivedClass (false)
{
	setIconName ("ModelElement:Class");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Model::Class* ClassBrowserNode::getClassElement () const
{
	return (Model::Class*)(Model::Element*)modelElement; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassBrowserNode::hasSubNodes () const
{
#if BROWSE_CLASS_RELATIONS
	if(const_cast<ClassBrowserNode*> (this)->browseRelations (nullptr))
		return true;
#endif

	return !getClassElement ()->getMethods ().isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassBrowserNode::canAutoExpand () const
{
	return !getClassElement ()->getMethods ().isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassBrowserNode::getSubNodes (Container& children, NodeFlags flags)
{
#if BROWSE_CLASS_RELATIONS
	browseRelations (&children);
#endif

	ForEach (getClassElement ()->getMethods (), Model::Method, method)
		children.add (NEW MethodBrowserNode (method, this));
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassBrowserNode::browseRelations (Container* children)
{
	bool result = false;
	Model::Class* thisClass = getClassElement ();
	Model::ClassRepository* repository = thisClass->getRepository ();
	ASSERT (repository != nullptr)

	if(isDerivedClass () == false)
		if(const Model::Class* superClass = repository->getSuperClass (thisClass))
		{
			result = true;
			if(children)
			{
				FolderNode* folder = NEW FolderNode (XSTR (SuperClass), this);
				ClassBrowserNode* subNode = NEW ClassBrowserNode (const_cast<Model::Class*> (superClass));
				subNode->setRelation (true);
				folder->add (subNode);
				children->add (folder);
			}
		}

	ObjectArray derivedClasses;
	repository->collectDerivedClasses (derivedClasses, thisClass);
	if(!derivedClasses.isEmpty ())
	{
		result = true;
		if(children)
		{
			FolderNode* folder = NEW FolderNode (XSTR (DerivedClasses), this);
			ArrayForEach (derivedClasses, Model::Class, c)
				ClassBrowserNode* subNode = NEW ClassBrowserNode (c);
				subNode->setDerivedClass (true);
				subNode->setRelation (true);
				folder->add (subNode);
			EndFor
			children->add (folder);
		}
	}
	return result;
}

//************************************************************************************************
// MethodBrowserNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (MethodBrowserNode, ModelElementBrowserNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

MethodBrowserNode::MethodBrowserNode (Model::Method* method, BrowserNode* parent)
: ModelElementBrowserNode (method, parent)
{
	setIconName ("ModelElement:Method");
}

//************************************************************************************************
// EnumBrowserNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (EnumBrowserNode, ModelElementBrowserNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

EnumBrowserNode::EnumBrowserNode (Model::Enumeration* e, BrowserNode* parent)
: ModelElementBrowserNode (e, parent)
{
	setIconName ("ModelElement:Enum");
}

//************************************************************************************************
// ObjectBrowserNode
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ObjectBrowserNode, ModelElementBrowserNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectBrowserNode::ObjectBrowserNode (Model::ObjectElement* object, BrowserNode* parent)
: ModelElementBrowserNode (object, parent)
{
	setIconName ("ModelElement:Object");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Model::ObjectElement* ObjectBrowserNode::getObjectElement () const
{
	return (Model::ObjectElement*)(Model::Element*)modelElement; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectBrowserNode::hasSubNodes () const
{
	Model::ObjectElement* object = getObjectElement ();
	return !object->getChildren ().isEmpty () || !object->getMethods ().isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectBrowserNode::getSubNodes (Container& children, NodeFlags flags)
{
	ForEach (getObjectElement ()->getChildren (), Model::ObjectElement, child)
		children.add (NEW ObjectBrowserNode (child, this));
	EndFor

	ForEach (getObjectElement ()->getMethods (), Model::Method, method)
		children.add (NEW MethodBrowserNode (method, this));
	EndFor
	return true;
}
