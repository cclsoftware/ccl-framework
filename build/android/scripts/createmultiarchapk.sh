#!/bin/sh

# --- This script creates a multi-architecture APK from individual architecture APKs ---
# ---
# --- Usage:   createmultiarchapk.sh [application] [configuration] [architectures separated by +]
# --- Example: createmultiarchapk.sh notionmobile release arm7+arm8
# ---
# --- Based on instructions found at:
#       https://developer.android.com/studio/build/building-cmdline#build_bundle

if [ "$3" == "" ]; then
  echo Usage:   createmultiarchapk.sh [application] [configuration] [architectures separated by +]
  echo Example: createmultiarchapk.sh notionmobile release arm7+arm8
  exit 1
fi

# --- Attempt to set SCRIPT_HOME ---
script_path=$0

SCRIPT_HOME=${script_path%"${script_path##*/}"}
SCRIPT_HOME=$( cd -P "${SCRIPT_HOME:-./}" > /dev/null && printf '%s\n' "$PWD" ) || exit

# --- Set tool paths ---
source $SCRIPT_HOME/sdktools.sh $SCRIPT_HOME

# --- Clean up ---
cleanup_proc () {
  echo Cleaning up...
  rm -fr staging
}

# --- Exit regularly ---
exit_proc () {
  cleanup_proc
  exit 0
}

# --- Exit in case of an error ---
error_proc () {
  cleanup_proc
  exit 1
}

# --- Script arguments ---
APP=$1
CONFIG=$2
ARCHES=$3

if [ "$4" == "-f" ]; then
  OUTPUT=$5
else
  OUTPUT=$APP-android-$CONFIG.apk
fi

# --- Create staging folders ---
echo Preparing staging area...
if [ -d staging ]; then
  rm -fr staging
fi
mkdir -p staging/res

# --- Collect libraries for bundle ---
if [ "$ARCHES" != "" ]; then
  IFS='+' read -a ARCHES <<< "$ARCHES"
  for ARCH in "${ARCHES[@]}"; do
    APK=$APP/build/outputs/apk/$ARCH/$CONFIG/$APP-$ARCH-$CONFIG.apk
    echo "  $APK"
    if [ ! -e $APK ]; then
      echo "  APK not found: $APK"
      error_proc
    fi
    $SEVENZIP x -aoa $APK lib -ostaging > /dev/null
    if [ $? -ne 0 ]; then error_proc; fi
  done
fi

# --- Link APK ---
echo Linking APK...
cd staging
cp ../$APP/build/outputs/apk/$ARCH/$CONFIG/$APP-$ARCH-$CONFIG.apk $OUTPUT
if [ $? -ne 0 ]; then error_proc; fi

$ZIPTOOL -r $OUTPUT lib > /dev/null
if [ $? -ne 0 ]; then error_proc; fi

$ZIPALIGN -P 16 -f 4 $OUTPUT ../$OUTPUT
if [ $? -ne 0 ]; then error_proc; fi
cd ..

# --- Sign APK ---
$SCRIPT_HOME/signapk.sh $OUTPUT
if [ $? -ne 0 ]; then error_proc; fi

exit_proc
