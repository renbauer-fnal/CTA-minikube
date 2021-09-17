# XRootD SSI/Protocol Buffer Interface Project

This project contains two parts:

1. Generic C++ headers which are used to bind a Google protocol buffer definition to the XRootD SSIv2
   transport layer.

2. Google protocol buffer definitions to instantiate specific interfaces.

The main use case is the EOS-CTA interface, but in principle the code can be used for any client-server
where the protocol is defined by Protocol Buffers and the transport layer is XRootD SSI.

## Overview

In the application, the compiled protocol buffer definitions should be provided as template parameters
to the generic headers. This creates a client-server interface which can send and receive protocol
buffers using XRootD SSIv2 as the transport layer.

The supported communication types are as follows:

1. Synchronous Request-Response. The Response is XRootD SSI Metadata.
2. Request-Data/Stream Response. There is an immediate synchronous response to indicate that the
   request was received. This Response is XRootD SSI Metadata as above. This is followed by an
   arbitrary number of protocol buffer records.
3. Alert Message. An arbitrary number of alert messages can be sent asynchronously at any time before
   the final Response, for example for logging or to report on progress of the Request.

## Usage

Note that the CMakeLists.txt files in this project are for building the test code only. They are only
intended to be used by the Continuous Integration system, not in production builds.

To use this project, it should be configured as a git submodule within the main project. The generic
header files should be included in source files in the main project and are therefore built from the
main project. See the test/ subdirectory for an example.

The main project also needs to provide a CMakeLists.txt file to compile the protocol buffers it needs
(using protoc3). Again, see test/ for an example.

