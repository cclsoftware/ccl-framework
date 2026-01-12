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
// Filename    : core/portable/corepersistence.cpp
// Description : Persistence classes
//
//************************************************************************************************

#define CORE_PROFILE (0 && DEBUG)

#include "core/portable/corepersistence.h"
#include "core/portable/coreprofiling.h"
#include "core/public/coreprimitives.h"
#include "core/text/corejsonhandler.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// Archiver::ErrorHandler
//************************************************************************************************

class Archiver::ErrorHandler: public Core::Text::Json::ErrorHandler
{
public:
	// ErrorHandler
	void onError (int64 position, CStringPtr errorMessage) override
	{
		#if DEBUG
		DebugPrintf ("JSON Archiver error at %d: %s\n", (int)position, errorMessage);
		ASSERT (0)
		#endif
	}
};

} // namespace Portable
} // namespace Core

using namespace Core;
using namespace Portable;

DEFINE_STATIC_SINGLETON (Core::Portable::SettingFile)

//************************************************************************************************
// DevelopmentSettings
//************************************************************************************************

DEFINE_STATIC_SINGLETON (DevelopmentSettings)

//////////////////////////////////////////////////////////////////////////////////////////////////

DevelopmentSettings::DevelopmentSettings ()
: Attributes (AttributeAllocator::getDefault ())
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DevelopmentSettings::load ()
{
	FileName fileName;
	FileUtils::getHomeDir (fileName);
	fileName.descend ("core-development.json");
	return ArchiveUtils::loadFromFile (*this, fileName);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DevelopmentSettings::getLocation (FileName& fileName, CStringPtr id) const
{
	fileName = getString (id);
	if(fileName.isEmpty ())
		return false;

	if(fileName.isRelative ())
	{
		FileName homeDir;
		FileUtils::getHomeDir (homeDir);
		fileName.makeAbsolute (homeDir);
	}
	return true;
}

//************************************************************************************************
// SettingFileHandler
//************************************************************************************************

SettingFileHandler::SettingFileHandler ()
: format (Archiver::kJSON),
  flags (0),
  streamSizeEstimate (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SettingFileHandler::~SettingFileHandler ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SettingFileHandler::init (CStringPtr companyName, CStringPtr productName, CStringPtr productFileName, Archiver::Format _format,  int _flags)
{
	flags = _flags;
	format = _format;

	FileUtils::getDataDir (filename);
	filename.descend (companyName);
	FileUtils::makeDirectory (filename);
	filename.descend (productName);
	FileUtils::makeDirectory (filename);

	if(!ConstString (productFileName).isEmpty ())
		filename.descend (productFileName);
	else
		filename.descend (productName); // use product name as file name
	
	altFilename = filename;
	altFilename.append ("_alt");

	if(format == Archiver::kUBJSON)
	{
		filename += ".bsettings";
		altFilename += ".bsettings";
	}
	else
	{
		filename += ".settings";
		altFilename += ".settings";
	}

	if(flags & kCompress)
	{
		filename += ".zz";
		altFilename += ".zz";
	}

	if(alternateFileNames ())
	{
		int64 time1 = FileUtils::fileLastModified (filename);
		int64 time2 = FileUtils::fileLastModified (altFilename);
		useAltFileName (time2 > time1);		
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SettingFileHandler::setFileModifyTimesToCurrent ()
{
	if(FileUtils::fileExists (filename))
		FileUtils::touchFile (filename);
	
	if(FileUtils::fileExists (altFilename))
		FileUtils::touchFile (altFilename);
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileName& SettingFileHandler::getFileName () const
{
	if(alternateFileNames () && useAltFileName ())
		return altFilename;
	return filename;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SettingFileHandler::swapFileName ()
{
	if(alternateFileNames ())
	{
		useAltFileName (!useAltFileName ());
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileStorageContext::Mode SettingFileHandler::getStorageMode (bool saving) const
{
	if(isCompressed ())
		return saving ? FileStorageContext::kCompress : FileStorageContext::kDecompress;
	else
		return FileStorageContext::kCopy;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileIOTaskID SettingFileHandler::saveInBackground (ArchiveUtils::AttributesPromise* attributePromise, FileIOCompletionHandler* completionHandler)
{
	struct SaveCompletionHandler: FileIOCompletionHandler
	{
		FileIOCompletionHandler* outer;
		SettingFileHandler& handler;

		SaveCompletionHandler (SettingFileHandler& _handler, FileIOCompletionHandler* _outer)
		: handler (_handler),
		  outer (_outer)
		{}
		
		~SaveCompletionHandler () { delete outer; }

		// FileIOCompletionHandler
		void onSaveFileCompleted (CStringPtr filename)  override
		{
			if(outer)
				outer->onSaveFileCompleted (filename);
			
			handler.swapFileName ();				
		}
		
		void onCancel () override
		{
			if(outer)
				outer->onCancel ();
		}
	};

	return ArchiveUtils::saveInBackground (
		getFileName (), 
		attributePromise, 
		NEW SaveCompletionHandler (*this, completionHandler), 
		getArchiveFormat (),
		getStorageMode (true),
		getStreamSizeEstimate ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SettingFileHandler::loadAttributes (Attributes& attributes, bool inplace)
{
	FileStorageContext* compressionContext = isCompressed () ? &FileStorageContext::getMainThreadInstance () : nullptr;
	bool success = ArchiveUtils::loadFromFile (attributes, getFileName (), format, compressionContext, getStreamSizeEstimate (), inplace);
	swapFileName ();
	if(success == false && alternateFileNames ())
	{
		success = ArchiveUtils::loadFromFile (attributes, getFileName (), format, compressionContext, getStreamSizeEstimate (), inplace);
		swapFileName ();
	}
	return success;
}

//************************************************************************************************
// SettingFile
//************************************************************************************************

SettingFile::SettingFile ()
: Attributes (AttributeAllocator::getDefault ())
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SettingFile::~SettingFile ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SettingFile::restore ()
{
	return loadAttributes (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SettingFile::store ()
{
	FileStorageContext* compressionContext = isCompressed () ? &FileStorageContext::getMainThreadInstance () : nullptr;
	if(ArchiveUtils::saveToFile (getFileName (), *this, format, compressionContext, getStreamSizeEstimate ()))
	{
		swapFileName ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SettingFile::storeInBackground (FileIOCompletionHandler* completionHandler)
{
	if(ArchiveUtils::saveInBackground (getFileName (), *this, completionHandler, format, getStorageMode (true), getStreamSizeEstimate ()))
	{
		swapFileName ();
		return true;
	}
	return false;
}

//************************************************************************************************
// ArchiveUtils
//************************************************************************************************

bool ArchiveUtils::saveToFile (CStringPtr filename, const Attributes& attributes, Archiver::Format format, 
							   FileStorageContext* compressionContext, int streamSizeEstimate)
{
	bool result = false;
	IO::MemoryStream data;
	if(saveToStream (data, attributes, format, streamSizeEstimate))
	{
		if(compressionContext)
			result = compressionContext->saveFile (filename, data, FileStorageContext::kCompress);
		else
			result = FileUtils::saveFile (filename, data);
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ArchiveUtils::loadFromFile (Attributes& attributes, CStringPtr filename, Archiver::Format format, 
								 FileStorageContext* compressionContext, int streamSizeEstimate, bool inplace)
{
	IO::MemoryStream* stream = nullptr;
	if(compressionContext)
		stream = compressionContext->loadFile (filename, FileStorageContext::kDecompress, streamSizeEstimate);
	else
		stream = FileUtils::loadFile (filename);
	if(stream == nullptr)
		return false;

	Deleter<IO::MemoryStream> streamDeleter (stream);

	bool result;
	CORE_PROFILE_START (archiveUtils)

	if(inplace == true)
	{		
		IO::Buffer& buffer = const_cast<IO::Buffer&> (stream->getBuffer ());
		buffer.setValidSize (stream->getBytesWritten ());
		result = Archiver::loadInplace (attributes, buffer, format);
	}
	else
	{
		Archiver archiver (stream, format);
		result = archiver.load (attributes);
	}

	CORE_PROFILE_STOP (archiveUtils, "ArchiveUtils::loadFromFile")
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ArchiveUtils::loadFromStream (Attributes& attributes, IO::MemoryStream& _stream, Archiver::Format format,
								   FileStorageContext* compressionContext, int streamSizeEstimate, bool inplace)
{
	IO::MemoryStream* stream = nullptr;
	Deleter<IO::MemoryStream> streamDeleter (nullptr);
	if(compressionContext)
	{
		stream = NEW IO::MemoryStream;
		streamDeleter._ptr = stream;
		if(streamSizeEstimate > 0)
			stream->allocateMemory (streamSizeEstimate);
		if(!compressionContext->decompress (*stream, _stream))
			return false;

		// sanity check - increase estimate if needed!
		ASSERT (streamSizeEstimate == 0 || stream->getBuffer ().getSize () == streamSizeEstimate)
	}
	else
		stream = &_stream;

	bool result;
	CORE_PROFILE_START (loadFromStream)

	if(inplace == true)
	{		
		IO::Buffer& buffer = const_cast<IO::Buffer&> (stream->getBuffer ());
		buffer.setValidSize (stream->getBytesWritten ());
		result = Archiver::loadInplace (attributes, buffer, format);
	}
	else
	{
		Archiver archiver (stream, format);
		result = archiver.load (attributes);
	}

	CORE_PROFILE_STOP (loadFromStream, "ArchiveUtils::loadFromStream")
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileIOTaskID ArchiveUtils::saveInBackground (CStringPtr filename, const Attributes& attributes, FileIOCompletionHandler* completionHandler,
									Archiver::Format format, FileStorageContext::Mode mode, int streamSizeEstimate)
{
	IO::MemoryStream* data = saveToStream (attributes, format, streamSizeEstimate);
	if(!data)	
		return nullptr;
	
	return FileIOManager::instance ().addSaveTask (filename, data, completionHandler, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileIOTaskID ArchiveUtils::saveInBackground (CStringPtr filename, AttributesPromise* attributePromise, FileIOCompletionHandler* completionHandler,
								    Archiver::Format format, FileStorageContext::Mode mode, int streamSizeEstimate)
{
	struct DataPromise: FileDataPromise
	{
		AttributesPromise* attributesPromise;
		Archiver::Format format;
		int streamSizeEstimate;

		DataPromise (AttributesPromise* _attributesPromise, Archiver::Format _format, int _streamSizeEstimate)
		: attributesPromise (_attributesPromise),
		  format (_format),
		  streamSizeEstimate (_streamSizeEstimate)
		{}
		
		~DataPromise () {delete attributesPromise;}

		// FileDataPromise
		IO::MemoryStream* createFileData () override
		{
			IO::MemoryStream* data = nullptr;

			CORE_PROFILE_START (saveInBackground)

			#if 0 // implementation using intermediate attributes (slower on embedded platforms)
			Attributes attributes;
			AttributesBuilder builder (attributes);
			if(attributesPromise->getAttributes (builder))
				data = saveToStream (attributes, format, streamSizeEstimate);

			#else // optimized pass-through to underlying file format
			data = NEW IO::MemoryStream;
			data->allocateMemory (streamSizeEstimate);
			bool result = false;
			if(format == Archiver::kUBJSON)
			{
				Text::Json::BinaryWriter writer (data);
				writer.startObject (nullptr);
				result = attributesPromise->getAttributes (writer);
				writer.endObject (nullptr);
			}
			else
			{
				ASSERT (format == Archiver::kJSON)
				Text::Json::Writer writer (data);
				writer.startObject (nullptr);
				result = attributesPromise->getAttributes (writer);
				writer.endObject (nullptr);				
				writer.flush ();
			}
			if(result == false)
			{
				delete data;
				data = nullptr;
			}
			#endif

			CORE_PROFILE_STOP (saveInBackground, "AttributesPromise::getAttributes serialized")

			return data;
		}
	};

	return FileIOManager::instance ().addSaveTask (
		filename, 
		NEW DataPromise (attributePromise, format, streamSizeEstimate), 
		completionHandler, 
		mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileIOTaskID ArchiveUtils::loadInBackground (CStringPtr filename, ArchiveUtils::LoadCompletionHandler* completionHandler,
									FileStorageContext::Mode mode)
{
	return FileIOManager::instance ().addLoadTask (filename, completionHandler, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ArchiveUtils::saveToStream (IO::MemoryStream& data, const Attributes& attributes, Archiver::Format format, int streamSizeEstimate)
{
	if(streamSizeEstimate > 0)
		data.allocateMemory (streamSizeEstimate);
	
	Archiver archiver (&data, format);
	if(!archiver.save (attributes))		
		return false;

	// sanity check - increase estimate if needed!
	ASSERT (streamSizeEstimate == 0 || data.getBuffer ().getSize () == streamSizeEstimate)
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IO::MemoryStream* ArchiveUtils::saveToStream (const Attributes& attributes, Archiver::Format format, int streamSizeEstimate, FileStorageContext* compressionContext)
{
	IO::MemoryStream* data = NEW IO::MemoryStream;
	if(saveToStream (*data, attributes, format, streamSizeEstimate) == false)
	{
		delete data;
		return nullptr;
	}

	if(compressionContext)
	{
		IO::MemoryStream* compressed = NEW IO::MemoryStream;
		bool success = compressionContext->compress (*compressed, *data);
		delete data;
		data = compressed;
		
		if(success == false)
		{
			delete data;
			return nullptr;		
		}	
	}

	return data;
}

//************************************************************************************************
// ArchiveUtils::LoadCompletionHandler
//************************************************************************************************

void ArchiveUtils::LoadCompletionHandler::onLoadFileCompleted (IO::MemoryStream* data, CStringPtr filename)
{
	if(completeAsync == false)
		onLoadFileCompletedInternal (data, filename);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ArchiveUtils::LoadCompletionHandler::onLoadFileCompletedAsync (IO::MemoryStream* data, CStringPtr filename)
{
	if(completeAsync == true)
		onLoadFileCompletedInternal (data, filename);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ArchiveUtils::LoadCompletionHandler::onCancel ()
{
	canceled = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ArchiveUtils::LoadCompletionHandler::onLoadFileCompletedInternal (IO::MemoryStream* data, CStringPtr filename)
{
	Attributes* attributes = nullptr;
	bool loaded = false;

	if(data && !canceled)
	{
		if(numPreAllocatedAttribs > 0)
			attributes = NEW PreAllocatedAttributes (numPreAllocatedAttribs);
		else
			attributes = NEW Attributes (AttributeAllocator::getDefault ());

		if(inplace)
		{
			IO::Buffer& buffer = const_cast<IO::Buffer&> (data->getBuffer ());
			buffer.setValidSize (data->getBytesWritten ());
			loaded = Archiver::loadInplace (*attributes, buffer, format);
		}
		else
		{
			Archiver archiver (data, format);
			loaded = archiver.load (*attributes);
		}
	}

	bool retainData = false;
	loadCompleted (loaded && !canceled ? attributes : nullptr, filename, retainData);
	
	if(retainData == false)
		delete attributes;
}

//************************************************************************************************
// Archiver
//************************************************************************************************

Archiver::Format Archiver::detectFormat (CStringPtr fileName)
{
	int index = ConstString (fileName).lastIndex ('.');
	if(index != -1)
	{
		ConstString ext (fileName + index + 1);
		if(ext == "json")
			return kJSON;
		if(ext == "ubj")
			return kUBJSON;
	}
	return kUnknown;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr Archiver::getFileType (Format format)
{
	switch(format)
	{
	case kJSON : return "json";
	case kUBJSON : return "ubj";
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Archiver::Archiver (IO::Stream* stream, Format format, int flags)
: stream (stream),
  format (format),
  flags (flags)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Archiver::save (const Attributes& attributes)
{
	// TODO: better error handling needed here!!!

	if(format == kUBJSON)
	{
		Text::Json::BinaryWriter handler (stream);
		AttributesWriter writer (handler);
		writer.writeObject (nullptr, attributes);
		return handler.getResult ();
	}
	else
	{
		Text::Json::Writer handler (stream);
		handler.setSuppressWhitespace ((flags & kCompact) != 0);
		AttributesWriter writer (handler);
		writer.writeObject (nullptr, attributes);
		return handler.flush ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Archiver::load (Attributes& attributes)
{
	bool result = false;
	CORE_PROFILE_START (load)
	attributes.removeAll ();
	if(format == kUBJSON)
	{
		AttributesBuilder builder (attributes, false);
		ErrorHandler errorHandler;
		Text::Json::BinaryParser parser (stream, &builder, &errorHandler);
		result = parser.parse ();
	}
	else
	{
		AttributesBuilder builder (attributes, false);
		ErrorHandler errorHandler;
		Text::Json::Parser parser (stream, &builder, &errorHandler, format == kJSON5);
		result = parser.parse ();
	}
	CORE_PROFILE_STOP (load, "Archiver::load")
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/ bool Archiver::loadInplace (Attributes& attributes, IO::Buffer& buffer, Format format)
{
	bool result = false;
	CORE_PROFILE_START (loadInplace)
	attributes.removeAll ();

	ASSERT (format == kUBJSON)
	if(format == kUBJSON)
	{
		AttributesBuilder builder (attributes, false);
		ErrorHandler errorHandler;
		Text::Json::BinaryInplaceParser parser (buffer, &builder, &errorHandler);
		result = parser.parse ();
	}

	if(result == true)
	{
		// keep buffer alive with attributes
		IO::Buffer* inplaceBuffer = NEW IO::Buffer;
		inplaceBuffer->take (buffer);
		ASSERT (attributes.getInplaceBuffer () == nullptr)
		attributes.setInplaceBuffer (inplaceBuffer);
	}
	CORE_PROFILE_STOP (loadInplace, "Archiver::loadInplace")
	return result;
}

//************************************************************************************************
// AttributesBuilder
//************************************************************************************************

AttributesBuilder::AttributesBuilder (Attributes& root, bool initState)
: currentState (nullptr),
  root (root)
{
	if(initState) // make ready to write to root attributes
		pushState (&root);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributesBuilder::pushState (const State& state)
{
	stateStack.add (state);
	currentState = &stateStack.at (stateStack.count () -  1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributesBuilder::popState ()
{
	int lastIndex = stateStack.count () -  1;
	if(lastIndex >= 0)
	{
		stateStack.removeAt (lastIndex);
		currentState = lastIndex == 0 ? nullptr : &stateStack.at (lastIndex - 1);
	}
	else
		currentState = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributesBuilder::startObject (CStringPtr id, int flags)
{
	Attributes* object = nullptr;
	if(currentState)
	{
		object = currentState->isObject
			? currentState->object->addAttributes (id, flags)
			: currentState->queue->appendAttributes (root.getAllocator ());
	}
	else
		object = &root;

	pushState (State (object));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributesBuilder::endObject (CStringPtr id, int /*flags*/)
{
	popState ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributesBuilder::startArray (CStringPtr id, int flags)
{
	AttributeQueue* queue = nullptr;

	if(currentState)
	{
		queue = currentState->isObject
			? currentState->object->addQueue (id, flags)
			: currentState->queue->appendQueue ();
	}
	else
	{
		// array on top level: make anonymous queue in root
		ASSERT (id == nullptr || *id == 0)
		queue = root.addQueue (id);
	}

	pushState (State (queue));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributesBuilder::endArray (CStringPtr /*id*/, int /*flags*/)
{
	popState ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CORE_HOT_FUNCTION
void AttributesBuilder::setValue (CStringPtr id, int64 value, int flags)
{
	if(currentState)
	{
		if(currentState->isObject)
			currentState->object->add (id, value, flags);
		else
			currentState->queue->append (value);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CORE_HOT_FUNCTION
void AttributesBuilder::setValue (CStringPtr id, double value, int flags)
{
	if(currentState)
	{
		if(currentState->isObject)
			currentState->object->add (id, value, flags);
		else
			currentState->queue->append (value);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributesBuilder::setValue (CStringPtr id, bool value, int flags)
{
	if(currentState)
	{
		if(currentState->isObject)
			currentState->object->add (id, value, flags);
		else
			currentState->queue->append ((int64)value);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributesBuilder::setValue (CStringPtr id, CStringPtr value, int flags)
{
	if(currentState)
	{
		if(currentState->isObject)
			currentState->object->add (id, value, flags);
		else
			currentState->queue->append (value);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributesBuilder::setNullValue (CStringPtr id, int /*flags*/)
{
}

//************************************************************************************************
// AttributesWriter
//************************************************************************************************

AttributesWriter::AttributesWriter (AttributeHandler& handler)
: handler (handler)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributesWriter::writeObject (CStringPtr id, const Attributes& object, int flags)
{
	handler.startObject (id, flags); // kShareID flags is valid for initial call only!

	int num = object.countAttributes ();
	for(int i = 0; i < num; i++)
		if(const Attribute* a = object.getAttribute (i))
			writeValue (a->getID (), *a);

	handler.endObject (id, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributesWriter::writeArray (CStringPtr id, const AttributeQueue& queue, int flags)
{
	handler.startArray (id, flags);

	VectorForEachFast (queue.getValues (), AttributeValue*, v)
		writeValue (nullptr, *v);
	EndFor

	handler.endArray (id, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributesWriter::writeValue (CStringPtr id, const AttributeValue& a, int flags)
{
	switch(a.getType ())
	{
	case Attribute::kInt:
		handler.setValue (id, a.getInt (), flags);
		break;

	case Attribute::kFloat:
		handler.setValue (id, a.getFloat (), flags);
		break;

	case Attribute::kString:
		handler.setValue (id, a.getString (), flags);
		break;

	case Attribute::kQueue:
		if(const AttributeQueue* queue = a.getQueue ())
			writeArray (id, *queue, flags);
		break;

	case Attribute::kAttributes:
		if(const Attributes* attribs = a.getAttributes ())
			writeObject (id, *attribs, flags);
		break;

	case 0:
		handler.setNullValue (id, flags);
		break;
	}
}
