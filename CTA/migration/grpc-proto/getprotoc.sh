#!/bin/bash

PROTOBUF_VERSION=3.0.0
PROTOC_FILENAME=protoc-${PROTOBUF_VERSION}-linux-x86_64.zip
pushd /home/travis
wget https://github.com/google/protobuf/releases/download/v${PROTOBUF_VERSION}/${PROTOC_FILENAME}
unzip -o  ${PROTOC_FILENAME}
bin/protoc --version
popd
