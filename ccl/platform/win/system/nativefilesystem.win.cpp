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
// Filename    : ccl/platform/win/system/nativefilesystem.win.cpp
// Description : Windows native file system
//
//************************************************************************************************

#define DEBUG_LOG DEBUG

#include "ccl/platform/win/system/nativefilesystem.win.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/collections/container.h"

#include "ccl/public/base/iprogress.h"

#include "ccl/platform/win/system/cclcom.h" // needs to be included before Windows headers
#include "ccl/platform/win/system/system.win.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/system/userthread.h"
#include "ccl/public/system/threadsync.h"

#include <Aclapi.h>

#ifndef ACCESS_READ
	#define ACCESS_READ         0x01
	#define ACCESS_WRITE        0x02
	#define ACCESS_EXEC         0x08
#endif

namespace CCL {

//************************************************************************************************
// DeferredFileOperation
//************************************************************************************************

class DeferredFileOperation: public Object,
							 public ::IFileOperationProgressSink
{
public:
	DELEGATE_COM_IUNKNOWN
	DECLARE_CLASS_ABSTRACT (DeferredFileOperation, Object)

	static DeferredFileOperation* create ();
	DeferredFileOperation ();

	tbool removeFile (UrlRef url);
	tbool perform (tbool withUndo, CCL::IProgressNotify* progress);

	CLASS_INTERFACES (Object)
protected:

	//IFileOperationProgressSink
	HRESULT STDMETHODCALLTYPE StartOperations () override;
    HRESULT STDMETHODCALLTYPE FinishOperations (/* [in] */ HRESULT hrResult) override;
    HRESULT STDMETHODCALLTYPE PreRenameItem (DWORD dwFlags, __RPC__in_opt IShellItem *psiItem, __RPC__in_opt_string LPCWSTR pszNewName) override {return S_OK;}
    HRESULT STDMETHODCALLTYPE PostRenameItem (DWORD dwFlags, __RPC__in_opt IShellItem *psiItem,  __RPC__in_string LPCWSTR pszNewName, HRESULT hrRename, __RPC__in_opt IShellItem *psiNewlyCreated) override {return S_OK;}
    HRESULT STDMETHODCALLTYPE PreMoveItem (DWORD dwFlags,  __RPC__in_opt IShellItem *psiItem,  __RPC__in_opt IShellItem *psiDestinationFolder, __RPC__in_opt_string LPCWSTR pszNewName) override {return S_OK;}
    HRESULT STDMETHODCALLTYPE PostMoveItem (DWORD dwFlags, __RPC__in_opt IShellItem *psiItem, __RPC__in_opt IShellItem *psiDestinationFolder, __RPC__in_opt_string LPCWSTR pszNewName,
            HRESULT hrMove, __RPC__in_opt IShellItem *psiNewlyCreated) override {return S_OK;}
	HRESULT STDMETHODCALLTYPE PreCopyItem (DWORD dwFlags, __RPC__in_opt IShellItem *psiItem, __RPC__in_opt IShellItem *psiDestinationFolder, __RPC__in_opt_string LPCWSTR pszNewName) override {return S_OK;}
	HRESULT STDMETHODCALLTYPE PostCopyItem (DWORD dwFlags, __RPC__in_opt IShellItem *psiItem,  __RPC__in_opt IShellItem *psiDestinationFolder, __RPC__in_opt_string LPCWSTR pszNewName,
            HRESULT hrCopy, __RPC__in_opt IShellItem *psiNewlyCreated) override {return S_OK;}
	HRESULT STDMETHODCALLTYPE PreDeleteItem (DWORD dwFlags, __RPC__in_opt IShellItem *psiItem) override;
	HRESULT STDMETHODCALLTYPE PostDeleteItem (DWORD dwFlags, __RPC__in_opt IShellItem *psiItem, HRESULT hrDelete, __RPC__in_opt IShellItem *psiNewlyCreated) override {return S_OK;}
	HRESULT STDMETHODCALLTYPE PreNewItem (DWORD dwFlags, __RPC__in_opt IShellItem *psiDestinationFolder, __RPC__in_opt_string LPCWSTR pszNewName) override {return S_OK;}
	HRESULT STDMETHODCALLTYPE PostNewItem (DWORD dwFlags, __RPC__in_opt IShellItem *psiDestinationFolder, __RPC__in_opt_string LPCWSTR pszNewName,  __RPC__in_opt_string LPCWSTR pszTemplateName,
            DWORD dwFileAttributes, HRESULT hrNew, __RPC__in_opt IShellItem *psiNewItem) override {return S_OK;}
	HRESULT STDMETHODCALLTYPE UpdateProgress (UINT iWorkTotal, UINT iWorkSoFar) override;
    HRESULT STDMETHODCALLTYPE ResetTimer () override {return E_NOTIMPL;}
	HRESULT STDMETHODCALLTYPE PauseTimer () override {return E_NOTIMPL;}
	HRESULT STDMETHODCALLTYPE ResumeTimer () override {return E_NOTIMPL;}

	Win32::ComPtr<IFileOperation> fileOp;
	CCL::IProgressNotify* progress;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

struct ErrorModeInitializer
{
	ErrorModeInitializer ()
	{
		// suppress dialog box if e.g. CD-ROM has been removed from drive
		::SetErrorMode (SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS);
	}
};

static ErrorModeInitializer theErrorModeInitializer;

//////////////////////////////////////////////////////////////////////////////////////////////////

static void AppendTrailingBackslash (NativePath& path)
{
	size_t length = ::wcslen (path);
	if(length > 0 && path[length-1] != '\\')
		::wcscat (path.path, L"\\");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void MakeRoot (NativePath& path)
{
	size_t length = ::wcslen (path);
	if(length > 2)
	{
		if(path[1] == ':')
		{
			path[2] = 0;
			::wcscat (path.path, L"\\");
			return;
		}
		if(path[0] == '\\' && path[1] == '\\')
		{
			wchar_t* p = wcsrchr (path.path + 2, '\\');
			if(p)
				*p = 0;
			::wcscat (path.path, L"\\");
			return;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void TranslateMode (int mode, DWORD& access, DWORD& sharing, DWORD& flags)
{
	access = 0;
	sharing = 0;
	flags = FILE_ATTRIBUTE_NORMAL;

	if(mode & CCL::IStream::kReadMode)
		access  |= GENERIC_READ;
	if(mode & CCL::IStream::kWriteMode)
		access |= GENERIC_WRITE;
	if(mode & CCL::IStream::kShareRead)
		sharing |= FILE_SHARE_READ;
	if(mode & CCL::IStream::kShareWrite)
		sharing |= FILE_SHARE_WRITE;
	if(mode & CCL::INativeFileStream::kWriteThru)
		flags |= FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH;
	if(mode & CCL::INativeFileStream::kReadNonBuffered)
		flags |= FILE_FLAG_NO_BUFFERING;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void WriteEnableFile (LPCWSTR fileName)
{
	DWORD attr = ::GetFileAttributes (fileName);
	if(attr != INVALID_FILE_ATTRIBUTES)
		if(attr & FILE_ATTRIBUTE_READONLY)
			::SetFileAttributes (fileName, attr &~FILE_ATTRIBUTE_READONLY);
}

//************************************************************************************************
// WindowsNativeFileSystem
//************************************************************************************************

NativeFileSystem& NativeFileSystem::instance ()
{
	static WindowsNativeFileSystem theInstance;
	return theInstance;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IStream* WindowsNativeFileSystem::openPlatformStream (UrlRef url, int mode)
{
	DWORD access, sharing, flags;
	TranslateMode (mode, access, sharing, flags);

	NativePath path (url);
	HANDLE handle = ::CreateFile (path, access, sharing, nullptr, (mode & IStream::kCreate) ? CREATE_ALWAYS : OPEN_EXISTING, flags, nullptr);
	if(handle == INVALID_HANDLE_VALUE)
	{
		int lastError = ::GetLastError ();
		onNativeError (lastError, &url);

		if(mode & IStream::kWriteMode)
		{ CCL_WARN ("CreateFile() function failed! Path: %ws Error: %x\n", path.path, lastError) }

		return nullptr;
	}

	WindowsFileStream* file = NEW WindowsFileStream (this, handle, mode);
	memcpy (file->path, path.path, sizeof(path.path));
	return file;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsNativeFileSystem::getFileInfo (FileInfo& info, UrlRef url)
{
	bool result = false;
	FILETIME ct, at, wt;
	int64 fileSize = 0;

	DWORD flags = url.isFolder () ? FILE_FLAG_BACKUP_SEMANTICS : FILE_ATTRIBUTE_NORMAL;

	NativePath path (url);
	HANDLE hFile = ::CreateFile (path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, flags, nullptr);
	if(hFile != INVALID_HANDLE_VALUE)
	{
		result = ::GetFileTime (hFile, &ct, &at, &wt) == TRUE;

		LARGE_INTEGER largeInt = {0};
		::GetFileSizeEx (hFile, &largeInt);
		fileSize = largeInt.QuadPart;

		::CloseHandle (hFile);
	}
	else
	{
		int lastError = ::GetLastError ();
		onNativeError (lastError, &url);
	}

	if(result)
	{
		info.fileSize = fileSize;

		SYSTEMTIME st;
		SYSTEMTIME lt;

		#define FROM_SYSTEM_TIME(dateTime, st) \
			dateTime.setTime (Time (st.wHour, st.wMinute, st.wSecond)); \
			dateTime.setDate (Date (st.wYear, st.wMonth, st.wDay));

		::FileTimeToSystemTime (&ct, &st);
		::SystemTimeToTzSpecificLocalTime (nullptr, &st, &lt);
		FROM_SYSTEM_TIME (info.createTime, lt)

		::FileTimeToSystemTime (&at, &st);
		::SystemTimeToTzSpecificLocalTime (nullptr, &st, &lt);
		FROM_SYSTEM_TIME (info.accessTime, lt)

		::FileTimeToSystemTime (&wt, &st);
		::SystemTimeToTzSpecificLocalTime (nullptr, &st, &lt);
		FROM_SYSTEM_TIME (info.modifiedTime, lt)

		#undef FROM_SYSTEM_TIME
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsNativeFileSystem::setFileTime (UrlRef url, const FileTime& modifiedTime)
{
	int lastError = 0;
	NativePath path (url);
	HANDLE hFile = ::CreateFile (path, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if(hFile != INVALID_HANDLE_VALUE)
	{
		SYSTEMTIME lt = {0};
		Win32::toSystemTime (lt, modifiedTime);

		SYSTEMTIME st = {0};
		::TzSpecificLocalTimeToSystemTime (nullptr, &lt, &st);

		FILETIME ft = {0};
		::SystemTimeToFileTime (&st, &ft);

		if(!::SetFileTime (hFile, nullptr, nullptr, &ft))
			lastError = ::GetLastError ();

		::CloseHandle (hFile);
	}
	else
		lastError = ::GetLastError ();

	if(lastError != 0)
	{
		onNativeError (lastError, &url);
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsNativeFileSystem::fileExists (UrlRef url)
{
	ErrorModeInitializer errorModeInitializer; // suppress open file error box

	NativePath path (url);
	DWORD attr = ::GetFileAttributes (path);
	if(attr != INVALID_FILE_ATTRIBUTES)
	{
		if(url.isFolder ())
			return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
		else
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsNativeFileSystem::removeFile (UrlRef url, int mode)
{
	DeferredFileOperation* op = (DeferredFileOperation*)getTransaction ();
	if(op)
		return op->removeFile (url);

	NativePath path (url);
	WriteEnableFile (path); // try to write-enable file

	if(mode & kDeleteToTrashBin)
	{
		int len = (int)::wcslen (path);
		ASSERT (len + 1 < IUrl::kMaxLength)
		if(len + 1 < IUrl::kMaxLength)
			path.path[len + 1] = 0; // make this a double 0 terminated list of paths

		SHFILEOPSTRUCT fileOperation = {nullptr};
		fileOperation.wFunc  = FO_DELETE;
		fileOperation.pFrom  = path;
		fileOperation.pTo    = nullptr;
		fileOperation.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
		int errorCode = SHFileOperation (&fileOperation);
		if(errorCode == 0)
			return true;
		else
		{
			// Note: According to MSDN this function is not compatible with GetLastError(),
			// but its return values largely map to Win32 error codes. In some cases this might be wrong,
			// but we get at least some more details most of the time.
			onNativeError (errorCode, &url);
		}
	}
	else
	{
		if(::DeleteFile (path))
			return true;
		else
		{
			int lastError = ::GetLastError ();
			onNativeError (lastError, &url);
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* CCL_API WindowsNativeFileSystem::newIterator (UrlRef url, int mode)
{
	if(url.getHostName ().isEmpty () && url.getPath ().isEmpty ())
		return NEW WindowsVolumesIterator;
	else
		return NEW WindowsFileIterator (url, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsNativeFileSystem::createPlatformFolder (UrlRef url)
{
	NativePath path (url);
	BOOL result = ::CreateDirectory (path, nullptr);
	if(result == FALSE)
	{
		int lastError = ::GetLastError ();
		onNativeError (lastError, &url);
	}
	return result != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowsNativeFileSystem::removePlatformFolder (UrlRef url, int mode)
{
	if(mode & kDeleteToTrashBin) // use SHFileOperation
		return removeFile (url, kDeleteToTrashBin) != 0;

	NativePath path (url);
	BOOL result = ::RemoveDirectory (path);
	if(result == FALSE)
	{
		int lastError = ::GetLastError ();
		onNativeError (lastError, &url);
	}
	return result != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsNativeFileSystem::getVolumeInfo (VolumeInfo& info, UrlRef url)
{
	ErrorModeInitializer errorModeInitializer; // suppress open file error box

	bool suppressSlowVolumeInfo = (info.type & kSuppressSlowVolumeInfo) != 0;
	info.type = VolumeInfo::kUnknown;

	NativePath path (url);
	MakeRoot (path);
	AppendTrailingBackslash (path);	// function needs a trailing backslash!

	info.type = VolumeInfo::kUnknown;
	UINT driveType = ::GetDriveType (path);
	switch(driveType)
	{
	case DRIVE_REMOVABLE : info.type = VolumeInfo::kRemovable; break;
	case DRIVE_FIXED     : info.type = VolumeInfo::kLocal; break;
	case DRIVE_REMOTE    : info.type = VolumeInfo::kRemote; break;
	case DRIVE_CDROM     : info.type = VolumeInfo::kOptical; break;
	}

	info.serialNumber.empty ();
	info.label.empty ();

	// GetVolumeInformation() can take long for Floppy-Drives / Card Readers, especially when empty
	bool tryVolumeName = driveType == DRIVE_FIXED || suppressSlowVolumeInfo == false;
	if(driveType == DRIVE_REMOVABLE)
	{
		DWORD mediaContent = 0;
		HRESULT hr = ::SHGetDriveMedia (path, &mediaContent);
		tryVolumeName = SUCCEEDED (hr) && (mediaContent & ARCONTENT_MASK) != ARCONTENT_NONE;
	}

	if(tryVolumeName == true)
	{
		WCHAR nameBuffer[256] = {0};
		DWORD serialNumber = 0, unused1 = 0, unused2 = 0;
		::GetVolumeInformation (path, nameBuffer, 255, &serialNumber, &unused1, &unused2, nullptr, 0);
		//ASSERT (result == TRUE) // (note: function fails for removable drives with no media inside or unformatted drives)

		info.label = nameBuffer;
		info.serialNumber.appendHexValue ((int64)serialNumber, 8); // "%08X"
	}

	if(driveType == DRIVE_FIXED || (suppressSlowVolumeInfo == false && driveType != DRIVE_CDROM))
	{
		ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
		DWORD result = ::GetDiskFreeSpaceEx (path, &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes);
		if(result)
		{
			info.bytesTotal = totalNumberOfBytes.QuadPart;
			info.bytesFree = freeBytesAvailable.QuadPart;
		}
		else if(driveType == DRIVE_FIXED)
		{
			int error = ::GetLastError ();
			ASSERT (error == ERROR_UNRECOGNIZED_VOLUME) // e:g. unformatted partition
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsNativeFileSystem::getPathType (int& type, UrlRef baseFolder, StringRef fileName)
{
	NativePath path (baseFolder);
	AppendTrailingBackslash (path);
	::wcscat (path.path, StringChars (fileName));

	DWORD attr = ::GetFileAttributes (path);
	if(attr == INVALID_FILE_ATTRIBUTES)
		return false;

	type = (attr & FILE_ATTRIBUTE_DIRECTORY) != 0 ? IUrl::kFolder : IUrl::kFile;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsNativeFileSystem::isHiddenFile (UrlRef url)
{
	NativePath path (url);
	DWORD attr = ::GetFileAttributes (path);
	if(attr == INVALID_FILE_ATTRIBUTES)
		return false;
	return (attr & FILE_ATTRIBUTE_HIDDEN) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsNativeFileSystem::isWriteProtected (UrlRef url)
{
	NativePath path (url);

	// check FAT file attributes
	// Note: According to MSDN, this attribute is not honored on directories.
	// see http://msdn.microsoft.com/en-us/library/ee332330%28VS.85%29.aspx
	if(url.isFile ())
	{
		DWORD attr = ::GetFileAttributes (path);
		if(attr != INVALID_FILE_ATTRIBUTES)
			if(attr & FILE_ATTRIBUTE_READONLY)
				return true;
	}

	// check security descriptor
	PACL pACL = nullptr;
	PSID sidOwner = nullptr;
	PSID sidGroup = nullptr;
	PSECURITY_DESCRIPTOR descriptor = nullptr;

	DWORD errorCode = ::GetNamedSecurityInfo (path, SE_FILE_OBJECT,
							   DACL_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|OWNER_SECURITY_INFORMATION,
							   &sidOwner, &sidGroup, &pACL, nullptr, &descriptor);

	if(errorCode == ERROR_FILE_NOT_FOUND || errorCode == ERROR_PATH_NOT_FOUND)
		return NativeFileSystem::isWriteProtected (url);

	DWORD grantedAccess = 0;
	if(errorCode == ERROR_SUCCESS)
	{
		// impersonation token is required by AccessCheck()
		BOOL result = ::ImpersonateSelf (SecurityImpersonation);
		ASSERT (result == TRUE)

		HANDLE hAccessToken = nullptr;
		result = ::OpenThreadToken (::GetCurrentThread (), TOKEN_QUERY, TRUE, &hAccessToken);
		ASSERT (result == TRUE)

		BOOL accessStatus = FALSE;
		PRIVILEGE_SET privilegeSet = {0};
		DWORD privilegeSetLength = sizeof(PRIVILEGE_SET);

		static GENERIC_MAPPING kAccessMapping =
			{ACCESS_READ, ACCESS_WRITE, ACCESS_EXEC, ACCESS_READ|ACCESS_WRITE|ACCESS_EXEC};

		result = ::AccessCheck (descriptor, hAccessToken, ACCESS_READ|ACCESS_WRITE, &kAccessMapping,
								&privilegeSet, &privilegeSetLength, &grantedAccess, &accessStatus);

		ASSERT (result == TRUE)
		#if DEBUG
		if(result == FALSE)
		{
			int error = ::GetLastError();
			error = error;
		}
		#endif

		result = ::CloseHandle (hAccessToken);
		ASSERT (result == TRUE)

		result = ::RevertToSelf ();
		ASSERT (result == TRUE)

		::LocalFree (descriptor);
	}

	return (grantedAccess & ACCESS_WRITE) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static DWORD CALLBACK CCLCopyProgressRoutine (LARGE_INTEGER TotalFileSize,
											  LARGE_INTEGER TotalBytesTransferred,
											  LARGE_INTEGER StreamSize,
											  LARGE_INTEGER StreamBytesTransferred,
											  DWORD dwStreamNumber,
											  DWORD dwCallbackReason,
											  HANDLE hSourceFile,
											  HANDLE hDestinationFile,
											  LPVOID lpData)
{
	CCL::IProgressNotify* progress = reinterpret_cast<CCL::IProgressNotify*> (lpData);
	if(progress == nullptr)
		return PROGRESS_QUIET;

	if(progress->isCanceled ())
		return PROGRESS_CANCEL;

	double percent = (double)TotalBytesTransferred.QuadPart / (double)TotalFileSize.QuadPart;
	progress->updateProgress (percent);

	return PROGRESS_CONTINUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsNativeFileSystem::moveFile (UrlRef _dstPath, UrlRef _srcPath, int mode, CCL::IProgressNotify* progress)
{
	createParentFolder (_dstPath); // create folder structure first

	NativePath dstPath (_dstPath);
	NativePath srcPath (_srcPath);

	bool overwrite = (mode & kDoNotOverwrite) == 0;
	bool acrossVolumes = (mode & kDoNotMoveAcrossVolumes) == 0;
	bool writeEnable = (mode & kDisableWriteProtection) != 0;

	if(writeEnable)
	{
		WriteEnableFile (srcPath); // try to write-enable source file

		if(overwrite) // try to write-enable old destination file if it exists
			WriteEnableFile (dstPath);
	}

	DWORD flags = 0;
	if(overwrite == true)
		flags |= MOVEFILE_REPLACE_EXISTING;
	if(acrossVolumes == true)
		flags |= MOVEFILE_COPY_ALLOWED;

	ProgressNotifyScope scope (progress);
	BOOL result = ::MoveFileWithProgress (srcPath, dstPath, CCLCopyProgressRoutine, reinterpret_cast<void*> (progress), flags);
	if(!result)
	{
		int error = ::GetLastError ();
		onNativeError (error, &_srcPath);
	}
	return (tbool) result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsNativeFileSystem::copyFile (UrlRef _dstPath, UrlRef _srcPath, int mode, CCL::IProgressNotify* progress)
{
	createParentFolder (_dstPath); // create folder structure first

	NativePath dstPath (_dstPath);
	NativePath srcPath (_srcPath);

	bool overwrite = (mode & kDoNotOverwrite) == 0;
	bool writeEnable = (mode & kDisableWriteProtection) != 0;

	if(writeEnable && overwrite) // try to write-enable old destination file if it exists
		WriteEnableFile (dstPath);

	DWORD flags = 0;
	if(overwrite == false)
		flags |= COPY_FILE_FAIL_IF_EXISTS;

	ProgressNotifyScope scope (progress);
	BOOL canceled = FALSE;
	BOOL result = ::CopyFileEx (srcPath, dstPath, CCLCopyProgressRoutine, reinterpret_cast<void*> (progress), &canceled, flags);
	if(!result)
	{
		int lastError = ::GetLastError ();
		onNativeError (lastError, &_dstPath);
	}

	if(result && writeEnable) // write-enable new destination file, could be copied from read-only media
		WriteEnableFile (dstPath);

	return (tbool) result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsNativeFileSystem::getWorkingDirectory (IUrl& url)
{
	NativePath path;
	if(!::GetCurrentDirectory (path.size (), path))
		return false;
	return url.fromNativePath (path, IUrl::kFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsNativeFileSystem::setWorkingDirectory (UrlRef url)
{
	NativePath path (url);
	return (tbool)::SetCurrentDirectory (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsNativeFileSystem::isCaseSensitive ()
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int WindowsNativeFileSystem::translateNativeError (int nativeError)
{
	switch(nativeError & 0x7FFF)
	{
		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:		return kFileNotFound;
		case ERROR_INVALID_ACCESS:
		case ERROR_ACCESS_DENIED:		return kAccesDenied;
		case ERROR_PATH_BUSY:
		case ERROR_BUSY:
		case ERROR_SHARING_VIOLATION:	return kFileInUse;
		case ERROR_ALREADY_EXISTS:
		case ERROR_FILE_EXISTS:			return kFileExists;
		//case ENOTDIR:					return kNotDirectory;
		//case EISDIR:					return kIsDirectory;

		case ERROR_NOACCESS:			// Invalid access to memory location.
		case ERROR_BAD_ARGUMENTS:
		case ERROR_INVALID_NAME:		return kInvalidArgument;
		case ERROR_TOO_MANY_OPEN_FILES:	return kTooManyOpenFiles;
		case ERROR_DISK_FULL:			return kOutOfDiscSpace;
		case ERROR_DIR_NOT_EMPTY:       return kDirNotEmpty;

		default:
			//CCL_DEBUGGER ("Error not translated!")
			return kUnknownError;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsNativeFileSystem::beginTransaction ()
{
	if(getTransaction ())
		return false;

	DeferredFileOperation* fileOp = DeferredFileOperation::create ();
	if(fileOp)
	{
		setTransaction (fileOp);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsNativeFileSystem::endTransaction (int mode, CCL::IProgressNotify* progress)
{
	DeferredFileOperation* op = (DeferredFileOperation*)getTransaction ();
	if(op)
	{
		if(mode != kCancelTransaction)
			op->perform (mode == kCommitTransactionWithUndo, progress);

		setTransaction (nullptr);
		return true;
	}
	return false;
}

//************************************************************************************************
// WindowsFileStream
//************************************************************************************************

WindowsFileStream::~WindowsFileStream ()
{
	if(file != INVALID_HANDLE_VALUE)
		::CloseHandle (file);

	file = INVALID_HANDLE_VALUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API WindowsFileStream::read (void* buffer, int size)
{
	if(size == 0)
		return 0;

	// If the file is open in non buffered read mode, the alignment must be enforced on Windows
	ASSERT(((options & kReadNonBuffered) == 0) || (((IntPtr)buffer & 0x1FF) == 0))

	DWORD numRead = 0;
	BOOL result = ::ReadFile (file, buffer, size, &numRead, nullptr);
	if(result == FALSE)
	{
		int error = ::GetLastError ();
		ASSERT(error != ERROR_INVALID_PARAMETER)
		onNativeError (error);
	}
	return numRead;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API WindowsFileStream::write (const void* buffer, int size)
{
	if(size == 0)
		return 0;

	DWORD numWritten = 0;
	::WriteFile (file, buffer, size, &numWritten, nullptr);
	if(numWritten <= 0)
	{
		int error = ::GetLastError ();
		onNativeError (error);
	}
	else if(options & kWriteFlushed)
		FlushFileBuffers (file);
	return numWritten;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API WindowsFileStream::seek (int64 pos, int mode)
{
	LARGE_INTEGER liPos, liNewPos;
	liPos.QuadPart = pos;
	liNewPos.QuadPart = 0;
	::SetFilePointerEx (file, liPos, &liNewPos, mode);
	return liNewPos.QuadPart;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API WindowsFileStream::tell ()
{
	return seek (0, kSeekCur);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsFileStream::getPath (IUrl& path)
{
	return path.fromNativePath (this->path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowsFileStream::setOptions (int _options)
{
	if((options & kOptionBits) != _options)
	{
		bool reopen = false;
		if(((options & kOptionBits) & ~kWriteFlushed) != ((_options & kOptionBits) & ~kWriteFlushed))
			reopen = true;

		options = (options & ~kOptionBits) | _options;

		if(reopen)
		{
			DWORD access, sharing, flags;
			TranslateMode (options, access, sharing, flags);

			/*if(0)
			{
				file = ::ReOpenFile (file, access, sharing, flags & ~FILE_ATTRIBUTE_NORMAL);
				if(file == INVALID_HANDLE_VALUE)
				{
					int lastError = ::GetLastError ();
					fileSystem->onNativeError (lastError, 0);
				}
			}
			else*/
			{
				HANDLE newHandle = ::CreateFile (path, access, sharing, nullptr, OPEN_EXISTING, flags, nullptr);
				if(newHandle != INVALID_HANDLE_VALUE)
				{
					::CloseHandle (file);
					file = newHandle;
				}
				else
				{
					int lastError = ::GetLastError ();
					onNativeError (lastError, nullptr);

					// Reopen only works with sharing mode set on the original handle
					ASSERT (lastError != ERROR_SHARING_VIOLATION)
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowsFileStream::setEndOfFile (int64 eof)
{
	bool result = true;
	int64 oldPos = tell ();
	seek (eof, kSeekSet);
	if(SetEndOfFile (file) == false)
	{
		int lastError = ::GetLastError ();
		onNativeError (lastError, nullptr);
		result = false;
	}
	if(oldPos < eof)
		seek (oldPos, kSeekSet);
	return result;
}

//************************************************************************************************
// WindowsFileIterator
//************************************************************************************************

WindowsFileIterator::WindowsFileIterator (UrlRef url, int mode)
: NativeFileIterator (url, mode)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsFileIterator::~WindowsFileIterator ()
{
	// happens if iterator does not run over all files
	if(iter && iter != INVALID_HANDLE_VALUE)
		::FindClose (iter);
	iter = INVALID_HANDLE_VALUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IUrl* CCL_API WindowsFileIterator::next ()
{
	if(iter == INVALID_HANDLE_VALUE)
		return nullptr;

	ErrorModeInitializer errorModeInitializer; // suppress open file error box

	WIN32_FIND_DATA found;
	bool isFolder = false;
	bool done;
	do
	{
		done = true;

		if(!iter) // first time
		{
			NativePath path (*baseUrl);
			::wcscat (path.path, L"\\*.*");
			iter = ::FindFirstFile (path, &found);
		}
		else
		{
			BOOL result = ::FindNextFile (iter, &found);
			if(!result) // finished
			{
				::FindClose (iter);
				iter = INVALID_HANDLE_VALUE;
			}
		}

		if(iter != INVALID_HANDLE_VALUE)
		{
			bool wantFolders = (mode & kFolders) != 0;
			bool wantFiles = (mode & kFiles) != 0;
			bool wantHidden = (mode & kIgnoreHidden) == 0;

			isFolder = (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			bool hidden = (found.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;

			if(!::wcscmp (found.cFileName, L".") || !::wcscmp (found.cFileName, L".."))
				done = false;
			else if((isFolder && !wantFolders) || (!isFolder && !wantFiles) || (hidden && !wantHidden))
				done = false;
		}
	} while(!done);

	if(iter != INVALID_HANDLE_VALUE)
	{
		current->assign (*baseUrl);
		current->descend (String (found.cFileName), isFolder ? Url::kFolder : Url::kFile);
		return current;
	}
	return nullptr;
}

//************************************************************************************************
// WindowsVolumesIterator
//************************************************************************************************

WindowsVolumesIterator::WindowsVolumesIterator ()
{
	WCHAR buffer[MAX_PATH] = {0};
	::GetLogicalDriveStrings (MAX_PATH, buffer);
	// returns a string like "c:\<null>d:\<null><null>"

	WCHAR* ptr = buffer;
	while(*ptr)
	{
		if(ptr[0] >= 'A')
		{
			Url* path = NEW Url;
			path->fromNativePath (ptr, Url::kFolder);
			volumes->add (path);
		}

		ptr += ::wcslen (ptr) + 1;
	}

	construct ();
}

//************************************************************************************************
// DeferredFileOperation
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DeferredFileOperation, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

DeferredFileOperation* DeferredFileOperation::create ()
{
	return NEW DeferredFileOperation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DeferredFileOperation::DeferredFileOperation ()
: progress (nullptr)
{
	::CoCreateInstance (CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_IFileOperation, fileOp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::tresult CCL_API DeferredFileOperation::queryInterface (CCL::UIDRef iid, void** ptr)
{
	QUERY_COM_INTERFACE (::IFileOperationProgressSink)
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool DeferredFileOperation::removeFile (UrlRef url)
{
	if(fileOp)
	{
		NativePath path (url);
		Win32::ComPtr<IShellItem> item;
		::SHCreateItemFromParsingName (path, nullptr, IID_IShellItem, item);
		if(item)
		{
			fileOp->DeleteItem (item, nullptr);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool DeferredFileOperation::perform (tbool withUndo, CCL::IProgressNotify* _progress)
{
	if(fileOp)
	{
		DWORD opFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
		if(withUndo)
			opFlags |= FOF_ALLOWUNDO;
		fileOp->SetOperationFlags (opFlags);

		progress = _progress;

		DWORD opCookie = 0;
		fileOp->Advise (this, &opCookie);
		fileOp->PerformOperations ();
		fileOp->Unadvise (opCookie);

		progress = nullptr;
		fileOp.release ();

		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE DeferredFileOperation::UpdateProgress (UINT iWorkTotal, UINT iWorkSoFar)
{
	if(progress && iWorkTotal > 0 && iWorkSoFar > 0)
	{
		double progressValue = double (iWorkSoFar) / double (iWorkTotal);
		progress->updateProgress (progressValue);
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE DeferredFileOperation::StartOperations ()
{
	if(progress)
	{
		progress->beginProgress ();
		progress->updateProgress (0.0);
	}
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE DeferredFileOperation::FinishOperations (/* [in] */ HRESULT hrResult)
{
	if(progress)
		progress->endProgress ();
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE DeferredFileOperation::PreDeleteItem (DWORD dwFlags, __RPC__in_opt IShellItem *psiItem)
{
	if(progress && progress->isCanceled ())
		return E_ABORT;

	return S_OK;
}

