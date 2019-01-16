/*
 * pg2arrow.h
 *
 * common header file
 */
#ifndef PG2ARROW_H
#define PG2ARROW_H

#include "postgres.h"
#include "access/htup_details.h"
#include "datatype/timestamp.h"
#include "utils/date.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include <libpq-fe.h>
#include "arrow_defs.h"

#define	ARROWALIGN(LEN)		TYPEALIGN(64, (LEN))

typedef struct SQLbuffer		SQLbuffer;
typedef struct SQLtable			SQLtable;
typedef struct SQLattribute		SQLattribute;

struct SQLbuffer
{
	char	   *ptr;
	uint32		usage;
	uint32		length;
};

struct SQLattribute
{
	char	   *attname;
	Oid			atttypid;
	int			atttypmod;
	short		attlen;
	bool		attbyval;
	uint8		attalign;		/* 1, 2, 4 or 8 */
	char		typtype;		/* pg_type.typtype */
	SQLtable   *subtypes;		/* valid, if composite type */
	SQLattribute *elemtype;		/* valid, if array type */
	ArrowType	arrow_type;		/* one of ArrowTypeXXXX */
	/* data buffer */
	size_t (*put_value)(SQLattribute *attr, int row_index,
						const char *addr, int sz);
	void   (*stat_update)(SQLattribute *attr,
						  const char *addr, int sz);
	long		nullcount;		/* number of null values */
	SQLbuffer	nullmap;		/* null bitmap */
	SQLbuffer	values;			/* main storage of values */
	SQLbuffer	extra;			/* extra buffer for varlena */
	/* statistics */
	bool		min_isnull;
	bool		max_isnull;
	Datum		min_value;
	Datum		max_value;
};

struct SQLtable
{
	const char *filename;		/* output filename */
	int			fdesc;			/* output file descriptor */
	size_t		segment_sz;		/* threshold of the memory usage */
	size_t		nitems;			/* current number of rows */
	int			nfields;		/* number of attributes */
	SQLattribute attrs[1];		/* flexible length */
};

/* buffer */
extern SQLtable	   *pgsql_create_buffer(PGconn *conn, PGresult *res,
								size_t segment_sz);
extern void			pgsql_append_results(SQLtable *table, PGresult *res);
extern void 		pgsql_writeout_buffer(SQLtable *table);
extern void			pgsql_dump_buffer(SQLtable *table);

/* arrow_write.c */
extern void			writeArrowRecordBatch(SQLtable *table);
extern void			writeArrowSchema(SQLtable *table);
extern void			writeArrowFooter(SQLtable *table);

/* arrow_read.c */
extern void			readArrowFile(const char *pathname);
/* arrow_dump.c */
extern void			dumpArrowNode(ArrowNode *node, FILE *out);

/*
 * Error message and exit
 */
#define Elog(fmt, ...)								\
	do {											\
		fprintf(stderr,"L%d: " fmt "\n", __LINE__, ##__VA_ARGS__);	\
		exit(1);									\
	} while(0)

/*
 * Memory operations
 */
static inline void *
pg_malloc(size_t sz)
{
	void   *ptr = malloc(sz);

	if (!ptr)
		Elog("out of memory");
	return ptr;
}

static inline void *
pg_zalloc(size_t sz)
{
	void   *ptr = malloc(sz);

	if (!ptr)
		Elog("out of memory");
	memset(ptr, 0, sz);
	return ptr;
}

static inline void *
pg_realloc(void *ptr, size_t sz)
{
	void   *result = realloc(ptr, sz);

	if (!result)
		Elog("out of memory");
	return result;
}

static inline char *
pg_strdup(const char *str)
{
	char	   *temp = strdup(str);

	if (!temp)
		Elog("out of memory");
	return temp;
}

static inline void
pg_free(void *ptr)
{
	free(ptr);
}

#endif	/* PG2ARROW_H */
