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
// Filename    : ccl/app/fileinfo/filepreviewcomponent.h
// Description : File Preview Component
//
//************************************************************************************************

#ifndef _ccl_filepreviewcomponent_h
#define _ccl_filepreviewcomponent_h

#include "ccl/app/component.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/signalsource.h"

#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/graphics/iimage.h"

namespace CCL {

interface IFileInfoComponent;
interface IFileSelectorCustomize;

//************************************************************************************************
// FilePreviewer
//************************************************************************************************

class FilePreviewComponent: public Component
{
public:
	DECLARE_CLASS (FilePreviewComponent, Component)

	FilePreviewComponent (StringRef name = nullptr, StringID skinNamespace = nullptr);
	~FilePreviewComponent ();

	PROPERTY_MUTABLE_CSTRING (skinNamespace, SkinNamespace)

	virtual void setFile (UrlRef path, IImage* icon, StringRef title);
	UrlRef getFile () const;
	void updateFile ();

	virtual void customizeFileSelector (IFileSelectorCustomize& fsc);

	// Component
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	virtual IFileInfoComponent* createInfoComponent (); ///< create a new component for the current item
	virtual tbool setPreviewContent (IFileInfoComponent& infoComponent); ///< try set a new item into the component

	bool isUsedInFileSelector () const;
	IFileInfoComponent* getCurrentInfoComponent () const;

	void updateView (bool isNewFile);

	// hooks for subclass
	virtual void loadParams () {};
	virtual void saveParams () {};
	virtual void onUpdateFile (IFileInfoComponent* infoComponent, bool isNewFile) {}
	virtual void releaseFile (CCL::UrlRef path); ///< release the given file, if it's currently open

	Url currentPath;
	SharedPtr<IImage> currentIcon;
	String currentTitle;

private:
	ISubject* infoView;
	bool usedInFileSelector;
	IFileInfoComponent* currentInfoComponent;
	AutoSignalSink fileSystemSink;
};

} // namespace CCL

#endif // _ccl_filepreviewcomponent_h
