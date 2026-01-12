#!/bin/sh

# --- This script signs an APK using the apksigner tool ---
# ---
# --- Usage:   signapk.sh apk
# --- Example: signapk.sh notionmobile-arm8-release.apk

if [ "$1" == "" ]; then
  echo Usage:   signapk.sh apk
  echo Example: signapk.sh notionmobile-arm8-release.apk
  exit 1
fi

# --- Attempt to set SCRIPT_HOME ---
script_path=$0

SCRIPT_HOME=${script_path%"${script_path##*/}"}
SCRIPT_HOME=$( cd -P "${SCRIPT_HOME:-./}" > /dev/null && printf '%s\n' "$PWD" ) || exit

# --- Set tool paths ---
source $SCRIPT_HOME/sdktools.sh $SCRIPT_HOME

# --- Exit regularly ---
exit_proc () {
  exit 0
}

# --- Exit in case of an error ---
error_proc () {
  exit 1
}

# --- Script arguments ---
APK=$1

# --- Sign APK ---
SIGNING_PROPERTIES=gradle/properties/signing.properties
if [ ! -e $SIGNING_PROPERTIES ]; then exit_proc; fi

echo Signing APK...
while IFS= read -r line; do
  if [[ $line =~ ^key.store=(.*)$ ]]; then
    KEYSTORE=`echo ${BASH_REMATCH[1]}`
  elif [[ $line =~ ^key.alias=(.*)$ ]]; then
    KEYALIAS=`echo ${BASH_REMATCH[1]}`
  elif [[ $line =~ ^key.store.password=(.*)$ ]]; then
    KEYSTOREPASS=`echo ${BASH_REMATCH[1]}`
  elif [[ $line =~ ^key.alias.password=(.*)$ ]]; then
    KEYALIASPASS=`echo ${BASH_REMATCH[1]}`
  fi
done < $SIGNING_PROPERTIES
"$JAVA" $APKSIGNER sign --ks "$KEYSTORE" --ks-pass pass:"$KEYSTOREPASS" --key-pass pass:"$KEYALIASPASS" --ks-key-alias "$KEYALIAS" $APK
if [ $? -ne 0 ]; then error_proc; fi

exit_proc
