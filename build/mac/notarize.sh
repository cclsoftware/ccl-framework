#!/bin/bash

if [ "$#" -lt 3 ]; then
    echo "Expected arguments privatekey, keyid and issuer."
    echo "Arguments provided: $@"
	exit
fi

notarize ()
{
	TARGET="$1"
	
	if [ $2 ]; then
		PRIVATEKEY=$2
	fi
	
	if [ $3 ]; then
		KEYID=$3
	fi
	
	if [ $4 ]; then
		ISSUER=$4
	fi
	
	FILENAME=$(basename -- "$TARGET")
	EXTENSION=${FILENAME##*.}
	FILENAME=${FILENAME%.*}
	if [[ $EXTENSION =~ "dmg" ]] || [[ $EXTENSION =~ "zip" ]]; then
		ARCHIVE=$TARGET
	else
		ARCHIVE=$TARGET.zip
		rm -f "$ARCHIVE"
		/usr/bin/ditto -c -k --keepParent "$TARGET" "$ARCHIVE"	
	fi

	RESPONSE=$(xcrun notarytool submit "$ARCHIVE" --wait -k "$PRIVATEKEY" -d $KEYID -i $ISSUER 2>&1 )
	echo $RESPONSE
	
	if [[ ! $RESPONSE =~ "status: Accepted" ]]; then
		echo "Upload failed: $RESPONSE"
		return 1
	fi
	
	if [[ ! $EXTENSION =~ "dylib" ]] && [[ ! $EXTENSION =~ "zip" ]]; then
		xcrun stapler staple "$TARGET"
	fi
		
	if [ -f "$TARGET.zip" ]
	then
		rm "$TARGET.zip"
		echo "removed temporary container $TARGET.zip"
	fi
	
	return 0
}

notarize "$1" "$2" "$3" "$4"
