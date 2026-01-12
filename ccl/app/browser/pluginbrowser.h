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
// Filename    : ccl/app/browser/pluginbrowser.h
// Description : Plug-in Browser
//
//************************************************************************************************

#ifndef _ccl_pluginbrowser_h
#define _ccl_pluginbrowser_h

#include "ccl/app/browser/browser.h"

namespace CCL {

namespace Browsable {
class PlugInCategoryNode; }

class FilePreviewComponent;
interface IClassDescription;

//************************************************************************************************
// PlugInBrowser
//************************************************************************************************

class PlugInBrowser: public Browser
{
public:
	DECLARE_CLASS (PlugInBrowser, Browser)

	PlugInBrowser (StringRef name = nullptr, StringRef title = nullptr, FilePreviewComponent* preview = nullptr);

	FilePreviewComponent* getPreview () { return preview; }

	bool selectPluginOrPreset (IAttributeList& metaInfo, UrlRef presetUrl);

protected:
	FilePreviewComponent* preview;
	class ImportPresetDragHandler;

	Browsable::PlugInCategoryNode* findCategoryNode (const IClassDescription& description) const;
	BrowserNode* findPluginOrPresetNode (IAttributeList& metaInfo, UrlRef presetUrl);
	void collectCategoryNodes (ObjectList& categoryNodes);

	// Browser
	void onViewModeChanged () override;
	void onNodeFocused (BrowserNode* node, bool inList) override;
	bool prepareRefresh () override;
	BrowserNode* findNodeWithUrl (UrlRef url) override;
	tbool canInsertData (BrowserNode* node, const IUnknownList& data, IDragSession* session, IView* targetView) override;
	tbool CCL_API paramChanged (IParameter* param) override;
};

} // namespace CCL

#endif // _ccl_pluginbrowser_h
