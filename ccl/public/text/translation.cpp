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
// Filename    : ccl/public/text/translation.cpp
// Description : String Translation
//
//************************************************************************************************

#include "ccl/public/text/translation.h"
#include "ccl/public/text/itranslationtable.h"
#include "ccl/public/text/cstring.h"

using namespace CCL;

//************************************************************************************************
// LocalString
//************************************************************************************************

ITranslationTable* LocalString::theTable = nullptr;
const char* LocalString::currentScope = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LocalString::hasTable ()
{
	return theTable != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITranslationTable* LocalString::getTable ()
{
	ASSERT (theTable != nullptr)
	return theTable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LocalString::setTable (ITranslationTable* table)
{
	ASSERT (theTable == nullptr || theTable == table)
	theTable = table;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LocalString::addCorrections (const EnglishCorrection corrections[], int count)
{
	ASSERT (theTable != nullptr)
	if(theTable == nullptr) // table must be set first!
		return;

	for(int i = 0; i < count; i++)
	{
		const EnglishCorrection& c = corrections[i];
		theTable->addString (c.scope, c.key, String (c.englishText));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LocalString::tableDestroyed ()
{
	theTable = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String LocalString::translate (StringID scope, StringRef keyString)
{
	String result;
	ASSERT (theTable != nullptr)
	if(theTable)
		theTable->getStringWithUnicodeKey (result, scope, keyString);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String LocalString::translate (StringID scope, StringID keyString)
{
	String result;
	ASSERT (theTable != nullptr)
	if(theTable)
		theTable->getString (result, scope, keyString);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LocalString::BeginScope::BeginScope (const char* name)
{
	currentScope = name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LocalString::EndScope::EndScope ()
{
	currentScope = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LocalString::LocalString (const char* key)
: key (key),
  scope (currentScope)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef LocalString::getText (const ITranslationTable* altTable) const
{
	if(text.isEmpty ())
	{
		ASSERT (theTable != nullptr)
		if(theTable)
		{
			if(theTable->getString (text, scope, key) == kResultFalse)
			{
				// fallback to alternative table
				if(altTable)
					altTable->getString (text, scope, key);
			}
		}
		else
			text = key;
	}
	return text;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LocalString::operator StringRef () const 
{ 
	return getText (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const char* LocalString::getKey () const
{
	return key; 
}
