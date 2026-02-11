//************************************************************************************************
//
// Skin Crush Tool
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
// Filename    : skincrush.cpp
// Description : Skin Crush Tool
//
//************************************************************************************************

#include "core/portable/corepersistence.h"
#include "core/portable/gui/corebitmap.h"

#include "core/public/coreversion.h"

using namespace Core;
using namespace Portable;

//************************************************************************************************
// CrushMode
//************************************************************************************************

enum CrushMode
{
	kUnknownMode = -1,
	kJSONtoUBJSON,
	kCrushBitmap
};

//************************************************************************************************
// FileCrusher
//************************************************************************************************

class FileCrusher
{
	IO::MemoryStream* inData;
	FileStream outStream;

	bool verify () const
	{
		if(!inData)
		{
			::printf ("Failed to open input file!\n");
			return false;
		}
		if(!outStream.isOpen ())
		{
			::printf ("Failed to create output file!\n");
			return false;
		}
		return true;
	}

public:
	FileCrusher (CStringPtr infile, CStringPtr outfile)
	: inData (0)
	{
		inData = FileUtils::loadFile (infile);
		outStream.create (outfile);
	}

	~FileCrusher ()
	{
		delete inData;
	}

	bool convertJSONtoUBJSON ()
	{
		if(!verify ())
			return false;

		Attributes a (AttributeAllocator::getDefault ());
		if(!Archiver (inData, Archiver::kJSON).load (a))
		{
			::printf ("Failed to parse input JSON file!\n");
			return false;
		}
		if(!Archiver (&outStream, Archiver::kUBJSON).save (a))
		{
			::printf ("Failed to write UBJSON file!\n");
			return false;
		}
		return true;
	}

	bool crushBitmap ()
	{
		if(!verify ())
			return false;

		int totalSize = (int)inData->getBytesWritten ();
		if(totalSize < 2)
		{
			::printf ("Invalid source bitmap!\n");
			return false;
		}

		// obfuscate bitmap header
		uint16* dataBuffer = const_cast<uint16*> (inData->getBuffer ().as<uint16> ());
		*dataBuffer = BitmapFileFormat::getCustomBitmapHeaderType ();

		return outStream.writeBytes (dataBuffer, totalSize) == totalSize;
	}
};

#define VERBOSE 0

//////////////////////////////////////////////////////////////////////////////////////////////////

int main (int argc, char* argv[])
{
	::printf ("Skin Crush " CORE_AUTHOR_COPYRIGHT "\n");
	if(argc < 3)
	{
		::printf ("Usage: skincrush -(option) infile|indir (outfile)\n\n");
		::printf ("Options:\n");
		::printf ("-ju(d) : Convert JSON to UBJSON (d for directory)\n");
		::printf ("-bp(d) : Convert BMP to proprietary bitmap (d for directory)\n");
		return 0;
	}

	ConstString option (argv[1]);
	CrushMode mode = kUnknownMode;
	if(option.startsWith ("-ju"))
		mode = kJSONtoUBJSON;
	else if(option.startsWith ("-bp"))
		mode = kCrushBitmap;

	if(mode == kUnknownMode)
	{
		::printf ("Unknown option.\n");
		return -1;
	}

	FileName inPath (argv[2]);
	FileName outPath (argc >= 4 ? argv[3] : "");
	if(outPath.isEmpty ())
		outPath = inPath;

	inPath.adjustPathDelimiters ();
	outPath.adjustPathDelimiters ();

	// make paths absolute to working directory
	FileName workDir;
	FileUtils::getWorkingDir (workDir);
	if(inPath.isRelative ())
		inPath.makeAbsolute (workDir);
	if(outPath.isRelative ())
		outPath.makeAbsolute (workDir);

	bool result = true;
	bool dirMode = option.endsWith ("d");
	if(dirMode == true)
	{
		#if VERBOSE
		::printf ("Working in directory mode in %s\n", inPath.str ());
		#endif

		int fileCount = 0;
		FileIterator iter (inPath);
		const FileIterator::Entry* entry;
		while((entry = iter.next ()) != 0 && result == true)
			if(entry->directory == false)
			{
				if(mode == kJSONtoUBJSON)
				{
					if(Archiver::detectFormat (entry->name) == Archiver::kJSON)
					{
						fileCount++;
						FileName outFileName (entry->name);
						outFileName.setExtension (Archiver::getFileType (Archiver::kUBJSON));
						FileCrusher crusher (entry->name, outFileName);
						if(!crusher.convertJSONtoUBJSON ())
							result = false;
					}
				}
				else if(mode == kCrushBitmap)
				{
					if(BitmapFileFormat::detectFormat (entry->name) == BitmapFileFormat::kBMP)
					{
						fileCount++;
						FileCrusher crusher (entry->name, entry->name);
						if(!crusher.crushBitmap ())
							result = false;
					}
				}
			}

		if(fileCount == 0)
			::printf ("No matching files found.\n");
		else
			::printf ("%d file(s) processed.\n", fileCount);
	}
	else
	{
		FileCrusher crusher (inPath, outPath);
		switch(mode)
		{
		case kJSONtoUBJSON :
			result = crusher.convertJSONtoUBJSON ();
			break;
		case kCrushBitmap :
			result = crusher.crushBitmap ();
			break;
		}
	}

	return result ? 0 : -1;
}
