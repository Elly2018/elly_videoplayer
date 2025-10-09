#!/bin/sh
projectpath=$(pwd)/../example/addons/videoplayer/lib/Dep/
if [ -d "$projectpath" ]; then
  echo "Directory '$projectpath' exists."
else
  echo "Directory '$projectpath' does not exist."
fi
sh ./linux_build_dependencise.sh $projectpath