Use as root element in the extension *metainfo.xml* file to parent package attribute defining elements.

.. code-block:: xml
   :caption: metainfo.xml example

   <?xml version="1.0" encoding="UTF-8"?>
   <MetaInformation>
     <Attribute id="Package:ID" value="com.vendor.package.{platform}"/>
     <Attribute id="Package:Name" value="Demo Package"/>
     <Attribute id="Package:Description" value="Demo Package Description"/>
     <Attribute id="Package:Version" value="1.0.0.0"/>
     <Attribute id="Package:Vendor" value="Demo Vendor"/>
     <Attribute id="Package:Website" value="https://www.vendor.com"/>
   </MetaInformation>

