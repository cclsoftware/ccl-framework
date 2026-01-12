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
// Filename    : ccl/app/components/breadcrumbscomponent.h
// Description : Breadcrumbs Component
//
//************************************************************************************************

#ifndef _ccl_breadcrumbscomponent_h
#define _ccl_breadcrumbscomponent_h

#include "ccl/app/component.h"

#include "ccl/base/collections/objectlist.h"
#include "ccl/base/storage/url.h"

namespace CCL {

interface IImage;

//************************************************************************************************
// BreadcrumbsComponent
/** Manages display and editing of a path presented as breadcrumbs. */
//************************************************************************************************

class BreadcrumbsComponent: public Component
{
public:
	DECLARE_CLASS_ABSTRACT (BreadcrumbsComponent, Component)

	BreadcrumbsComponent (CCL::StringRef name = CCLSTR ("Breadcrumbs"));
	~BreadcrumbsComponent ();

	void setPath (StringRef path, StringRef displayPath = nullptr, bool forceUpdate = false);
	StringRef getPath () const;

	DECLARE_STRINGID_MEMBER (kPathSelected)		///< arg[0]: path string
	DECLARE_STRINGID_MEMBER (kQuerySubFolders)	///< arg[0]: ISubFolderQuery

	interface ISubFolderQuery;

private:
	ObjectList segments;
	String path;

	class Segment;
	class SubFolderQuery;

	Segment* getSegment (int index) const;

	// Component
	IObjectNode* CCL_API findChild (StringRef id) const override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
};

//************************************************************************************************
// BreadcrumbsComponent::ISubFolderQuery
//************************************************************************************************

interface BreadcrumbsComponent::ISubFolderQuery: IUnknown
{
	/** The parent path whose subFolders are queried. */
	virtual StringRef getParentPath () const = 0;

	/** Add a subFolder. */
	virtual void addSubFolder (StringRef name, StringRef title = nullptr, IImage* icon = nullptr) = 0;

	DECLARE_IID (ISubFolderQuery)
};

} // namespace CCL

#endif // _ccl_breadcrumbscomponent_h
