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
// Filename    : ccl/platform/win/gui/fontresource.win.cpp
// Description : Windows Font Resource
//
//************************************************************************************************

#include "ccl/gui/system/fontresource.h"

#include "ccl/public/base/memorystream.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/platform/win/cclwindows.h"

#pragma warning (disable : 4189)

namespace CCL {

//************************************************************************************************
// WindowsFontResource
//************************************************************************************************

class WindowsFontResource: public FontResource
{
public:
	WindowsFontResource (CCL::IStream& stream, StringRef name, int fontStyle);
	~WindowsFontResource ();

private:
	HANDLE handle;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// WindowsFontResource
//************************************************************************************************

FontResource* FontResource::install (CCL::IStream& stream, StringRef name, int fontStyle)
{
	return NEW WindowsFontResource (stream, name, fontStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsFontResource::WindowsFontResource (CCL::IStream& stream, StringRef name, int fontStyle)
: handle (nullptr)
{
	AutoPtr<IMemoryStream> fontStream = System::GetFileUtilities ().createStreamCopyInMemory (stream);
	if(!fontStream)
		return;

	PVOID ptr = fontStream->getMemoryAddress ();
	DWORD size = fontStream->getBytesWritten ();

	bool installed = NativeGraphicsEngine::instance ().installFontFromMemory (ptr, size, name, fontStyle);
	if(installed)
	{
		// register for GDI
		DWORD numInstalled = 0;
		handle = ::AddFontMemResourceEx (ptr, size, nullptr, &numInstalled);
		if(handle == nullptr)
		{
			#if DEBUG
			DWORD error = ::GetLastError ();
			#endif
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsFontResource::~WindowsFontResource ()
{
	if(handle)
		::RemoveFontMemResourceEx (handle);
}
