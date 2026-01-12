#!/bin/sh
DESTINATION="$PWD"
PRODUCT="Application Template"

BUILD_PATH="(RelPathToRoot)/build/cmake/ios/Release"
if [ $1 ]; then
	BUILD_PATH=$1
fi

BUNDLE="${BUILD_PATH}/$PRODUCT.app"
(RelPathToRoot)/build/mac/codesign.sh "$BUNDLE"
(RelPathToRoot)/build/ios/PackageApplication -v "$BUNDLE" -o "$DESTINATION/$PRODUCT.ipa"
