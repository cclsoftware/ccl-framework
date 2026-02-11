.. code-block:: xml
   :caption: <InstallFile> element example

   <?xml version="1.0" encoding="utf-8"?>
   <InstallManifest>
      <InstallFile>
         <ExcludedApp id="someapp.demo"/>
         <RequiredApp id="someapp.*" minVersion="1.5.0.0" maxVersion="*"/> 
      </InstallFile>	
   </InstallManifest>
   