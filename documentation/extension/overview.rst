.. include:: reference/metainfo-classmodel.ref.rst
.. include:: reference/installdata-classmodel.ref.rst

########
Overview
########

===========================
Deployment and Installation
===========================

Extensions are deployed as self-contained installation packages which are basically digitally signed ZIP archives with the *.install* file name extension and a predefined file and folder structure inside the archive. Installation is done by the CCL-based host application.

The application verifies the digital signature and extracts all files to the host-specific extension location. To uninstall an extension, the host removes the extension's sub folder recursively. No further installation steps are supported.


==============================
Installation Package Structure
==============================

An installable extension package is composed of the actual extension content alongside a package information *metainfo.xml*, an installation manifest *installdata.xml* file, and an extension icon *package.iconset*. The *metainfo.xml* defines the package identifier and display information, the *installdata.xml* file defines installation process instructions like the expected host application identifier and version.

The content of the *metainfo.xml* and *installdata.xml* files look like this:

.. code-block:: xml
  :caption: metainfo.xml

  <?xml version="1.0" encoding="UTF-8"?>
  <MetaInformation>
    <Attribute id="Package:ID" value="com.vendor.package.{platform}"/>
    <Attribute id="Package:Name" value="Demo Package"/>
    <Attribute id="Package:Description" value="Demo Package Description"/>
    <Attribute id="Package:Version" value="1.0.0.0"/>
    <Attribute id="Package:Vendor" value="Demo Vendor"/>
    <Attribute id="Package:Website" value="https://www.vendor.com"/>
  </MetaInformation>

.. code-block:: XML
  :caption: installdata.xml

  <?xml version="1.0" encoding="utf-8"?>
  <InstallManifest>
    <InstallFile>
      <RequiredApp id="demoapp.*" minVersion="1.0.0.0" maxVersion="*"/> 
    </InstallFile>
  </InstallManifest>


See the :ref:`reference section<extension_xml_reference>` for an XML format specification.

The extension icon should be provided in multiple sizes and for 100% (1x) and 200% (2x) DPI scaling. A .iconset file is a ZIP file with multiple PNG images inside:

.. code-block:: rst
  :caption: package.iconset

  icon_48x48.png
  icon_48x48@2x.png
  icon_128x128.png
  icon_128x128@2x.png
  icon_256x256.png
  icon_256x256@2x.png
  icon_512x512.png
  icon_512x512@2x.png

======================
Extension Code Signing
======================

Extensions need to be digitally signed by the extension vendor. This can be done with the CCL CLI Tools publicly available at `ccl.dev <https://ccl.dev>`_.

Use the following command line for signing:

.. code-block:: rst

		cclcrypt -vendorsign [OUTPUT_FILE] [INPUT_FILE] [DEVELOPER_TOKEN_FILE]

Conceptually, extension signing is based on standard RSA public key cryptography. The developer token is a JWT (`JSON Web Token <https://jwt.io/>`_) typically stored in a .license file that holds all information needed for signing, including your official vendor name, your private RSA key, and a wrapped version of your public RSA key counter-signed by the host application vendor.

These steps are required to create a new developer token:

1. Create a new RSA key pair using

  .. code-block:: rst

    cclcrypt -generate [OUTPUT_FOLDER] [KEY_NAME]

  This will create two files in the output folder "[KEY_NAME].privatekey" with the private RSA key and "[KEY_NAME].publickey" with the public RSA key. Make sure the private key is stored safely and never leaves your secure environment.

2. Send the "[KEY_NAME].publickey" file to the host application vendor for counter-signing. You will receive your public vendor token (in JWT format) in return. It contains your official vendor name as agreed upon with the host vendor. 

  .. note::

    Please use the exact same spelling of the vendor name in the extension *metainfo.xml* file, otherwise signature verification will fail.

3. Merge the counter-signed public vendor token and your private RSA key to a fully-qualified developer token using

  .. code-block:: rst

    cclcrypt -create-private-token [OUTPUT_FILE] [PRIVATE_VENDOR_KEY_FILE] [PUBLIC_VENDOR_TOKEN_FILE]

  Make sure the developer token is stored safely and never leaves your secure environment.

