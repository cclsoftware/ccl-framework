#!/bin/sh

if [ "$JAVA_HOME" == "" ]; then
  echo Error: JAVA_HOME not set!
  exit 1
fi

SCRIPT_HOME=$1

if [ ! -e $SCRIPT_HOME/sdktools.sh ]; then
  echo Error: Bad arguments!
  exit 1
fi

BUILD_TOOLS_VERSION=36.1.0
BUNDLETOOL_VERSION=1.18.1

# --- Detect OS ---
windows=false
macos=false
case "$( uname )" in                         #(
  MSYS* | MINGW* | CYGWIN* ) windows=true ;; #(
  Darwin* )                  macos=true   ;;
esac

# --- Set tool paths ---
SDKPATH=$HOME/Android/Sdk

SEVENZIP=7za
ZIPTOOL=zip

if [ $windows == true ]; then
  TOOLPATH=$SCRIPT_HOME/../../../tools/bin/win
  SDKPATH=$LOCALAPPDATA/Android/Sdk

  SEVENZIP=$TOOLPATH/7za
  ZIPTOOL=$TOOLPATH/zip
elif [ $macos == true ]; then
  SDKPATH=$HOME/Library/Android/sdk
fi

JAVA="$JAVA_HOME/bin/java"

AAPT2=$SDKPATH/build-tools/$BUILD_TOOLS_VERSION/aapt2
ZIPALIGN=$SDKPATH/build-tools/$BUILD_TOOLS_VERSION/zipalign
APKSIGNER="-jar $SDKPATH/build-tools/$BUILD_TOOLS_VERSION/lib/apksigner.jar"

BUNDLETOOL="-jar $SCRIPT_HOME/../../../tools/bin/android/apktools/bundletool-all-$BUNDLETOOL_VERSION.jar"
JARSIGNER="$JAVA_HOME/bin/jarsigner"

if [ $windows == true ] && [ "${BUNDLETOOL}" != "${BUNDLETOOL/\/cygdrive\//}" ]; then
  BUNDLETOOL=`echo ${BUNDLETOOL} | sed -r -e "s/\/cygdrive\/(.)\//\1:\//"` 
fi
