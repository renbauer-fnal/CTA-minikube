#!/usr/bin/python
# @project        The CERN Tape Archive (CTA)
# @copyright      Copyright(C) 2019-2021 CERN
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

'''Command line tool to remove all CASTOR-imported tapes from a CTA tapepool'''

import sys
import getopt
from time import sleep, time
from datetime import datetime
from threading import Thread

import castor_tools


def usage(exitcode):
    '''prints usage'''
    print __doc__
    print 'Usage : ' + sys.argv[0] + ' [-h|--help] -t|--tapepool <tapepool>'
    sys.exit(exitcode)


def connectToCTA():
    '''Connects to the CTA catalogue database, cf. castor_tools'''
    user, passwd, dbname = castor_tools.getNSDBConnectParam('CTACONFIG')
    return castor_tools.connectToDB(user, passwd, dbname, '0.0', enforceCheck=False)


def async_remove_castor_tapes(conn, tapepool):
    '''helper function to execute the PL/SQL procedure in a separate thread'''
    cur = conn.cursor()
    cur.execute('BEGIN removeCASTORMetadata(:tapepool); END;', tapepool=tapepool)


def run():
    '''main code'''
    tapepool = None
    # first parse the options
    try:
        options, _ = getopt.getopt(sys.argv[1:], 'ht:', ['help', 'tapepool='])
    except Exception as e:
        print(e)
        usage(1)
    for f, v in options:
        if f == '-h' or f == '--help':
            usage(0)
        elif f == '-t' or f == '--tapepool':
            tapepool = v
        else:
            print 'Unknown option: ' + f
            usage(1)

    # deal with arguments
    if not tapepool:
        print 'Missing argument(s)\n'
        usage(1)

    try:
        # connect to the CTA catalogue and execute the async_remove_castor_tapes function on a separate thread, to be able to babysit it
        ctaconn_async = connectToCTA()
        runner = Thread(target=async_remove_castor_tapes, args=[ctaconn_async, tapepool])
        runner.start()

        # at the same time, connect again to the Nameserver for monitoring the process
        sleep(1)
        ctaconn = connectToCTA()
        cur = ctaconn.cursor()
        querylog = '''
          SELECT timestamp, message FROM CNS_CTAMigrationLog
           WHERE tapepool = :tapepool AND timestamp > :t ORDER BY timestamp ASC
        '''

        # poll the NS database for logs about the ongoing execution
        t = time() - 24*3600
        lastprinttime = time()
        while True:
            cur.execute(querylog, tapepool=tapepool, t=t)
            rows = cur.fetchall()
            if rows and 'Removal of CASTOR tapes metadata completed successfully' in rows[-1][1]:
                for r in rows:
                    print datetime.fromtimestamp(int(r[0])).isoformat(), ' ', r[1]
                # export is over, terminate
                break

            # exit also in case of premature termination
            if not runner.isAlive():
                break

            # no news, keep printing something every minute
            if time() - lastprinttime > 60:
                lastprinttime = time()
                for r in rows:
                    print datetime.fromtimestamp(int(r[0])).isoformat(), ' ', r[1]
                if rows:
                    t = rows[-1][0]
                else:
                    print datetime.now().isoformat().split('.')[0], '  .'
            sleep(5)

        # that ought to be immediate now
        runner.join()

        # close DB connections
        castor_tools.disconnectDB(ctaconn_async)
        castor_tools.disconnectDB(ctaconn)

    except Exception as e:
        print(e)
        import traceback
        traceback.print_exc()
        sys.exit(-1)


if __name__ == '__main__':
    run()
