.. include:: extlinks.rst

############################
Plug-ins on Mobile Platforms
############################

=======
Android
=======

Dynamic libraries reside as .so files in the 

.. code-block:: rst

  lib/<architecture>/ 
  
folder inside of an APK. They can be loaded from there using *dlopen*. The virtual resource file system presented to the application, however, is restricted to the *assets/* folder.

Thus our plugin manager cannot scan the available .so files directly. Instead, we place .plugin files for each plugin in

.. code-block:: rst

 assets/Plugins/
 
for the plugin manager to scan and load the corresponding .so.

As Linux/Android does not really provide a way of bundling shared objects with resources, we had to come up with our own solution for this. Resources for a specific plugin are placed in

.. code-block:: rst

  assets/Plugins/<pluginid>/ 
  
The resource file system handler implemented in *CCL::AndroidResourceFileSystem* transparently redirects file open requests to the respective plugin folder.

For a plugin named *myplugin* with two resource files, the resulting file layout in the APK would look like this:

.. code-block:: rst
  :caption: APK

  assets/
    Plugins/
      myplugin/
        default.skin
        logo.png
      myplugin.plugin
  lib/
    arm64-v8a/
      libmyplugin.so

Native Android code often needs to interface with Java code using the `Java Native Interface`_ (JNI). In this case, the .java files need to be compiled into the main application's classes.dex file. This can be achieved using a plugin-specific .vcxitems file that references the .java sources and is itself referenced by the application's packaging project.

An example for this is the *Android Audio* service residing in

.. code-block:: rst

  services/devices/androidaudio/

and it's integration into the *Notion Mobile* and *Capture Remote Android* projects. Please use these projects as references for future developments.

===
iOS
===

Loading of dynamic libraries is supported on iOS 8 and later. To combine resources with the machine code binary we use framework bundles, with extension ".framework" which are installed into the "Frameworks" directory within the app bundle.

While using .bundle packages in the "PlugIns" directory is an option when running on an iOS device in debug mode locally, the Apple app store currently (2019) rejects any binary inside an .ipa bundle, which does not reside in a framework and is placed in the "Frameworks" directory.
Therfore, like for Android, the CCL framework instead scans placeholder files with a .plugin extension at the "PlugIns" location in the application bundle and loads the corresponding bundle from the "Frameworks" directory.

Contrary to bundles, frameworks are not recognized as packages by the file system, i.e. they are treated as directories, not single files.

The Xcode projects for our iOS plugins are losely based on the Xcode project template for "Cocoa Touch Framework". 

