/*-------------------------------------------------------------------------
 *
 *	  UTF8 and Cyrillic
 *
 * Portions Copyright (c) 1996-2008, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/utils/mb/conversion_procs/utf8_and_cyrillic/utf8_and_cyrillic.c,v 1.21.2.1 2009/01/29 19:23:57 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "fmgr.h"
#include "mb/pg_wchar.h"
#include "../../Unicode/utf8_to_koi8r.map"
#include "../../Unicode/koi8r_to_utf8.map"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(utf8_to_koi8r);
PG_FUNCTION_INFO_V1(koi8r_to_utf8);

extern Datum utf8_to_koi8r(PG_FUNCTION_ARGS);
extern Datum koi8r_to_utf8(PG_FUNCTION_ARGS);

/* ----------
 * conv_proc(
 *		INTEGER,	-- source encoding id
 *		INTEGER,	-- destination encoding id
 *		CSTRING,	-- source string (null terminated C string)
 *		CSTRING,	-- destination string (null terminated C string)
 *		INTEGER		-- source string length
 * ) returns VOID;
 * ----------
 */

Datum
utf8_to_koi8r(PG_FUNCTION_ARGS)
{
	unsigned char *src = (unsigned char *) PG_GETARG_CSTRING(2);
	unsigned char *dest = (unsigned char *) PG_GETARG_CSTRING(3);
	int			len = PG_GETARG_INT32(4);

	CHECK_ENCODING_CONVERSION_ARGS(PG_UTF8, PG_KOI8R);

	UtfToLocal(src, dest, ULmapKOI8R, NULL,
			 sizeof(ULmapKOI8R) / sizeof(pg_utf_to_local), 0, PG_KOI8R, len);

	PG_RETURN_VOID();
}

Datum
koi8r_to_utf8(PG_FUNCTION_ARGS)
{
	unsigned char *src = (unsigned char *) PG_GETARG_CSTRING(2);
	unsigned char *dest = (unsigned char *) PG_GETARG_CSTRING(3);
	int			len = PG_GETARG_INT32(4);

	CHECK_ENCODING_CONVERSION_ARGS(PG_KOI8R, PG_UTF8);

	LocalToUtf(src, dest, LUmapKOI8R, NULL,
			 sizeof(LUmapKOI8R) / sizeof(pg_local_to_utf), 0, PG_KOI8R, len);

	PG_RETURN_VOID();
}
