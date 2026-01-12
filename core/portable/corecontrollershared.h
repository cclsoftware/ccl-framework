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
// Filename    : core/portable/corecontrollershared.h
// Description : Shared Controller Code
//
//************************************************************************************************

#ifndef _corecontrollershared_h
#define _corecontrollershared_h

#include "core/public/corestringbuffer.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// TParamPath
//************************************************************************************************

template<int kStringSize>
struct TParamPath
{
	CStringBuffer<kStringSize> childName;
	CStringBuffer<kStringSize> paramName;

	TParamPath (CStringPtr path)
	{
		ASSERT (ConstString (path).length () < kStringSize) // check for truncation
		int index = ConstString (path).lastIndex ('/');
		if(index != -1)
		{
			childName = path;
			childName.subString (paramName, index + 1);
			if(index == 0)
				childName = "/";
			else
				childName.truncate (index);
		}
		else
			paramName = path;
	}
};

typedef TParamPath<64> ParamPath64;

//************************************************************************************************
// TControllerFinder
//************************************************************************************************

template <class TController>
struct TControllerFinder
{
	static TController* lookup (const TController* This, CStringPtr path)
	{
		return TControllerFinder<TController>::lookupInternal<CStringTokenizer, CStringPtr> (This, path);
	}

	static TController* lookupInplace (const TController* This, char* pathBuffer)
	{
		return lookupInternal<CStringTokenizerInplace, char*> (This, pathBuffer);
	}

private:
	template <class Tokenizer, typename StringType>
	static TController* lookupInternal (const TController* This, StringType _path)
	{
		ConstString path (_path);
		if(path.isEmpty ())
			return nullptr;

		const TController* current = This;
		if(path.firstChar () == '/')
		{
			while(current->getParent ())
				current = current->getParent ();	
		}

		CStringPtr token = nullptr;
		Tokenizer tokenizer (_path, "/");
		while((token = tokenizer.next ()) != nullptr)
		{
			if(ConstString (token) == "..")
				current = current->getParent ();
			else
				current = current->findChild (token);
			if(!current)
				break;
		}
		return const_cast<TController*> (current);
	}
};

} // namespace Portable
} // namespace Core

#endif // _corecontrollershared_h
