.. _virtual-file-system:

###################
Virtual File System
###################

The so called "Virtual File System" unifies addressing files and folders from various sources via urls of different protocols. All protocols can be used with e.g. methods of the :cref:`CCL::INativeFileSystem` interface, aquired via ``System::GetFileSystem ()``.
   
.. list-table:: Overview of filesystem protocols
   :header-rows: 1

   * - Protocol
     - FileSystem class
     - Remarks
   * - file
     - NativeFS   
     - access to local files ("C:\Windows" becomes "file:///c:/windows")
   * - resource
     - SimpleFS
     - "resource://modulename/..."
   * -
     -
     - note: folders are supported, but can't be iterated on all platforms! 
   * - local
     - SimpleFS
     - allows "local://$symbol/..." 
   * - 
     - 
     - hostname can be SYSTEM, PROGRAMS, TEMP, USERDOCS, USERCONTENT, USERMUSIC, USERSETTINGS, APPSUPPORT, DEPLOYMENT, APPSETTINGS, DESKTOP
   * - 
     - 
     - see System::GetSystem ().resolveLocation ()
   * - preset
     - SimpleFS
     - access to memory presets (unused yet)
   * - class
     - (none)
     - used internally between PluginBrowser and PluginFileInfo "class://{xxxx-xxxx...}"
   * - package
     - VolumeFS
     - Package Handler System::GetPackageHandler ()
   * - memory
     - SimpleFS
     - Memory-based file system; does not support folders in the path below the hostname part, e.g. "memory://Secret/data.bin"
   * - webfs
     - VolumeFS
     - Web File Service System::GetWebFileService ()

