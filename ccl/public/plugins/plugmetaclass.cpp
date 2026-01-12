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
// Filename    : ccl/public/plugins/plugmetaclass.cpp
// Description : Plug-in Meta Class
//
//************************************************************************************************

#include "ccl/public/plugins/plugmetaclass.h"

#include "ccl/public/storage/iurl.h"

using namespace CCL;

//************************************************************************************************
// PluginMetaClass::ResourceEntry
//************************************************************************************************

struct PluginMetaClass::ResourceEntry
{
	MutableCString id;
	MutableCString language;
	AutoPtr<IUrl> url;
	
	ResourceEntry (StringID id,
				   StringID language,
				   UrlRef urlRef)
	: id (id),
	  language (language)
	{
		urlRef.clone (url);
		ASSERT (url != nullptr)
	}
};

//************************************************************************************************
// PluginMetaClass
//************************************************************************************************

PluginMetaClass::~PluginMetaClass ()
{
	VectorForEach (resources, ResourceEntry*, r)
		delete r;
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PluginMetaClass::addResource (StringID id, UrlRef url, StringID language)
{
	resources.add (NEW ResourceEntry (id, language, url));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PluginMetaClass::getResourceLocation (IUrl& url, StringID id, StringID language)
{
	VectorForEach (resources, ResourceEntry*, r)
		if(r->id == id)
		{
			if(r->language.isEmpty () || r->language == language)
			{
				ASSERT (r->url)
				url.assign (*r->url);
				return kResultOk;
			}
		}
	EndFor
	return kResultFalse;
}
