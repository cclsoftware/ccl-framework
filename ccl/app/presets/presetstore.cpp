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
// Filename    : ccl/app/presets/presetstore.cpp
// Description : Preset Store
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/presets/presetstore.h"
#include "ccl/app/presets/presetsystem.h"
#include "ccl/app/presets/presetfile.h"
#include "ccl/app/presets/presetfileprimitives.h"
#include "ccl/app/presets/simplepreset.h"

#include "ccl/base/storage/filefilter.h"
#include "ccl/base/storage/persistence/expression.h"
#include "ccl/base/collections/linkablelist.h"

#include "ccl/public/app/presetmetainfo.h"

#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/collections/variantvector.h"
#include "ccl/public/base/iprogress.h"
#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/isearcher.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

using namespace CCL;
using namespace Persistence;

//************************************************************************************************
// PresetLocation
//************************************************************************************************

class PresetLocation: public DataItem
{
public:
	DECLARE_CLASS (PresetLocation, DataItem)
	DECLARE_PROPERTY_NAMES (PresetLocation)

	PresetLocation ();

	PROPERTY_FLAG (flags, 1<<0, isNew)
	PROPERTY_FLAG (flags, 1<<1, isRemoved)
	PROPERTY_FLAG (flags, 1<<2, needsUpdate)

	PROPERTY_VARIABLE (int, flags, Flags)
	PROPERTY_STRING (subFolderPrefix, SubFolderPrefix)

	// IPersistentObject
	void CCL_API storeMembers (Persistence::IObjectState& state) const override;
	void CCL_API restoreMembers (Persistence::IObjectState& state) override;
};

//************************************************************************************************
// PresetStore::PresetSearcher
//************************************************************************************************

class PresetStore::PresetSearcher: public Object,
								   public AbstractSearcher
{
public:
	PresetSearcher (ISearchDescription& searchDescription, DataStore& dataStore);

	// ISearcher
	tresult CCL_API find (ISearchResultSink& resultSink, IProgressNotify* progress) override;

	CLASS_INTERFACE (ISearcher, Object)

protected:
	DataStore& dataStore;
};

//************************************************************************************************
// PresetStore::PresetFilter
//************************************************************************************************

class PresetStore::PresetFilter
{
public:
	inline PresetFilter (IPresetFileRegistry& registry, IAttributeList* metaInfo);

	inline void add (PresetDescriptor* descriptor);
	inline ObjectList& getVisibleDescriptors ();

private:
	Url writeLocation;
	ObjectList userDescriptors;
	ObjectList otherDescriptors;

	inline bool isHiddenByUserPreset (PresetDescriptor* descriptor);
};

//************************************************************************************************
// PresetStore::PresetFilter
//************************************************************************************************

inline PresetStore::PresetFilter::PresetFilter (IPresetFileRegistry& registry, IAttributeList* metaInfo)
{
	userDescriptors.objectCleanup (true);
	otherDescriptors.objectCleanup (true);

	IPresetFileHandler* handler = metaInfo ? SimplePresetHandler::findHandler (*metaInfo) : nullptr;
	if(!handler)
		handler = registry.getDefaultHandler ();
	if(handler)
		handler->getWriteLocation (writeLocation, metaInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void PresetStore::PresetFilter::add (PresetDescriptor* descriptor)
{
	descriptor->retain ();
	if(writeLocation.contains (descriptor->getUrl ()))
		userDescriptors.add (descriptor);
	else
		otherDescriptors.add (descriptor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool PresetStore::PresetFilter::isHiddenByUserPreset (PresetDescriptor* descriptor)
{
	// preset files in the write location hide presets from other locations with the same name & subFolder
	ListForEachObject (userDescriptors, PresetDescriptor, userDescriptor)
		if(userDescriptor->getTitle () == descriptor->getTitle ()
			&& !userDescriptor->getTitle ().isEmpty ()
			&& userDescriptor->getSubFolder () == descriptor->getSubFolder ())
		{
			#if DEBUG_LOG
			MutableCString t (descriptor->getTitle ());
			MutableCString u1 (UrlFullString (descriptor->getUrl ()));
			MutableCString u2 (UrlFullString (userDescriptor->getUrl ()));
			CCL_PRINTF ("hidden preset: %s (\"%s\")\n", u1.str (), t.str ());
			CCL_PRINTF ("           by: %s\n", u2.str ());
			#endif
			return true;
		}
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline ObjectList& PresetStore::PresetFilter::getVisibleDescriptors ()
{
	ListForEachObject (otherDescriptors, PresetDescriptor, otherDescriptor)
		if(!isHiddenByUserPreset (otherDescriptor))
		{
			otherDescriptor->retain ();
			userDescriptors.add (otherDescriptor);
		}
	EndFor
	return userDescriptors;
}

//************************************************************************************************
// PresetStore::PresetSearcher
//************************************************************************************************

PresetStore::PresetSearcher::PresetSearcher (ISearchDescription& searchDescription, DataStore& dataStore)
: AbstractSearcher (searchDescription),
  dataStore (dataStore)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PresetStore::PresetSearcher::find (ISearchResultSink& resultSink, IProgressNotify* progress)
{
	UrlRef startPoint = searchDescription.getStartPoint ();
	StringRef reference = startPoint.getPath ();
	ASSERT (!reference.isEmpty ())
	
	// translate search terms with simple wildcard "*" to sql LIKE syntax
	// hmm, knowledge of sql syntax should not be required here...
	StringRef simpleWildcard = CCLSTR ("*");
	StringRef sqlWildcard = CCLSTR ("%");

	Expression condition;
	StringList searchTokens;
	if(searchDescription.getSearchTokenCount () > 0)
	{
		for(int i = 0; i < searchDescription.getSearchTokenCount (); i++)
		{
			StringRef searchToken = searchDescription.getSearchToken (i);
			if(!searchToken.isEmpty ())
				searchTokens.add (searchToken);
		}
	}
	else
		searchTokens.add (searchDescription.getSearchTerms ());
		
	for(const auto& searchToken : searchTokens)
	{
		String searchStringBeginning; // matches search terms only at beginning of a member string (no leading wildcard)
		ForEachStringToken (*searchToken, simpleWildcard, token)
			searchStringBeginning << token << sqlWildcard;
		EndFor

		String searchStringAnywhere;
		searchStringAnywhere << sqlWildcard << searchStringBeginning;

		String searchStringSubFolder;
		searchStringSubFolder << sqlWildcard << Url::strPathChar << searchStringBeginning; // match search term as beginning of a subFolder segment

		Expression tokenCondition = Member ("title").like (searchStringAnywhere)
			|| Member ("vendor").like (searchStringAnywhere)
			|| Member ("creator").like (searchStringAnywhere)
			|| Member ("subFolder").like (searchStringBeginning)
			|| Member ("subFolder").like (searchStringSubFolder);
			
		if(condition.isValid ())
		{
			if(searchDescription.getOptions () & ISearchDescription::kMatchAllTokens)
				condition = condition && tokenCondition;
			else
				condition = condition || tokenCondition;
		}
		else
			condition = tokenCondition;
	}

	if(condition.isValid ())
	{
		MutableCString propertyKey;
		if(startPoint.getProtocol () == "class")
			propertyKey = "classID";
		else if(startPoint.getProtocol () == "category")
			propertyKey = "category";
		else return kResultFailed;

		condition = condition && Member (propertyKey) == reference;
		IterForEach (dataStore.query<PresetDescriptor> (condition), PresetDescriptor, descriptor)
			if(progress && progress->isCanceled ())
				return kResultAborted;

			Url* url = NEW Url (descriptor->getUrl ());
			resultSink.addResult (url->asUnknown ());
		EndFor
	}
	
	return kResultOk;
}

//************************************************************************************************
// PresetLocation
//************************************************************************************************

DEFINE_CLASS (PresetLocation, DataItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (PresetLocation)
	DEFINE_PROPERTY_TYPE ("subFolder", ITypeInfo::kString)
END_PROPERTY_NAMES (PresetLocation)

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetLocation::PresetLocation ()
: flags (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetLocation::storeMembers (IObjectState& state) const
{
	SuperClass::storeMembers (state);

	state.set ("subFolder", subFolderPrefix);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetLocation::restoreMembers (IObjectState& state)
{
	SuperClass::restoreMembers (state);

	subFolderPrefix = state.get ("subFolder");
}

//************************************************************************************************
// PresetStore
//************************************************************************************************

PresetStore::PresetStore ()
{
	// uses datastore at default location ("DataStore.db")

	// prepare DataStore for storing PresetDescriptor
	dataStore.registerClass (ccl_typeid<PresetDescriptor> ());
	dataStore.setMemberFlags (ccl_typeid<PresetDescriptor> (), "category", kIndexRequired);
	dataStore.setMemberFlags (ccl_typeid<PresetDescriptor> (), "classID", kIndexRequired);

	dataStore.registerClass (ccl_typeid<PresetLocation> ());

	collectClassKeys ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetStore::collectClassKeys ()
{
	CCL_PROFILE_START (PresetStore_collectClassKeys)
	cachedClassKeys.removeAll ();

	// 1.) collect non-empty classIDs
	VariantVector classIDs;
	Expression cidNotEmpty = Member ("classID") != String::kEmpty;
	dataStore.collectValues<PresetDescriptor> (classIDs, "classID", cidNotEmpty);

	VectorForEachFast (classIDs, Variant, v)
		CCL_PRINTF ("PresetStore: classKey: %s\n", MutableCString (v).str ())
		cachedClassKeys.add (v);
	EndFor

	// 2.) also add alternative classes
	VectorForEachFast (classIDs, Variant, v)
		UID classId;
		classId.fromString (v);	
		if(const IClassDescription* alternativeClass = System::GetPlugInManager ().getAlternativeClass (classId))
		{
			CCL_PRINTF ("PresetStore: classKey (alternative): %s\n", MutableCString (UIDString (alternativeClass->getClassID ())).str ())
			cachedClassKeys.addOnce (UIDString (alternativeClass->getClassID ()));
		}
	EndFor

	// 2.) collect categories (where classID is empty)
	VariantVector categories;
	Expression cidEmpty = Member ("classID") == String::kEmpty;
	dataStore.collectValues<PresetDescriptor> (categories, "category", cidEmpty);

	VectorForEachFast (categories, Variant, v)
		CCL_PRINTF ("PresetStore: category: %s\n", MutableCString (v).str ())
		cachedClassKeys.add (v);
	EndFor

	CCL_PROFILE_STOP (PresetStore_collectClassKeys)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetStore::collectSubFolders (IMutableArray& subFolders, const IAttributeList* metaInfo)
{
	Expression classCondition;
	if(metaInfo)
		classCondition = makeClassCondition (*metaInfo);

	dataStore.collectValues<PresetDescriptor> (subFolders, "subFolder", classCondition);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PresetStore::getClassKey (const IAttributeList& metaInfo)
{
	PresetMetaAttributes metaAttribs (ccl_const_cast (metaInfo));
	return metaAttribs.getClassKey (); // classID or category
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Expression PresetStore::makeClassCondition (const IAttributeList& metaInfo)
{
	PresetMetaAttributes metaAttribs (ccl_const_cast (metaInfo));

	// query by classID or category
	String classIdString (metaAttribs.getString (Meta::kClassID));
	if(!classIdString.isEmpty ())
	{
		String alternativeClassIdString;
		if(metaAttribs.getString (alternativeClassIdString, Meta::kAlternativeClassID) && !alternativeClassIdString.isEmpty ())
			return Member ("classID") == classIdString || Member ("classID") == alternativeClassIdString;
		else
			return Member ("classID") == classIdString;
	}
	else
		return Member ("category") == metaAttribs.getCategory ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetStore::isEmpty () const
{
	return IterHasData (dataStore.query<PresetDescriptor> ()) == false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetStore::hasPresets (IAttributeList& metaInfo) const
{
	return cachedClassKeys.contains (getClassKey (metaInfo));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetStore::addPreset (UrlRef url, const IPreset& preset)
{
	FileInfo fileInfo;
	System::GetFileSystem ().getFileInfo (fileInfo, url);

	AutoPtr<PresetDescriptor> descriptor (NEW PresetDescriptor ());
	descriptor->initWithPreset (preset, url, fileInfo.modifiedTime);
	addPreset (descriptor);
	flush (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetStore::onPresetCreated (UrlRef url, const IPreset& preset)
{
	// find preset in store and check if file has changed
	AutoPtr<PresetDescriptor> descriptor = getPresetDescriptor (url);
	if(descriptor)
	{
		FileInfo fileInfo;
		System::GetFileSystem ().getFileInfo (fileInfo, url);

		if(descriptor->wasLastModifiedAt (fileInfo.modifiedTime))
			return;

		descriptor->initWithPreset (preset, url, fileInfo.modifiedTime);
		updatePreset (descriptor);
		flush (true);
	}
	else
		addPreset (url, preset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetStore::onPresetRemoved (UrlRef url, const IPreset& preset)
{
	// find preset object in store and remove it
	AutoPtr<PresetDescriptor> descriptor = getPresetDescriptor (url);
	if(descriptor)
	{
		dataStore.removeItem (descriptor);
		flush (true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearcher* PresetStore::createSearcher (ISearchDescription& description)
{
	return NEW PresetSearcher (description, dataStore);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetDescriptor* PresetStore::getPresetDescriptor (UrlRef url) const
{
	Expression condition = Member ("url") == UrlFullString (url, true);

	IterForEach (dataStore.query<PresetDescriptor> (condition), PresetDescriptor, descriptor)
		descriptor->retain ();
		return descriptor;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* PresetStore::queryFolderDeep (UrlRef folder) const
{
	UrlFullString folderPattern (folder);
	ASSERT (folderPattern.endsWith (Url::strPathChar))
	folderPattern << "%";
	Expression condition = Member ("url").like (folderPattern);

	return query (condition);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownList* PresetStore::getPresets (IAttributeList* metaInfo, IProgressNotify* progress) const
{
	CCL_PROFILE_START (PresetStore_getPresets)

	// todo: progress (cancel)
	Expression condition;
	UID requestedClassId;
	bool hasAlternativeClassId = false;
	if(metaInfo)
	{
		condition = makeClassCondition (*metaInfo);

		if(hasAlternativeClassId = PresetMetaAttributes (*metaInfo).getAlternativeClassID (requestedClassId))
			PresetMetaAttributes (*metaInfo).getClassID (requestedClassId);
	}

	UnknownList* presets = NEW UnknownList;
	IPresetFileRegistry& registry (System::GetPresetFileRegistry ());

	PresetFilter presetFilter (registry, metaInfo);
	IterForEach (dataStore.query<PresetDescriptor> (condition), PresetDescriptor, descriptor)
		presetFilter.add (descriptor);
	EndFor

	ForEach (presetFilter.getVisibleDescriptors (), PresetDescriptor, descriptor)
		if(IPresetFileHandler* handler = registry.getHandlerForFile (descriptor->getUrl ()))
			if(IPreset* preset = handler->openPreset (descriptor->getUrl (), descriptor))
			{
				// assign requested classID (for presets of alternative class)
				if(hasAlternativeClassId)
					PresetMetaAttributes (*preset->getMetaInfo ()).setClassID (requestedClassId);

				descriptor->applySubFolder (*preset);
				presets->add (preset);
			}
			// else: remove(?)
	EndFor

	if(metaInfo)
	{
		if(presets->isEmpty ())
			cachedClassKeys.remove (getClassKey (*metaInfo)); // (if exists)
		else
			cachedClassKeys.addOnce (getClassKey (*metaInfo)); // (if not already)
	}

	CCL_PROFILE_STOP (PresetStore_getPresets)

	return presets;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetStore::presetExists (IAttributeList* metaInfo, StringRef name, const FileType* fileType) const
{
	Expression condition = Member ("title") == name;
	if(metaInfo)
		condition = makeClassCondition (*metaInfo) && condition;

	if(metaInfo)
		condition = condition && Member ("subFolder") == PresetMetaAttributes (*metaInfo).getSubFolder ();

	AutoPtr<Iterator> iter (dataStore.query<PresetDescriptor> (condition));
	for(auto descriptor : iterate_as<PresetDescriptor> (*iter))
		if(!fileType || descriptor->getUrl ().getFileType () == *fileType)
			return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetStore::getPresetLocations (Container& locations)
{
	// collect current locations from preset handlers
	ObjectList currentFolders;
	currentFolders.objectCleanup (true);

	IPresetFileRegistry& registry (System::GetPresetFileRegistry ());
	for(int h = 0, numhandlers = registry.countHandlers (); h < numhandlers; h++)
		if(IPresetFileHandler* handler = registry.getHandler (h))
		{
			int i = 0;
			Url folder;
			while(handler->getReadLocation (folder, nullptr, i++))
				if(!currentFolders.contains (folder))
					currentFolders.add (NEW Url (folder));
		}

	// get locations from preset store
	locations.objectCleanup (true);
	ObjectList lostLocations;
	ObjectList updatedLocations;

	IterForEach (dataStore.query<PresetLocation> (), PresetLocation, location)
		location->retain ();
		location->setFlags (0);
		locations.add (location);

		// check if it's still a location
		// note: if the stored location's path had a leading "/" too much (happened in soundsets), and the handler's location is repaired now (difference ignored by isEqualUrl),
		//       we treat the old & new path as removed & added locations to trigger a rescan / repair of all contained PresetDescriptor urls
		Url* url = (Url*)currentFolders.findEqual (location->getUrl ());
		if(url && url->getPath ().startsWith (Url::strPathChar) == location->getUrl ().getPath ().startsWith (Url::strPathChar))
		{
			currentFolders.remove (url);
			url->release ();

			// check if subfolder prefix has changed
			StringRef subFolder = System::GetPresetFileRegistry ().getSubFolderPrefix (location->getUrl ());
			if(subFolder != location->getSubFolderPrefix ())
			{
				location->needsUpdate (true);
				location->setSubFolderPrefix (subFolder);
				updatedLocations.add (location);
			}
		}
		else
			lostLocations.add (location); // must be removed from store
	EndFor

	// remaining folders are new ones that must be added to the store
	ForEach (currentFolders, Url, folder)
		PresetLocation* location = NEW PresetLocation;
		location->setUrl (*folder);
		location->setSubFolderPrefix (System::GetPresetFileRegistry ().getSubFolderPrefix (location->getUrl ()));
		location->isNew (true);
		locations.add (location);

		CCL_PRINTF ("PresetStore: new location: %s\n", MutableCString (UrlFullString (location->getUrl ())).str ())
		dataStore.addItem (location);
	EndFor

	// remove the lost locations from store
	ForEach (lostLocations, PresetLocation, location)
		CCL_PRINTF ("PresetStore: remove location: %s\n", MutableCString (UrlFullString (location->getUrl ())).str ())
		location->isRemoved (true);
		dataStore.removeItem (location);
	EndFor

	// update locations with new subFolder
	ForEach (updatedLocations, PresetLocation, location)
		dataStore.updateItem (location);
	EndFor

	dataStore.flush (true);
}

//************************************************************************************************
// PresetStoreSynchronizer::PresetFolder
//************************************************************************************************

class PresetStoreSynchronizer::PresetFolder: public Linkable
{
public:
	PresetFolder (StringRef name)
	: name (name)
	{
		subFolders.objectCleanup (true);
		descriptors.objectCleanup (true);
	}

	PROPERTY_STRING (name, Name)
	PROPERTY_OBJECT (LinkableList, subFolders, SubFolders)
	PROPERTY_OBJECT (ObjectList, descriptors, Descriptors);

	void buildPresetTree (PresetStore& store, UrlRef& location)
	{
		// get all presets in location (deep), sort them into a folder structure
		StringRef rootPath (location.getPath ());
		int rootLength = rootPath.length ();
		int subPathStart = rootPath.endsWith (Url::strPathChar) ? rootLength : rootLength + 1;

		IterForEach (store.queryFolderDeep (location), PresetDescriptor, descriptor)
			String relativePath (descriptor->getUrl ().getPath ().subString (subPathStart));
			
			PresetFolder* targetFolder = this;
			
			String folderName;
			ForEachStringToken (relativePath, Url::strPathChar, token)
				// process previous folderName (ignore last token: fileName)
				if(!folderName.isEmpty ())
				{
					if(PresetFolder* subFolder = targetFolder->getSubFolder (folderName))
						targetFolder = subFolder;
					else
						targetFolder = targetFolder->addSubFolder (folderName);
				}

				folderName = token;
			EndFor

			targetFolder->addPreset (descriptor);
		EndFor
	}

	PresetFolder* getSubFolder (StringRef name)
	{
		ListForEachLinkableFast (subFolders, PresetFolder, subFolder)
			if(subFolder->getName () == name)
				return subFolder;
		EndFor
		return nullptr;
	}

	PresetFolder* addSubFolder (StringRef name)
	{
		PresetFolder* subFolder = NEW PresetFolder (name);
		subFolders.addSorted (subFolder);
		return subFolder;
	}

	void addPreset (PresetDescriptor* descriptor)
	{
		descriptor->retain ();

		// add sorted
		ListForEachObject (descriptors, PresetDescriptor, d)
			if(d->getUrl ().getPath ().compare (descriptor->getUrl ().getPath ()) > 0)
			{
				descriptors.insertBefore (d, descriptor);
				return;
			}
		EndFor
		descriptors.append (descriptor);
	}

	// Object
	int compare (const Object& obj) const override
	{
		PresetFolder* other = (PresetFolder*)&obj;
		return name.compare (other->getName ());
	}
};

//************************************************************************************************
// PresetStoreSynchronizer::UrlItem
//************************************************************************************************

class PresetStoreSynchronizer::UrlItem: public Linkable
{
public:
	UrlItem (UrlRef url)
	: url (url)
	{
		url.getName (name);
	}

	PROPERTY_STRING (name, Name)
	PROPERTY_OBJECT (Url, url, Url)

	// Object
	int compare (const Object& obj) const override
	{
		UrlItem* other = (UrlItem*)&obj;
		return name.compare (other->getName ());
	}
};

//************************************************************************************************
// PresetStoreSynchronizer
//************************************************************************************************

PresetStoreSynchronizer::PresetStoreSynchronizer (PresetStore& store)
: store (store),
  registry (System::GetPresetFileRegistry ()),
  progress (nullptr),
  forceFileUpdate (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* PresetStoreSynchronizer::openPresetFile (const Url& url, StringRef subFolder)
{
	if(IPresetFileHandler* handler = registry.getHandlerForFile (url))
		if(IPreset* preset = handler->openPreset (url))
		{
			if(IAttributeList* metaInfo = preset->getMetaInfo ())
			{
				String sub (PresetFilePrimitives::determineRelativeSubFolder (*handler, *metaInfo, subFolder));
				PresetMetaAttributes (*metaInfo).setSubFolder (sub);
			}
			return preset;
		}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetStoreSynchronizer::checkFoundPresetFile (UrlRef path, StringRef subFolder, PresetDescriptor* existingDescriptor)
{
	FileInfo fileInfo;
	System::GetFileSystem ().getFileInfo (fileInfo, path);

	// check if file is new or has changed
	bool mustUpdate = !existingDescriptor || !existingDescriptor->wasLastModifiedAt (fileInfo.modifiedTime) || forceFileUpdate;
	if(!mustUpdate)
	{
		// force updating the file if the descriptor has no data but the handler (now) wants to store data
		ASSERT (existingDescriptor)
		if(!existingDescriptor->hasData ())
			if(IPresetFileHandler* handler = registry.getHandlerForFile (path))
				mustUpdate = (handler->getFlags () & IPresetFileHandler::kStoresDescriptorData) != 0;
	}
	if(mustUpdate)
	{
		AutoPtr<IPreset> preset (openPresetFile (path, subFolder));
		if(preset)
		{
			if(preset->getMetaInfo ())
			{
				if(existingDescriptor)
				{
					existingDescriptor->initWithPreset (*preset, path, fileInfo.modifiedTime);
					store.updatePreset (existingDescriptor);
				}
				else
				{
					AutoPtr<PresetDescriptor> newDescriptor (NEW PresetDescriptor ());
					newDescriptor->initWithPreset (*preset, path, fileInfo.modifiedTime);
					store.addPreset (newDescriptor);
				}
			}

			if(progress)
				progress->setProgressText (preset->getPresetName ());
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PresetStoreSynchronizer::descendSubFolderString (StringRef subFolder, Url subFolderUrl)
{
	String folderName;
	subFolderUrl.getName (folderName);
	String subSubFolder (subFolder);
	if(!subSubFolder.isEmpty ())
		subSubFolder << Url::strPathChar;
	subSubFolder << folderName;
	return subSubFolder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetStoreSynchronizer::scanLocations (IProgressNotify* progress, bool onlyChangedLocations)
{
	ScopedVar<IProgressNotify*> guard (this->progress, progress);

	fileTypes.getContent ().removeAll ();
	registry.collectFileTypes (fileTypes);

	CCL_PROFILE_START (PresetStoreSynchronizer_Scan)

	ObjectList locations;
	store.getPresetLocations (locations);
	ListForEachObject (locations, PresetLocation, location)
		if(progress && progress->isCanceled ())
			break;

		String subFolder (location->getSubFolderPrefix ());
		ScopedVar<bool> scope (forceFileUpdate, location->needsUpdate ());

		if(location->isNew ())
			scanNewFolder (location->getUrl (), subFolder);
		else if(location->isRemoved ())
			removeFolder (location->getUrl ());
		else if(!onlyChangedLocations || forceFileUpdate)
		{
			AutoPtr<PresetFolder> rootFolder (NEW PresetFolder (nullptr));
			rootFolder->buildPresetTree (store, location->getUrl ());
			synchronizeFolder (location->getUrl (), subFolder, rootFolder);
		}
	EndFor

	// a preset handler might require rescanning it's folders 
	if(onlyChangedLocations)
		for(int h = 0, numhandlers = registry.countHandlers (); h < numhandlers; h++)
		{
			IPresetFileHandler* handler = registry.getHandler (h);
			if(handler && (handler->getFlags () & IPresetFileHandler::kRescanRegularly))
			{
				int i = 0;
				Url folder;
				while(handler->getReadLocation (folder, nullptr, i++))
				{
					bool alreadyScanned = false;
					ListForEachObject (locations, PresetLocation, location)
						alreadyScanned = location->getUrl () == folder && location->isNew ();
						if(alreadyScanned)
							break;
					EndFor

					if(!alreadyScanned)
					{
						AutoPtr<PresetFolder> rootFolder (NEW PresetFolder (nullptr));
						rootFolder->buildPresetTree (store, folder);
						synchronizeFolder (folder, String (), rootFolder);
						// note: we don't expect a change of the subFolder in this "regular scan" case
					}
				}
			}
		}

	CCL_PROFILE_STOP (PresetStoreSynchronizer_Scan)

	store.flush (true);
	store.collectClassKeys ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetStoreSynchronizer::scanNewFolder (UrlRef folder, StringRef subFolder)
{
	CCL_PRINTF ("PresetStoreSynchronizer::scanNewFolder %s\n", MutableCString (UrlDisplayString (folder)).str ())
	ASSERT (folder.isFolder ())

	FileFilter filter (folder);

	ForEachFile (System::GetFileSystem ().newIterator (folder, IFileIterator::kAll), p)
		if(progress)
		{
			progress->updateAnimated ();
			if(progress->isCanceled ())
				break;
		}

		if(p->isFolder ())
		{
			if(!filter.matches (*p))
				continue;

			String subSubFolder (descendSubFolderString (subFolder, *p));
			if(progress)
				progress->setProgressText (subSubFolder);

			scanNewFolder (*p, subSubFolder); // recursion
		}
		else if(fileTypes.matches (*p))
		{
			checkFoundPresetFile (*p, subFolder, nullptr);
		}
	EndFor

	store.flush (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetStoreSynchronizer::synchronizeFolder (UrlRef folder, StringRef subFolder, PresetFolder* presetFolder)
{
	CCL_PRINTF ("PresetStoreSynchronizer::synchronizeFolder %s\n", MutableCString (UrlDisplayString (folder)).str ())
	ASSERT (folder.isFolder ())

	// collect ordered UrlItem lists of subFolders and files
	LinkableList folders;
	LinkableList files;
	folders.objectCleanup (true);
	files.objectCleanup (true);

	FileFilter filter (folder);

	ForEachFile (System::GetFileSystem ().newIterator (folder, IFileIterator::kAll), p)
		if(progress)
		{
			progress->updateAnimated ();
			if(progress->isCanceled ())
				break;
		}

		if(p->isFolder ())
		{
			if(filter.matches (*p))
				folders.addSorted (NEW UrlItem (*p));
		}
		else if(fileTypes.matches (*p))
			files.addSorted (NEW UrlItem (*p));
	EndFor

	// synchronize files in this folder
	FastLinkableListIterator diskFilesIter (files);
	ObjectListIterator storePresetsIter (presetFolder->getDescriptors ());

	UrlItem* nextDiskFile = (UrlItem*)diskFilesIter.next ();
	PresetDescriptor* nextStoreDescriptor = (PresetDescriptor*)storePresetsIter.next ();

	while(nextDiskFile || nextStoreDescriptor)
	{
		if(progress)
		{
			progress->updateAnimated ();
			if(progress->isCanceled ())
				break;
		}

		int cmp = (nextDiskFile == nullptr) ? 1 : (nextStoreDescriptor == nullptr) ? -1 : nextDiskFile->getUrl ().getPath ().compare (nextStoreDescriptor->getUrl ().getPath ());
		if(cmp <= 0)
		{
			ASSERT (nextDiskFile)
			UrlRef diskFileUrl (nextDiskFile->getUrl ());

			if(cmp == 0)
			{
				// existing file on disk and in store
				CCL_PRINTF ("%s existing file: %s\n", CCL_INDENT, MutableCString (UrlDisplayString (diskFileUrl)).str ())
				checkFoundPresetFile (diskFileUrl, subFolder, nextStoreDescriptor);

				nextStoreDescriptor = (PresetDescriptor*)storePresetsIter.next ();
			}
			else
			{
				// disk file not in store
				CCL_PRINTF ("%s new file: %s\n", CCL_INDENT, MutableCString (UrlDisplayString (diskFileUrl)).str ())
				checkFoundPresetFile (diskFileUrl, subFolder, nullptr);
			}
			nextDiskFile = (UrlItem*)diskFilesIter.next ();
		}
		else
		{
			ASSERT (nextStoreDescriptor)

			// no disk file for store file: remove from store
			CCL_PRINTF ("%s removed file: %s\n", CCL_INDENT, MutableCString (UrlDisplayString (nextStoreDescriptor->getUrl ())).str ())
			store.removePreset (nextStoreDescriptor);

			nextStoreDescriptor = (PresetDescriptor*)storePresetsIter.next ();
		}
	}

	// synchronize subFolders of current folder
	FastLinkableListIterator diskFoldersIter (folders);
	FastLinkableListIterator storeFoldersIter (presetFolder->getSubFolders ());

	UrlItem* nextDiskFolder = (UrlItem*)diskFoldersIter.next ();
	PresetFolder* nextStoreFolder = (PresetFolder*)storeFoldersIter.next ();

	while(nextDiskFolder || nextStoreFolder)
	{
		if(progress)
		{
			progress->updateAnimated ();
			if(progress->isCanceled ())
				break;
		}

		int cmp = (nextDiskFolder == nullptr) ? 1 : (nextStoreFolder == nullptr) ? -1 : nextDiskFolder->getName ().compare (nextStoreFolder->getName ());
		if(cmp <= 0)
		{
			ASSERT (nextDiskFolder)
			UrlRef diskFolderUrl (nextDiskFolder->getUrl ());

			String subSubFolder (descendSubFolderString (subFolder, diskFolderUrl));
			if(progress)
				progress->setProgressText (subSubFolder);

			CCL_ADD_INDENT (2)
			if(cmp == 0)
			{
				// existing folder on disk and in store
				CCL_PRINTF ("%s sync existing folder: %s\n", CCL_INDENT, MutableCString (UrlDisplayString (diskFolderUrl)).str ())
				synchronizeFolder (diskFolderUrl, subSubFolder, nextStoreFolder); // recursion

				nextStoreFolder = (PresetFolder*)storeFoldersIter.next ();
			}
			else
			{
				// disk folder not in store: scan new folder
				CCL_PRINTF ("%s new folder: %s\n", CCL_INDENT, MutableCString (UrlDisplayString (diskFolderUrl)).str ())
				scanNewFolder (diskFolderUrl, subSubFolder); // "recursion"
			}
			nextDiskFolder = (UrlItem*)diskFoldersIter.next ();
		}
		else
		{
			ASSERT (nextStoreFolder)

			// no disk folder for store folder: remove from store
			CCL_PRINTF ("%s removed folder: %s\n", CCL_INDENT, MutableCString (nextStoreFolder->getName ()).str ())
			removeFolder (*nextStoreFolder);

			nextStoreFolder = (PresetFolder*)storeFoldersIter.next ();
		}
	}

	store.flush (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetStoreSynchronizer::removeFolder (PresetFolder& folder)
{
	// remove presets in this folder
	ListForEachObject (folder.getDescriptors (), PresetDescriptor, d)
		CCL_PRINTF ("  remove preset: %s\n", MutableCString (UrlDisplayString (d->getUrl ())).str ())
		store.removePreset (d);
	EndFor

	// subFolder recursion
	ListForEachLinkableFast (folder.getSubFolders (), PresetFolder, subFolder)
		removeFolder (*subFolder);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetStoreSynchronizer::removeFolder (UrlRef folder)
{
	// remove presets in this location
	IterForEach (store.queryFolderDeep (folder), PresetDescriptor, d)
		CCL_PRINTF ("  remove preset: %s\n", MutableCString (UrlDisplayString (d->getUrl ())).str ())
		store.removePreset (d);
	EndFor
}
