#!/bin/bash
# Copies firmware/secrets.h into every sketch folder that includes "secrets.h".
# (Arduino only sees files inside the sketch folder, so one shared file is not enough.)
# The copies are git-ignored. Edit firmware/secrets.h once, then re-run this script.
cd "$(dirname "$0")" || exit 1
if [ ! -f secrets.h ]; then
  cp secrets.example.h secrets.h
  echo "Created firmware/secrets.h from the template. Fill in your values, then run this script again."
  exit 0
fi
for sketch in $(grep -rl --include=*.ino "include \"secrets.h\"" . | xargs -n1 dirname); do
  cp secrets.h "$sketch/secrets.h" && echo "  -> $sketch"
done
