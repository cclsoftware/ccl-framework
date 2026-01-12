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
// Filename    : ccl/text/strings/jsonhandler.h
// Description : JSON Handler
//
//************************************************************************************************

#ifndef _ccl_jsonhandler_h
#define _ccl_jsonhandler_h

#include "ccl/public/text/iattributehandler.h"

namespace CCL {

interface IStream;

//************************************************************************************************
// JsonHandler
//************************************************************************************************

class JsonHandler
{
public:
	static tresult parse (IStream& srcStream, IAttributeHandler& handler);
	static IAttributeHandler* stringify (IStream& dstStream, int options = 0);
	
	static tresult parseBinary (IStream& srcStream, IAttributeHandler& handler);
	static IAttributeHandler* writeBinary (IStream& dstStream, int options = 0);

protected:
	class HandlerDelegate;
	class BaseWriter;
	class TextWriter;
	class BinaryWriter;
};

//************************************************************************************************
// Json5Handler
//************************************************************************************************

class Json5Handler: public JsonHandler
{
public:
	static tresult parse (IStream& srcStream, IAttributeHandler& handler);
};
} // namespace CCL

#endif // _ccl_jsonhandler_h
