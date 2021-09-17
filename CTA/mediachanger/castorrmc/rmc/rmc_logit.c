/*
 * @project        The CERN Tape Archive (CTA)
 * @copyright      Copyright(C) 2001-2021 CERN
 * @license        This program is free software: you can redistribute it and/or modify
 *                 it under the terms of the GNU General Public License as published by
 *                 the Free Software Foundation, either version 3 of the License, or
 *                 (at your option) any later version.
 *
 *                 This program is distributed in the hope that it will be useful,
 *                 but WITHOUT ANY WARRANTY; without even the implied warranty of
 *                 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *                 GNU General Public License for more details.
 *
 *                 You should have received a copy of the GNU General Public License
 *                 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include "rmc_constants.h"
#include "rmc_logit.h"
extern int jid;

int rmc_logit(const char *const func, const char *const msg, ...)
{
	va_list args;
	char prtbuf[RMC_PRTBUFSZ];
	int save_errno;
	struct tm *tm;
	time_t current_time;
	int fd_log;

	save_errno = errno;
	va_start (args, msg);
	time (&current_time);		/* Get current time */
	tm = localtime (&current_time);
	sprintf (prtbuf, "%02d/%02d %02d:%02d:%02d %5d %s: ", tm->tm_mon+1,
		tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, jid, func);
	vsprintf (prtbuf+strlen(prtbuf), msg, args);
	va_end (args);
	fd_log = open("/var/log/cta/cta-rmcd.log", O_WRONLY | O_CREAT | O_APPEND, 0664);
        if (fd_log < 0) return -1;
	write (fd_log, prtbuf, strlen(prtbuf));
	close (fd_log);
	errno = save_errno;
	return (0);
}
