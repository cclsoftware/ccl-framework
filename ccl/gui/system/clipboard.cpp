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
// Filename    : ccl/gui/system/clipboard.cpp
// Description : Clipboard
//
//************************************************************************************************

#include "ccl/gui/system/clipboard.h"

#include "ccl/base/objectconverter.h"
#include "ccl/base/boxedtypes.h"

#include "ccl/public/base/streamer.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// GUI Service APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IClipboard& CCL_API System::CCL_ISOLATED (GetClipboard) ()
{
	return Clipboard::instance ();
}

//************************************************************************************************
// Clipboard
//************************************************************************************************

DEFINE_CLASS (Clipboard, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Clipboard::Clipboard ()
: content (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Clipboard::~Clipboard ()
{
	empty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Clipboard::isEmpty () const
{
	return content == nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API Clipboard::getContent () const
{
	const_cast<Clipboard*> (this)->checkNativeContent ();

	return content;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Clipboard::setContent (IUnknown* object)
{
	empty ();
	content = object;

	// try text conversion for platform clipboard
	String text;
	if(toText (text, content))
		setNativeText (text);

	// "consume" any pending change in the native clipboard, since our newer content now supersedes it
	// (includes a change triggered by ourselves above in setNativeText: avoid round-trip conversion of our original content)
	hasNativeContentChanged ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Clipboard::setText (StringRef text)
{
	return setContent (ccl_as_unknown (NEW Boxed::String (text)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Clipboard::getText (String& text) const
{
	if(Boxed::String* string = unknown_cast<Boxed::String> (getContent ()))
	{
		text = *string;
		return true;
	}

	// do not try to convert here
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Clipboard::checkNativeContent ()
{
	if(!hasNativeContentChanged ())
		return false;

	String text;
	if(getNativeText (text))
	{
		CCL_PRINTF ("Clipboard::checkNativeContent: %s\n", MutableCString (text).str ())

		empty ();
		if(IUnknown* newContent = fromText (text))
			content = newContent;
		else
			content = ccl_as_unknown (NEW Boxed::String (text));
		return true;
	}

	// todo: other formats (Url)
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Clipboard::empty ()
{
	if(content)
		content->release (),
		content = nullptr;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Clipboard::registerFilter (IConvertFilter* filter)
{
	ObjectConverter::instance ().registerFilter (filter);

	UnknownPtr<IImportFilter> importer (filter);
	if(importer)
		ObjectConverter::instance ().registerImporter (importer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Clipboard::unregisterFilter (IConvertFilter* filter)
{
	ObjectConverter::instance ().unregisterFilter (filter);

	UnknownPtr<IImportFilter> importer (filter);
	if(importer)
		ObjectConverter::instance ().unregisterImporter (importer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Clipboard::toText (String& text, IUnknown* object)
{
	text.empty ();

	if(Boxed::String* string = unknown_cast<Boxed::String> (object))
	{
		text = *string;
		return true;
	}
	else if(AutoPtr<IUnknown> unk = ObjectConverter::instance ().convert (object, ClipboardFormat::UnicodeText))
	{
		UnknownPtr<IMemoryStream> ms (unk);
		ASSERT (ms.isValid ())
		if(ms)
		{
			const uchar* chars = (uchar*)ms->getMemoryAddress ();
			int length = ms->getBytesWritten ()/sizeof(uchar);

			// remove BOM
			if(length > 0 && chars[0] == Streamer::kByteOrderMark)
				chars++,
				length--;

			text.append (chars, length);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* Clipboard::fromText (StringRef text)
{
	// try to convert text to an object
	if(!text.isEmpty ())
	{
		StringChars chars (text);
		return ObjectConverter::instance ().importText (chars, text.length () * sizeof(uchar), true);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Clipboard::setNativeText (StringRef text)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Clipboard::getNativeText (String& text) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Clipboard::hasNativeContentChanged ()
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (Clipboard)
	DEFINE_METHOD_ARGS ("setText", "text")
	DEFINE_METHOD_ARGR ("getText", "", "string")
END_METHOD_NAMES (Clipboard)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Clipboard::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "setText")
	{
		String text;
		if(msg.getArgCount ())
			text = msg[0].asString ();
		setText (text);
		return true;
	}
	else if(msg == "getText")
	{
		String text;
		getText (text);

		returnValue = text;
		returnValue.share ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}
