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
// Filename    : core/portable/gui/corebitmap.cpp
// Description : Bitmap class
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "corebitmap.h"

#include "core/gui/corepnghandler.h"
#include "core/gui/corebmphandler.h"
#include "core/gui/corebitmapprimitives.impl.h"
#include "core/gui/coreskinformat.impl.h"

#include "core/public/corestreamaccessor.h"
#include "core/text/coretexthelper.h"
#include "core/portable/corepersistence.h"
#include "core/portable/coreprofiling.h"

#define LOG_TOTAL_BITMAP_MEMORY \
	CORE_PRINTF ("Total bitmap memory used: %.2f MB\n", (float)Bitmap::getTotalBitmapMemory () / 1024.f / 1024.f)

namespace Core {
namespace Portable {

//************************************************************************************************
// OptimizedBitmap
//************************************************************************************************

class OptimizedBitmap: public Bitmap
{
public:
	IO::Buffer& getDataBuffer () { return dataBuffer; }

protected:
	IO::Buffer dataBuffer;
};

//************************************************************************************************
// BitmapReader
//************************************************************************************************

class BitmapReader
{
public:
	virtual ~BitmapReader ()
	{}

	virtual Bitmap* readImage () = 0;
};

//************************************************************************************************
// PNGReader
//************************************************************************************************

class PNGReader: public BitmapReader,
				 public PNGHandler
{
public:
	PNGReader (IO::Stream& stream, BitmapPixelFormat requestedFormat, bool explicitFormat)
	: PNGHandler (stream),
	  requestedFormat (requestedFormat),
	  explicitFormat (explicitFormat)
	{}

	// BitmapReader
	Bitmap* readImage () override
	{
		int width = 0, height = 0;
		bool hasAlpha = false;
		if(!readInfo (width, height, hasAlpha))
			return nullptr;

		Bitmap* bitmap = nullptr;
		if(requestedFormat == kBitmapMonochrome)
			bitmap = NEW Bitmap (width, height, kBitmapMonochrome, Bitmap::kUninitialized);
		else if(requestedFormat == kBitmapRGB565 && (explicitFormat || !hasAlpha))
			bitmap = NEW Bitmap (width, height, kBitmapRGB565, Bitmap::kUninitialized);
		else
			bitmap = NEW Bitmap (width, height, kBitmapRGBAlpha, Bitmap::kUninitialized);

		bitmap->setAlphaChannelUsed (hasAlpha);
		BitmapData& bitmapData = bitmap->accessForWrite ();
		readBitmapData (bitmapData);
		return bitmap;
	}

protected:
	BitmapPixelFormat requestedFormat;
	bool explicitFormat;
};

//************************************************************************************************
// BMPReader
//************************************************************************************************

class BMPReader: public BitmapReader,
				 public BMPHandler
{
public:
	static bool readInplace (Bitmap& bitmap, const uint8* bitmapFileData, uint32 bitmapFileLength)
	{
		ASSERT (bitmapFileLength > kBitmapFileHeaderSize + kBitmapInfoHeaderSize)
		if(bitmapFileLength <= kBitmapFileHeaderSize + kBitmapInfoHeaderSize)
			return false;

		const BitmapFileHeader* header = reinterpret_cast<const BitmapFileHeader*> (bitmapFileData);
		ASSERT (isKnownType (header->type))
		if(!isKnownType (header->type))
			return false;

		const BitmapInfoHeader* info = reinterpret_cast<const BitmapInfoHeader*> (bitmapFileData + kBitmapFileHeaderSize);
		BitmapPixelFormat pixelFormat = getKnownFormat (*info);
		if(pixelFormat != kBitmapAny)
		{
			// image is usually stored top-down and double-word aligned
			int options = Bitmap::kDoubleWordAligned|Bitmap::kUninitialized;
			if(info->height >= 0)
				options |= Bitmap::kTopDown;

			const uint8* bits = bitmapFileData + header->offsetToBits;
			uint32 imageSize = info->getSizeSafe ();
			ASSERT (bitmapFileLength >= header->offsetToBits + imageSize)
			bitmap.construct (info->width, abs (info->height), pixelFormat, options, bits, imageSize);
			bitmap.setAlphaChannelUsed (pixelFormat == kBitmapRGBAlpha);
			return true;
		}
		else
		{
			ASSERT (0) // implement me!
			return false;
		}
	}

	BMPReader (IO::Stream& stream)
	: BMPHandler (stream)
	{}

	// BitmapReader
	Bitmap* readImage () override
	{		
		#if 1 // try to transfer buffer ownership to avoid additional allocation/copy operations
		if(IO::BufferProvider* provider = stream.getBufferProvider ())
		{
			OptimizedBitmap* bitmap = NEW OptimizedBitmap;
			IO::Buffer& buffer = bitmap->getDataBuffer ();
			provider->moveBufferTo (buffer);
			if(readInplace (*bitmap, buffer.as<uint8> (), buffer.getSize ()))
			{
				bitmap->setAlphaChannelUsed (bitmap->getFormat () == kBitmapRGBAlpha);
				return bitmap;
			}
			else
			{
				delete bitmap;
				return nullptr;
			}
		}
		#endif

		if(!readInfo ())
			return nullptr;

		Bitmap* bitmap = nullptr;
		BitmapPixelFormat pixelFormat = getKnownFormat (info);
		if(pixelFormat != kBitmapAny)
		{
			// image is usually stored top-down and double-word aligned
			int options = Bitmap::kTopDown|Bitmap::kDoubleWordAligned|Bitmap::kUninitialized;
			if(info.height < 0)
			{
				info.height = abs (info.height);
				options &= ~Bitmap::kTopDown;
			}			

			bitmap = NEW Bitmap (info.width, info.height, pixelFormat, options);	
			readData (bitmap->getBufferAddress (), bitmap->getBufferSize ());
			bitmap->setAlphaChannelUsed (pixelFormat == kBitmapRGBAlpha);
		}
		else
		{
			ASSERT (0) // implement me!
		}
		
		return bitmap;
	}
};

} // namespace Portable
} // namespace Core

using namespace Core;
using namespace Portable;

//************************************************************************************************
// DpiSetting
//************************************************************************************************

DEFINE_STATIC_SINGLETON (DpiSetting)

//************************************************************************************************
// ResourceAttributes
//************************************************************************************************

int ResourceAttributes::hashIntKey (const uint32& key, int size)
{
	return key % size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr ResourceAttributes::makeHiResFileName (CString256& result, CStringPtr _fileName)
{
	FileName fileName (_fileName);
	int index = fileName.lastIndex ('.');
	if(index != -1)
	{
		FileName ext;
		fileName.subString (ext, index);
		fileName.truncate (index);
		if(DpiSetting::instance ().getScaleFactor () == 3.f)
			fileName += "@3x";
		else
			fileName += "@2x";
		fileName += ext;
	}
	result = fileName;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect ResourceAttributes::getSize (const Attributes& a, CStringPtr name)
{
	Rect size;
	if(const Attribute* sizeAttr = a.lookup (name))
		decodeSize (size, *sizeAttr);
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect ResourceAttributes::getSize (const Attributes& a)
{
	if(const Attribute* sizeAttr = a.lookup (kSize))
	{
		Rect size;
		decodeSize (size, *sizeAttr);
		return size;
	}
	else
	{
		int width = (int)a.getInt (kWidth);
		int height = (int)a.getInt (kHeight);
		return Rect (0, 0, width, height);
	}
}

//************************************************************************************************
// Bitmap
//************************************************************************************************

Bitmap* Bitmap::loadPNGImage (IO::Stream& stream, BitmapPixelFormat requestedFormat, bool explicitFormat)
{
	ASSERT (requestedFormat == kBitmapAny ||
			requestedFormat == kBitmapRGBAlpha || 
			requestedFormat == kBitmapMonochrome ||
			requestedFormat == kBitmapRGB565)

	PNGReader reader (stream, requestedFormat, explicitFormat);
	if(!reader.construct ())
		return nullptr;

	return reader.readImage ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Bitmap* Bitmap::loadBMPImage (IO::Stream& stream)
{
	return BMPReader (stream).readImage ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Bitmap::saveBMPImage (IO::Stream& stream, const Bitmap& bitmap)
{
	static const int kBitfieldsSize = 16; // RGB bit masks
	static const int kResolution = 2834; // 72 DPI x 39.3701 inches per metre yields 2834.6472

	const BitmapData& data = bitmap.accessForRead ();
	uint32 bitmapDataSize = data.rowBytes * data.height;
	ASSERT (bitmapDataSize <= bitmap.getBufferSize ())

	uint32 infoHeaderSize = BMPReader::kBitmapInfoHeaderSize;
	if(data.format == kBitmapRGB565)
		infoHeaderSize += kBitfieldsSize;
		
	uint32 bitmapHeaderSize = BMPReader::kBitmapFileHeaderSize + infoHeaderSize;

	BMPReader::BitmapFileHeader fileHeader = {};
	fileHeader.type = BMPReader::kRegularBitmapType;
	fileHeader.size = bitmapHeaderSize + bitmapDataSize;
	fileHeader.offsetToBits = bitmapHeaderSize;
	if(stream.writeBytes (&fileHeader, BMPReader::kBitmapFileHeaderSize) != BMPReader::kBitmapFileHeaderSize)
		return false;

	BMPReader::BitmapInfoHeader infoHeader = {};
	infoHeader.size = infoHeaderSize;
	infoHeader.width = data.width;
	infoHeader.height = -data.height;
	infoHeader.planes = 1;
	infoHeader.bitCount = static_cast<uint16> (data.bitsPerPixel);
	infoHeader.compression = BMPReader::kUncompressed;
	infoHeader.sizeImage = bitmapDataSize;
	infoHeader.xPixelsPerMeter = kResolution;
	infoHeader.yPixelsPerMeter = kResolution;

	if(data.format == kBitmapRGB565)
	    infoHeader.compression = BMPReader::kBitfields;
	if(stream.writeBytes (&infoHeader, BMPReader::kBitmapInfoHeaderSize) != BMPReader::kBitmapInfoHeaderSize)
		return false;

	// write bit masks for channels
	if(data.format == kBitmapRGB565)
	{
		const uint8 bitMasks[kBitfieldsSize] = {0x00, 0xf8, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		if(stream.writeBytes (bitMasks, kBitfieldsSize) != kBitfieldsSize)
			return false;
	}

	return stream.writeBytes (bitmap.getBufferAddress (), bitmapDataSize) == static_cast<int> (bitmapDataSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 Bitmap::totalBitmapMemory = 0;
uint32 Bitmap::getTotalBitmapMemory ()
{
	return totalBitmapMemory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Bitmap::construct (int width, int height, BitmapPixelFormat format, int options, 
						const uint8* externalBuffer,  uint32 externalBufferSize)
{
	data.init (width, height, format, (options & kDoubleWordAligned) != 0);

	uint8* bufferStart = nullptr;
	uint32 bufferByteSize = data.height * data.rowBytes;
	if(externalBuffer == nullptr)
	{
		pixelBuffer.setAlignment (kAlignment);
		pixelBuffer.resize (bufferByteSize);
		bufferStart = (uint8*)pixelBuffer.getAddressAligned ();
		ASSERT (bufferStart != nullptr)
	}
	else
	{		
		// external buffer is referenced only, no memory is allocated or copied here
		ASSERT (externalBufferSize >= bufferByteSize)
		bufferStart = const_cast<uint8*> (externalBuffer);
		IO::Buffer temp (bufferStart, get_min (externalBufferSize, bufferByteSize), false);
		pixelBuffer.take (temp);
	}

	data.initScan0 (bufferStart, (options & kTopDown) != 0);

	if(!(options & kUninitialized))
		BitmapPrimitives::clear (data);

	totalBitmapMemory += pixelBuffer.getSize () + pixelBuffer.getAlignment ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Bitmap::Bitmap ()
: alphaChannelUsed (false),
  frameCount (1),
  useCount (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Bitmap::Bitmap (const BitmapData& data)
:  alphaChannelUsed (false),
  frameCount (1),
  data (data),
  useCount (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Bitmap::Bitmap (int width, int height, BitmapPixelFormat format, int options)
: alphaChannelUsed (false),
  frameCount (1),
  useCount (0)
{
	construct (width, height, format, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Bitmap::Bitmap (const uint8* bitmapFileData, uint32 bitmapFileLength)
: alphaChannelUsed (false),
  frameCount (1),
  useCount (0)
{
	BMPReader::readInplace (*this, bitmapFileData, bitmapFileLength);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Bitmap::~Bitmap ()
{
	totalBitmapMemory -= pixelBuffer.getSize () + pixelBuffer.getAlignment ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& Bitmap::getFrame (Rect& frameRect, int frameIndex) const
{
	getSize (frameRect);
	if(getFrameCount () > 1)
	{
		Coord frameHeight = frameRect.getHeight ()/getFrameCount ();
		frameRect.setHeight (frameHeight);
		frameRect.offset (0, frameIndex * frameHeight);
	}
	return frameRect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Bitmap::copyFrom (const Bitmap& bitmap)
{
	uint32 bytesToCopy = bitmap.getBufferSize ();
	if(getBufferSize () != bytesToCopy)
		return false;

	// make sure to use aligned buffer addresses here!
	::memcpy (getBufferAddress (), bitmap.getBufferAddress (), bytesToCopy);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Bitmap::copyFrom (const Bitmap& bitmap, RectRef rect)
{
	const BitmapData& srcData = bitmap.accessForRead ();
	if(srcData.format != data.format)
		return false;

	switch(data.format)
	{
	case kBitmapRGBAlpha :
		BitmapPrimitives32::copyPart (data, rect.left, rect.top, srcData, rect.left, rect.top, rect.getWidth (), rect.getHeight ());
		break;
	case kBitmapRGB565 :
		BitmapPrimitives16::copyPart (data, rect.left, rect.top, srcData, rect.left, rect.top, rect.getWidth (), rect.getHeight ());
		break;
	default :
		return false;
	}
	return true;
}

//************************************************************************************************
// BitmapFileFormat
//************************************************************************************************

BitmapFileFormat::Format BitmapFileFormat::detectFormat (CStringPtr fileName)
{
	int index = ConstString (fileName).lastIndex ('.');
	if(index != -1)
	{
		ConstString ext (fileName + index + 1);
		if(ext == "png")
			return kPNG;
		else if(ext == "bmp")
			return kBMP;
	}
	return kUnknown;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr BitmapFileFormat::getExtension (Format format)
{
	switch(format)
	{
	case kPNG : return "png";
	case kBMP : return "bmp";
	default : return nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint16 BitmapFileFormat::getCustomBitmapHeaderType ()
{
	return BMPReader::kCustomBitmapType;
}

//************************************************************************************************
// BitmapManager::InternalStore
//************************************************************************************************

BitmapManager::InternalStore::InternalStore ()
: hashMap (256, ResourceAttributes::hashIntKey)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

BitmapManager::InternalStore::~InternalStore ()
{
	hashMap.removeAll ();
	toDecode.removeAll ();

	BitmapDescriptor* descriptor = nullptr;
	while((descriptor = descriptors.removeFirst ()) != nullptr)
		delete descriptor;
}

//************************************************************************************************
// BitmapManager
//************************************************************************************************

DEFINE_STATIC_SINGLETON (BitmapManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

BitmapManager::BitmapManager ()
: defaultFormat (kBitmapRGBAlpha),
  memoryLimit (0),
  store (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

BitmapManager::~BitmapManager ()
{
	delete store;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BitmapManager::InternalStore& BitmapManager::getStore ()
{
	// allocate on demand, might not be used on low-end platforms
	if(store == nullptr)
		store = NEW InternalStore;
	return *store;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int BitmapManager::loadBitmaps (FilePackage& package, bool delayDecoding)
{
	CORE_PROFILE_START (loadBitmaps)
	int bitmapCount = 0;
	Archiver::Format primaryFormat = Archiver::kJSON;
	IO::Stream* jsonStream = package.openStream (Skin::FileNames::kBitmapFile1);
	if(jsonStream == nullptr)
	{
		jsonStream = package.openStream (Skin::FileNames::kBitmapFile2);
		primaryFormat = Archiver::kUBJSON;
	}
	if(jsonStream != nullptr)
	{
		Deleter<IO::Stream> deleter (jsonStream);
		Attributes a (AttributeAllocator::getDefault ());
		if(Archiver (jsonStream, primaryFormat).load (a))
		{
			const AttributeQueue* bitmapArray = a.getQueue (nullptr);
			if(bitmapArray != nullptr)
			{
				bitmapCount = bitmapArray->getValues ().count ();
				InternalStore& s = getStore ();

				// avoid multiple reallocation for descriptors
				if(delayDecoding == true)
					s.toDecode.resize (s.toDecode.count () + bitmapCount);

				VectorForEach (bitmapArray->getValues (), AttributeValue*, value)
					if(const Attributes* bitmapAttr = value->getAttributes ())
					{
						CStringPtr name = bitmapAttr->getString (ResourceAttributes::kName);
						CStringPtr fileName = bitmapAttr->getString (ResourceAttributes::kFile);
						bool monochrome = bitmapAttr->getInt (ResourceAttributes::kMonochrome) != 0;
						bool alwaysCached = bitmapAttr->getInt (ResourceAttributes::kAlwaysCached) != 0;
						
						int frameCount = 1;
						if(bitmapAttr->contains (ResourceAttributes::kFrames))
							frameCount = (int)bitmapAttr->getInt (ResourceAttributes::kFrames);
						
						FileName fileNameHiRes;
						if(DpiSetting::instance ().isHighResolution ())
							fileName = ResourceAttributes::makeHiResFileName (fileNameHiRes, fileName);

						BitmapDescriptor* descriptor = NEW BitmapDescriptor;
						descriptor->setPackage (&package);
						descriptor->setFileName (fileName);
						ASSERT (descriptor->getFileName () == fileName) // check for truncation
						descriptor->setMonochrome (monochrome);
						descriptor->setFrameCount (frameCount);
						descriptor->setAlwaysCached (alwaysCached);

						addBitmap (name, descriptor);
						if(delayDecoding == true)
						{
							s.toDecode.add (descriptor);
							observers.notify (&BitmapManagerObserver::onDelayLoadingBitmap, descriptor->getFileName ().str ());
						}
						else if(isAboveMemoryLimit () == false)
							loadBitmap (*descriptor);						
					}
				EndFor
			}
		}
	}
	CORE_PROFILE_STOP (loadBitmaps, "BitmapManager::loadBitmaps")
	return bitmapCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapManager::addBitmap (CStringPtr name, BitmapDescriptor* descriptor)
{
	InternalStore& s = getStore ();
	s.descriptors.prepend (descriptor); // make new head (Most Recently Used)

	uint32 key = ResourceAttributes::nameToInt (name);
	ASSERT (s.hashMap.lookup (key) == 0)
	s.hashMap.add (key, descriptor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BitmapManager::loadBitmap (BitmapDescriptor& descriptor)
{
	ASSERT (descriptor.getBitmap () == nullptr)
	FilePackage* package = descriptor.getPackage ();
	IO::Stream* stream = package ? package->openStream (descriptor.getFileName ()) : nullptr;
	if(stream)
	{
		BitmapFileFormat::Format format = BitmapFileFormat::detectFormat (descriptor.getFileName ());
		Bitmap* bitmap = nullptr;
		switch(format)
		{
		case BitmapFileFormat::kPNG : 
			// keep explicitFormat = false to respect alpha channel of source file!
			bitmap = Bitmap::loadPNGImage (*stream, descriptor.isMonochrome () ? kBitmapMonochrome : defaultFormat, false);
			break;

		case BitmapFileFormat::kBMP :
			bitmap = Bitmap::loadBMPImage (*stream);
			break;

		default :
			ASSERT (0)
			break;
		}

		if(bitmap)
		{
			bitmap->setFrameCount (descriptor.getFrameCount ());
			descriptor.setBitmap (bitmap);
		}
		delete stream;
	}
	#if DEBUG
	else
		DebugPrintf ("BitmapManager::loadBitmap failed to open %s\n", descriptor.getFileName ().str ());
	#endif
	bool success = descriptor.getBitmap () != nullptr;
	if(success)
		observers.notify (&BitmapManagerObserver::onBitmapLoaded, descriptor.getFileName ().str ());
	return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BitmapManager::isAboveMemoryLimit () const
{
	return memoryLimit > 0 && Bitmap::getTotalBitmapMemory () >= memoryLimit;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapManager::reduceBitmaps ()
{
	bool success = false;
	// The strategy is to start at tail of descriptor list (Least Recently Used) and
	// remove as many unused bitmaps as needed to get below memory limit
	InternalStore& s = getStore ();
	IntrusiveListForEachReverse (s.descriptors, BitmapDescriptor, descriptor)
		const Bitmap* bitmap = descriptor->getBitmap ();
		if(bitmap && !bitmap->isReferenced () && !descriptor->isAlwaysCached ())
		{
			CORE_PRINTF ("Unloading bitmap %s\n", descriptor->getFileName ().str ())
			descriptor->unload ();
			if(!isAboveMemoryLimit ())
			{
				success = true;
				break;
			}
		}
	EndFor

	if(success)
	{
	#if DEBUG_LOG
		LOG_TOTAL_BITMAP_MEMORY
	#endif
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BitmapReference BitmapManager::getBitmap (CStringPtr name)
{
	if(ConstString (name).isEmpty ())
		return BitmapReference ();
	
	CORE_PROFILE_START (getBitmap)
	InternalStore& s = getStore ();
	uint32 key = ResourceAttributes::nameToInt (name);
	BitmapDescriptor* descriptor = s.hashMap.lookup (key);
	if(descriptor == nullptr)
	{
		#if DEBUG
		DebugPrintf ("BitmapManager::getBitmap failed to find %s\n", name);
		#endif
		return BitmapReference ();
	}

	if(descriptor->getBitmap () == nullptr)
	{
		// reduce bitmaps before loading new ones
		if(isAboveMemoryLimit ())
			reduceBitmaps ();

		s.toDecode.remove (descriptor);
		CORE_PRINTF ("Loading bitmap %s\n", descriptor->getFileName ().str ())
		loadBitmap (*descriptor);	
	}

	CORE_PROFILE_STOP (getBitmap, "BitmapManager::getBitmap")	

	if(descriptor != s.descriptors.getFirst ())
	{
		// make new head (Most Recently Used)
		s.descriptors.remove (descriptor);
		s.descriptors.prepend (descriptor);
	}

	return BitmapReference (descriptor->getBitmap ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapManager::idle ()
{
	InternalStore& s = getStore ();
	if(!s.toDecode.isEmpty ())
	{
		//CORE_PRINTF ("%d encoded bitmaps remaining...\n", s.toDecode.count ())		
		#if 1
		int decodeIndex = s.toDecode.count ()-1; // decode last one first so we don't have to shift the array
		#else
		int decodeIndex = 0;
		#endif
		
		BitmapDescriptor* descriptor = s.toDecode[decodeIndex];
		loadBitmap (*descriptor);
		s.toDecode.removeAt (decodeIndex);

		if(isAboveMemoryLimit ()) // stop loading if limit exceeded
		{
			CORE_PRINT ("Bitmap memory limit reached\n")
			s.toDecode.removeAll ();
		}

		#if DEBUG_LOG
		if(s.toDecode.isEmpty ())
			LOG_TOTAL_BITMAP_MEMORY
		#endif
	}
}
