#!/bin/sh

COMPANY_NAME="CCL"
APP_NAME="Application Template"
IMAGE_NAME="${COMPANY_NAME} ${APP_NAME}.dmg"
BUILD_PATH="../../cmake/mac/build/Release"
if [ $1 ]; then
	BUILD_PATH=$1
fi

rm -f "$IMAGE_NAME"
rm -rf Dist
mkdir Dist

cp -R -p "${BUILD_PATH}/${APP_NAME}.app" "Dist/${APP_NAME}.app"
cd Dist
find . -name *pdn -exec rm {} \;
find . -name *psd -exec rm {} \;
find . -name .svn -exec rm -r -f {} \;

ln -s /Applications Applications
cd ..

mkdir Dist/.background
cp Desktop/background.png Dist/.background
cp Desktop/DS_Store Dist/.DS_Store

hdiutil create -size 1000m -fs HFS+ -volname "${COMPANY_NAME} ${APP_NAME}" -srcfolder Dist "$IMAGE_NAME"
