#!/bin/sh

shadowhashdat="$(plutil -extract 'ShadowHashData.0' raw -o - "$1" | base64 -d | plutil -extract SALTED-SHA512-PBKDF2 xml1 -o - -)"
iter="$(echo "$shadowhashdat" | plutil -extract iterations raw -o - -)"
salt="$(echo "$shadowhashdat" | plutil -extract salt raw -o - - | base64 -d | xxd -p -c 256)"
entropy="$(echo "$shadowhashdat" | plutil -extract entropy raw -o - - | base64 -d | xxd -p -c 256)"
echo "\$ml\$$iter\$$salt\$$entropy"
