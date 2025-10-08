#!/bin/sh
projectpath=$(pwd)/../example/addons/videoplayer/lib/Linux-AMD64/
if [ -d "$projectpath" ]; then
  echo "Directory '$projectpath' exists."
else
  echo "Directory '$projectpath' does not exist."
fi
sh ./linux_build_post.sh $projectpath