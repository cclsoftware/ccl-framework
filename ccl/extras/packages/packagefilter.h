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
// Filename    : ccl/extras/packages/packagefilter.h
// Description : Package Filter
//
//************************************************************************************************

#ifndef _ccl_packagefilter_h
#define _ccl_packagefilter_h

#include "ccl/app/component.h"

#include "ccl/public/base/irecognizer.h"
#include "ccl/public/storage/filetype.h"
#include "ccl/public/plugins/versionnumber.h"

namespace CCL {
namespace Packages {

class UnifiedPackage;
class PackageManager;

//************************************************************************************************
// PackageFilterComponent
/** Filter used in conjunction with a PackageManager. */
//************************************************************************************************

class PackageFilterComponent: public Component,
							  public IObjectFilter
{
public:
	DECLARE_CLASS_ABSTRACT (PackageFilterComponent, Component);

	PackageFilterComponent (PackageManager* manager = nullptr, StringRef name = nullptr, StringRef title = nullptr);
	~PackageFilterComponent ();

	virtual void update ();
	virtual void select (int index);
	virtual void select (StringRef value);
	virtual void reset ();
	virtual int getSelection () const;

	void setHidden (bool state);
	bool isHidden () const;
	void setEnabled (bool state);
	bool isEnabled () const;

	// IObjectFilter
	tbool CCL_API matches (IUnknown* object) const override;

	// Component
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (IObjectFilter, Component)

protected:
	Vector<String> items;
	PackageManager* manager;
	IParameter* selectionParameter;
	bool hidden;
	bool enabled;

	virtual void addItem (StringRef title, int index = -1);
	virtual void removeItem (int index);
	virtual String getItemTitle (int index) const;
	virtual bool matches (const UnifiedPackage& package) const = 0;
};

//************************************************************************************************
// MultiOptionPackageFilterComponent
/** Package Filter which allows multiple selected values (checkboxes). */
//************************************************************************************************

class MultiOptionPackageFilterComponent: public PackageFilterComponent
{
public:
	DECLARE_CLASS_ABSTRACT (MultiOptionPackageFilterComponent, PackageFilterComponent);

	MultiOptionPackageFilterComponent (PackageManager* manager = nullptr, StringRef name = nullptr, StringRef title = nullptr);
	
	// PackageFilterComponent
	virtual void reset () override;
	tbool CCL_API paramChanged (IParameter* param) override;
	
	CLASS_INTERFACE (IObjectFilter, Component)

protected:
	IParameter* getItemParam (int index) const;

	// PackageFilterComponent
	void addItem (StringRef title, int index = -1) override;
};

//************************************************************************************************
// PackageSearchComponent
//************************************************************************************************

class PackageSearchComponent: public PackageFilterComponent
{
public:
	DECLARE_CLASS (PackageSearchComponent, PackageFilterComponent);

	PackageSearchComponent (PackageManager* manager = nullptr);

	// PackageFilterComponent
	bool matches (const UnifiedPackage& package) const override;
	void update () override;
	void reset () override;
	tbool CCL_API paramChanged (IParameter* param) override;
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

private:
	IParameter* searchParam;
	IParameter* clearParam;
};

//************************************************************************************************
// StaticFileTypePackageFilterComponent
//************************************************************************************************

class StaticFileTypePackageFilterComponent: public PackageFilterComponent
{
public:
	DECLARE_CLASS (StaticFileTypePackageFilterComponent, PackageFilterComponent);

	StaticFileTypePackageFilterComponent (PackageManager* manager = nullptr);

	void addFileType (const FileType& type);
	const Vector<FileType>& getFileTypes () const;

	// PackageFilterComponent
	bool matches (const UnifiedPackage& package) const override;
	void update () override;

private:
	Vector<FileType> fileTypes;

	bool matches (const UnifiedPackage& package, bool includeChildrenWithoutFiles) const;
};

//************************************************************************************************
// FileTypePackageFilterComponent
//************************************************************************************************

class FileTypePackageFilterComponent: public MultiOptionPackageFilterComponent
{
public:
	DECLARE_CLASS (FileTypePackageFilterComponent, MultiOptionPackageFilterComponent);

	FileTypePackageFilterComponent (PackageManager* manager = nullptr);

	void addFileType (const FileType& type, StringRef title = nullptr);

	// MultiOptionPackageFilterComponent
	bool matches (const UnifiedPackage& package) const override;
	void update () override;

protected:
	Vector<FileType> fileTypes;

	bool matches (const UnifiedPackage& package, bool includeChildrenWithoutFiles) const;
};

//************************************************************************************************
// InstallStatePackageFilterComponent
//************************************************************************************************

class InstallStatePackageFilterComponent: public PackageFilterComponent
{
public:
	DECLARE_CLASS (InstallStatePackageFilterComponent, PackageFilterComponent);

	InstallStatePackageFilterComponent (PackageManager* manager = nullptr);

	PROPERTY_BOOL (strict, Strict)
	PROPERTY_BOOL (filterChildren, FilteringChildren)

	enum States {kAny = 0, kInstalled, kAvailable};

	// PackageFilterComponent
	bool matches (const UnifiedPackage& package) const override;
	void update () override;
};

//************************************************************************************************
// OriginPackageFilterComponent
//************************************************************************************************

class OriginPackageFilterComponent: public MultiOptionPackageFilterComponent
{
public:
	DECLARE_CLASS (OriginPackageFilterComponent, MultiOptionPackageFilterComponent);

	OriginPackageFilterComponent (PackageManager* manager = nullptr);

	// MultiOptionPackageFilterComponent
	bool matches (const UnifiedPackage& package) const override;
	void update () override;

private:
	// MultiOptionPackageFilterComponent
	String getItemTitle (int index) const override;
};

//************************************************************************************************
// SinglePackageFilterComponent
//************************************************************************************************

class SinglePackageFilterComponent: public PackageFilterComponent
{
public:
	DECLARE_CLASS (SinglePackageFilterComponent, PackageFilterComponent);

	SinglePackageFilterComponent (PackageManager* manager = nullptr, StringRef title = nullptr);

	void setPackageId (StringRef id);
	void enable (bool state);

	// PackageFilterComponent
	bool matches (const UnifiedPackage& package) const override;

private:
	CCL::String packageId;
	bool enabled;
};

//************************************************************************************************
// AppVersionPackageFilterComponent
//************************************************************************************************

class AppVersionPackageFilterComponent: public PackageFilterComponent
{
public:
	DECLARE_CLASS (AppVersionPackageFilterComponent, PackageFilterComponent);

	AppVersionPackageFilterComponent (PackageManager* manager = nullptr, StringRef title = nullptr);

	void addSupportedVersion (StringRef identity, const VersionNumber& version);

	// PackageFilterComponent
	bool matches (const UnifiedPackage& package) const override;

private:
	struct VersionItem
	{
		String identity;
		VersionNumber version;
	};
	Vector<VersionItem> supportedVersions;

	bool checkManifest (bool& foundManifest, const UnifiedPackage& package) const;
};

//************************************************************************************************
// TagPackageFilterComponent
//************************************************************************************************

class TagPackageFilterComponent: public MultiOptionPackageFilterComponent
{
public:
	DECLARE_CLASS (TagPackageFilterComponent, MultiOptionPackageFilterComponent);

	TagPackageFilterComponent (PackageManager* manager = nullptr);

	// MultiOptionPackageFilterComponent
	bool matches (const UnifiedPackage& package) const override;
	void update () override;
	void reset () override;
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
};

} // namespace Packages
} // namespace CCL

#endif // _ccl_packagefilter_h
