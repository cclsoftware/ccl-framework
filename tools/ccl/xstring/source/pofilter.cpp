//************************************************************************************************
//
// CCL String Extractor
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
// Filename    : pofilter.cpp
// Description : Portable Object Filter
//
//************************************************************************************************

#include "pofilter.h"
#include "xstringparser.h"

#include "ccl/base/storage/textfile.h"
#include "ccl/public/text/translationformat.h"

using namespace CCL;
using namespace XString;

//************************************************************************************************
// PortableObjectFilter
//************************************************************************************************

PortableObjectFilter::PortableObjectFilter (Bundle& bundle, UrlRef path)
: Filter (bundle, path)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PortableObjectFilter::create ()
{
	TextFile file (path, Text::kASCII, Text::kLFLineFormat);	
	ASSERT (file.isValid ())
	if(!file.isValid ())
		return false;

	PortableObjectFormat::FormatWriter formatWriter (file);

	ForEach (bundle, Translated, t)

		Translated::ScopeList scopes;
		t->getScopes (scopes);
		if(scopes.isEmpty ())
			scopes.add (MutableCString ());

		VectorForEach (scopes, MutableCString, scope)

			ObjectArray references;
			t->getScopeReferences (references, scope);

			ForEach (references, Reference, r)
				formatWriter.writeReference (String () << r->getFileName () /* << ":" << r->getLineNumber ()*/);
			EndFor

			String key = SourceParser::escape (String (t->getKey ()));
			formatWriter.writeMessage (String (scope), key, String::kEmpty);
		EndFor

	EndFor

	return true;
}
