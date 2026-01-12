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
// Filename    : ccl/public/systemservices.h
// Description : System Service APIs
//
//************************************************************************************************

#ifndef _ccl_systemservices_h
#define _ccl_systemservices_h

#include "ccl/public/cclexports.h"
#include "ccl/public/system/ithreading.h"
#include "ccl/public/system/atomic.h"

namespace CCL {

interface IAllocator;
interface IAtomTable;
interface ISignalHandler;
interface IErrorHandler;
interface INativeFileSystem;
interface ISystemInformation;
interface IExecutableLoader;
interface IFileManager;
interface IFileUtilities;
interface IFileTypeRegistry;
interface IFileSystemSecurityStore;
interface IPackageHandler;
interface ILocaleManager;
interface IMediaThreadService;
interface ISafetyManager;
interface IDiagnosticStore;
interface IAnalyticsManager;

namespace Threading {
interface ILockable; 
interface IThreadPool; 
interface ISharedMemory;
interface ISemaphore;
interface INamedPipe; 
interface IMultiWorker; }

namespace System {

interface IConsole;
interface ILogger;

/** \addtogroup ccl_system
@{ */

//////////////////////////////////////////////////////////////////////////////////////////////////
// Debugging APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Print C-String to debug output. */
CCL_EXPORT void CCL_API CCL_ISOLATED (DebugPrintCString) (CStringPtr string);
inline void DebugPrintCString (CStringPtr string) { CCL_ISOLATED (DebugPrintCString) (string); }

/** Print Unicode String to debug output. */
CCL_EXPORT void CCL_API CCL_ISOLATED (DebugPrintString) (StringRef string);
inline void DebugPrintString (StringRef string) { CCL_ISOLATED (DebugPrintString) (string); }

/** Report debug warning (release build). */
CCL_EXPORT void CCL_API CCL_ISOLATED (DebugReportWarning) (ModuleRef module, StringRef message);
inline void DebugReportWarning (ModuleRef module, StringRef message) { CCL_ISOLATED (DebugReportWarning) (module, message); }

/** Throw breakpoint exception if debugger present. */
CCL_EXPORT void CCL_API CCL_ISOLATED (DebugBreakPoint) ();
inline void DebugBreakPoint () { CCL_ISOLATED (DebugBreakPoint) (); }

/** Terminate execution if debugger present. */
CCL_EXPORT void CCL_API CCL_ISOLATED (DebugExitProcess) (int exitCode);
inline void DebugExitProcess (int exitCode) { CCL_ISOLATED (DebugExitProcess) (exitCode); }

/** Returns the fastest counter in seconds. */
CCL_EXPORT double CCL_API CCL_ISOLATED (GetProfileTime) ();
inline double GetProfileTime () { return CCL_ISOLATED (GetProfileTime) (); }

/** Get system up-time in milliseconds. */
CCL_EXPORT int64 CCL_API CCL_ISOLATED (GetSystemTicks) ();
inline int64 GetSystemTicks () { return CCL_ISOLATED (GetSystemTicks) (); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// Threading APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Thread description for CreateNativeThread(). */
struct ThreadDescription
{
	Threading::ThreadFunction function = nullptr;
	CStringPtr name = nullptr;
	void* arg = nullptr;
};

/** Create new thread object executing the specified function. */
CCL_EXPORT Threading::IThread* CCL_API CCL_ISOLATED (CreateNativeThread) (const ThreadDescription& description);
inline Threading::IThread* CreateNativeThread (const ThreadDescription& description) { return CCL_ISOLATED (CreateNativeThread) (description); }

/** Create new synchronization object. */
CCL_EXPORT Threading::ISyncPrimitive* CCL_API CCL_ISOLATED (CreateSyncPrimitive) (UIDRef cid) ;
inline Threading::ISyncPrimitive* CreateSyncPrimitive (UIDRef cid) { return CCL_ISOLATED (CreateSyncPrimitive) (cid); }

/** Create high-level synchronization object. */
CCL_EXPORT Threading::ILockable* CCL_API CCL_ISOLATED (CreateAdvancedLock) (UIDRef cid);
inline Threading::ILockable* CreateAdvancedLock (UIDRef cid) { return CCL_ISOLATED (CreateAdvancedLock) (cid); }

/** Create new atomic stack. */
CCL_EXPORT Threading::IAtomicStack* CCL_API CCL_ISOLATED (CreateAtomicStack) ();
inline Threading::IAtomicStack* CreateAtomicStack () { return CCL_ISOLATED (CreateAtomicStack) (); }

/** Suspend execution of current thread for specified interval. */
CCL_EXPORT void CCL_API CCL_ISOLATED (ThreadSleep) (uint32 milliseconds);
inline void ThreadSleep (uint32 milliseconds) { CCL_ISOLATED (ThreadSleep) (milliseconds); }

/** Get current thread instance, must be released by caller! */
CCL_EXPORT Threading::IThread* CCL_API CCL_ISOLATED (CreateThreadSelf) ();
inline Threading::IThread* CreateThreadSelf () { return CCL_ISOLATED (CreateThreadSelf) (); }

/** Get current thread identifier. */
CCL_EXPORT Threading::ThreadID CCL_API CCL_ISOLATED (GetThreadSelfID) ();
inline Threading::ThreadID GetThreadSelfID () { return CCL_ISOLATED (GetThreadSelfID) (); }

/** Get the main thread. */
CCL_EXPORT Threading::IThread& CCL_API CCL_ISOLATED (GetMainThread) ();
inline Threading::IThread& GetMainThread () { return CCL_ISOLATED (GetMainThread) (); }

/** Make calling thread the main thread. Use inside service only. */
CCL_EXPORT void CCL_API CCL_ISOLATED (SwitchMainThread) ();
inline void SwitchMainThread () { CCL_ISOLATED (SwitchMainThread) (); }

/** Check if calling thread is the main thread. */
inline bool IsInMainThread ()
{ return GetThreadSelfID () == GetMainThread ().getThreadID (); }

/** Get thread instance with identifier, must be released by caller! */
CCL_EXPORT Threading::IThread* CCL_API CCL_ISOLATED (CreateThreadWithIdentifier) (Threading::ThreadID threadId);
inline Threading::IThread* CreateThreadWithIdentifier (Threading::ThreadID threadId) { return CCL_ISOLATED (CreateThreadWithIdentifier) (threadId); }

/** Get thread pool singleton. */
CCL_EXPORT Threading::IThreadPool& CCL_ISOLATED (GetThreadPool) ();
inline Threading::IThreadPool& GetThreadPool () { return CCL_ISOLATED (GetThreadPool) (); }

/** Thread pool description for CreateThreadPool(). */
struct ThreadPoolDescription
{
	int maxThreadCount = 1;
	Threading::ThreadPriority priority = Threading::kPriorityBelowNormal;
	CStringPtr name = nullptr;
	int idleTimeout = -1;
};

/** Create new thread pool. */
CCL_EXPORT Threading::IThreadPool* CCL_ISOLATED (CreateThreadPool) (const ThreadPoolDescription& description);
inline Threading::IThreadPool* CreateThreadPool (const ThreadPoolDescription& description) { return CCL_ISOLATED (CreateThreadPool) (description); }

/** Get Multimedia thread service singleton. */
CCL_EXPORT IMediaThreadService& CCL_API CCL_ISOLATED (GetMediaThreadService) ();
inline IMediaThreadService& GetMediaThreadService () { return CCL_ISOLATED (GetMediaThreadService) (); }

/** Multi-threading worker description for CreateMultiThreadWorker(). */
struct MultiThreadWorkerDescription
{
	int numberOfCPUs = 1;
	int cpuOffset = 0;
	Threading::ThreadPriority priority = Threading::kPriorityHigh;
	tbool useCpuAffinity = false;
	CStringPtr name = nullptr;
	Threading::WorkgroupID workgroup = nullptr;
};

/** Create new multi-threading worker.*/
CCL_EXPORT Threading::IMultiWorker* CCL_ISOLATED (CreateMultiThreadWorker) (const MultiThreadWorkerDescription& description);
inline Threading::IMultiWorker* CreateMultiThreadWorker (const MultiThreadWorkerDescription& description) { return CCL_ISOLATED (CreateMultiThreadWorker) (description); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// Process and Interprocess Communication APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Get identifier of current process. */
CCL_EXPORT Threading::ProcessID CCL_API CCL_ISOLATED (GetProcessSelfID) ();
inline Threading::ProcessID GetProcessSelfID () { return CCL_ISOLATED (GetProcessSelfID) (); }

/** Create new shared memory object. */
CCL_EXPORT Threading::ISharedMemory* CCL_API CCL_ISOLATED (CreateIPCSharedMemory) ();
inline Threading::ISharedMemory* CreateIPCSharedMemory () { return CCL_ISOLATED (CreateIPCSharedMemory) (); }

/** Create new interprocess semaphore object. */
CCL_EXPORT Threading::ISemaphore* CCL_API CCL_ISOLATED (CreateIPCSemaphore) ();
inline Threading::ISemaphore* CreateIPCSemaphore () { return CCL_ISOLATED (CreateIPCSemaphore) (); }

/** Create new interprocess pipe. */
CCL_EXPORT Threading::INamedPipe* CCL_API CCL_ISOLATED (CreateIPCPipe) ();
inline Threading::INamedPipe* CreateIPCPipe () { return CCL_ISOLATED (CreateIPCPipe) (); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// Spin Lock APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Try to lock spin lock. */
CCL_EXPORT tbool CCL_API CCL_ISOLATED (SpinLockTryLock) (int32 volatile& lock);
inline tbool SpinLockTryLock (int32 volatile& lock) { return CCL_ISOLATED (SpinLockTryLock) (lock); }

/** Lock spin lock. */
CCL_EXPORT void CCL_API CCL_ISOLATED (SpinLockLock) (int32 volatile& lock);
inline void SpinLockLock (int32 volatile& lock) { CCL_ISOLATED (SpinLockLock) (lock); }

/** Unlock spin lock. */
CCL_EXPORT void CCL_API CCL_ISOLATED (SpinLockUnlock) (int32 volatile& lock);
inline void SpinLockUnlock (int32 volatile& lock) { CCL_ISOLATED (SpinLockUnlock) (lock); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// Thread Local Storage APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Create new TLS slot. */
CCL_EXPORT Threading::TLSRef CCL_API CCL_ISOLATED (CreateThreadLocalSlot) (Threading::ThreadLocalDestructor destructor);
inline Threading::TLSRef CreateThreadLocalSlot (Threading::ThreadLocalDestructor destructor) { return CCL_ISOLATED (CreateThreadLocalSlot) (destructor); }

/** Destroy TLS slot. */
CCL_EXPORT tbool CCL_API CCL_ISOLATED (DestroyThreadLocalSlot) (Threading::TLSRef slot);
inline tbool DestroyThreadLocalSlot (Threading::TLSRef slot) { return CCL_ISOLATED (DestroyThreadLocalSlot) (slot); }

/** Set per-thread data for TLS slot. */
CCL_EXPORT tbool CCL_API CCL_ISOLATED (SetThreadLocalData) (Threading::TLSRef slot, void* data);
inline tbool SetThreadLocalData (Threading::TLSRef slot, void* data) { return CCL_ISOLATED (SetThreadLocalData) (slot, data); }

/** Get per-thread data from TLS slot. */
CCL_EXPORT void* CCL_API CCL_ISOLATED (GetThreadLocalData) (Threading::TLSRef slot);
inline void* GetThreadLocalData (Threading::TLSRef slot) { return CCL_ISOLATED (GetThreadLocalData) (slot); }

/** Cleanup on thread exit. Only needed for threads spawned by the OS and not created with CreateNativeThread(). */
CCL_EXPORT void CCL_API CCL_ISOLATED (CleanupThreadLocalStorage) ();
inline void CleanupThreadLocalStorage () { CCL_ISOLATED (CleanupThreadLocalStorage) (); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Management APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Hash byte array. */
CCL_EXPORT uint32 CCL_API CCL_ISOLATED (Hash) (const void* key, uint32 length, uint32 initialValue);
inline uint32 Hash (const void* key, uint32 length, uint32 initialValue) { return CCL_ISOLATED (Hash) (key, length, initialValue); }

/** Get file system singleton. */
CCL_EXPORT INativeFileSystem& CCL_API CCL_ISOLATED (GetFileSystem) ();
inline INativeFileSystem& GetFileSystem () { return CCL_ISOLATED (GetFileSystem) (); }

/** Get file manager singleton. */
CCL_EXPORT IFileManager& CCL_API CCL_ISOLATED (GetFileManager) ();
inline IFileManager& GetFileManager () { return CCL_ISOLATED (GetFileManager) (); }

/** Get file utilities singleton. */
CCL_EXPORT IFileUtilities& CCL_API CCL_ISOLATED (GetFileUtilities) ();
inline IFileUtilities& GetFileUtilities () { return CCL_ISOLATED (GetFileUtilities) (); }

/** Get file type registry singleton. */
CCL_EXPORT IFileTypeRegistry& CCL_API CCL_ISOLATED (GetFileTypeRegistry) ();
inline IFileTypeRegistry& GetFileTypeRegistry () { return CCL_ISOLATED (GetFileTypeRegistry) (); }

/** Get file system security store singleton. */
CCL_EXPORT IFileSystemSecurityStore& CCL_API CCL_ISOLATED (GetFileSystemSecurityStore) ();
inline IFileSystemSecurityStore& GetFileSystemSecurityStore () { return CCL_ISOLATED (GetFileSystemSecurityStore) (); }

/** Get package handler singleton. */
CCL_EXPORT IPackageHandler& CCL_API CCL_ISOLATED (GetPackageHandler) ();
inline IPackageHandler& GetPackageHandler () { return CCL_ISOLATED (GetPackageHandler) (); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// Module Management APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

/**	Get native module reference of main application.
	Windows: HINSTANCE
	macOS/iOS: CFBundleRef
	Linux/Android: dlopen handle */
CCL_EXPORT ModuleRef CCL_API CCL_ISOLATED (GetMainModuleRef) (); 
inline ModuleRef GetMainModuleRef () { return CCL_ISOLATED (GetMainModuleRef) (); }

/** Get native reference of calling module. 
	Note: This function is implemented locally, i.e. it's not exported! */
ModuleRef GetCurrentModuleRef ();

/** Check if calling module is the main module. */
inline bool IsInMainModule ()
{ return GetCurrentModuleRef () == GetMainModuleRef (); }

/** Get identifier with module reference. */
CCL_EXPORT StringRef CCL_API CCL_ISOLATED (GetModuleIdentifier) (String& id, ModuleRef module);
inline StringRef GetModuleIdentifier (String& id, ModuleRef module) { return CCL_ISOLATED (GetModuleIdentifier) (id, module); }

/** Get module reference with identifier. */
CCL_EXPORT ModuleRef CCL_API CCL_ISOLATED (GetModuleWithIdentifier) (StringRef id);
inline ModuleRef GetModuleWithIdentifier (StringRef id) { return CCL_ISOLATED (GetModuleWithIdentifier) (id); }

/** Get executable loader singleton. */
CCL_EXPORT IExecutableLoader& CCL_API CCL_ISOLATED (GetExecutableLoader) ();
inline IExecutableLoader& GetExecutableLoader () { return CCL_ISOLATED (GetExecutableLoader) (); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// Misc. APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Generate new 16 byte unique identifier. */
CCL_EXPORT tresult CCL_API CCL_ISOLATED (CreateUID) (UIDBytes& uid);
inline tresult CreateUID (UIDBytes& uid) { return CCL_ISOLATED (CreateUID) (uid); }

/** Get system information singleton. */
CCL_EXPORT ISystemInformation& CCL_API CCL_ISOLATED (GetSystem) ();
inline ISystemInformation& GetSystem () { return CCL_ISOLATED (GetSystem) (); }

/** Get locale manager singleton. */
CCL_EXPORT ILocaleManager& CCL_API CCL_ISOLATED (GetLocaleManager) ();
inline ILocaleManager& GetLocaleManager () { return CCL_ISOLATED (GetLocaleManager) (); }

/** Get memory allocator singleton. */
CCL_EXPORT IAllocator& CCL_API CCL_ISOLATED (GetMemoryAllocator) ();
inline IAllocator& GetMemoryAllocator () { return CCL_ISOLATED (GetMemoryAllocator) (); }

/** Lock virtual memory into physical memory. */
CCL_EXPORT void CCL_API CCL_ISOLATED (LockMemory) (tbool state, void* address, int size);
inline void LockMemory (tbool state, void* address, int size) { return CCL_ISOLATED (LockMemory) (state, address, size); }

/** Get atom table singleton. */
CCL_EXPORT IAtomTable& CCL_API CCL_ISOLATED (GetAtomTable) ();
inline IAtomTable& GetAtomTable () { return CCL_ISOLATED (GetAtomTable) (); }

/** Get signal handler singleton. */
CCL_EXPORT ISignalHandler& CCL_API CCL_ISOLATED (GetSignalHandler) ();
inline ISignalHandler& GetSignalHandler () { return CCL_ISOLATED (GetSignalHandler) (); }

/** Get error handler singleton. */
CCL_EXPORT IErrorHandler& CCL_API CCL_ISOLATED (GetErrorHandler) ();
inline IErrorHandler& GetErrorHandler () { return CCL_ISOLATED (GetErrorHandler) (); }

/** Get console singleton for character-mode applications. */
CCL_EXPORT IConsole& CCL_API CCL_ISOLATED (GetConsole) ();
inline IConsole& GetConsole () { return CCL_ISOLATED (GetConsole) (); }

/** Get logger singleton. */
CCL_EXPORT ILogger& CCL_API CCL_ISOLATED (GetLogger) ();
inline ILogger& GetLogger () { return CCL_ISOLATED (GetLogger) (); }

/** Get safety manager singleton. */
CCL_EXPORT ISafetyManager& CCL_API CCL_ISOLATED (GetSafetyManager) ();
inline ISafetyManager& GetSafetyManager () { return CCL_ISOLATED (GetSafetyManager) (); }

/** Get diagnostic store singleton. */
CCL_EXPORT IDiagnosticStore& CCL_API CCL_ISOLATED (GetDiagnosticStore) ();
inline IDiagnosticStore& GetDiagnosticStore () { return CCL_ISOLATED (GetDiagnosticStore) (); }

/** Get analytics manager singleton. */
CCL_EXPORT IAnalyticsManager& CCL_API CCL_ISOLATED (GetAnalyticsManager) ();
inline IAnalyticsManager& GetAnalyticsManager () { return CCL_ISOLATED (GetAnalyticsManager) (); }

//////////////////////////////////////////////////////////////////////////////////////////////////
/** @}*/

#if CCL_STATIC_LINKAGE
/** System Framework Initialization. */
tbool CCL_API InitializeSystemFramework (tbool state);
#endif

} // namespace System
} // namespace CCL

#endif // _ccl_systemservices_h
