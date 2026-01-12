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
// Filename    : ccl/app/browser/browserextender.cpp
// Description : Browser Extender
//
//************************************************************************************************

#include "ccl/app/browser/browserextender.h"
#include "ccl/app/browser/browsernode.h"

#include "ccl/base/message.h"

#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/plugins/stubobject.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// BrowserExtensionStub
//************************************************************************************************

class BrowserExtensionStub: public StubObject,
							public IBrowserExtension
{
public:
	DECLARE_STUB_METHODS (IBrowserExtension, BrowserExtensionStub)

	// IBrowserExtension
	tresult CCL_API extendBrowserNodeMenu (IBrowserNode* node, IContextMenu& menu, IUnknownList* selectedNodes) override
	{
		stubCalled = true;
		Variant returnValue;
		invokeMethod (returnValue, Message ("extendBrowserNodeMenu", node, &menu, selectedNodes));
		return returnValue.asResult ();
	}

	static bool stubCalled;
};

bool BrowserExtensionStub::stubCalled = false;

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Stub registration
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (BrowserStubClasses, kFirstRun)
{
	REGISTER_STUB_CLASS (IBrowserExtension, BrowserExtensionStub)
	return true;
}

//************************************************************************************************
// BrowserExtender
//************************************************************************************************

DEFINE_CLASS_HIDDEN (BrowserExtender, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserExtender::BrowserExtender ()
: Component (CCLSTR ("BrowserExtender"))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserExtender::~BrowserExtender ()
{
	ASSERT (extensions.isEmpty ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BrowserExtender::addExtension (IBrowserExtension* extension)
{
	extensions.append (extension);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BrowserExtender::addExtensionPlugIns (StringRef category)
{
	ForEachPlugInClass (PLUG_CATEGORY_BROWSEREXTENSION, description)
		if(description.getSubCategory () == category)
		{
			IBrowserExtension* extension = ccl_new<IBrowserExtension> (description.getClassID ());
			ASSERT (extension != nullptr)
			if(extension)
				addExtension (extension);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BrowserExtender::destroyAll ()
{
	ListForEach (extensions, IBrowserExtension*, e)
		if(Object* object = unknown_cast<Object> (e))
			object->release ();
		else
			ccl_release (e);
	EndFor
	extensions.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API BrowserExtender::terminate ()
{
	destroyAll ();

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BrowserExtender::extendBrowserNodeMenu (BrowserNode* node, IContextMenu& menu, Container* selectedNodes)
{
	if(extensions.isEmpty ())
		return false;

	AutoPtr<IUnknownList> selectedUnknowns; // might be kept by extensions!
	if(selectedNodes)
	{
		selectedUnknowns = NEW UnknownList;
		ForEach (*selectedNodes, BrowserNode, node)
			selectedUnknowns->add (node->asUnknown (), true);
		EndFor
	}

	BrowserExtensionStub::stubCalled = false;
	ListForEach (extensions, IBrowserExtension*, e)
		e->extendBrowserNodeMenu (node, menu, selectedUnknowns);
	EndFor

	if(BrowserExtensionStub::stubCalled) // TODO: replace by ccl_markGC()!?
		ccl_forceGC ();

	return true;
}
