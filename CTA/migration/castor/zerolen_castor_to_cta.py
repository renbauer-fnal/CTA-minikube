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

'''Command line tool for the actual copy of zero length file's metadata from CASTOR to CTA'''

import sys
import getopt
from time import sleep, time
from datetime import datetime
from threading import Thread

import castor_tools


def usage(exitcode):
    '''prints usage'''
    print __doc__
    print 'Usage : ' + sys.argv[0] + ' [-h|--help] -v|--vo <VO> -i|--eosctainstance <eosctainstance> --dryrun|--doit'
    sys.exit(exitcode)


def connectToCTA():
    '''Connects to the CTA catalogue database, cf. castor_tools'''
    user, passwd, dbname = castor_tools.getNSDBConnectParam('CTACONFIG')
    return castor_tools.connectToDB(user, passwd, dbname, '0.0', enforceCheck=False)


def async_import(conn, vo, eosctainstance, dryrun):
    '''helper function to execute the PL/SQL procedure in a separate thread'''
    cur = conn.cursor()
    cur.execute('BEGIN importFromCASTOR(NULL, :vo, :eosctainstance, 1, :dryrun); END;', \
                vo=vo, eosctainstance=eosctainstance, dryrun=dryrun)


def run():
    '''main code'''
    success = False
    dryrun = None
    vo = None
    eosctaInstance = None
    # first parse the options
    try:
        options, _ = getopt.getopt(sys.argv[1:], 'hv:i:d', ['help', 'vo=', 'eosctainstance=', 'dryrun', 'doit'])
    except Exception, e:
        print e
        usage(1)
    for f, v in options:
        if f == '-h' or f == '--help':
            usage(0)
        elif f == '-v' or f == '--vo':
            vo = v
        elif f == '-i' or f == '--eosctainstance':
            eosctaInstance = v
        elif f == '--dryrun':
            dryrun = 1
        elif f == '--doit':
            dryrun = 0
        else:
            print "unknown option : " + f
            usage(1)

    # deal with arguments
    if not vo or not eosctaInstance or dryrun == None:
        print 'Missing argument(s). Either --dryrun or --doit is mandatory.\n'
        usage(1)

    try:
        # connect to CTA and execute the import on a separate thread, to be able to babysit it
        ctaconn = connectToCTA()
        runner = Thread(target=async_import, args=(ctaconn, vo, eosctaInstance, dryrun))
        runner.start()

        # at the same time, connect to nameserver for monitoring the import process
        sleep(1)
        nsconn = castor_tools.connectToNS()
        nscur = nsconn.cursor()
        querylog = '''
          SELECT timestamp, message FROM CTAMigrationLog
           WHERE tapepool = upper(:vo) || '::*' AND timestamp > :t ORDER BY timestamp ASC
        '''

        # poll the NS database for logs about the ongoing migration
        t = time() - 12*3600
        while True:
            nscur.execute(querylog, vo=vo, t=t)
            rows = nscur.fetchall()
            for r in rows:
                print datetime.fromtimestamp(int(r[0])).isoformat(), ' ', r[1]
            if not rows:
                # keep printing something when no news
                print datetime.now().isoformat().split('.')[0], '  .'
            elif 'CASTOR metadata import completed successfully' in rows[-1][1]:
                # export is over, terminate
                success = True
                break
            if rows:
                t = rows[-1][0]
            # exit also in case of premature termination
            if not runner.isAlive():
                break
            sleep(60)

        # that ought to be immediate now
        runner.join()

        # close DB connections
        castor_tools.disconnectDB(ctaconn)
        castor_tools.disconnectDB(nsconn)

        # goodbye
        if success:
            print datetime.now().isoformat().split('.')[0], \
                  '  Please now inject the metadata to the EOS namespace'
        else:
            sys.exit(-1)
    except Exception, e:
        print e
        import traceback
        traceback.print_exc()
        sys.exit(-1)


if __name__ == '__main__':
    run()
