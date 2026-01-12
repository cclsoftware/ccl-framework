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
// Filename    : ccl/public/app/ipluginpresentation.h
// Description : Plug-in Presentation Interface
//
//************************************************************************************************

#ifndef _ccl_ipluginpresentation_h
#define _ccl_ipluginpresentation_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/text/cclstring.h"

namespace CCL {

interface IUnknownIterator;
interface IImage;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Signals
{
	/** args[0]: sender [object] or change type [string]  (optional, see type strings in IPluginPresentation)
		args[1]: affected plug-in category (optional, for certain change types) */
	DEFINE_STRINGID (kPluginPresentationChanged, "PluginPresentationChanged")
}

//************************************************************************************************
// IPluginPresentation
/**	\ingroup app_inter */
//************************************************************************************************

interface IPluginPresentation: IUnknown
{
	/** Hidden state. */
	virtual tbool CCL_API isHidden (UIDRef cid) const = 0;
	virtual void CCL_API setHidden (UIDRef cid, tbool state) = 0;

	/** Favorite state and optional sort folder. */
	virtual tbool CCL_API isFavorite (UIDRef cid) const = 0;
	virtual String CCL_API getFavoriteFolder (UIDRef cid) const = 0;
	virtual void CCL_API setFavorite (UIDRef cid, tbool state, StringRef folder = nullptr) = 0;

	typedef StringRef CategoryRef;
	virtual void CCL_API addFavoriteFolder (CategoryRef category, StringRef path) = 0;
	virtual void CCL_API removeFavoriteFolder (CategoryRef category, StringRef path) = 0;
	virtual void CCL_API moveFavoriteFolder (CategoryRef category, StringRef oldPath, StringRef newPath) = 0;
	virtual void CCL_API renameFavoriteFolder (CategoryRef category, StringRef path, StringRef newName) = 0;
	virtual IUnknownIterator* CCL_API getFavoriteFolders (CategoryRef category) const = 0;
	virtual tbool CCL_API hasFavoriteFolder (CategoryRef category, StringRef path) const = 0;

	/** Usage tracking. */
	virtual int64 CCL_API getLastUsage (UIDRef cid) const = 0; ///< seconds since 1970, or 0 if not used
	virtual void CCL_API logUsage (UIDRef cid) = 0;

	/** System DPI scaling (Windows only). */
	virtual tbool CCL_API isSystemScalingEnabled (UIDRef cid) const = 0;
	virtual void CCL_API setSystemScalingEnabled (UIDRef cid, tbool state) = 0;

	/** Additonal attributes. */
	virtual tbool CCL_API getAttribute (Variant& value, UIDRef cid, StringID attrId) const = 0;
	virtual void CCL_API setAttribute (UIDRef cid, StringID attrId, VariantRef value) = 0;
	virtual void CCL_API removeAttribute (UIDRef cid, StringID attrId) = 0;

	/** Sort paths. */
	virtual String CCL_API getSortPath (UIDRef cid) const = 0;
	virtual void CCL_API setSortPath (UIDRef cid, StringRef path) = 0;
	
	virtual void CCL_API addSortFolder (CategoryRef category, StringRef path) = 0;
	virtual void CCL_API removeSortFolder (CategoryRef category, StringRef path) = 0;
	virtual void CCL_API moveSortFolder (CategoryRef category, StringRef oldPath, StringRef newPath) = 0;
	virtual void CCL_API renameSortFolder (CategoryRef category, StringRef path, StringRef newName) = 0;

	virtual IUnknownIterator* CCL_API getSortFolders (CategoryRef category) const = 0;
	virtual tbool CCL_API hasSortFolder (CategoryRef category, StringRef path) const = 0;

	/** Reset to default state. */
	virtual void CCL_API reset () = 0;

	/** Revert to last saved state. */
	virtual void CCL_API revert () = 0;

	/** Save state in settings. */
	virtual void CCL_API saveSettings () = 0;

	// Change type argument for Signals::kPluginPresentationChanged (arg[0])
	DECLARE_STRINGID_MEMBER (kAttributeChanged) ///< attribute modifications 
	DECLARE_STRINGID_MEMBER (kSnapshotChanged)	///< plug-in snapshot image changed; arg[1]: category
	DECLARE_STRINGID_MEMBER (kUsageChanged)		///< plug-in was used (see getLastUsage); arg[1]: category

	DECLARE_IID (IPluginPresentation)
};

DEFINE_IID (IPluginPresentation, 0x6dfd7c4, 0x29bd, 0x413a, 0x83, 0x4d, 0x3f, 0xd, 0xc5, 0xcd, 0xdd, 0xd2)
DEFINE_STRINGID_MEMBER (IPluginPresentation, kAttributeChanged, "attributeChanged")
DEFINE_STRINGID_MEMBER (IPluginPresentation, kSnapshotChanged, "snapshotChanged")
DEFINE_STRINGID_MEMBER (IPluginPresentation, kUsageChanged, "usageChanged")

//************************************************************************************************
// IPlugInSnapshots
/**	\ingroup app_inter */
//************************************************************************************************

interface IPlugInSnapshots: IUnknown
{
	DECLARE_STRINGID_MEMBER (kDefault)

	/** Get snapshot image for given plug-in class. */
	virtual IImage* CCL_API getSnapshot (UIDRef cid, StringID which = kDefault) const = 0;

	/** Check if user has created a snapshot image for the given plug-in class. */
	virtual tbool CCL_API hasUserSnapshot (UIDRef cid) const = 0;

	/** Set user snapshot image for given plug-in class. */
	virtual tbool CCL_API setUserSnapshot (UIDRef cid, IImage* image) = 0;
	
	/** Set default snapshot image for given plug-in class. */
	virtual tbool CCL_API setDefaultSnapshot (UrlRef snapshotFile, UIDRef cid, UrlRef imageFile1x, UrlRef imageFile2x) = 0;

	/** Get description associated with snapshot. */
	virtual tbool CCL_API getSnapshotDescription (String& description, UIDRef cid, StringID which = kDefault) const = 0;

	/** Check if given plug-in class should be highlighted. */
	virtual tbool CCL_API isHighlight (UIDRef cid) const = 0;

	/** Add snapshot location. */
	virtual int CCL_API addLocation (UrlRef path, tbool deep = true) = 0;
	
	/** Add snapshot json file location. */
	virtual tbool CCL_API addSnapshotFile (UrlRef path) = 0;

	/** Remove snapshot location. */
	virtual void CCL_API removeLocation (UrlRef path) = 0;

	/** Check if given snapshot location has been added. */
	virtual tbool CCL_API hasLocation (UrlRef path) const = 0;

	DECLARE_IID (IPlugInSnapshots)
};

DEFINE_IID (IPlugInSnapshots, 0x7ef21068, 0xdfdc, 0x43bd, 0x92, 0x98, 0x24, 0xbe, 0xbe, 0xde, 0xff, 0x85)
DEFINE_STRINGID_MEMBER (IPlugInSnapshots, kDefault, "default")

} // namespace CCL

#endif // _ccl_ipluginpresentation_h
