####################
Authorization Policy
####################

============
Introduction
============

The authorization policy used by the CCL **security framework** is a way of unlocking a set of features at runtime based on the application identity. The application identity can either be hardcoded or initialized from a .license file. Certain shared services and plug-ins require an authorization policy to be present in the host application.

The policy can be written in XML or JSON. The format is inspired by access control lists (https://en.wikipedia.org/wiki/Access_control_list). The policy is digitally signed and stored encrypted in the application code. Each application needs its own secret key for the encryption and a new public/private key pair for signing.

The application-wide policy is managed by the **IAuthorizationManager** singleton in CCL. There is a simplified implementation available in corelib for embedded use.

=====
Setup
=====

The following steps are required to set up a new authorization policy:

* Setup folder structure following our naming convention:

  * Add a "security" folder to the application for XML files and scripts
  * Add a "source/bincpp" folder for generated source files
  * Add a new appsecurity.cpp (or plugsecurity.cpp) file

* Generate a new 128 bit AES cipher key and initialization vector (IV)

  * Use whatever tool you like

    .. code-block:: rst
      :caption: Example, Mac

      openssl enc -aes-128-cbc -k secret -P -md sha1


  * Add key/IV to appsecurity.cpp (e.g. kAuthPolicySecretKey and kAuthPolicyCipherIV)
  * Add key/IV to cipher.xml in "security" folder (base-64 encoded)

    .. code-block::
      :caption: Example, Mac
      
      echo '6A151BD08111BDFBA3494A41B4B1B7A2' | xxd -r -p | base64

      <Cipher algorithm="0" mode="0">
        <Attributes x:id="initialVector" material="..."/>
        <Attributes x:id="secretKey" material="..."/>
      </Cipher>

* Generate a new RSA public/private key pair

  * Use the CCL crypt tool
  * Store in authorizer.privatekey and authorizer.publickey
  * Encrypt the public key with the AES cipher key
  * Convert the encrypted public key to a C++ file using the CCL makebin tool and add it to the application project

* Create the actual authpolicy.xml file
  
  * Create a script to sign, encrypt, and convert the policy to a C++ file (e.g. make_authpolicy.bat/.sh)
  * Always call this script after editing the policy
  * Add the policy C++ file to the project and it to the **ICryptoKeyStore** during application initialization.

* Make sure to call **IAuthorizationManager::setAppIdentity()** and **IAuthorizationManager::loadPolicy()**