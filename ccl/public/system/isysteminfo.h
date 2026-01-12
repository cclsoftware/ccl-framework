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
// Filename    : ccl/public/system/isysteminfo.h
// Description : System Information
//
//************************************************************************************************

#ifndef _ccl_isysteminfo_h
#define _ccl_isysteminfo_h

#include "ccl/public/text/cclstring.h"
#include "ccl/public/base/datetime.h"

namespace CCL {

interface IAttributeList;
interface IAsyncOperation;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Constants
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace System
{
	/** Folder types. */
	DEFINE_ENUM (FolderType)
	{
		// system-wide locations
		kSystemFolder = 'Syst',					///< [R] folder where the OS is installed
		kProgramsFolder = 'Prog',				///< [R] programs/applications folder
		kSharedDataFolder = 'ShDt',				///< [R] Data folder for all users 
		kSharedSettingsFolder = 'ShSe',			///< [R] Settings folder for all users
		kSharedSupportFolder = 'ShSp',			///< [R] application support files for all apps/users (scripts, plug-ins, etc.)

		// user-specific locations
		kTempFolder = 'Temp',					///< [R] user temporary folder
		kDesktopFolder = 'Desk',				///< [R] user desktop folder
		kUserSettingsFolder = 'Sett',			///< [R] user "Settings" folder		
		kUserPreferencesFolder = 'Pref',		///< [R] user "Preferences" folder (differs on Mac OS only, otherwise same as kUserSettingsFolder)
		kUserDocumentFolder = 'Docs',			///< [R] user "Documents" folder
		kUserMusicFolder = 'Msic',				///< [R] user "Music" folder (fallback to "Documents")
		kUserDownloadsFolder = 'Dwnl',			///< [R] user "Downloads" folder (fallback to "Documents")
		kUserAppSupportFolder = 'UPrg',			///< [R] user application support folder

		// company-wide locations
		kCompanySettingsFolder = 'CSet',		///< [R] company-wide settings folder (resides in kSharedSettingsFolder)
		kCompanySupportFolder = 'CSup',			///< [R] company-wide application support folder (resides in kSharedSupportFolder)
		kCompanyContentFolder = 'CCnt',			///< [R] company-wide content for all users (resides in kSharedDataFolder)

		// app-specific locations
		kUserContentFolder = 'Cont',			///< [R/W] user "Documents\AppName" folder (usually in kUserDocumentFolder)
		kSharedContentFolder = 'SCnt',			///< [R] shared application content for all users (resides in kSharedDataFolder)
		kAppFactoryContentFolder = 'FCnt',		///< [R] application factory content folder (usually same as kUserContentFolder, dependes on platform)
		kAppSettingsFolder = 'Apps',			///< [R] user application settings folder (resides in kUserSettingsFolder)
		kAppSettingsPlatformFolder = 'xAps',	///< [R] platform-specific settings folder (resides in kAppSettingsFolder)
		kSharedAppSettingsFolder = 'ShAp',		///< [R] shared application settings folder (resides in kCompanySettingsFolder)
		kAppSupportFolder = 'Supp',				///< [R] application support files (scripts, plug-ins, etc., usually in kProgramsFolder)
		kAppDeploymentFolder = 'Depl',			///< [R/W] application support files, can differ in debug builds (defaults to kAppSupportFolder)
		kAppPluginsFolder = 'Plug'				///< [R] plug-ins to be loaded by application (usually in kAppSupportFolder/Plugins)
	};

	/** Memory information. */
	struct MemoryInfo
	{
		uint64 physicalRAMSize;			///< installed physical RAM amount
		uint64 processMemoryTotal;		///< size of virtual address space of the calling process
		uint64 processMemoryAvailable;	///< currently available virtual memory amount of the calling process

		MemoryInfo ()
		: physicalRAMSize (0),
		  processMemoryTotal (0),
		  processMemoryAvailable (0)
		{}
	};

	/** Flags for computer information. */
	enum ComputerInfoFlags
	{
		kQueryExtendedComputerInfo = 1<<0,	///< query extended computer information (might take a while)
		kQueryEthernetAdapterList = 1<<1	///< query list of ethernet adapters
	};

	// Computer Information
	DEFINE_STRINGID (kOSName, "OSName")								///< operating system name
	DEFINE_STRINGID (kOSVersion, "OSVersion")						///< operating system version
	DEFINE_STRINGID (kDeviceModel, "DeviceModel")					///< device model
    DEFINE_STRINGID (kDeviceModelSubtype, "DeviceModelSubtype")     ///< device model subtype
    DEFINE_STRINGID (kDeviceIdentifier, "DeviceIdentifier")			///< device identifier
	
	DEFINE_STRINGID (kCPUSpeed, "CPUSpeed")							///< CPU speed in arbitrary units	
	DEFINE_STRINGID (kCPUSpeedMHz, "CPUSpeedMHz")					///< CPU speed in MHz
	DEFINE_STRINGID (kCPUIdentifier, "CPUIdentifier")				///< CPU identifier 
	DEFINE_STRINGID (kCPUModelHumanReadable, "CPUModel")			///< human readable CPU model description
	DEFINE_STRINGID (kPhysicalRAMSize, "PhysicalRAMSize")			///< installed physical RAM amount

	DEFINE_STRINGID (kDiskModelHumanReadable, "DiskModel")			///< human readable model of first physical disk
	DEFINE_STRINGID (kDiskSerialNumber, "DiskSerialNumber")			///< serial number of first physical disk
	DEFINE_STRINGID (kVolumeSerialNumber, "VolumeSerialNumber")		///< serial number of system volume (generated during formatting)
	DEFINE_STRINGID (kSystemFolderFSID, "SystemFolderFSID")			///< file system identifier of system folder

	DEFINE_STRINGID (kMACAddress, "MACAddress")						///< MAC address of primary ethernet adapter
	DEFINE_STRINGID (kEthernetAdapter, "EthernetAdapter")			///< primary ethernet adapter
	DEFINE_STRINGID (kEthernetAdapterList, "EthernetAdapterList")	///< list of ethernet adapters

    DEFINE_STRINGID (kProcessIsTranslated, "ProcessIsTranslated")   ///< if true, the current process runs with an emulated CPU architecture
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Signals
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Signals
{
	/** Signals related to System Information. */
	DEFINE_STRINGID (kSystemInformation, "CCL.SystemInformation")

		/** (OUT) arg[0]: IUrl with new content location; arg[1]: IUrl with old location. */
		DEFINE_STRINGID (kContentLocationChanged, "ContentLocationChanged")

		/** (OUT) arg[0]: IUrl with new deployment location. */
		DEFINE_STRINGID (kDeploymentLocationChanged, "DeploymentLocationChanged")
}

//************************************************************************************************
// ISystemInformation
/**	\ingroup ccl_system */
//************************************************************************************************

interface ISystemInformation: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Folder Locations
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Set company and product folder names of main application. */
	virtual void CCL_API setApplicationName (StringRef companyName, StringRef productName, int productVersion = 0) = 0;
	
	/** Set writable location. */
	virtual tbool CCL_API setLocation (System::FolderType type, UrlRef url) = 0;

	/** Get real file system location on target computer for symbolic folder type. */
	virtual tbool CCL_API getLocation (IUrl& url, System::FolderType type) const = 0;

	/** Resolve symbolic names in given location. */
	virtual tbool CCL_API resolveLocation (IUrl& resolved, UrlRef url) const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Time
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Get current local time. */
	virtual void CCL_API getLocalTime (DateTime& dateTime) const = 0;
	
	/** Convert local time to UTC (Coordinated Universal Time). */
	virtual void CCL_API convertLocalTimeToUTC (DateTime& utc, const DateTime& localTime) const = 0;
	
	/** Convert UTC (Coordinated Universal Time) to local time. */
	virtual void CCL_API convertUTCToLocalTime (DateTime& localTime, const DateTime& utc) const = 0;

	/** Get Unix epoch time (seconds since January 1 1970 00:00 UTC). */
	virtual int64 CCL_API getUnixTime () const = 0;

	/** Convert Unix epoch time to UTC (Coordinated Universal Time). */
	virtual void CCL_API convertUnixTimeToUTC (DateTime& utc, int64 unixTime) const = 0;

	/** Convert UTC (Coordinated Universal Time) to Unix epoch time. */
	virtual int64 CCL_API convertUTCToUnixTime (const DateTime& utc) const = 0;

	/** Get Unix epoch time (seconds since January 1 1970 00:00 UTC) from a secure source. */
	virtual int64 CCL_API getSecureComputerTime () const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Hardware and Software
	//////////////////////////////////////////////////////////////////////////////////////////////
	
	/** Get name of local computer. */
	virtual void CCL_API getComputerName (String& name, int flags = 0) const = 0;
	
	/** Get user name of calling thread. */
	virtual void CCL_API getUserName (String& name, int flags = 0) const = 0;
	    
	/** Get the number of active CPUs in the system. */
	virtual int CCL_API getNumberOfCPUs () const = 0;
	
	/** Get the number of active CPU cores in the system. */
	virtual int CCL_API getNumberOfCores () const = 0;

	/** Get memory information. */
	virtual void CCL_API getMemoryInfo (System::MemoryInfo& memoryInfo) const = 0;
	
	/** Get computer information from underlying OS. */
	virtual void CCL_API getComputerInfo (IAttributeList& attributes, int flags = 0) const = 0;
	
	/** Determine if the application runs in a sandbox where certain restrictions to system objects may apply. */
	virtual tbool CCL_API isProcessSandboxed () const = 0;

	/** Search installed applications. Filter is a comma or semicolon separated list of search tokens. The tokens are not case sensitive
		Result of async operation is IUnknownList with IAttributeList objects.
		Attributes: kPackageName, kPackageVersion, kPackageVendor, kPackageExecutable */
	virtual IAsyncOperation* CCL_API searchApplications (StringRef filter) const = 0; 	

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Other
	//////////////////////////////////////////////////////////////////////////////////////////////
	
	/** Cleanup. */
	virtual void CCL_API terminate () = 0;	
		
	DECLARE_IID (ISystemInformation)
};

DEFINE_IID (ISystemInformation, 0xb301d0f2, 0x6d72, 0x42d3, 0x92, 0x34, 0x85, 0x3f, 0x38, 0xe3, 0xf, 0x9b)

} // namespace CCL

#endif // _ccl_isysteminfo_h
