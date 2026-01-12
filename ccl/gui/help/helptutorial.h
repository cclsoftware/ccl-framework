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
// Filename    : ccl/gui/help/helptutorial.h
// Description : Help Tutorial
//
//************************************************************************************************

#ifndef _ccl_helptutorial_h
#define _ccl_helptutorial_h

#include "ccl/public/gui/framework/ihelpmanager.h"
#include "ccl/public/gui/graphics/iimage.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/storableobject.h"
#include "ccl/base/collections/objectarray.h"

namespace CCL {

//************************************************************************************************
// HelpTutorialCollection
//************************************************************************************************

class HelpTutorialCollection: public StorableObject
{
public:
	DECLARE_CLASS (HelpTutorialCollection, StorableObject)

	HelpTutorialCollection ();

	PROPERTY_STRING (contentType, ContentType)
	PROPERTY_STRING (contentReference, ContentReference)

	const ObjectArray& getTutorials () const { return tutorials; }

	// StorableObject
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	ObjectArray tutorials;
};

//************************************************************************************************
// HelpTutorial
//************************************************************************************************

class HelpTutorial: public Object,
					public IHelpTutorial
{
public:
	DECLARE_CLASS (HelpTutorial, Object)
	DECLARE_METHOD_NAMES (HelpTutorial)

	HelpTutorial ();
	
	class Step;
	class StepContent;

	PROPERTY_OBJECT (UID, eventHandlerClass, EventHandlerClassUID)

	PROPERTY_STRING (contentType, ContentType)
	PROPERTY_STRING (contentReference, ContentReference)

	PROPERTY_OBJECT (Url, baseFolder, BaseFolder)
	bool detectContentPath (Url& contentPath) const;

	const ObjectArray& getSteps () const { return steps; }

	// IHelpTutorial
	StringRef CCL_API getID () const override;
	StringRef CCL_API getTitle () const override;
	StringRef CCL_API getCategory () const override;

	// Object
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	CLASS_INTERFACE (IHelpTutorial, Object)

protected:
	String id;
	String title;
	String category;
	ObjectArray steps;
};

//************************************************************************************************
// HelpTutorial::Step
//************************************************************************************************

class HelpTutorial::Step: public Object
{
public:
	DECLARE_CLASS (Step, Object)

	PROPERTY_STRING (id, ID)
	PROPERTY_STRING (contentReference, ContentReference)

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
};

//************************************************************************************************
// HelpTutorial::StepContent
//************************************************************************************************

class HelpTutorial::StepContent
{
public:
	DECLARE_STRINGID_MEMBER (kPrimaryText)
	DECLARE_STRINGID_MEMBER (kHeadingText)
	DECLARE_STRINGID_MEMBER (kCoverImage)
	DECLARE_STRINGID_MEMBER (kHorizontalContentImage)
	DECLARE_STRINGID_MEMBER (kVerticalContentImage)
	DECLARE_STRINGID_MEMBER (kLinkTitle)
	DECLARE_STRINGID_MEMBER (kLinkUrl)

	PROPERTY_STRING (primaryText, PrimaryText)
	PROPERTY_STRING (headingText, HeadingText)
	PROPERTY_SHARED_AUTO (IImage, coverImage, CoverImage)
	PROPERTY_SHARED_AUTO (IImage, horizontalContentImage, HorizontalContentImage)
	PROPERTY_SHARED_AUTO (IImage, verticalContentImage, VerticalContentImage)
	PROPERTY_STRING (linkTitle, LinkTitle)
	PROPERTY_STRING (linkUrl, LinkUrl)
};

} // namespace CCL

#endif // _ccl_helptutorial_h
