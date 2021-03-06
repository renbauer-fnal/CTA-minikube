/*
 * @project        The CERN Tape Archive (CTA)
 * @copyright      Copyright(C) 2007-2021 CERN
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

#include "Csnprintf.h"

/* Hide the snprintf and al. call v.s. different OS. */
/* Sometimes a different name, sometimes do not exist */

int Csnprintf(char *str, size_t size, const char *format, ...) {
	int rc;
	va_list args;

	va_start (args, format);
	/* Note: we cannot call sprintf again, because a va_list is a real */
	/* parameter on its own - it cannot be changed to a real list of */
	/* parameters on the stack without being not portable */
	rc = Cvsnprintf(str,size,format,args);
	va_end (args);
	return(rc);
}

int Cvsnprintf(char *str, size_t size, const char *format, va_list args)
{
	return(vsnprintf(str, size, format, args));
}
