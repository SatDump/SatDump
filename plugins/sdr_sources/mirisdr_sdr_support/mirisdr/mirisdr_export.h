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

#ifndef __MIRISDR_EXPORT_H
#define __MIRISDR_EXPORT_H

#if defined __GNUC__
#  if __GNUC__ >= 4
#    define __SDR_EXPORT   __attribute__((visibility("default")))
#    define __SDR_IMPORT   __attribute__((visibility("default")))
#  else
#    define __SDR_EXPORT
#    define __SDR_IMPORT
#  endif
#else
#  if defined _MSC_VER
#    define __SDR_EXPORT     __declspec(dllexport)
#    define __SDR_IMPORT     __declspec(dllimport)
#  else
#    define __SDR_EXPORT
#    define __SDR_IMPORT
#  endif
#endif

#ifndef mirisdr_STATIC
#  ifdef mirisdr_EXPORTS
#    define MIRISDR_API __SDR_EXPORT
#  else
#    define MIRISDR_API __SDR_IMPORT
#  endif
#else
#  define MIRISDR_API
#endif
#endif /* __MIRISDR_EXPORT_H */
