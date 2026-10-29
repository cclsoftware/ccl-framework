//************************************************************************************************
//
// CCL Spy
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
// Filename    : docbrowser.cpp
// Description : Documentation Browser
//
//************************************************************************************************

#include "docbrowser.h"
#include "doceditor.h"
#include "plugversion.h"

#include "ccl/extras/modeling/modelbrowser.h"
#include "ccl/extras/modeling/modelinspector.h"

#include "ccl/app/browser/nodenavigator.h"

#include "ccl/base/message.h"
#include "ccl/base/development.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/filefilter.h"
#include "ccl/base/collections/stringdictionary.h"

#include "ccl/public/text/xmlcontentparser.h"

namespace Spy {

//************************************************************************************************
// ClassModelDocument
//************************************************************************************************

class ClassModelDocument: public CCL::Model::ClassRepository
{
public:
	PROPERTY_OBJECT (CCL::Url, path, Path)
};

//************************************************************************************************
// DocFileNode
//************************************************************************************************

class DocFileNode: public CCL::BrowserNode
{
public:
	DECLARE_CLASS (DocFileNode, BrowserNode)

	PROPERTY_SHARED_AUTO (DocumentationFile, file, File)
};

} // namespace Spy

using namespace Spy;
using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Spy")
	XSTRING (RelatedPages, "Related Pages")
END_XSTRINGS

//************************************************************************************************
// DocFileNode
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DocFileNode, BrowserNode)

//************************************************************************************************
// DocumentationBrowser
//************************************************************************************************
	
DEFINE_CLASS_HIDDEN (DocumentationBrowser, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentationBrowser::DocumentationBrowser ()
: Component ("DocumentationBrowser"),
  classBrowser (nullptr),
  elementInspector (nullptr),
  documentationEditor (nullptr),
  scanDone (false)
{
	addComponent (classBrowser = NEW ClassModelBrowser);
	classBrowser->addObserver (this);
	classBrowser->addComponent (NEW BrowserNodeNavigator (*classBrowser));

	addComponent (elementInspector = NEW ElementInspector);
	elementInspector->setBrowser (classBrowser);

	addComponent (documentationEditor = NEW DocumentationEditor);

	classModels.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentationBrowser::~DocumentationBrowser ()
{
	classBrowser->removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentationBrowser::scanModels ()
{
	if(scanDone == true)
		return;

	scanDone = true;

	Url modelFolder;
	FileFilter::getGlobalConditions ().setEntry ("plugid", PLUG_ID);
	bool canEdit = false;

	#if (1 && DEBUG)
	canEdit = true;
	
	GET_DEVELOPMENT_FOLDER_LOCATION (modelFolder, CCL_FRAMEWORK_DIRECTORY, "classmodels")
	
	#else
	modelFolder = ResourceUrl (CCLSTR ("models"), Url::kFolder);
	#endif

	ForEachFile (File (modelFolder).newIterator (IFileIterator::kFiles), path)
		if(path->getFileType () == Model::ClassRepository::getFileType ())
		{
			AutoPtr<ClassModelDocument> doc = NEW ClassModelDocument;
			if(doc->loadFromFile (*path))
			{
				doc->setPath (*path);
				classBrowser->addRepository (doc);
				classModels.add (doc.detach ());
			}
		}
	EndFor

	ASSERT (classModels.isEmpty () == false)

	elementInspector->setEnabled (canEdit);
	if(canEdit == true)
		Model::Element::setSaveMode (Model::Element::kPrepareDoc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentationBrowser::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "isDocFileFocused")
	{
		var = ccl_cast<DocFileNode> (classBrowser->getFocusNode ()) != nullptr;
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentationBrowser::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "scanModels")
	{
		scanModels ();
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentationBrowser::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Browser::kNodeFocused)
	{
		if(ModelElementBrowserNode* elementNode = unknown_cast<ModelElementBrowserNode> (msg[0].asUnknown ()))
			elementInspector->setInspectedElement (elementNode->getModelElement (), elementNode->getIcon ());
		else if(UnknownPtr<Browsable::IFileNode> searchResultNode = msg[0].asUnknown ()) // try search result node
		{
			UrlRef url = searchResultNode->getFilePath ();
			Model::ClassRepository* repository = classBrowser->findRepository (MutableCString (url.getHostName ()));
			const Model::Element* element = repository ? Model::ElementUrl::findElement (*repository, url) : nullptr;
			if(element)
			{
				if(!ClassModelBrowser::canDisplayAsNode (*element))
					element = element->getEnclosure ();

				BrowserNode* node = unknown_cast<BrowserNode> (searchResultNode);
				elementInspector->setInspectedElement (const_cast<Model::Element*> (element), node ? node->getIcon () : nullptr);
			}
		}
		else if(DocFileNode* docFileNode = unknown_cast<DocFileNode> (msg[0].asUnknown ()))
			documentationEditor->setFile (docFileNode->getFile ());

		signal (Message (kPropertyChanged));
	}
}
