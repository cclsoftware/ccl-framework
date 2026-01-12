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
// Filename    : ccl/extras/modeling/modelbrowser.h
// Description : Class Model Browser
//
//************************************************************************************************

#ifndef _modelbrowser_h
#define _modelbrowser_h

#include "ccl/app/browser/browser.h"
#include "ccl/app/browser/browsernode.h"

#include "ccl/extras/modeling/classrepository.h"

namespace CCL {

//************************************************************************************************
// ClassModelBrowser
//************************************************************************************************

class ClassModelBrowser: public CCL::Browser
{
public:
	DECLARE_CLASS_ABSTRACT (ClassModelBrowser, Browser)

	ClassModelBrowser ();
	~ClassModelBrowser ();

	void addRepository (Model::ClassRepository* repository);
	Model::ClassRepository* findRepository (StringID name) const;
	const Model::Class* findClass (StringID name) const;

	static bool canDisplayAsNode (const Model::Element& element);

	// Browser
	BrowserNode* findNodeWithUrl (UrlRef url) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	bool makeBrowserPath (MutableCString& path, const Model::Element& element);
	void revealElementNode (const Model::Element& element);
};

//************************************************************************************************
// RepositoryNode
//************************************************************************************************

class RepositoryNode: public BrowserNode
{
public:
	DECLARE_CLASS_ABSTRACT (RepositoryNode, BrowserNode)

	RepositoryNode ();
	~RepositoryNode ();

	void setRepository (Model::ClassRepository* repository);
	Model::ClassRepository* getRepository () const { return repository; }

	// BrowserNode
	bool isFolder () const override { return true; }
	bool hasSubNodes () const override { return true; }
	bool getSubNodes (Container& children, NodeFlags flags) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	SharedPtr<Model::ClassRepository> repository;
};

//************************************************************************************************
// ModelElementBrowserNode
//************************************************************************************************

class ModelElementBrowserNode: public BrowserNode
{
public:
	DECLARE_CLASS_ABSTRACT (ModelElementBrowserNode, BrowserNode)

	ModelElementBrowserNode (Model::Element* modelElement, BrowserNode* parent = nullptr);
	~ModelElementBrowserNode ();

	PROPERTY_MUTABLE_CSTRING (iconName, IconName)
	Model::Element* getModelElement () const { return modelElement; }

	// ModelElementBrowserNode
	IImage* getIcon () override;
	bool drawIconOverlay (const IItemModel::DrawInfo& info) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	SharedPtr<Model::Element> modelElement;
	bool hasDocumentation;
};

//************************************************************************************************
// ClassBrowserNode
//************************************************************************************************

class ClassBrowserNode: public ModelElementBrowserNode
{
public:
	DECLARE_CLASS_ABSTRACT (ClassBrowserNode, ModelElementBrowserNode)

	ClassBrowserNode (Model::Class* classElement, BrowserNode* parent = nullptr);
	
	PROPERTY_BOOL (relation, Relation)
	PROPERTY_BOOL (derivedClass, DerivedClass)

	Model::Class* getClassElement () const;

	// ModelElementBrowserNode
	bool isFolder () const override { return true; }
	bool hasSubNodes () const override;
	bool canAutoExpand () const override;
	bool getSubNodes (Container& children, NodeFlags flags) override;

protected:
	bool browseRelations (Container* children);
};

//************************************************************************************************
// MethodBrowserNode
//************************************************************************************************

class MethodBrowserNode: public ModelElementBrowserNode
{
public:
	DECLARE_CLASS_ABSTRACT (MethodBrowserNode, ModelElementBrowserNode)

	MethodBrowserNode (Model::Method* method, BrowserNode* parent = nullptr);

	// ModelElementBrowserNode
	bool isFolder () const override { return false; }
	bool hasSubNodes () const override { return false; }
};

//************************************************************************************************
// EnumBrowserNode
//************************************************************************************************

class EnumBrowserNode: public ModelElementBrowserNode
{
public:
	DECLARE_CLASS_ABSTRACT (EnumBrowserNode, ModelElementBrowserNode)

	EnumBrowserNode (Model::Enumeration* e, BrowserNode* parent = nullptr);

	// ModelElementBrowserNode
	bool isFolder () const override { return true; }
	bool hasSubNodes () const override { return false; }
};

//************************************************************************************************
// ObjectBrowserNode
//************************************************************************************************

class ObjectBrowserNode: public ModelElementBrowserNode
{
public:
	DECLARE_CLASS_ABSTRACT (ObjectBrowserNode, ModelElementBrowserNode)

	ObjectBrowserNode (Model::ObjectElement* object, BrowserNode* parent = nullptr);

	Model::ObjectElement* getObjectElement () const;

	// ModelElementBrowserNode
	bool isFolder () const override { return true; }
	bool hasSubNodes () const override;
	bool getSubNodes (Container& children, NodeFlags flags) override;
};

} // namespace CCL

#endif // _modelbrowser_h
