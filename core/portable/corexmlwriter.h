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
// Filename    : core/portable/corexmlwriter.h
// Description : XML Writer
//
//************************************************************************************************

#ifndef _corexmlwriter_h
#define _corexmlwriter_h

#include "core/public/corestream.h"
#include "core/public/corestringbuffer.h"
#include "core/public/coremacros.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// XmlWriter
/** Helper class to programmatically generate XML output.
\ingroup core_portable */
//************************************************************************************************

class XmlWriter
{
public:
	XmlWriter (IO::Stream& stream)
	: stream (stream),
	  depth (0) 
	{}

	XmlWriter& beginDocumentUTF8 ()
	{
		static const uint8 kBOM[3] = {0xEF, 0xBB, 0xBF};
		stream.writeBytes (kBOM, 3);
		*this << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
		return *this;
	}

	struct Attribute
	{
		CStringPtr key;
		CStringPtr value;
	};

	XmlWriter& startElement (CStringPtr name, const Attribute attributes[] = 0, int attrCount = 0, bool closed = false)
	{
		writeIndent ();
		*this << "<";
		*this << name;
		if(attrCount > 0)
		{
			for(int i = 0; i < attrCount; i++)
			{
				*this << " ";
				*this << attributes[i].key;
				*this << "=\"";
				writeEncoded (attributes[i].value);
				*this << "\"";
			}
		}
		if(closed)
			*this << "/";
		else
			depth++;
		*this << ">\n";
		return *this;
	}

	XmlWriter& endElement (CStringPtr name)
	{
		depth--;
		writeIndent ();
		*this << "</" << name << ">\n";
		return *this;
	}

protected:
	IO::Stream& stream;
	int depth;

	XmlWriter& operator << (char c)
	{
		stream.writeBytes (&c, 1);
		return *this;
	}

	XmlWriter& operator << (CStringPtr string)
	{
		stream.writeBytes (string, ConstString (string).length ());
		return *this;
	}

	void writeIndent ()
	{
		for(int i = 0; i < depth; i++)
			*this << "\t";
	}

	static CStringPtr findEntity (char c)
	{
		struct Entity
		{
			char c;
			CStringPtr entity;
		};

		static const Entity standardEntities[] =
		{
			{'"',	"quot"},
			{'&',	"amp"},	
			{'\'',	"apos"},
			{'<',	"lt"},
			{'>',	"gt"}
		};

		for(int i = 0; i < ARRAY_COUNT (standardEntities); i++)
			if(c == standardEntities[i].c)
				return standardEntities[i].entity;
		return 0;
	}

	void writeEncoded (CStringPtr string)
	{
		CStringPtr ptr = string;
		for(char c = *ptr; c != 0; c = *++ptr)
		{
			if(CStringPtr entity = findEntity (c))
			{
				*this << "&";
				*this << entity;
				*this << ";";
			}
			else
				*this << c;
		}
	}
};

} // namespace Portable
} // namespace Core

#endif // _corexmlwriter_h
