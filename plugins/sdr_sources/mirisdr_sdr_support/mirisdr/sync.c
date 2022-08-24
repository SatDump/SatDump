/*
 * Copyright (C) 2013 by Miroslav Slugen <thunder.m@email.cz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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

/* pouze pro bulk transfer a je třeba doplnit konverze formátů */
int mirisdr_read_sync (mirisdr_dev_t *p, void *buf, int len, int *n_read) {
    if (!p) goto failed;

    return libusb_bulk_transfer(p->dh, 0x81, buf, len, n_read, DEFAULT_BULK_TIMEOUT);

failed:
    return -1;
}
