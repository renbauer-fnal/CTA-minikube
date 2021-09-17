# XRootD SSIv2/Google Protocol Buffers 3 bindings

This directory contains generic classes which bind Google Protocol Buffer definitions to the
XRootD SSI transport layer. It contains the following files:

## Client Side

* **XrdSsiPbServiceClientSide.hpp** : Wraps up the Service factory object with Protocol Buffer integration
  and synchronisation between Requests and Responses
* **XrdSsiPbRequest.hpp** : Send Requests and handle Responses
* **XrdSsiPbIStreamBuffer.hpp** : Input Stream Buffer to handle SSI Data/Stream Responses

## Server Side

* **XrdSsiPbService.hpp** : Defines Service on server side: bind Request to Request Processor and Execute
* **XrdSsiPbRequestProc.hpp** : Process Request and send Response
* **XrdSsiPbOStreamBuffer.hpp** : Output Stream Buffer to send SSI Data/Stream Responses

## Both Client and Server Side

* **XrdSsiPbAlert.hpp** : Optional Alerts from Service to Client (_e.g._ log messages) 
* **XrdSsiPbConfig.hpp** : Read XRootD configuration files and environment variables
* **XrdSsiPbException.hpp** : Convert XRootD SSI and Protocol Buffer errors to exceptions
* **XrdSsiPbLog.hpp** : Logging and Protocol Buffer inspection
