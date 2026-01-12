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
// Filename    : core/portable/gui/corefont.cpp
// Description : Bitmap Font
//
//************************************************************************************************

#define DEBUG_LOG (0 && DEBUG)

#include "corefont.h"

#include "core/portable/corepersistence.h"
#include "core/public/coreprimitives.h"
#include "core/text/coreutfcodec.h"
#include "core/system/coredebug.h"

namespace Core {
namespace Portable {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Bitmap Font File Format
//	http://www.angelcode.com/products/bmfont/
//  https://angelcode.com/products/bmfont/doc/file_format.html
//	http://71squared.com/en/glyphdesigner/bmfont-binary-format
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace BMF 
{	
	enum BlockType
	{
		kInfo = 1,
		kCommon = 2,
		//kPages = 3,
		kChars = 4,
		kKerningPairs = 5
	};
	
    // These packing commands will result in unaligned word accesses on the 
    // ARM5 architecture.  
    #ifndef ARM_TI32        
	#pragma pack(push,1)	// push current alignment, force 1-byte alignment
    #endif

	struct FileHeader
	{	
		char magic[3];
		uint8 version;

		bool isValid () const
		{
			return	magic[0] == 'B' && magic[1] == 'M' && magic[2] == 'F' &&
					version == 3;
		}
	};

	struct InfoBlock
	{
		enum Flags
		{
			kSmooth  = 1<<0,
			kUnicode = 1<<1,
			kItalic  = 1<<2,
			kBold    = 1<<3
		};
	
		uint16 fontSize;
		uint8 flags;
		uint8 charSet;
		uint16 stretchH;
		uint8 aa;
		uint8 paddingUp;
		uint8 paddingRight;
		uint8 paddingDown;
		uint8 paddingLeft;
		uint8 spacingHoriz;
		uint8 spacingVert;
		uint8 outline;
		//char fontName[...];
	};
		
	struct CommonBlock
	{
		uint16 lineHeight;
		uint16 base;
		uint16 scaleW;
		uint16 scaleH;
		uint16 pages;
		uint8 flags;
		//...
	}; 
	
    struct CharInfo
    {
        uint32 id;
        uint16 x;
        uint16 y;
        uint16 width;
        uint16 height;
        int16 xoffset;
        int16 yoffset;
        int16 xadvance;
        uint8 page;
		uint8 chn1;
    };
	
    struct KerningPair
    {
        uint32 first;
        uint32 second;
        int16 amount;
	};

    #ifndef ARM_TI32
	#pragma pack(pop)	// restore alignment
    #endif
}

//************************************************************************************************
// BitmapFont::LoadedMap
//************************************************************************************************

struct BitmapFont::CharDescriptor
{
	// position on texture
	int x;
	int y;
	int width;
	int height;
		
	int xOffset;	///< offset from cursor
	int yOffset;
	int xAdvance;
		
	CharDescriptor (int unused = 0)
	: x (0),
	  y (0),
	  width (0),
	  height (0),
	  xOffset (0),
	  yOffset (0),
	  xAdvance (0)
	{}

	void assign (const BMF::CharInfo& charInfo)
	{
		x = charInfo.x;
		y = charInfo.y;
		width = charInfo.width;
		height = charInfo.height;
		xOffset = charInfo.xoffset;
		yOffset = charInfo.yoffset;
		xAdvance = charInfo.xadvance;
	}
};

struct BitmapFont::KerningPair
{
	uchar32 first;
	uchar32 second;
	int amount;

	KerningPair (int unused = 0)
	: first (0),
	  second (0),
	  amount (0)
	{}
};

struct BitmapFont::LoadedMap: BitmapFont::CharMap
{
	HashMap<uchar32, CharDescriptor> charMap;
	Vector<KerningPair> kerningPairs;

	static int hashChar (const uchar32& key, int size)
	{
		return key % size;
	}

	LoadedMap ()
	: charMap (128, hashChar)
	{}

	// CharMap
	bool lookup (CharDescriptor& desc, uchar32 c) const override
	{
		desc = charMap.lookup (c);
		return desc.width > 0;
	}

	int getKerningAmount (uchar32 first, uchar32 second) const override
	{
		if(!kerningPairs.isEmpty () && first != 0 && second != 0)
			for(int i = 0; i < kerningPairs.count (); i++)
			{
				const KerningPair& pair = kerningPairs[i];
				if(pair.first == first && pair.second == second)
				{
					#if DEBUG_LOG
					DebugPrintf ("Kerning pair '%c' '%c' %d\n", (char)first, (char)second, pair.amount);
					#endif
					return -pair.amount;
				}
			}
		return 0;
	}
};

} // namespace Portable
} // namespace Core

using namespace Core;
using namespace Portable;

//************************************************************************************************
// BitmapFont::InplaceMap
//************************************************************************************************

bool BitmapFont::InplaceMap::lookup (CharDescriptor& desc, uchar32 c) const
{
	const BMF::CharInfo* charInfo = reinterpret_cast<const BMF::CharInfo*> (charData);
	for(int i = 0; i < charCount; i++)
		if(charInfo[i].id == c)
		{
			desc.assign (charInfo[i]);
			return true;
		}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int BitmapFont::InplaceMap::getKerningAmount (uchar32 first, uchar32 second) const
{
	const BMF::KerningPair* kerningPair =  reinterpret_cast<const BMF::KerningPair*> (pairData);
	for(int i = 0; i < pairCount; i++, kerningPair++)
		if(kerningPair->first == first && kerningPair->second == second)
			return kerningPair->amount;
	return 0;
}

//************************************************************************************************
// BitmapFont
//************************************************************************************************

const Color BitmapFont::kInvertColor (1);

//////////////////////////////////////////////////////////////////////////////////////////////////

BitmapFont::BitmapFont (CStringPtr name, Bitmap* bitmap, bool ownBitmap)
: name (name),
  fontNumber (0),
  bitmap (bitmap),
  ownBitmap (ownBitmap),
  loadedMap (nullptr),
  lineHeight (0),
  colorCachingEnabled (true)
{
	ASSERT (bitmap != nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BitmapFont::~BitmapFont ()
{
	if(ownBitmap)
		delete bitmap;

	if(loadedMap)
		delete loadedMap;

	VectorForEachFast (colorBitmaps, ColorEntry, entry)
		delete entry.bitmap;
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BitmapFont::loadDescriptor (IO::Stream& stream)
{
	BMF::FileHeader header = {};
	stream.readBytes (&header, sizeof(BMF::FileHeader));
	if(!header.isValid ())
		return false;

	if(loadedMap == nullptr)
		loadedMap = NEW LoadedMap;
		
	uint8 blockType = 0;
	while(stream.readBytes (&blockType, 1) > 0)
	{
		int32 blockSize = 0;
		stream.readBytes (&blockSize, 4);	

		int64 oldPos = stream.getPosition ();
		switch(blockType)
		{
		#if 0//DEBUG
		case BMF::kInfo :
			{
				BMF::InfoBlock infoBlock = {0};
				stream.readBytes (&infoBlock, sizeof(BMF::InfoBlock));
			}
			break;
		#endif
			
		case BMF::kCommon :
			{
				BMF::CommonBlock commonBlock = {};
				stream.readBytes (&commonBlock, sizeof(BMF::CommonBlock));
				ASSERT (commonBlock.pages == 1)
				this->lineHeight = commonBlock.lineHeight;
			}
			break;
			
		case BMF::kChars :
			{
				int charCount = blockSize / sizeof(BMF::CharInfo);
				for(int i = 0; i < charCount; i++)
				{
					BMF::CharInfo charInfo = {};
					stream.readBytes (&charInfo, sizeof(BMF::CharInfo));

					CharDescriptor desc;
					desc.assign (charInfo);

					#if 0//DEBUG_LOG
					DebugPrintf ("char = '%c' x = %d y = %d w = %d h = %d xadv = %d xoff = %d yoff = %d\n",
								charInfo.id, desc.x, desc.y, desc.width, desc.height, desc.xAdvance, desc.xOffset, desc.yOffset);
					#endif

					loadedMap->charMap.add (charInfo.id, desc);
				}
			}
			break;

		case BMF::kKerningPairs :
			{
				int pairCount = blockSize / sizeof(BMF::KerningPair);
				loadedMap->kerningPairs.resize (pairCount);
				for(int i = 0; i < pairCount; i++)
				{
					BMF::KerningPair data = {};
					stream.readBytes (&data, sizeof(BMF::KerningPair));

					KerningPair pair;
					pair.first = data.first;
					pair.second = data.second;
					pair.amount = data.amount;
					loadedMap->kerningPairs.add (pair);
				}
			}
			break;
		}

		// skip missing bytes
		int32 missing = blockSize - int32(stream.getPosition () - oldPos);
		if(missing != 0)
			stream.setPosition (missing, IO::kSeekCur);
	}
			
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BitmapFont::loadInplace (const uint8* fontFileData, uint32 fontFileLength)
{
	ASSERT (fontFileLength > sizeof(BMF::FileHeader))
	const uint8* src = fontFileData;
	uint32 remaining = fontFileLength;

	const BMF::FileHeader* header = reinterpret_cast<const BMF::FileHeader*> (src);
	if(!header->isValid ())
		return false;

	src += sizeof(BMF::FileHeader);
	remaining -= sizeof(BMF::FileHeader);
	while(remaining > 0)
	{
		uint8 blockType = *src++;
		int32 blockSize = *(int32*)src;
		src += 4;

		// sanity check
		if(blockType == 0 || blockSize <= 0 || static_cast<uint32> (blockSize) > remaining)
			break;

		switch(blockType)
		{
		case BMF::kCommon :
			{
				const BMF::CommonBlock* commonBlock = reinterpret_cast<const BMF::CommonBlock*> (src);
				ASSERT (commonBlock->pages == 1)
				this->lineHeight = commonBlock->lineHeight;
			}
			break;

		case BMF::kChars :
			{
				this->inplaceMap.charData = src;
				this->inplaceMap.charCount = blockSize / sizeof(BMF::CharInfo);
			}
			break;

		case BMF::kKerningPairs :
			{
				this->inplaceMap.pairData = src;
				this->inplaceMap.pairCount = blockSize / sizeof(BMF::KerningPair);
			}
			break;
		}

		src += blockSize;
		remaining -= blockSize;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BitmapFont::RenderScope::RenderScope (const BitmapFont& font, Graphics& graphics, ColorRef color)
: graphics (graphics),
  oldRenderMode (0),
  renderModeChanged (false),
  bitmap (nullptr)
{
	if(font.bitmap->getFormat () == kBitmapMonochrome)
	{
		if(color == kInvertColor) // render in inverted mode
		{
			oldRenderMode = graphics.setMode (Graphics::kInvert);
			renderModeChanged = true;
		}

		bitmap = font.bitmap;
	}
	else
	{
		ASSERT (font.bitmap->isAlphaChannelUsed ())
		if(font.colorCachingEnabled)
			bitmap = const_cast<BitmapFont&> (font).prepareColorBitmap (color);
		else
		{
			bitmap = font.bitmap;
			bitmapMode = BitmapMode::kColored;
			bitmapMode.color = color;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BitmapFont::RenderScope::~RenderScope ()
{
	if(renderModeChanged)
		graphics.setMode (oldRenderMode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Bitmap* BitmapFont::prepareColorBitmap (ColorRef color)
{
	ASSERT (bitmap->getFormat () == kBitmapRGBAlpha)
	ASSERT (colorCachingEnabled == true)

	// check if color is cached already
	for(int i = 0; i < colorBitmaps.count (); i++)
	{
		const ColorEntry& entry = colorBitmaps[i];
		if(entry.color == color)
			return entry.bitmap;
	}

	Bitmap* colorBitmap = nullptr;
	if(colorBitmaps.count () < kMaxColorBitmaps)
	{
		colorBitmap = NEW Bitmap (bitmap->getWidth (), bitmap->getHeight ());
		colorBitmap->setAlphaChannelUsed (true);
		colorBitmaps.add (ColorEntry (colorBitmap, color));
	}
	else
	{
		// randomly pick existing entry to overwrite
		int index = rand () % kMaxColorBitmaps;
		ColorEntry& entry = colorBitmaps[index];
		colorBitmap = entry.bitmap;
		entry.color = color;
	}

	BitmapData& dstData = colorBitmap->accessForWrite ();
	const BitmapData& srcData = bitmap->accessForRead ();
	BitmapPrimitives32::colorize (dstData, srcData, color);
	BitmapPrimitives32::premultiplyAlpha (dstData, dstData);

	return colorBitmap;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const BitmapFont::CharMap& BitmapFont::getMap() const
{
	if(loadedMap)
		return *loadedMap;
	else
		return inplaceMap;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int BitmapFont::getStringWidth (CStringPtr string, int length) const
{
	int x = 0;

	Text::UTF8Reader reader (string, length);
	uchar32 c = 0, lastChar = 0;
	
	const CharMap& map = getMap ();
	CharDescriptor desc;
	while((c = reader.getNext ()))
	{
		if(!map.lookup (desc, c))
			continue;

		int amount = map.getKerningAmount (lastChar, c);
		x += amount;

		x += desc.xAdvance;
		lastChar = c;
	}

	return x;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapFont::render (Graphics& graphics, Point leftTopPos, CStringPtr string, int length, ColorRef color) const
{
	int x = leftTopPos.x;
	int y = leftTopPos.y;

	RenderScope scope (*this, graphics, color);
	Bitmap& srcBitmap = *scope.bitmap;
	BitmapMode& mode = scope.bitmapMode;

	Text::UTF8Reader reader (string, length);
	uchar32 c = 0, lastChar = 0;
	
	const CharMap& map = getMap ();
	CharDescriptor desc;
	while((c = reader.getNext ()))
	{
		if(!map.lookup (desc, c))
			continue;

		int amount = map.getKerningAmount (lastChar, c);
		x += amount;

		#if 0//DEBUG_LOG
		DebugPrintf ("char = '%c' x = %d\n", (char)c, x);
		#endif
		
		Point dstPos (x + desc.xOffset, y + desc.yOffset);
		Rect srcRect (0, 0, desc.width, desc.height);
		srcRect.offset (desc.x, desc.y);
		graphics.drawBitmap (dstPos, srcBitmap, srcRect, &mode);

		x += desc.xAdvance;
		lastChar = c;
	}
}

//************************************************************************************************
// BitmapInplaceFont
//************************************************************************************************

BitmapInplaceFont::BitmapInplaceFont (CStringPtr name,
									  const uint8* bitmapFileData, uint32 bitmapFileLength,
									  const uint8* fontFileData, uint32 fontFileLength)
: BitmapFont (name, &fontBitmap, false),
  fontBitmap (bitmapFileData, bitmapFileLength)
{
	loadInplace (fontFileData, fontFileLength);
	colorCachingEnabled = false; // avoid heap allocations from caching
}

//************************************************************************************************
// FontManager
//************************************************************************************************

DEFINE_STATIC_SINGLETON (FontManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

FontManager::FontManager ()
: defaultColorFont (nullptr),
  defaultMonoFont (nullptr),
  externalFontProvider (nullptr),
  fontMap (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FontManager::~FontManager ()
{
	if(fontMap)
	{
		HashMapIterator<uint32, BitmapFont*> iter (*fontMap);
		while(!iter.done ())
			delete iter.next ();
		fontMap->removeAll ();
		delete fontMap;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FontManager::FontMap& FontManager::getFontMap ()
{
	// allocate on demand, might not be used on low-end platforms
	if(fontMap == nullptr)
		fontMap = NEW FontMap (10, ResourceAttributes::hashIntKey);
	return *fontMap;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int FontManager::loadFonts (FilePackage& package, BitmapFileFormat::Format bitmapFormat)
{
	int count = 0;
	Archiver::Format primaryFormat = Archiver::kJSON;
	IO::Stream* jsonStream = package.openStream (Skin::FileNames::kFontFile1);
	if(jsonStream == nullptr)
	{
		jsonStream = package.openStream (Skin::FileNames::kFontFile2);
		primaryFormat = Archiver::kUBJSON;
	}
	if(jsonStream != nullptr)
	{
		Deleter<IO::Stream> deleter (jsonStream);
		Attributes a (AttributeAllocator::getDefault ());
		AttributePoolSuspender suspender; // don't allocate from memory pool
		if(Archiver (jsonStream, primaryFormat).load (a))
			if(const AttributeQueue* fontArray = a.getQueue (nullptr))
				VectorForEach (fontArray->getValues (), AttributeValue*, value)
					if(const Attributes* fontAttr = value->getAttributes ())
					{
						CStringPtr fontName = fontAttr->getString (ResourceAttributes::kName);
						CStringPtr descriptorName = fontAttr->getString (ResourceAttributes::kFile);
						FileName bitmapName = descriptorName;
						bitmapName.setExtension (BitmapFileFormat::getExtension (bitmapFormat));
						bool isMonochrome = fontAttr->getInt (ResourceAttributes::kMonochrome) != 0;
						int fontNumber = (int)fontAttr->getInt (ResourceAttributes::kFontNumber);
						
						FileName descriptorNameHiRes;
						if(DpiSetting::instance ().isHighResolution ())
						{
							ResourceAttributes::makeHiResFileName (bitmapName, bitmapName);
							descriptorName = ResourceAttributes::makeHiResFileName (descriptorNameHiRes, descriptorName);
						}

						Bitmap* bitmap = nullptr;
						if(IO::Stream* bitmapStream = package.openStream (bitmapName))
						{
							switch(bitmapFormat)
							{
							case BitmapFileFormat::kPNG :
								bitmap = Bitmap::loadPNGImage (*bitmapStream, isMonochrome ? kBitmapMonochrome : kBitmapAny);
								break;
							case BitmapFileFormat::kBMP :
								bitmap = Bitmap::loadBMPImage (*bitmapStream);
								break;
							default :
								break;
							}
							delete bitmapStream;
						}

						if(bitmap == nullptr)
						{
							#if DEBUG
							DebugPrintf ("FontManager::loadFonts failed for %s\n", bitmapName.str ());
							#endif
							continue;
						}

						BitmapFont* font = NEW BitmapFont (fontName, bitmap);
						font->setFontNumber (fontNumber);

						bool loaded = false;
						if(IO::Stream* descriptorStream = package.openStream (descriptorName))
						{
							loaded = font->loadDescriptor (*descriptorStream);
							delete descriptorStream;
						}

						if(loaded == true)
						{
							addFont (font);
							count++;
							
							// init default font
							if(fontAttr->getInt (ResourceAttributes::kDefault))
							{
								if(isMonochrome)
									setDefaultMonoFont (font);
								else
									setDefaultColorFont (font);
							}
						}
						else
							delete font;
					}
				EndFor
	}
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FontManager::addFont (BitmapFont* font)
{
	ASSERT (!font->getName ().isEmpty ())
	uint32 key = ResourceAttributes::nameToInt (font->getName ());
	ASSERT (getFontMap ().lookup (key) == 0)
	getFontMap ().add (key, font);
	observers.notify (&FontManagerObserver::onFontAdded, font->getName ().str ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const BitmapFont* FontManager::getFont (CStringPtr name) const
{
	const BitmapFont* font = nullptr;
	if(!ConstString (name).isEmpty ())
	{
		if(externalFontProvider != nullptr)
			font = externalFontProvider->getFont (name);
		else
			font = fontMap ? fontMap->lookup (ResourceAttributes::nameToInt (name)) : 0;
	}
	return font;
}
