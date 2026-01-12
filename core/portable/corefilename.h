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
// Filename    : core/portable/corefilename.h
// Description : Filename class
//
//************************************************************************************************

#ifndef _corefilename_h
#define _corefilename_h

#include "core/public/corestringbuffer.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// FileName
/** String representing a file name using a platform-dependent path delimiter. 
	\ingroup core_portable */
//************************************************************************************************

class FileName: public CString256
{
public:
	FileName (CStringPtr filename = nullptr);
	
	static CStringPtr kPathDelimiter;

	/** Ascend one directory level. */
	FileName& ascend ();

	/** Descend one directory level, i.e. append name with path delimiter. */
	FileName& descend (CStringPtr name);

	/** Replace invalid file name characters with '_'. */
	FileName& makeValid ();

	enum PathDelimiterType { kPathChar, kForwardSlash };
	
	/** Adjust path delimiters. */
	FileName& adjustPathDelimiters (PathDelimiterType type = kPathChar);

	/** Check for relative path. */
	bool isRelative () const;

	/** Make absolute in given base folder. */
	FileName& makeAbsolute (CStringPtr baseFolder);

	/** Append (or replace) file name extension. */
	FileName& setExtension (CStringPtr ext, bool replace = true);
	
	/** Remove file name extension. */
	FileName& removeExtension ();
	
	/** Get file name extension. */
	bool getExtension (FileName& extension) const;

	/** Get file name from full path string. */
	void getName (FileName& name) const;
};

//************************************************************************************************
// FindFileData
/** Data structure used by file iterator. 
	\ingroup core_portable */
//************************************************************************************************

struct FindFileData
{
	FileName name;		///< absolute file name incl. directory name
	bool directory;		///< true for directories
	bool hidden;		///< true if file is hidden

	FindFileData ()
	: directory (false),
	  hidden (false)
	{}
};
	
} // namespace Portable
} // namespace Core

#endif // _corefilename_h
