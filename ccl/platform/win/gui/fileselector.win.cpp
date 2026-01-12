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
// Filename    : ccl/platform/win/gui/fileselector.win.cpp
// Description : platform-specific file selector code
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/dialogs/fileselector.h"
#include "ccl/gui/windows/systemwindow.h"

#include "ccl/base/asyncoperation.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/gui/iparameter.h"

#include "ccl/platform/win/cclwindows.h"
#include "ccl/platform/win/system/cclcom.h"
#include "ccl/platform/win/gui/dpihelper.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileSelector")
	XSTRING (AllFiles, "All Files (*.*)")
	XSTRING (AllSupportedFiles, "All Supported Files")
END_XSTRINGS

namespace CCL {
namespace Win32 {

//************************************************************************************************
// FileFilter
//************************************************************************************************

struct FileFilter
{
	String description;
	String extensions;

	FileFilter (StringRef description = nullptr, StringRef extensions = nullptr)
	: description (description),
	  extensions (extensions)
	{}

	FileFilter (const FileType& fileType)
	: description (fileType.getDescription ())
	{
		addExtension (fileType.getExtension ());
	}

	void addExtension (StringRef ext)
	{
		if(!extensions.isEmpty ())
			extensions << ";";
		extensions << "*." << ext;
	}

	bool operator == (const FileFilter& other) const
	{
		return description == other.description;
	}
};

//************************************************************************************************
// FileFilterList
//************************************************************************************************

class FileFilterList: public Vector<FileFilter>
{
public:
	void build (const Container& fileTypes, bool allFiles)
	{
		// filter for all supported files
		if(allFiles && fileTypes.count () > 1)
		{
			FileFilter filter (XSTR (AllSupportedFiles));
			ForEach (fileTypes, Boxed::FileType, ft)
				filter.addExtension (ft->getExtension ());
			EndFor
			add (filter);
		}

		// collect unique descriptions
		Vector<FileFilter> uniqueFilters;
		ForEach (fileTypes, Boxed::FileType, ft)
			FileFilter filter (*ft);
			int index = uniqueFilters.index (filter);
			if(index != -1)
				uniqueFilters[index].addExtension (ft->getExtension ());
			else
				uniqueFilters.add (filter);
		EndFor

		VectorForEach (uniqueFilters, FileFilter, filter)
			filter.description = filter.description << " (" << filter.extensions << ")";
			add (filter);
		EndFor

		// filter for all files
		if(allFiles)
		{
			FileFilter filter (XSTR (AllFiles), "*.*");
			add (filter);
		}
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

static Url* ShellItemToFrameworkPath (::IShellItem* item, int type = Url::kDetect)
{
	LPWSTR path = nullptr;
	if(item)
		item->GetDisplayName (SIGDN_FILESYSPATH, &path);

	Url* result = nullptr;
	if(path)
	{
		if(type == Url::kDetect)
		{
			// Note that Windows reports .zip files as folders so this isn't realiable.
			// Caller should specify type explicitly.
			SFGAOF attr = 0;
			item->GetAttributes (SFGAO_FOLDER, &attr);			
			bool isFolder = (attr & SFGAO_FOLDER) != 0;
			type = isFolder ? Url::kFolder : Url::kFile;
		}

		result = NEW Url;
		result->fromNativePath (path, type);
		::CoTaskMemFree (path);
	}
	return result;
}

static ::IShellItem* FrameworkPathToShellItem (UrlRef path)
{
	ComPtr<IShellItem> item;
	::SHCreateItemFromParsingName (StringChars (UrlDisplayString (path)), nullptr, IID_IShellItem, item);
	return item.detach ();
}

//************************************************************************************************
// FileSelectorCustomize
//************************************************************************************************

class FileSelectorCustomize: public Object,
							 public IFileSelectorCustomize,
							 public ::IFileDialogEvents,
							 public ::IFileDialogControlEvents
{
public:
	FileSelectorCustomize (NativeFileSelector& fileSelector, ::IFileDialogCustomize* _fdc)
	: fileSelector (fileSelector),
	  nextId (100)
	{
		fdc.share (_fdc);
	}

	~FileSelectorCustomize ()
	{
		removeAll ();
	}

	// IUnknown
	PARENT_REFCOUNT (Object)
	DELEGATE_COM_IUNKNOWN
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override
	{
		QUERY_COM_INTERFACE (IFileDialogEvents)
		QUERY_COM_INTERFACE (IFileDialogControlEvents)
		QUERY_INTERFACE (IFileSelectorCustomize)
		return Object::queryInterface (iid, ptr);
	}

	// IFileSelectorCustomize
	void CCL_API beginGroup (StringRef title) override
	{
		HRESULT hr = fdc->StartVisualGroup (nextId++, StringChars (title));
		ASSERT (SUCCEEDED (hr))
	}

	void CCL_API endGroup () override
	{
		HRESULT hr = fdc->EndVisualGroup ();
		ASSERT (SUCCEEDED (hr))
	}

	void CCL_API addTextBox (IParameter* p) override
	{
		ControlItem c (nextId++, p, ControlItem::kTextBox);
		addInternal (c);

		HRESULT hr = fdc->AddText (c.id, L"");
		ASSERT (SUCCEEDED (hr))
		updateText (c);
		updateEnabled (c);
	}

	void CCL_API addButton (IParameter* p, StringRef title) override
	{
		ControlItem c (nextId++, p, ControlItem::kButton);
		addInternal (c);

		HRESULT hr = fdc->AddPushButton (c.id, StringChars (title));
		ASSERT (SUCCEEDED (hr))
		updateEnabled (c);
	}

	void CCL_API addCheckBox (IParameter* p, StringRef title) override
	{
		ControlItem c (nextId++, p, ControlItem::kCheckBox);
		addInternal (c);

		HRESULT hr = fdc->AddCheckButton (c.id, StringChars (title), p->getValue ().asBool ());
		ASSERT (SUCCEEDED (hr))
		updateEnabled (c);
	}

	void CCL_API notify (ISubject* subject, MessageRef msg) override
	{
		if(msg == kChanged)
			if(UnknownPtr<IParameter> p = subject)
				if(const ControlItem* c = find (p))
					updateControl (*c);
	}

	// IFileDialogEvents
	HRESULT STDMETHODCALLTYPE OnFileOk (IFileDialog *pfd) override
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnFolderChanging (IFileDialog *pfd, IShellItem *psiFolder) override
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnFolderChange (IFileDialog *pfd) override
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnSelectionChange (IFileDialog *pfd) override
	{
		if(UnknownPtr<IFileSelectorHook> hook = fileSelector.getHook ())
		{
			ComPtr<IShellItem> item;
			pfd->GetCurrentSelection (item);
			if(AutoPtr<Url> path = ShellItemToFrameworkPath (item, Url::kFile))
				hook->onSelectionChanged (fileSelector, *path);
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnShareViolation (IFileDialog *pfd, IShellItem *psi, FDE_SHAREVIOLATION_RESPONSE *pResponse) override
	{
		return E_NOTIMPL; // The implementer should return E_NOTIMPL if this method is not implemented
	}

	HRESULT STDMETHODCALLTYPE OnTypeChange (IFileDialog *pfd) override
	{
		if(UnknownPtr<IFileSelectorHook> hook = fileSelector.getHook ())
		{
			UINT index = 0;
			pfd->GetFileTypeIndex (&index);
			hook->onFilterChanged (fileSelector, index);
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnOverwrite (IFileDialog *pfd, IShellItem *psi, FDE_OVERWRITE_RESPONSE *pResponse) override
	{
		return E_NOTIMPL; // The implementer should return E_NOTIMPL if this method is not implemented
	}

	// IFileDialogControlEvents
	HRESULT STDMETHODCALLTYPE OnItemSelected (IFileDialogCustomize *pfdc, DWORD dwIDCtl, DWORD dwIDItem) override
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnButtonClicked (IFileDialogCustomize *pfdc, DWORD dwIDCtl) override
	{
		if(IParameter* p = find (dwIDCtl))
		{
			ASSERT (p->isEnabled ())
			p->beginEdit ();
			p->setValue (1, true);
			p->setValue (0, false);
			p->endEdit ();
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnCheckButtonToggled (IFileDialogCustomize *pfdc, DWORD dwIDCtl, BOOL bChecked) override
	{
		if(IParameter* p = find (dwIDCtl))
		{
			ASSERT (p->isEnabled ())
			p->beginEdit ();
			p->setValue (bChecked, true);
			p->endEdit ();
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnControlActivating (IFileDialogCustomize *pfdc, DWORD dwIDCtl) override
	{
		return S_OK;
	}

protected:
	NativeFileSelector& fileSelector;
	ComPtr<IFileDialogCustomize> fdc;
	DWORD nextId;

	struct ControlItem
	{
		enum Type { kButton, kCheckBox, kTextBox };

		DWORD id;
		IParameter* parameter;
		Type type;

		ControlItem (DWORD id = 0, IParameter* p = nullptr, Type type = kButton)
		: id (id),
		  parameter (p),
		  type (type)
		{}
	};

	Vector<ControlItem> controls;

	void addInternal (const ControlItem& c)
	{
		c.parameter->retain ();
		ISubject::addObserver (c.parameter, this);
		controls.add (c);
	}

	void removeAll ()
	{
		VectorForEach (controls, ControlItem, c)
			ISubject::removeObserver (c.parameter, this);
			c.parameter->release ();
		EndFor
		controls.removeAll ();
	}

	IParameter* find (DWORD id) const
	{
		VectorForEach (controls, ControlItem, c)
			if(c.id == id)
				return c.parameter;
		EndFor
		return nullptr;
	}

	const ControlItem* find (IParameter* p) const
	{
		for(int i = 0; i < controls.count (); i++)
		{
			const ControlItem& c = controls[i];
			if(c.parameter == p)
				return &c;
		}
		return nullptr;
	}

	void updateControl (const ControlItem& c)
	{
		switch(c.type)
		{
		case ControlItem::kTextBox :
			updateText (c);
			updateEnabled (c);
			break;

		case ControlItem::kButton :
			updateEnabled (c);
			break;

		case ControlItem::kCheckBox :
			updateChecked (c);
			updateEnabled (c);
			break;
		}
	}

	void updateText (const ControlItem& c)
	{
		String text;
		c.parameter->toString (text);
		HRESULT hr = fdc->SetControlLabel (c.id, StringChars (text));
		ASSERT (SUCCEEDED (hr))
	}

	void updateEnabled (const ControlItem& c)
	{
		CDCONTROLSTATEF state = CDCS_VISIBLE;
		if(c.parameter->isEnabled ())
			state |= CDCS_ENABLED;
		HRESULT hr = fdc->SetControlState (c.id, state);
		ASSERT (SUCCEEDED (hr))
	}

	void updateChecked (const ControlItem& c)
	{
		HRESULT hr = fdc->SetCheckButtonState (c.id, c.parameter->getValue ().asBool ());
		ASSERT (SUCCEEDED (hr))
	}
};

} // namespace Win32
} // namespace CCL

namespace CCL {

//************************************************************************************************
// WindowsFileSelector
//************************************************************************************************

class WindowsFileSelector: public NativeFileSelector
{
public:
	DECLARE_CLASS (WindowsFileSelector, NativeFileSelector)

	// NativeFileSelector
	bool runPlatformSelector (int type, StringRef title, int filterIndex, IWindow* window) override;
	IAsyncOperation* runPlatformSelectorAsync (int type, StringRef title, int filterIndex, IWindow* window) override;
};

//************************************************************************************************
// WindowsFolderSelector
//************************************************************************************************

class WindowsFolderSelector: public NativeFolderSelector
{
public:
	DECLARE_CLASS (WindowsFolderSelector, NativeFolderSelector)

	// NativeFolderSelector
	bool runPlatformSelector (StringRef title, IWindow* window) override;
	IAsyncOperation* runPlatformSelectorAsync (StringRef title, IWindow* window) override;
};

} // namespace CCL

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// WindowsFileSelector
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (WindowsFileSelector, NativeFileSelector, "FileSelector")
DEFINE_CLASS_UID (WindowsFileSelector, 0xacfd316a, 0x371d, 0x4ba2, 0x9b, 0x7e, 0x45, 0xce, 0xc8, 0x7a, 0x2c, 0xbf) // ClassID::FileSelector

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsFileSelector::runPlatformSelector (int type, StringRef title, int filterIndex, IWindow* window)
{
	ComPtr<::IFileDialog> fd = com_new<::IFileDialog> (type == kSaveFile ? CLSID_FileSaveDialog : CLSID_FileOpenDialog);
	ASSERT (fd.isValid ())
	if(fd == 0)
		return false;

	// add filters
	FileFilterList filterList;
	filterList.build (filters, type != kSaveFile);
	Vector<COMDLG_FILTERSPEC> filterSpecs;
	for(int i = 0; i < filterList.count (); i++)
	{
		// This works because Unicode string pointer remains valid.
		const FileFilter& filter = filterList[i];
		COMDLG_FILTERSPEC spec = {StringChars (filter.description), StringChars (filter.extensions)};
		filterSpecs.add (spec);
	}

	HRESULT hr = S_OK;
	if(!filterSpecs.isEmpty ())
		hr = fd->SetFileTypes (filterSpecs.count (), filterSpecs);
	ASSERT (SUCCEEDED (hr))

	String defaultExt;
	if(getFilter ())
		defaultExt = getFilter ()->getExtension ();
	if(!defaultExt.isEmpty ())
		hr = fd->SetDefaultExtension (StringChars (defaultExt));
	ASSERT (SUCCEEDED (hr))

	Url initialFolder (getInitialFolder ());
	if(initialFolder.isEmpty () == false)
	{
		ComPtr<::IShellItem> shellItem = FrameworkPathToShellItem (initialFolder);
		hr = fd->SetFolder (shellItem);
		ASSERT (SUCCEEDED (hr))
	}

	if(!getInitialFileName ().isEmpty ())
	{
		hr = fd->SetFileName (StringChars (getInitialFileName ()));
		ASSERT (SUCCEEDED (hr))
	}

	//hr = fd->SetFileTypeIndex (filterIndex);
	//ASSERT (SUCCEEDED (hr))

	if(!title.isEmpty ())
		hr = fd->SetTitle (StringChars (title));
	ASSERT (SUCCEEDED (hr))

	// set options
	FILEOPENDIALOGOPTIONS options =
		FOS_NOCHANGEDIR|
		FOS_FORCEFILESYSTEM|
		FOS_PATHMUSTEXIST|
		FOS_DONTADDTORECENT;

	switch(type)
	{
	case kSaveFile :
		options |= FOS_OVERWRITEPROMPT|FOS_NOREADONLYRETURN;
		break;

	case kOpenFile :
	case kOpenMultipleFiles :
		options |= FOS_FILEMUSTEXIST;
		if(type == kOpenMultipleFiles)
			options |= FOS_ALLOWMULTISELECT;
		break;
	}

	hr = fd->SetOptions (options);
	ASSERT (SUCCEEDED (hr))

	// customization
	DWORD cookie = 0;
	ComPtr<IFileDialogCustomize> fdc;
	fd.as (fdc);
	if(hook && fdc)
	{
		AutoPtr<FileSelectorCustomize> customizer = NEW FileSelectorCustomize (*this, fdc);
		if(UnknownPtr<IFileSelectorHook> fsHook = hook)
			fsHook->onCustomize (*customizer);

		hr = fd->Advise (customizer, &cookie);
		ASSERT (SUCCEEDED (hr))
	}

	// show modal window
	Win32::DpiAwarenessScope dpiScope (Win32::gDpiInfo, Win32::kDpiContextSystemAware);
	AutoPtr<IWindow> systemWindow (NEW ModalSystemWindow);
	HWND hwndOwner = window ? (HWND)window->getSystemWindow () : nullptr;
	hr = fd->Show (hwndOwner);
	bool canceled = FAILED (hr);

	if(cookie != 0)
	{
		hr = fd->Unadvise (cookie);
		ASSERT (SUCCEEDED (hr))
	}

	if(canceled == false)
	{
		if(type == kOpenMultipleFiles)
		{
			ComPtr<IShellItemArray> resultArray;
			ComPtr<::IFileOpenDialog> fod;
			fd.as (fod);
			if(fod)
				hr = fod->GetResults (resultArray);

			DWORD count = 0;
			if(resultArray)
				hr = resultArray->GetCount (&count);
			for(DWORD i = 0; i < count; i++)
			{
				ComPtr<IShellItem> resultItem;
				if(SUCCEEDED (resultArray->GetItemAt (i, resultItem)))
				{
					if(Url* path = ShellItemToFrameworkPath (resultItem, Url::kFile))
						paths.add (path);
				}
			}
		}
		else
		{
			ComPtr<IShellItem> resultItem;
			hr = fd->GetResult (resultItem);
			ASSERT (SUCCEEDED (hr))
			if(Url* path = ShellItemToFrameworkPath (resultItem, Url::kFile))
				paths.add (path);
		}
	}

	return !canceled && !paths.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* WindowsFileSelector::runPlatformSelectorAsync (int type, StringRef title, int filterIndex, IWindow* window)
{
	int result = runPlatformSelector (type, title, filterIndex, window);

	return AsyncOperation::createCompleted (result);
}

//************************************************************************************************
// WindowsFolderSelector
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (WindowsFolderSelector, NativeFolderSelector, "FolderSelector")
DEFINE_CLASS_UID (WindowsFolderSelector, 0x898fbf4d, 0x15d, 0x4754, 0x93, 0xa, 0xf1, 0x7a, 0xa7, 0x0, 0x82, 0xfc) // ClassID::FolderSelector

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsFolderSelector::runPlatformSelector (StringRef title, IWindow* window)
{
	ComPtr<::IFileDialog> fd = com_new<::IFileDialog> (CLSID_FileOpenDialog);
	ASSERT (fd.isValid ())
	if(fd == 0)
		return false;

	HRESULT hr = S_OK;
	Url initialFolder (getInitialPath ());
	if(initialFolder.isEmpty () == false)
	{
		ComPtr<::IShellItem> shellItem = FrameworkPathToShellItem (initialFolder);
		hr = fd->SetFolder (shellItem);
		ASSERT (SUCCEEDED (hr))
	}

	if(!title.isEmpty ())
		hr = fd->SetTitle (StringChars (title));
	ASSERT (SUCCEEDED (hr))

	// set options
	FILEOPENDIALOGOPTIONS options =
		FOS_NOCHANGEDIR|
		FOS_PICKFOLDERS| // <-- this makes it a folder dialog
		FOS_FORCEFILESYSTEM|
		FOS_PATHMUSTEXIST|
		FOS_DONTADDTORECENT;

	hr = fd->SetOptions (options);
	ASSERT (SUCCEEDED (hr))

	// show modal window
	Win32::DpiAwarenessScope dpiScope (Win32::gDpiInfo, Win32::kDpiContextSystemAware);
	AutoPtr<IWindow> systemWindow (NEW ModalSystemWindow);
	HWND hwndOwner = window ? (HWND)window->getSystemWindow () : nullptr;
	hr = fd->Show (hwndOwner);
	bool canceled = FAILED (hr);

	if(canceled == false)
	{
		ComPtr<IShellItem> resultItem;
		hr = fd->GetResult (resultItem);
		ASSERT (SUCCEEDED (hr))
		if(AutoPtr<Url> resultPath = ShellItemToFrameworkPath (resultItem, Url::kFolder))
			setPath (*resultPath);
	}

	return !canceled && !getPath ().isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* WindowsFolderSelector::runPlatformSelectorAsync (StringRef title, IWindow* window)
{
	int result = runPlatformSelector (title, window);

	return AsyncOperation::createCompleted (result);
}
