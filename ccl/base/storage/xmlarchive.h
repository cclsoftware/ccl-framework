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
// Filename    : ccl/base/storage/xmlarchive.h
// Description : XML Archive
//
//************************************************************************************************

#ifndef _ccl_xmlarchive_h
#define _ccl_xmlarchive_h

#include "ccl/base/storage/archive.h"

namespace CCL {

class Container;
interface IUnknown;
interface IXmlWriter;
class FileType;

//************************************************************************************************
// XmlArchive
/** XML storage archive */
//************************************************************************************************

class XmlArchive: public Archive
{
public:
	XmlArchive (IStream& stream, Attributes* context = nullptr, StringID saveType = nullptr);

	static const CString defaultRootTag;		///< default root tag
	static const FileType& getFileType ();		///< default file type

	enum Flags 
	{
		kCharDataUTF8 = 1<<0,		///< treat character data as UTF-8 instead of UTF-16 (attribute "CDATA" of type IStream)
		kDefineNamespace = 1<<1,	///< define XML namespace for "x:" prefix
		kDontFailOnXmlError = 1<<2,	///< don't fail to load object when XML error occurs
		kSilentOnErrors = 1<<3		///< suppress error reporting / break in debug build (e.g. when failure is likely).
	};

	PROPERTY_FLAG (flags, kCharDataUTF8, charDataUTF8)  
	PROPERTY_FLAG (flags, kDefineNamespace, defineNamespace) 
	PROPERTY_FLAG (flags, kDontFailOnXmlError, dontFailOnXmlError) 
	PROPERTY_FLAG (flags, kSilentOnErrors, silentOnErrors)

	// Archive
	ArchiveType getArchiveType () const override { return kXmlArchive; }
	bool isAnonymous () const override { return false; }
	bool saveAttributes (ObjectID root, const Attributes& attributes) override;
	bool loadAttributes (ObjectID root, Attributes& attributes) override;
	using Archive::saveObject;

protected:
	bool saveObject (StringRef name, IUnknown* object, IXmlWriter& writer);
	bool saveData (StringRef name, VariantRef data, IXmlWriter& writer);
	bool saveList (StringRef name, const Container& list, IXmlWriter& writer);
	bool writeAttributes (StringRef tagName, StringRef objectName, const Attributes& attributes, IXmlWriter& writer);
};

//************************************************************************************************
// XmlArchiveUtils
//************************************************************************************************

namespace XmlArchiveUtils
{
	void setCharDataFromString (Attributes& a, StringRef string);
	bool getStringFromCharData (String& string, const Attributes& a);
}

} // namespace CCL

#endif // _ccl_xmlarchive_h
