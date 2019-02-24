#!/bin/bash

set -o nounset
set -o errexit
set -o pipefail

pubbase=dnnr/hlwm
docker pull $pubbase
docker build -t hlwm-ci-checks --cache-from=$pubbase - <Dockerfile.iwyu

docker run --rm --volume="$PWD":/hlwm:ro hlwm-ci-checks \
    bash -c 'cd /hlwm && ./iwyu.sh'
