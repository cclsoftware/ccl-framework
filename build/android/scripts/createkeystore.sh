#!/bin/sh

# === Key Tool ===
keytool="$JAVA_HOME/bin/keytool"

# === Key Store Settings ===
keyalias=
keypassword=
keystorefile=
keystorepassword=

# === Certificate Information ===
# Common Name
CN=
# Organizational Unit
OU=
# Organization
O=
# Locality
L=
# Country
C=

# Key Store Creation
$keytool -genkey -noprompt -alias $keyalias -dname "CN=$CN, OU=$OU, O=$O, L=$L, C=$C" -keystore $keystorefile -storepass $keystorepassword -keypass $keypassword -keyalg RSA -keysize 2048 -storetype pkcs12

# Verification
$keytool -list -noprompt -keystore $keystorefile -storepass $keystorepassword
