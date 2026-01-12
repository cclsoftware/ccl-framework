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
// Filename    : ccl/base/storage/settings.h
// Description : User Settings
//
//************************************************************************************************

#ifndef _ccl_settings_h
#define _ccl_settings_h

#include "ccl/base/storage/isettings.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/collections/objectlist.h"
#include "ccl/public/collections/unknownlist.h"

namespace CCL {

class FileType;
class Settings;
class SignalSink;

namespace Configuration {
interface IRegistry; }

//////////////////////////////////////////////////////////////////////////////////////////////////
// Settings Signals
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Signals {

	/** Signals related to Settings */
	DEFINE_STRINGID (kSettings, "CCL.Settings")

		/** Auto-save Settings. */
		DEFINE_STRINGID (kAutoSaveSettings, "AutoSaveSettings")

		/** Backup Settings (args[0]: IUnknownList to collect IUrls) */
		DEFINE_STRINGID (kBackupSettings, "BackupSettings")

} // namespace Signals

//************************************************************************************************
// SettingsSaver
/** Base class implementing ISettingsSaver. */
//************************************************************************************************

class SettingsSaver: public Object,
					 public ISettingsSaver
{
public:
	CLASS_INTERFACE (ISettingsSaver, Object)
};

//************************************************************************************************
// ConfigurationSaver
/** Transfer value from configuration registry to user settings. */
//************************************************************************************************

class ConfigurationSaver: public SettingsSaver
{
public:
	ConfigurationSaver (StringID section, StringID key,
						Configuration::IRegistry* registry = nullptr);

	static Attributes& getAttributes (Settings&);

	PROPERTY_MUTABLE_CSTRING (section, Section)
	PROPERTY_MUTABLE_CSTRING (key, Key)
	PROPERTY_POINTER (Configuration::IRegistry, registry, Registry)

	MutableCString getAttributeName () const;
	Configuration::IRegistry& getRegistry ();

	ConfigurationSaver* migrateSettingFrom (Settings& settings, StringID oldSection, StringID oldKey); ///< must be called before restoring from settings; returns 'this' for convenience

	// SettingsSaver
	void restore (Settings&) override;
	void flush (Settings&) override;
};

//************************************************************************************************
// Settings
/** User Settings base class. */
//************************************************************************************************

class Settings: public Object
{
public:
	DECLARE_CLASS (Settings, Object)
	DECLARE_METHOD_NAMES (Settings)

	Settings (StringRef name = nullptr, int version = 1);
	~Settings ();

	static Settings& instance (); ///< global user settings
	static void autoSaveAll (); ///< signal to auto-save all settings
	static void backupAll (IUnknownList& pathList);	///< signal to backup all settings

	void init (StringRef name, int version = 1);
	void init (StringRef companyName, StringRef productName, StringRef settingsName, int version = 1);
	void copyFrom (const Settings& settings); ///< copies only the actual settings attributes
	
	StringRef getName () const;
	StringRef getCompanyName () const;
	StringRef getProductName () const;
	int getVersion () const;

	PROPERTY_FLAG (flags, 1<<0, checkName)
	PROPERTY_FLAG (flags, 1<<1, checkVersion)
	PROPERTY_FLAG (flags, 1<<2, checkLanguage)
	PROPERTY_FLAG (flags, 1<<3, isPlatformSpecific)
	PROPERTY_FLAG (flags, 1<<4, isAutoSaveEnabled)
	PROPERTY_FLAG (flags, 1<<5, isBackupEnabled)
	PROPERTY_FLAG (flags, 1<<6, isApplicationIndependent)

	void addSaver (ISettingsSaver* saver);
	void removeSaver (ISettingsSaver* saver);
	bool containsSaver (ISettingsSaver* saver) const;
	
	void enableSignals (bool state);

	//********************************************************************************************
	// Section
	//********************************************************************************************

	class Section: public Object
	{
	public:
		DECLARE_CLASS (Section, Object)

		Section (StringRef path = nullptr);
		Section (const Section& section);
		~Section ();

		bool isEmpty () const;
		StringRef getPath () const;
		Attributes& getAttributes () const;

		// shortcut to attributes
		template <class T> T* getObject (StringID id) const;
		Object* getObject (StringID id, MetaClassRef typeId) const;
		void setObject (StringID id, Object* obj);

		int getInt (StringID id) const;
		void setInt (StringID id, int value);

		// Object
		bool load (const Storage&) override;
		bool save (const Storage&) const override;
		int compare (const Object& obj) const override;
	
	protected:
		String path;
		mutable Attributes* attributes;
	};
	
	//////////////////////////////////////////////////////////////////////////////////////////////

	Section* getSection (StringRef path, bool create = true);
	Attributes& getAttributes (StringRef path);	///< shortcut to section attributes

	bool isEmpty () const;
	Iterator* getSections () const;
	bool isEmpty (StringRef path) const;
	bool remove (StringRef path);
	void removeAll ();

	virtual bool restore ();	///< load settings from persistent storage
	virtual bool flush ();		///< write changes to persistent storage
	
	// Object
	bool load (const Storage&) override;
	bool save (const Storage&) const override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	String companyName;
	String productName;
	String name;
	int version;
	int flags;
	ObjectArray settings;
	InterfaceList<ISettingsSaver> savers;
	SignalSink* signalSink;

	Section* lookup (StringRef path) const;

	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// XmlSettings
/** User Settings stored as XML File. */
//************************************************************************************************

class XmlSettings: public Settings
{
public:
	DECLARE_CLASS (XmlSettings, Settings)

	XmlSettings (StringRef name = nullptr, int version = 1);
	~XmlSettings ();

	static const FileType& getFileType ();
	static String getNameWithLanguage (StringRef name);
	static void removeSettings (StringRef name, bool anyLanguage, bool anyPlatform);

	void setPath (UrlRef url);
	UrlRef getPath ();

	void setFileName (StringRef fileName);

	// Settings
	bool restore () override;
	bool flush () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	IUrl* path;

	bool loadSettings (UrlRef url);
	bool saveSettings (UrlRef url);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Settings inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline StringRef Settings::getName () const	
{ return name; }

inline StringRef Settings::getCompanyName () const	
{ return companyName; }

inline StringRef Settings::getProductName () const	
{ return productName; }

inline int Settings::getVersion () const	
{ return version; }

inline Attributes& Settings::getAttributes (StringRef path)
{ return getSection (path, true)->getAttributes (); }

inline Iterator* Settings::getSections () const
{ return settings.newIterator (); }

inline StringRef Settings::Section::getPath () const 
{ return path; }
	
inline Attributes& Settings::Section::getAttributes () const
{ if(!attributes) attributes = NEW Attributes; return *attributes; }

inline bool Settings::Section::isEmpty () const
{ return !attributes || (attributes && attributes->isEmpty ()); }

template <class T> T* Settings::Section::getObject (StringID id) const
{ return getAttributes ().getObject<T> (id); }

inline Object* Settings::Section::getObject (StringID id, MetaClassRef typeId) const
{ return getAttributes ().getObject (id, typeId); }

inline void Settings::Section::setObject (StringID id, Object* obj)
{ getAttributes ().set (id, obj, Attributes::kOwns); }

inline int Settings::Section::getInt (StringID id) const
{ return getAttributes ().getInt (id); }

inline void Settings::Section::setInt (StringID id, int value)
{ getAttributes ().set (id, value); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_settings_h
