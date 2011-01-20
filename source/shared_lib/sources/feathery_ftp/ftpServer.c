/* FEATHERY FTP-Server
 * Copyright (C) 2005-2010 Andreas Martin (andreas.martin@linuxmail.org)
 * <https://sourceforge.net/projects/feathery>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#if 0

#include "ftpIfc.h"
int main(void)
{
	ftpCreateAccount("anonymous", "", "./", FTP_ACC_RD | FTP_ACC_LS | FTP_ACC_DIR);
	ftpCreateAccount("nothing", "", "./", 0);
	ftpCreateAccount("reader", "", "./", FTP_ACC_RD);
	ftpCreateAccount("writer", "", "./", FTP_ACC_WR);
	ftpCreateAccount("lister", "", "./", FTP_ACC_LS);
    ftpCreateAccount("admin", "xxx", "./", FTP_ACC_RD | FTP_ACC_WR | FTP_ACC_LS | FTP_ACC_DIR);

	ftpStart();
	while(1)
		ftpExecute();
	ftpShutdown();

}

#endif
