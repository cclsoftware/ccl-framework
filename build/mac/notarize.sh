#!/bin/bash

if [ "$#" -lt 3 ]; then
    echo "Expected arguments privatekey, keyid and issuer."
    echo "Arguments provided: $@"
	exit
fi

notarize ()
{
	TARGET="$1"
	
	if [ -n "$2" ]; then
		PRIVATEKEY=$2
	fi

	if [ -n "$3" ]; then
		KEYID=$3
	fi

	if [ -n "$4" ]; then
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

	# Submit for notarization, retrying on transient server-side errors (e.g. HTTP 504).
	MAX_ATTEMPTS=5
	ATTEMPT=1
	RESPONSE=""
	while true; do
		RESPONSE=$(xcrun notarytool submit "$ARCHIVE" --wait -k "$PRIVATEKEY" -d "$KEYID" -i "$ISSUER" 2>&1)
		echo "$RESPONSE"

		# Definitive verdict from the service: don't retry, accepted or not.
		if [[ $RESPONSE =~ "status: Accepted" ]] || [[ $RESPONSE =~ "status: Invalid" ]] || [[ $RESPONSE =~ "status: Rejected" ]]; then
			break
		fi

		# Retry only on transient failures (gateway/timeout/5xx/connection).
		LOWER=$(echo "$RESPONSE" | tr '[:upper:]' '[:lower:]')
		if [[ $LOWER =~ (http status code: 5|bad gateway|gateway timeout|timed out|timeout|could not connect|connection reset|connection refused|temporarily unavailable|service unavailable|try again later) ]] && [ $ATTEMPT -lt $MAX_ATTEMPTS ]; then
			DELAY=$((ATTEMPT * 30))
			echo "notarytool hit a transient error (attempt $ATTEMPT/$MAX_ATTEMPTS); retrying in ${DELAY}s..."
			sleep $DELAY
			ATTEMPT=$((ATTEMPT + 1))
			continue
		fi

		# Non-transient error, or retries exhausted.
		break
	done

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
