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
// Filename    : core/portable/corepersistence.h
// Description : Persistence classes
//
//************************************************************************************************

#ifndef _corepersistence_h
#define _corepersistence_h

#include "core/portable/corefile.h"
#include "core/portable/coreattributes.h"
#include "core/portable/coresingleton.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// AttributesBuilder
/** Build attributes using AttributeHandler interface. \ingroup core_portable */
//************************************************************************************************

class AttributesBuilder: public AttributeHandler
{
public:
	AttributesBuilder (Attributes& root, bool initState = true);

	// AttributeHandler
	void startObject (CStringPtr id, int flags = 0) override;
	void endObject (CStringPtr id, int flags = 0) override;
	void startArray (CStringPtr id, int flags = 0) override;
	void endArray (CStringPtr id, int flags = 0) override;
	void setValue (CStringPtr id, int64 value, int flags = 0) override;
	void setValue (CStringPtr id, double value, int flags = 0) override;
	void setValue (CStringPtr id, bool value, int flags = 0) override;
	void setValue (CStringPtr id, CStringPtr value, int flags = 0) override;
	void setNullValue (CStringPtr id, int flags = 0) override;

private:
	struct State
	{
		bool isObject;
		union
		{
			Attributes* object;
			AttributeQueue* queue;
		};

		State (Attributes* object = nullptr): isObject (true), object (object) {}
		State (AttributeQueue* queue): isObject (false), queue (queue) {}
	};

	Vector<State> stateStack;
	State* currentState;
	Attributes& root;

	void pushState (const State& state);
	void popState ();
};

//************************************************************************************************
// AttributesWriter
/** Write (serialize) attributes to AttributeHandler interface. \ingroup core_portable */
//************************************************************************************************

class AttributesWriter
{
public:
	AttributesWriter (AttributeHandler& handler);

	void writeObject (CStringPtr id, const Attributes& object, int flags = 0);
	void writeArray (CStringPtr id, const AttributeQueue& queue, int flags = 0);
	void writeValue (CStringPtr id, const AttributeValue& a, int flags = 0);

private:
	AttributeHandler& handler;
};

//************************************************************************************************
// Archiver
/** An archiver is used to save/load structured data to/from disk in JSON or UBJSON format.
	\ingroup core_portable */
//************************************************************************************************

class Archiver
{
public:
	enum Format
	{
		kUnknown = -1,
		kJSON,			///< JSON (text-based)
		kUBJSON,		///< Universal Binary JSON
		kJSON5			///< JSON5 (text-based), an extension to JSON
	};

	enum Flags
	{
		kCompact = 1<<0	///< use compact formatting (suppress whitespace)
	};

	Archiver (IO::Stream* stream, Format format = kJSON, int flags = 0);

	static Format detectFormat (CStringPtr fileName);
	static CStringPtr getFileType (Format format);

	bool save (const Attributes& attributes);
	bool load (Attributes& attributes);

	static bool loadInplace (Attributes& attributes, IO::Buffer& buffer, Format format);

protected:
	class ErrorHandler;

	IO::Stream* stream;
	Format format;
	int flags;
};

//************************************************************************************************
// ArchiveUtils
/** Archive utility functions. 
	\ingroup core_portable */
//************************************************************************************************

class ArchiveUtils
{
public:
	static bool saveToFile (CStringPtr filename, const Attributes& attributes, 
							Archiver::Format format = Archiver::kJSON,
							FileStorageContext* compressionContext = nullptr,
							int streamSizeEstimate = 0);

	static bool loadFromFile (Attributes& attributes, CStringPtr filename,
							  Archiver::Format format = Archiver::kJSON,
							  FileStorageContext* compressionContext = nullptr,
							  int streamSizeEstimate = 0,
							  bool inplace = false);

	static bool loadFromStream (Attributes& attributes, IO::MemoryStream& stream, 
								Archiver::Format format = Archiver::kJSON,
								FileStorageContext* compressionContext = nullptr,
								int streamSizeEstimate = 0,
								bool inplace = false);

	static FileIOTaskID saveInBackground (CStringPtr filename, const Attributes& attributes,
								  FileIOCompletionHandler* completionHandler = nullptr,
								  Archiver::Format format = Archiver::kJSON,
								  FileStorageContext::Mode mode = FileStorageContext::kCopy,
								  int streamSizeEstimate = 0);

	struct AttributesPromise;
	static FileIOTaskID saveInBackground (CStringPtr filename, AttributesPromise* attributePromise, 
								  FileIOCompletionHandler* completionHandler = nullptr,
								  Archiver::Format format = Archiver::kJSON,
								  FileStorageContext::Mode mode = FileStorageContext::kCopy,
								  int streamSizeEstimate = 0);

	struct LoadCompletionHandler;
	static FileIOTaskID loadInBackground (CStringPtr filename, LoadCompletionHandler* completionHandler,
										  FileStorageContext::Mode mode = FileStorageContext::kCopy);

	static IO::MemoryStream* saveToStream (const Attributes& attributes, 
	                          Archiver::Format format = Archiver::kJSON, int streamSizeEstimate = 0, FileStorageContext* compressionContext = nullptr);

	static bool saveToStream (IO::MemoryStream& data, const Attributes& attributes, 
	                          Archiver::Format format = Archiver::kJSON, int streamSizeEstimate = 0);
};

//************************************************************************************************
// ArchiveUtils::AttributePromise
/** Promise for attributes created on demand. Used in ArchiveUtils::saveInBackground().  
    \ingroup core_portable */
//************************************************************************************************

struct ArchiveUtils::AttributesPromise
{
	virtual ~AttributesPromise () {}
	virtual bool getAttributes (AttributeHandler& writer) = 0;
};

//************************************************************************************************
// ArchiveUtils::LoadCompletionHandler
/** Callback interface for asynchronous archiving operations.
	\ingroup core_portable */
//************************************************************************************************

struct ArchiveUtils::LoadCompletionHandler: FileIOCompletionHandler
{
	Archiver::Format format;
	int numPreAllocatedAttribs;
	bool inplace;
	bool completeAsync;
	bool canceled;
	
	LoadCompletionHandler (Archiver::Format format = Archiver::kJSON, int numPreAllocatedAttribs = 0, bool inplace = false, bool completeAsync = false)
	: format (format),
	  numPreAllocatedAttribs (numPreAllocatedAttribs),
	  inplace (inplace),
	  completeAsync (completeAsync),
	  canceled (false)
	{}
	
	virtual void loadCompleted (Attributes* attributes, CStringPtr filename, bool& retainData) = 0;
	
	// FileIOCompletionHandler
	void onLoadFileCompleted (IO::MemoryStream* data, CStringPtr filename) override;
	void onLoadFileCompletedAsync (IO::MemoryStream* data, CStringPtr filename) override;
	void onCancel () override;

private:
	void onLoadFileCompletedInternal (IO::MemoryStream* data, CStringPtr filename);
};
	
//************************************************************************************************
// SettingFileHandler
/** Persistent application settings file handler.
	\ingroup core_portable */
//************************************************************************************************

class SettingFileHandler
{
public:
	SettingFileHandler ();
	virtual ~SettingFileHandler ();

	enum Flags { kCompress = 1 << 0, kAlternateFileNames = 1 << 1 };

	void init (CStringPtr companyName, CStringPtr productName, 
			   CStringPtr productFileName = nullptr,
			   Archiver::Format format = Archiver::kJSON,
			   int flags = 0);

	Archiver::Format getArchiveFormat () const { return format; }
	const FileName& getFileName () const;
	bool swapFileName ();
	bool setFileModifyTimesToCurrent ();

	FileIOTaskID saveInBackground (ArchiveUtils::AttributesPromise* promise, FileIOCompletionHandler* completionHandler);
	bool loadAttributes (Attributes& attributes, bool inplace = false);

	FileStorageContext::Mode getStorageMode (bool saving) const;

	PROPERTY_FLAG (flags, kAlternateFileNames, alternateFileNames)
	PROPERTY_FLAG (flags, kDirty, isDirty)
	PROPERTY_VARIABLE (int, streamSizeEstimate, StreamSizeEstimate) // avoid multiple memory allocations when saving

protected:
	Archiver::Format format;
	FileName filename;
	FileName altFilename;
	enum InternalFlags { kDirty = 1 << 2, kUseAltFileName = 1 << 3 };
	PROPERTY_FLAG (flags, kUseAltFileName, useAltFileName)
	PROPERTY_FLAG (flags, kCompress, isCompressed)
	int flags;
};

//************************************************************************************************
// SettingFile
/** Persistent application settings.
	\ingroup core_portable */
//************************************************************************************************

class SettingFile: public SettingFileHandler,
                   public Attributes,
				   public StaticSingleton<SettingFile>
{
public:
	SettingFile ();
	~SettingFile ();

	Attributes& getSection (CStringPtr id)
	{
		Attributes* a = getAttributes (id);
		if(a == nullptr)
			a = makeAttributes (id);
		ASSERT (a != nullptr)
		return *a;
	}
		
	void storeIfNeeded ()
	{
		if(isDirty ())
		{
			store ();
			isDirty (false);
		}
	}

	void setDirty (bool state = true)
	{
		isDirty (state);
	}

	bool restore ();
	bool store ();
	bool storeInBackground (FileIOCompletionHandler* completionHandler = nullptr);
};

//************************************************************************************************
// DevelopmentSettings
/** Settings used for development, per user, available in debug and release build.
	\ingroup core_portable */
//************************************************************************************************

class DevelopmentSettings: public Attributes,
						   public StaticSingleton<DevelopmentSettings>
{
public:
	DevelopmentSettings ();

	bool load ();
	bool getLocation (FileName& fileName, CStringPtr id) const;
};

} // namespace Portable
} // namespace Core

#endif // _corepersistence_h
