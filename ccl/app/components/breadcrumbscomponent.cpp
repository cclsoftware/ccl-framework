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
// Filename    : ccl/app/components/breadcrumbscomponent.cpp
// Description : Breadcrumbs Component
//
//************************************************************************************************

#include "ccl/app/components/breadcrumbscomponent.h"
#include "ccl/app/params.h"

#include "ccl/base/collections/stringlist.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/message.h"
#include "ccl/base/boxedtypes.h"

#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/gui/iparameter.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum BreadcrumbTags
	{
		kTitle,
		kSelectFolder = 1000,	// indexed
		kSubFolders	  = 2000	// indexed
	};
};

//************************************************************************************************
// BreadcrumbsComponent::Segment
//************************************************************************************************

class BreadcrumbsComponent::Segment: public Component
{
public:
	Segment (BreadcrumbsComponent* breadcrumbs, UrlRef url, StringRef title);

	PROPERTY_OBJECT (Url, url, Url)

	void makeParams (int index);
};

//************************************************************************************************
// BreadcrumbsComponent::Segment
//************************************************************************************************

BreadcrumbsComponent::Segment::Segment (BreadcrumbsComponent* breadcrumbs, UrlRef url, StringRef title)
: url (url)
{
	paramList.setController (breadcrumbs);
	paramList.addString ("title", Tag::kTitle)->setValue (title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BreadcrumbsComponent::Segment::makeParams (int index)
{
	paramList.addParam ("selectFolder", Tag::kSelectFolder + index);
	paramList.addMenu ("subFolders", Tag::kSubFolders + index);
}

//************************************************************************************************
// BreadcrumbsComponent::SubFolderQuery
//************************************************************************************************

DEFINE_IID_ (BreadcrumbsComponent::ISubFolderQuery, 0x8c3a2b37, 0x93b9, 0x4464, 0xbe, 0x9f, 0x65, 0x21, 0xb2, 0xf0, 0xef, 0xea)

//////////////////////////////////////////////////////////////////////////////////////////////////

class BreadcrumbsComponent::SubFolderQuery: public Object,
											public ISubFolderQuery
{
public:
	StringRef parentPath;
	ObjectList subFolders;

	struct SubFolderItem: public Object
	{
		PROPERTY_STRING (name, Name)
		PROPERTY_STRING (title, Title)
		PROPERTY_SHARED_AUTO (IImage, icon, Icon)
	};

	SubFolderQuery (BreadcrumbsComponent& component, UrlRef folder)
	: parentPath (folder.getPath ())
	{
		subFolders.objectCleanup ();

		// send out message to query sub folders of selected path
		component.signal (Message (kQuerySubFolders, asUnknown ()));
	}

	// SubFolderQuery
	StringRef getParentPath () const override { return parentPath; }

	void addSubFolder (StringRef name, StringRef title, IImage* icon) override
	{
		SubFolderItem* item = NEW SubFolderItem;
		item->setName (name);
		item->setTitle (title);
		item->setIcon (icon);
		subFolders.add (item);
	}

	CLASS_INTERFACE (BreadcrumbsComponent::ISubFolderQuery, Object)
};

//************************************************************************************************
// BreadcrumbsComponent
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (BreadcrumbsComponent, Component)
DEFINE_STRINGID_MEMBER_ (BreadcrumbsComponent, kPathSelected, "pathSelected")
DEFINE_STRINGID_MEMBER_ (BreadcrumbsComponent, kQuerySubFolders, "querySubFolders")

//////////////////////////////////////////////////////////////////////////////////////////////////

BreadcrumbsComponent::BreadcrumbsComponent (StringRef name)
: Component (name)
{
	segments.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BreadcrumbsComponent::~BreadcrumbsComponent ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef BreadcrumbsComponent::getPath () const
{
	return path; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BreadcrumbsComponent::setPath (StringRef path, StringRef displayPath, bool forceUpdate)
{
	if(forceUpdate|| path != getPath ())
	{
		this->path = path;

		segments.removeAll ();

		Url url (nullptr, nullptr, path, Url::kFolder);
		Url displayUrl (nullptr, nullptr, displayPath.isEmpty () ? path : displayPath, Url::kFolder);

		while(!url.getPath ().isEmpty ())
		{
			String title;
			displayUrl.getName (title);

			segments.prepend (NEW Segment (this, url, title));

			url.ascend ();
			displayUrl.ascend ();
		}

		int index = 0;
		for(auto segment : iterate_as<Segment> (segments))
			segment->makeParams (index++);

		if(Segment* lastSegment = static_cast<Segment*> (segments.getLast ()))
		{
			// disable (hide) last subFolder menu if empty
			SubFolderQuery query (*this, lastSegment->getUrl ());
			if(query.subFolders.isEmpty ())
				lastSegment->findParameter ("subFolders")->enable (false);
		}

		signal (Message (kPropertyChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BreadcrumbsComponent::Segment* BreadcrumbsComponent::getSegment (int index) const
{
	return static_cast<Segment*> (segments.at (index));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* CCL_API BreadcrumbsComponent::findChild (StringRef id) const
{
	int64 index = -1;
	if(id.getIntValue (index))
		return getSegment ((int)index);

	return SuperClass::findChild (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BreadcrumbsComponent::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "hasSegments")
	{
		var = !segments.isEmpty ();
		return true;
	}
	else if(propertyId == "numSegments")
	{
		var = segments.count ();
		return true;
	}
	else
		return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BreadcrumbsComponent::paramChanged (IParameter* param)
{
	int tag = param->getTag ();
	int index = tag % 1000;
	if(tag >= 1000)
		tag = (tag / 1000) * 1000;

	switch(tag)
	{
	case Tag::kSelectFolder:
		if(Segment* segment = getSegment (index))
		{
			StringRef selectedPath = segment->getUrl ().getPath ();
			signal (Message (kPathSelected, selectedPath));
		}
		return true;
	}
	return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API BreadcrumbsComponent::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IParameter::kExtendMenu)
	{
		UnknownPtr<IParameter> param (subject);
		int segmentIndex = param->getTag () % 1000;
		SharedPtr<Segment> segment = getSegment (segmentIndex);

		UnknownPtr<IMenu> menu (msg.getArg (0));
		if(menu && segment)
		{
			StringRef selectedPath = segment->getUrl ().getPath ();

			String nextSegmentName;
			if(Segment* nextSegment = getSegment (segmentIndex + 1))
				nextSegment->getUrl ().getName (nextSegmentName);

			// send out message to query sub folders of selected path
			SubFolderQuery query (*this, segment->getUrl ());

			// build subFolder menu
			for(auto subFolder : iterate_as<SubFolderQuery::SubFolderItem> (query.subFolders))
			{
				StringRef title = subFolder->getTitle ();
				StringRef displayTitle (title.isEmpty () ? subFolder->getName () : title);

				IMenuItem* menuItem = menu->addCommandItem (displayTitle, CSTR ("Edit"), CSTR ("SubFolder"),
					makeCommandDelegate ([this, segment] (const CommandMsg& msg, VariantRef data)
					{
						if(!msg.checkOnly ())
						{
							String subFolder = data.asString ();
							if(!subFolder.isEmpty ())
							{
								// select subFolder
								Url subFolderUrl (segment->getUrl ());
								subFolderUrl.descend (subFolder, Url::kFolder);
								signal (Message (kPathSelected, subFolderUrl.getPath ()));
							}
						}
						return true;
					}, String (title)));

				if(menuItem)
				{
					if(subFolder->getName () == nextSegmentName)
						menuItem->setItemAttribute (IMenuItem::kItemChecked, true);

					if(subFolder->getIcon ())
						menuItem->setItemAttribute (IMenuItem::kItemIcon, subFolder->getIcon ());
				}
			}
		}
	}
	else
		return SuperClass::notify (subject, msg);
}
