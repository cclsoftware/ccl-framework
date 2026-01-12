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
// Filename    : ccl/base/storage/propertyfile.h
// Description : Java Property Files
//
//************************************************************************************************

#ifndef _ccl_propertyfile_h
#define _ccl_propertyfile_h

#include "ccl/base/storage/storableobject.h"
#include "ccl/base/collections/stringdictionary.h"

namespace CCL {
namespace Java {

//************************************************************************************************
// Java::PropertyParser
//************************************************************************************************

class PropertyParser
{
public:
	PropertyParser (StringDictionary& properties);

	bool parse (StringRef string);
	bool parse (IStream& stream);

protected:
	StringDictionary& properties;
};

//************************************************************************************************
// Java::PropertyWriter
//************************************************************************************************

class PropertyWriter
{
public:
	PropertyWriter (const StringDictionary& properties);

	bool write (IStream& stream);

protected:
	const StringDictionary& properties;
};

//************************************************************************************************
// Java::PropertyFile
//************************************************************************************************

class PropertyFile: public StorableObject
{
public:
	DECLARE_CLASS (PropertyFile, StorableObject)

	StringDictionary& getProperties ();
	const StringDictionary& getProperties () const;

	// StorableObject
	tbool CCL_API getFormat (FileType& format) const override;
	tbool CCL_API save (IStream& stream) const override;
	tbool CCL_API load (IStream& stream) override;

protected:
	StringDictionary properties;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline StringDictionary& PropertyFile::getProperties ()
{ return properties; }

inline const StringDictionary& PropertyFile::getProperties () const
{ return properties; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Java
} // namespace CCL

#endif // _ccl_propertyfile_h
