# @project        The CERN Tape Archive (CTA)
# @copyright      Copyright(C) 2003-2021 CERN
# @license        This program is free software: you can redistribute it and/or modify
#                 it under the terms of the GNU General Public License as published by
#                 the Free Software Foundation, either version 3 of the License, or
#                 (at your option) any later version.
#
#                 This program is distributed in the hope that it will be useful,
#                 but WITHOUT ANY WARRANTY; without even the implied warranty of
#                 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#                 GNU General Public License for more details.
#
#                 You should have received a copy of the GNU General Public License
#                 along with this program.  If not, see <http://www.gnu.org/licenses/>.

'''
This class implements the distributed logging facility of CASTOR for
the python tools. It basically uses syslog to send well formatted
messages to the DLF server
'''

#import syslog
import castor_tools
import thread
import traceback
import sys
import logging
from logging.handlers import SysLogHandler as syslog

def enum(*args, **kwds):
    '''a little, useful enum type, with parameterized base value'''
    try:
        base = kwds['base']
    except KeyError:
        base = 0
    enums = dict(zip(args, range(base, base+len(args))))
    return type('Enum', (), enums)


_loglevels = { logging.CRITICAL: "Emerg",  # 50
               logging.ERROR:    "Error",  # 40
               logging.WARNING:  "Warn",   # 30
               25:               "Notice",
               logging.INFO:     "Info",   # 20
               logging.DEBUG:    "Debug"   # 10
             }
_loglevelmapping = { syslog.LOG_EMERG  : logging.CRITICAL,
                     syslog.LOG_ALERT  : logging.CRITICAL,
                     syslog.LOG_CRIT   : logging.CRITICAL,
                     syslog.LOG_ERR    : logging.ERROR,
                     syslog.LOG_WARNING: logging.WARNING,
                     syslog.LOG_NOTICE : 25,
                     syslog.LOG_INFO   : logging.INFO,
                     syslog.LOG_DEBUG  : logging.DEBUG
                   }
_initialized = False

logger = None
_handler = None

def _kvToLog(param, value):
    # Integers
    try:
        return '%s="%d"' % (param[0:20], int(value))
    except ValueError:
        pass
    except TypeError:
        pass
    except AttributeError:
        pass
    # Floats
    try:
        return '%s="%f"' % (param[0:20], float(value))
    except ValueError:
        pass
    except TypeError:
        pass
    except AttributeError:
        pass
    # others -> converted to string
    value = str(value)
    value = value.replace('\n', ' ') # remove newlines
    value = value.replace('\t', ' ') # remove tabs
    value = value.replace('"', '\'') # escape double quotes
    value = value.strip("'") # remove trailing and leading signal quotes
    return '%s="%s"' % (param[0:20], value[0:1024])

def _buildMessageFromParams(message, params):
    '''Writes a log message.
    Parameters can be passed as a dictionary of name/value.
    Some parameter names will trigger a specific interpretation :
      - reqid : will be treated as the UUID of the ongoing request
      - subreqid : will be treated as the UUID of the ongoing subrequest
      - fileid : will be treated as a pair(nshost, fileid)
    '''
    # build the message raw text
    if params.has_key('fileId'):
        params['NSHOSTNAME'] = params['fileId'][0]
        params['NSFILEID'] = params['fileId'][1]
        del params['fileId']
    if params.has_key('fileid'):
        params['NSHOSTNAME'] = params['fileid'][0]
        params['NSFILEID'] = params['fileid'][1]
        del params['fileid']
    if params.has_key('reqId') and params['reqId']:
        params['REQID'] = params['reqId']
        del params['reqId']
    if params.has_key('reqid') and params['reqid']:
        params['REQID'] = params['reqid']
        del params['reqid']
    if params.has_key('subreqId') and params['subreqId']:
        params['SUBREQID'] = params['subreqId']
        del params['subreqId']
    if params.has_key('subreqid') and params['subreqid']:
        params['SUBREQID'] = params['subreqid']
        del params['subreqid']
    rawmsg = 'MSG="%s" ' % message
    for param in params:
        value = params[param]
        if value == None:
            continue
        rawmsg += _kvToLog(param, value) + " "
    return rawmsg.rstrip()

class CastorFormatter(logging.Formatter):
    '''Logs formatter
    NB: timestamp and hostname are handled by rsyslog'''
    def format(self, record):
        # build main message. We distinguish 2 cases :
        #  - if record.args is a dictionnary, record.msg is a pure
        #    string and record.args has to be added to it in k=v syntax
        #  - else record.msg is a formatting string and record.args
        #    is used as input for this formatting
        # Note the very special case for a single, empty dictionnary argument.
        # This mess is created inthe constructor of LogRecord, see the comment
        # there about args[0] checking
        if isinstance(record.args, dict):
            message = _buildMessageFromParams(record.msg, record.args)
        elif len(record.args) == 1 and isinstance(record.args[0], dict) and not record.args[0]:
            message = _buildMessageFromParams(record.msg, {})
        else:
            message = 'MSG="' + record.msg % record.args + '"'
        # for LOG_ERR and above, build exception context
        exceptStr = ''
        if record.levelno >= logging.ERROR:
            exc_type, exc_value, exc_traceback = sys.exc_info()
            if exc_type != None:
                exceptStr = _kvToLog('TraceBack', ''.join(traceback.format_exception(exc_type, exc_value, exc_traceback)))
                if hasattr(exc_value, '_remote_tb'):
                    exceptStr += ' ' + _kvToLog('RemoteTraceBack', exc_value._remote_tb[0])
        # return formatted log
        # note that the thread id is not the actual linux thread id but the
        # "fake" python one
        # note also that we cut it to 5 digits (potentially creating colisions)
        # so that it does not exceed the length expected by some other components
        # (e.g. syslog)
        return "".join([record.name, "[", str(record.process), "]: ",
                        'LVL="%s" ' % _loglevels[record.levelno],
                        'TID="%d" ' % (record.thread%10000),
                        message + " " + exceptStr])

def init(facility):
    '''Initializes the distributed logging facility.
    Sets the facility name and the mask'''
    global _initialized, logger, _handler
    if _initialized:
        raise ValueError('DLF cannot be initialized twice. Ignoring new attempt.\n'
                         'Previous one was for facility %s' % logger.name)
    # Find out which log level should be used from castor.conf
    config = castor_tools.castorConf()
    smask = config.getValue("LogMask", facility, "LOG_INFO")
    try:
        logmask = getattr(syslog, smask)
    except AttributeError:
        # Unknown mask name
        raise AttributeError('Invalid Argument: ' + smask + ' for ' +
                              facility + '/LogMask option in castor.conf')
    # Create logger with proper log level
    logger = logging.getLogger(facility)
    logger.setLevel(_loglevelmapping[logmask])
    # use syslog handler with our own formatter
    logsocket = config.getValue('Syslog', 'SocketName', '/var/run/castorlog.sock')
    _handler = syslog(address=logsocket, facility=syslog.LOG_LOCAL3)
    _handler.setFormatter(CastorFormatter())
    logger.addHandler(_handler)
    _initialized = True

def shutdown():
    '''Close/shutdown the distributed logging facility interface'''
    global _initialized
    if not _initialized:
        return
    # close sysloghandler
    _handler.close()
    # Reset internal variables
    _initialized = False

def init_required(func):
    '''Only calls given function if the login was initialized'''
    def wrapper(*args, **kwargs):
        if not _initialized:
            return None
        return func(*args, **kwargs)
    return wrapper

@init_required
def writedebug(_message, **_params):
    '''Writes a log message with the LOG_DEBUG priority'''
    logger.debug(_message, _params)

@init_required
def write(_message, **_params):
    '''Writes a log message with the LOG_INFO priority'''
    writeinfo(_message, **_params)

@init_required
def writeinfo(_message, **_params):
    '''Writes a log message with the LOG_INFO priority'''
    logger.info(_message, _params)

@init_required
def writenotice(_message, **_params):
    '''Writes a log message with the LOG_NOTICE priority'''
    logger.log(25, _message, _params)

@init_required
def writewarning(_message, **_params):
    '''Writes a log message with the LOG_WARNING priority'''
    logger.warning(_message, _params)

@init_required
def writeerr(_message, **_params):
    '''Writes a log message with the LOG_ERR priority'''
    logger.error(_message, _params)

@init_required
def writeemerg(_message, **_params):
    '''Writes a log message with the LOG_EMERG priorityd'''
    logger.critical(_message, _params)
