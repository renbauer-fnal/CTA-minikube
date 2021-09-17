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

'''Command line tool to complete the current export from CASTOR to CTA'''

import sys
from time import sleep, time
from datetime import datetime
from threading import Thread

import castor_tools


def async_complete_export(conn):
    '''helper function to execute the PL/SQL procedure in a separate thread'''
    cur = conn.cursor()
    cur.execute('BEGIN completeCTAExport(); END;')


def run():
    '''main code'''
    try:
        # work out the tape pool being dealt with
        nsconn = castor_tools.connectToNS()
        nscur = nsconn.cursor()
        querylog = '''
          SELECT tapepool, timestamp, message FROM (
            SELECT * FROM CTAMigrationLog
             ORDER BY timestamp DESC)
          WHERE ROWNUM <= 1
        '''
        try:
            nscur.execute(querylog)
            rows = nscur.fetchall()
            if rows:
                tapepool, t, msg = rows[0]
                if 'Export from CASTOR fully completed' in msg:
                    raise ValueError
            else:
                raise ValueError
        except ValueError as e:
            print datetime.now().isoformat().split('.')[0], '  No ongoing files export to CTA, nothing to do'
            sys.exit(0)

        # now reconnect to the Nameserver and execute the complete_export procedure on a separate thread,
        # to be able to babysit it
        nsconn_async = castor_tools.connectToNS()
        runner = Thread(target=async_complete_export, args=[nsconn_async])
        runner.start()

        # poll the NS database for logs about the ongoing execution
        querylog = '''
          SELECT timestamp, message FROM CTAMigrationLog
           WHERE tapepool = :tapepool AND timestamp > :t ORDER BY timestamp ASC
        '''
        nscur = nsconn.cursor()
        lastprinttime = time()
        while True:
            nscur.execute(querylog, tapepool=tapepool, t=t)
            rows = nscur.fetchall()
            if rows:
                t = rows[-1][0]
                if 'Export from CASTOR fully completed' in rows[-1][1]:
                    print datetime.fromtimestamp(int(rows[-1][0])).isoformat(), ' ', rows[-1][1]
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
                if not rows:
                    print datetime.now().isoformat().split('.')[0], '  .'
            sleep(5)

        # that ought to be immediate now
        runner.join()

        # close DB connections
        castor_tools.disconnectDB(nsconn_async)
        castor_tools.disconnectDB(nsconn)

    except Exception as e:
        print(e)
        import traceback
        traceback.print_exc()
        sys.exit(-1)


if __name__ == '__main__':
    run()
