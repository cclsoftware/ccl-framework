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
// Filename    : ccl/gui/help/helptutorial.cpp
// Description : Help Tutorial
//
//************************************************************************************************

#include "ccl/gui/help/helptutorial.h"

#include "ccl/base/storage/file.h"
#include "ccl/base/storage/storage.h"

#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/text/itranslationtable.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// HelpTutorialCollection
//************************************************************************************************

DEFINE_CLASS_HIDDEN (HelpTutorialCollection, StorableObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

HelpTutorialCollection::HelpTutorialCollection()
{
	tutorials.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HelpTutorialCollection::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();	
	contentType = a.getString ("contentType");
	contentReference = a.getString ("content");
	a.unqueue (tutorials, nullptr, ccl_typeid<HelpTutorial> ());

	// tutorials can inherit attributes from collection
	if(!contentType.isEmpty () || !contentReference.isEmpty ())
		for(auto t : iterate_as<HelpTutorial> (tutorials))
		{
			if(t->getContentType ().isEmpty ())
				t->setContentType (contentType);
			if(t->getContentReference ().isEmpty ())
				t->setContentReference (contentReference);
		}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HelpTutorialCollection::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	a.set ("contentType", contentType);
	a.set ("content", contentReference);
	a.queue (nullptr, tutorials, Attributes::kShare);
	return true;
}

//************************************************************************************************
// HelpTutorial
//************************************************************************************************

DEFINE_CLASS (HelpTutorial, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

HelpTutorial::HelpTutorial ()
{
	steps.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API HelpTutorial::getID () const
{
	return id;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API HelpTutorial::getTitle () const
{
	return title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API HelpTutorial::getCategory () const
{
	return category;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HelpTutorial::detectContentPath (Url& contentPath) const
{
	ForEachStringToken (contentReference, CCLSTR (";"), fileName)
		Url path (fileName, baseFolder, Url::kDetect);
		if(File (path).exists ())
		{
			contentPath.assign (path);
			return true;
		}
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (HelpTutorial)
	DEFINE_METHOD_ARGR ("getID", "", "string")
	DEFINE_METHOD_ARGR ("getTitle", "", "string")
	DEFINE_METHOD_ARGR ("getCategory", "", "string")
END_METHOD_NAMES (HelpTutorial)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API HelpTutorial::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "getID")
	{
		returnValue = getID ();
		return true;
	}
	else if(msg == "getTitle")
	{
		returnValue = getTitle ();
		return true;
	}
	else if(msg == "getCategory")
	{
		returnValue = getCategory ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HelpTutorial::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	id = a.getString ("id");
	
	String originalTitle = a.getString ("title");
	String translatedTitle;
	
	String tableId = a.getString ("stringTableID");
	if(!tableId.isEmpty ())
	{
		if(auto table = System::GetLocaleManager ().getStrings (MutableCString (tableId)))
		{
			String translation;
			if(table->getStringWithUnicodeKey (translation, nullptr, originalTitle) == kResultOk)
				translatedTitle = translation;
		}
	}
	if(translatedTitle.isEmpty ())
	{
		MutableCString localizedKey ("title-");
		localizedKey += System::GetLocaleManager ().getLanguage ();
		title = a.getString (localizedKey);
	}
	
	title = translatedTitle.isEmpty () ? originalTitle : translatedTitle;

	category = a.getString ("category");

	contentType = a.getString ("contentType"); // optional, can be inherited from collection
	contentReference = a.getString ("content"); // optional, can be inherited from collection

	eventHandlerClass.fromString (a.getString ("eventHandler"));
	
	a.unqueue (steps, nullptr, ccl_typeid<Step> ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HelpTutorial::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	a.set ("id", id);
	a.set ("title", title);

	if(!category.isEmpty ())
		a.set ("category", category);
	
	a.set ("contentType", contentType);
	a.set ("content", contentReference);

	if(eventHandlerClass.isValid ())
		a.set ("eventHandler", UIDString (eventHandlerClass));
	
	a.queue (nullptr, steps, Attributes::kShare);
	return true;
}

//************************************************************************************************
// HelpTutorial::Step
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (HelpTutorial::Step, Object, "HelpTutorial.Step")

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HelpTutorial::Step::load (const Storage& storage)
{
	const Attributes& a = storage.getAttributes ();
	id = a.getString ("id");
	contentReference = a.getString ("content");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HelpTutorial::Step::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	a.set ("id", id);
	a.set ("content", contentReference);
	return true;
}

//************************************************************************************************
// HelpTutorial::StepContent
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (HelpTutorial::StepContent, kPrimaryText, "primary")
DEFINE_STRINGID_MEMBER_ (HelpTutorial::StepContent, kHeadingText, "heading")
DEFINE_STRINGID_MEMBER_ (HelpTutorial::StepContent, kCoverImage, "cover")
DEFINE_STRINGID_MEMBER_ (HelpTutorial::StepContent, kHorizontalContentImage, "horizontal")
DEFINE_STRINGID_MEMBER_ (HelpTutorial::StepContent, kVerticalContentImage, "vertical")
DEFINE_STRINGID_MEMBER_ (HelpTutorial::StepContent, kLinkUrl, "linkUrl")
DEFINE_STRINGID_MEMBER_ (HelpTutorial::StepContent, kLinkTitle, "linkTitle")
