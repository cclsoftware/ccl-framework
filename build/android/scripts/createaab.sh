#!/bin/sh

# --- This script creates an Android App Bundle from already compiled APKs ---
# ---
# --- Usage:   createaab.sh [application] [configuration] [architectures separated by +]
# --- Example: createaab.sh capturemobile release arm7+arm8
# ---
# --- Based on instructions found at:
#       https://developer.android.com/studio/build/building-cmdline#build_bundle

if [ "$3" == "" ]; then
  echo Usage:   createaab.sh [application] [configuration] [architectures separated by +]
  echo Example: createaab.sh capturemobile release arm7+arm8
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

# --- Set Gradle architecture and ABI from project architecture ID ---
setarch_proc () {
  if [ "$1" == "arm7" ]; then
    GRADLE_ARCH=arm7
    GRADLE_ABI=armeabi-v7a
  elif [ "$1" == "arm8" ]; then
    GRADLE_ARCH=arm8
    GRADLE_ABI=arm64-v8a
  elif [ "$1" == "x86" ]; then
    GRADLE_ARCH=x86
    GRADLE_ABI=x86
  elif [ "$1" == "x86_64" ]; then
    GRADLE_ARCH=x86_64
    GRADLE_ABI=x86_64
  fi
}

# --- Special handling for resource files which are case sensitive ---
packres_proc () {
  $SEVENZIP a $APP.zip res resources.pb -sdel -x!res/??*_?.* > /dev/null
  if [ $? -ne 0 ]; then error_proc; fi

  for i in {1..9}; do
    if [ ! -d res ]; then return 0; fi

    cd res
    for file in $(ls ??*_${i}.*); do
      mv $file ${file/_${i}/}
    done
    cd ..
    $SEVENZIP a $APP.zip res -ssc -sdel -x!res/??*_?.* > /dev/null
    if [ $? -ne 0 ]; then error_proc; fi
  done
}

# --- Script arguments ---
APP=$1
CONFIG=$2
ARCHES=$3

# --- Create staging folders ---
echo Preparing staging area...
if [ -d staging ]; then
  rm -fr staging
fi
mkdir -p staging/res

# --- Convert and extract resources ---
echo "  Converting resources..."
IFS='+' read -a ARCHES <<< "$ARCHES"
ARCH=${ARCHES[0]}

APK=$APP/build/outputs/apk/$ARCH/$CONFIG/$APP-$ARCH-$CONFIG.apk
$AAPT2 convert --output-format proto $APK -o staging/temporary.apk
if [ $? -ne 0 ]; then error_proc; fi

echo "  Extracting assets..."
APK=staging/temporary.apk
$SEVENZIP x -aoa $APK classes.dex -ostaging/dex > /dev/null
if [ $? -ne 0 ]; then error_proc; fi

$SEVENZIP x -aoa $APK assets -ostaging > /dev/null
if [ $? -ne 0 ]; then error_proc; fi

$SEVENZIP x -aou $APK res -ostaging > /dev/null
if [ $? -ne 0 ]; then error_proc; fi

$SEVENZIP x -aoa $APK resources.pb -ostaging > /dev/null
if [ $? -ne 0 ]; then error_proc; fi

$SEVENZIP x -aoa $APK AndroidManifest.xml -ostaging/manifest > /dev/null
if [ $? -ne 0 ]; then error_proc; fi
rm $APK

# --- Collect libraries for bundle ---
echo "  Extracting libraries..."
for ARCH in "${ARCHES[@]}"; do
  setarch_proc $ARCH

  APK=$APP/build/outputs/apk/$ARCH/$CONFIG/$APP-$ARCH-$CONFIG.apk
  echo "    $APK"
  if [ ! -e $APK ]; then
    echo "    APK not found: $APK"
    error_proc
  fi
  $SEVENZIP x -aoa $APK lib -ostaging > /dev/null
  if [ $? -ne 0 ]; then error_proc; fi

  if [ -e $APP/build/outputs/mapping/${GRADLE_ARCH}${CONFIG}/mapping.txt ]; then
    cp $APP/build/outputs/mapping/${GRADLE_ARCH}${CONFIG}/mapping.txt staging/proguard.map > /dev/null
  fi
done

# --- Package staging area ---
echo Packaging staged files...
cd staging
$SEVENZIP a $APP.zip assets dex lib manifest > /dev/null
if [ $? -ne 0 ]; then error_proc; fi

packres_proc
cd ..

# --- Create bundle ---
echo Creating app bundle...
if [ -e BundleConfig.json ]; then BUNDLECONFIG="--config=BundleConfig.json"; fi
if [ -e staging/proguard.map ]; then PROGUARDMAP="--metadata-file=com.android.tools.build.obfuscation/proguard.map:staging/proguard.map"; fi

for ARCH in "${ARCHES[@]}"; do
  setarch_proc $ARCH

  if [ -e $APP/build/outputs/native-debug-symbols/${GRADLE_ARCH}${CONFIG}/native-debug-symbols.zip ]; then
    $SEVENZIP x -aoa $APP/build/outputs/native-debug-symbols/${GRADLE_ARCH}${CONFIG}/native-debug-symbols.zip -ostaging/BUNDLE-METADATA/com.android.tools.build.debugsymbols $GRADLE_ABI > /dev/null
  fi
done

"$JAVA" $BUNDLETOOL build-bundle --modules=staging/$APP.zip --output=$APP-android-$CONFIG.aab --overwrite $BUNDLECONFIG $PROGUARDMAP
if [ $? -ne 0 ]; then error_proc; fi

# --- Add debug symbols ---
if [ -d staging/BUNDLE-METADATA ]; then
  echo Adding debug symbols...
  cd staging
  $ZIPTOOL -u -r -q -D ../$APP-android-$CONFIG.aab BUNDLE-METADATA
  if [ $? -ne 0 ]; then error_proc; fi
  cd ..
fi

# --- Sign bundle ---
SIGNING_PROPERTIES=gradle/properties/signing.properties
if [ ! -e $SIGNING_PROPERTIES ]; then exit_proc; fi

echo Signing app bundle...
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
"$JARSIGNER" -keystore "$KEYSTORE" -storepass "$KEYSTOREPASS" -keypass "$KEYALIASPASS" $APP-android-$CONFIG.aab "$KEYALIAS"
if [ $? -ne 0 ]; then error_proc; fi

exit_proc
