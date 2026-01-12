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
// Filename    : ccl/public/storage/filetype.h
// Description : File Type
//
//************************************************************************************************

#ifndef _ccl_filetype_h
#define _ccl_filetype_h

#include "ccl/public/text/cclstring.h"

namespace CCL {

//************************************************************************************************
// PlainFileType
/** The filetype class below is binary equivalent to this C structure. 
	\ingroup base_io */
//************************************************************************************************

struct PlainFileType
{
	String description;		///< description (e.g. "Text File")
	String extension;		///< OS-specific file extension (e.g. "txt")
	String mimeType;		///< MIME type (e.g. "text/plain")
};

//************************************************************************************************
// FileType
/** File type information (extension + description + MIME type).
	\ingroup base_io */
//************************************************************************************************

class FileType: protected PlainFileType
{
public:
	FileType (CStringPtr _description = nullptr,
			  CStringPtr _extension = nullptr,
			  CStringPtr _mimeType = nullptr)
	{
		description = _description;
		extension = _extension,
		mimeType = _mimeType;
	}

	bool isValid () const;
	void clear ();

	StringRef getDescription () const;
	void setDescription (StringRef desc);

	StringRef getExtension () const;
	void setExtension (StringRef ext);

	StringRef getMimeType () const;
	void setMimeType (StringRef mimeType);

	bool equals (const FileType&) const;

	bool operator == (const FileType&) const;
	bool operator != (const FileType&) const;

	// qualification based on MIME type
	bool isTextType () const;
	bool isHumanReadable () const;
};

//************************************************************************************************
/** Predefined File Types
	\ingroup base_io */
//************************************************************************************************

namespace FileTypes
{
	enum DefaultTypes
	{
		kEmpty,
		kText,
		kXml,
		kHtml,
		kRtf,
		kPdf,
		kProperties,
		kBinary,
		kApp,
		kModule,
		kZip,
		kPackage,
		kJson,
		kUBJson,
		kCsv
	};
	
	const FileType& getDefault (int which);
	
	inline const FileType& Empty ()			{ return getDefault (kEmpty); }
	inline const FileType& Text ()			{ return getDefault (kText); }
	inline const FileType& Xml ()			{ return getDefault (kXml); }
	inline const FileType& Html ()			{ return getDefault (kHtml); }
	inline const FileType& Rtf ()			{ return getDefault (kRtf); }
	inline const FileType& Pdf ()			{ return getDefault (kPdf); }
	inline const FileType& Properties ()	{ return getDefault (kProperties); }
	inline const FileType& Binary ()		{ return getDefault (kBinary); }
	inline const FileType& App ()			{ return getDefault (kApp); }
	inline const FileType& Module ()		{ return getDefault (kModule); }
	inline const FileType& Zip ()			{ return getDefault (kZip); }
	inline const FileType& Package ()		{ return getDefault (kPackage); }
	inline const FileType& Json ()			{ return getDefault (kJson); }
	inline const FileType& UBJson ()		{ return getDefault (kUBJson); }
	inline const FileType& Csv ()			{ return getDefault (kCsv); }

	inline const FileType& init (FileType& fileType, StringRef description)
	{
		if(fileType.getDescription ().isEmpty ())
			fileType.setDescription (description);
		return fileType;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// FileType inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline StringRef FileType::getDescription () const { return description; }
inline void FileType::setDescription (StringRef desc) { description = desc; }
inline StringRef FileType::getExtension () const { return extension; }
inline void FileType::setExtension (StringRef ext) { extension = ext; }
inline StringRef FileType::getMimeType () const { return mimeType; }
inline void FileType::setMimeType (StringRef mt) { mimeType = mt; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_filetype_h
