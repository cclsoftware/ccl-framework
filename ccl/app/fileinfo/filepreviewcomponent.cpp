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
// Filename    : ccl/app/fileinfo/filepreviewcomponent.cpp
// Description : File Preview Component
//
//************************************************************************************************

#include "ccl/app/fileinfo/filepreviewcomponent.h"
#include "ccl/app/fileinfo/fileinfocomponent.h"

#include "ccl/base/message.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/ifileselector.h"
#include "ccl/public/system/inativefilesystem.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum FilePreviewComponentTags
	{
		kFileInfo1 = 'Inf1',
		kFileInfo2 = 'Inf2'
	};
}

BEGIN_XSTRINGS ("FileInfo")
	XSTRING (FileInformation, "File information")
END_XSTRINGS

//************************************************************************************************
// FilePreviewComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (FilePreviewComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

FilePreviewComponent::FilePreviewComponent (StringRef name, StringID _skinNamespace)
: Component (name),
  skinNamespace (_skinNamespace),
  fileSystemSink (CCL::Signals::kFileSystem),
  infoView (nullptr),
  usedInFileSelector (false),
  currentInfoComponent (nullptr)
{
	if(skinNamespace.isEmpty ())
		skinNamespace = CSTR ("CCL");

	fileSystemSink.setObserver (this);
	fileSystemSink.enable (true);

	paramList.addString (IFileInfoComponent::kFileInfo1, Tag::kFileInfo1);
	paramList.addString (IFileInfoComponent::kFileInfo2, Tag::kFileInfo2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FilePreviewComponent::~FilePreviewComponent ()
{
	fileSystemSink.enable (false);
	if(infoView)
		notify (infoView, Message (kDestroyed));
	safe_release (currentInfoComponent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FilePreviewComponent::setFile (UrlRef path, IImage* icon, StringRef title)
{
	currentPath = path;
	currentIcon = icon;
	currentTitle = title;
	updateView (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef FilePreviewComponent::getFile () const
{
	return currentPath;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FilePreviewComponent::updateFile ()
{
	updateView (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileInfoComponent* FilePreviewComponent::createInfoComponent ()
{
	if(currentPath.isEmpty ())
		return nullptr;

	IFileInfoComponent* component = FileInfoRegistry::instance ().createComponent (currentPath);
	if(component)
		component->setDisplayAttributes (currentIcon, currentTitle);

	// assign namespace
	FileInfoComponent* c = unknown_cast<FileInfoComponent> (component);
	if(c)
		c->assignSkinNamespace (skinNamespace);

	return component;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool FilePreviewComponent::setPreviewContent (IFileInfoComponent& infoComponent)
{
	if(infoComponent.setFile (currentPath))
	{
		infoComponent.setDisplayAttributes (currentIcon, currentTitle);
		return true;
	}
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FilePreviewComponent::isUsedInFileSelector () const
{
	return usedInFileSelector;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileInfoComponent* FilePreviewComponent::getCurrentInfoComponent () const
{
	if(usedInFileSelector == true)
		return currentInfoComponent;
	else
	{
		IView* firstView = infoView ? ViewBox (infoView).getChildren ().getFirstView () : nullptr;
		UnknownPtr<IFileInfoComponent> infoComponent (firstView ? (ViewBox (firstView).getController ()) : nullptr);
		return infoComponent;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FilePreviewComponent::updateView (bool isNewFile)
{
	if(usedInFileSelector == false && infoView == nullptr)
		return;

	AutoPtr<IFileInfoComponent> infoComponent;

	// check if existing component can handle this file ...
	SharedPtr<IFileInfoComponent> oldComponent = getCurrentInfoComponent ();
	if(oldComponent && !oldComponent->isDefault () && setPreviewContent (*oldComponent))
	{
		infoComponent.share (oldComponent);

		UnknownPtr<ISubject> subject (infoComponent);
		if(subject)
			subject->signal (Message (kPropertyChanged));
	}
	else
	{
		if(infoView)
			ViewBox (infoView).getChildren ().removeAll ();

		infoComponent = createInfoComponent ();

		if(infoComponent && infoView)
		{
			ViewBox view (infoView);
			UnknownPtr<IViewFactory> viewFactory (infoComponent);
			IView* childView = viewFactory ? viewFactory->createView ("FileInfo", 0, Rect ()) : nullptr;
			ASSERT (childView != nullptr)
			if(childView)
			{
				Rect size (view->getSize ());
				size.moveTo (Point ());
				childView->setSize (size);

				view.getChildren ().add (childView);
			}
		}
	}

	onUpdateFile (infoComponent, isNewFile);

	if(usedInFileSelector == true)
	{
		take_shared<IFileInfoComponent> (currentInfoComponent, infoComponent);

		// update file information strings for customized file selector
		String infoString1, infoString2;
		if(infoComponent)
		{
			infoComponent->getFileInfoString (infoString1, IFileInfoComponent::kFileInfo1);
			infoComponent->getFileInfoString (infoString2, IFileInfoComponent::kFileInfo2);
		}
		paramList.byTag (Tag::kFileInfo1)->fromString (infoString1);
		paramList.byTag (Tag::kFileInfo2)->fromString (infoString2);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API FilePreviewComponent::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "FileInfoFrame")
	{
		loadParams ();

		IView* view = ViewBox (ClassID::View, bounds);
		infoView = UnknownPtr<ISubject> (view);
		infoView->addObserver (this);

		updateView (true);

		return view;
	}
	return SuperClass::createView (name, data, bounds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FilePreviewComponent::releaseFile (UrlRef path)
{
	if(currentPath.isEqualUrl (path))
		setFile (Url (), nullptr, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FilePreviewComponent::notify (ISubject* subject, MessageRef msg)
{
	if(subject == infoView && msg == kDestroyed)
	{
		infoView->removeObserver (this);
		infoView = nullptr;

		saveParams ();

		// make subclass release references to info component
		onUpdateFile (nullptr, true);
	}
	else if(msg == Signals::kReleaseFile)
	{
		UnknownPtr<IUrl> path (msg[0]);
		if(path)
			releaseFile (*path);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FilePreviewComponent::customizeFileSelector (IFileSelectorCustomize& fsc)
{
	usedInFileSelector = true;

	fsc.beginGroup (XSTR (FileInformation));
	fsc.addTextBox (paramList.byTag (Tag::kFileInfo1));
	fsc.addTextBox (paramList.byTag (Tag::kFileInfo2));
	fsc.endGroup ();
}
