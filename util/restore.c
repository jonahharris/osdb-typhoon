/*----------------------------------------------------------------------------
 * File    : restore.c
 * Program : tybackup
 * OS      : UNIX, OS/2, DOS
 * Author  : Thomas B. Pedersen
 *
 * Copyright (c) 1994 Thomas B. Pedersen.  All rights reserved.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the above
 * copyright notice and the following two  paragraphs appear (1) in all 
 * source copies of this software and (2) in accompanying documentation
 * wherever the programatic interface of this software, or any derivative
 * of it, is described.
 *
 * IN NO EVENT SHALL THOMAS B. PEDERSEN BE LIABLE TO ANY PARTY FOR DIRECT,
 * INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT OF
 * THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF HE HAS BEEN 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THOMAS B. PEDERSEN SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" 
 * BASIS, AND THOMAS B. PEDERSEN HAS NO OBLIGATION TO PROVIDE MAINTENANCE,
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Description:
 *   Contains the tyrestore utility.
 *
 * Functions:
 *
 *--------------------------------------------------------------------------*/

static char rcsid[] = "$Id: restore.c,v 1.2 1999/10/03 23:36:49 kaz Exp $";

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <setjmp.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include "typhoon.h"
#include "ty_dbd.h"
#include "ty_type.h"
#include "ty_glob.h"
#include "ty_prot.h"
#include "ty_log.h"
#include "util.h"

/*------------------------------- Constants --------------------------------*/
#define VERSION			"1.00"
#define BLOCK_SIZE		512
#define OUTBUF_SIZE		(BLOCK_SIZE * 128)
#define INBUF_SIZE		(256 * 1024)

/*-------------------------- Function prototypes ---------------------------*/
static void 	SignalHandler			PRM( (int); )
static void		Read					PRM( (void *, ulong); )
static void		DisplayProgress			PRM( (char *, ulong); )
static void		RestoreDatabase			PRM( (void); )
static void		RestoreFile				PRM( (char *); )
static void		RestoreDbdFile			PRM( (void); )
static void		RestoreLogFile			PRM( (void); )
static void 	help					PRM( (void); )
       void		FixLog					PRM( (char *); )

/*---------------------------- Global variables ----------------------------*/
static ArchiveMediaHeader	mediahd;	/* Archive media header				*/
static Dbentry	dbd;					/* DBD anchor						*/
static int		archive_fh = -1;		/* Archive file handle				*/
static char		dbname[DBNAME_LEN+1];	/* Database name					*/
static char		*device = "/dev/null";	/* Archive device					*/
static ulong	curr_id;				/* Current block header id			*/
static char		*inbuf;					/* Input buffer						*/
static ulong	inreadpos;				/* Current read position in buffer	*/
static ulong	inbytes;				/* Number of bytes in outbuf		*/
static ulong	total_read = 0;			/* Total number of bytes read 		*/
       jmp_buf	err_jmpbuf;				/* Jumped to on error				*/
       char		*datapath = "";			/* Database file path				*/
       int		db_status;				/* Required by ../read_dbd.c		*/
       int		verbose = 0;			/* Be verbose?						*/



/*--------------------------------------------------------------------------*\
 *
 * Function  : SignalHandler
 *
 * Purpose   : 
 *
 * Parameters: 
 *
 * Returns   : 
 *
 */
static void SignalHandler(sig)
int sig;
{
	printf("signal %d\n", sig);

	if( sig == SIGSEGV )
		puts("Segmentation violation");
	else if( sig == SIGBUS )
		puts("Bus error");

	dbd.shm->restore_active = 0;
	shm_free(&dbd);
	d_close();
	unlink(LOG_FNAME);
	puts("Restore aborted.");
	exit(1);
}


/*--------------------------------------------------------------------------*\
 *
 * Function  : Read
 *
 * Purpose   : Read x bytes from the archive. If the data is not entirely
 *			   in the buffer, the rest will read immediately. If the end
 *			   of the current media has been reached, the user is promted
 *			   for the next media.
 *
 * Parameters: buf		- Buffer to write.
 *			   size		- Number of bytes in buffer.
 *
 * Returns   : Nothing.
 *
 */
static void Read(buf, size)
void *buf;
ulong size;
{
	char s[20];
	ulong copymax;
	long numread;


/*printf("Read: size=%d, inbytes=%ld, inreadpos=%ld\n",
	size, inbytes, inreadpos);*/

	/* Copy as much as possible to the output buffer */
	if( inbytes > inreadpos )
	{
		copymax = size;
		if( copymax > inbytes-inreadpos )
			copymax = inbytes-inreadpos;

		memcpy(buf, inbuf+inreadpos, copymax);
		inreadpos += copymax;

		if( copymax == size )
			return;
	}
	else
		copymax = 0;

	while( archive_fh == -1 )
	{
		clock_off();
		printf("Insert backup media no %d [enter]", mediahd.seqno);
		fflush(stdout);
		gets(s);
		
		if( s[0] == 'q' )
			longjmp(err_jmpbuf, 2);

		clock_on();			
		if( (archive_fh = open(device, O_RDONLY)) == -1 )
			printf("Cannot open '%s' (errno %d)\n", device, errno);
	}

	if( inreadpos == inbytes )
	{
		numread = read(archive_fh, inbuf, INBUF_SIZE);
/*printf("size=%ld, read=%ld, copymax=%ld, inreadpos=%ld, inbytes=%ld\n", 
	size, numread, copymax, inreadpos, inbytes);*/

		if( numread == -1 )
		{
			printf("Read error (errno %d)\n", errno);
			longjmp(err_jmpbuf, 1);
		}
		else if( numread != INBUF_SIZE )
		{
			/* The number of bytes read is less than requested. If the
			 * Read request can be satisfied by the number of bytes actually
			 * returned by read(), we assume that the end of the current
			 * media has been reached.
			 */
			if( numread + copymax < size )
			{
				printf("Read error (errno %d)\n", errno);
				longjmp(err_jmpbuf, 1);
			}

			mediahd.seqno++;
			close(archive_fh);
			archive_fh = -1;
		}

		/* Now copy the rest of the buffer */
		inbytes		= numread;
		inreadpos	= size - copymax;
		memcpy((char *)buf + copymax, inbuf, inreadpos);

		total_read += numread;

/*printf("size=%ld, read=%ld, copymax=%ld, inreadpos=%ld, inbytes=%ld\n", 
	size, numread, copymax, inreadpos, inbytes);*/
	}
}




/*--------------------------------------------------------------------------*\
 *
 * Function  : DisplayProgress
 *
 * Purpose   : Prints the number of bytes read so far from the current table.
 *
 * Parameters: table		- Table name.
 *			   bytes		- Number of bytes read.
 *
 * Returns   : Nothing.
 *
 */
static void DisplayProgress(table, bytes)
char *table;
ulong bytes;
{
	printf("\rWriting %-26.26s  %10s bytes", table, printlong(bytes));
	fflush(stdout);
}



/*--------------------------------------------------------------------------*\
 *
 * Function  : RestoreDatabase
 *
 * Purpose   : 
 *
 * Parameters: None.
 *
 * Returns   : Nothing.
 *
 */
static void RestoreDatabase()
{
	ArchiveTableHeader	tablehd;
	ArchiveRecordHeader	recordhd;
	ulong				prev_bytecount;
	ulong				bytecount;
	char				outbuf[OUTBUF_SIZE];
	char				fname[128];
	char				objname[128];
	int					fh;

	Read(&curr_id, sizeof curr_id);

	for( ;; )
	{
		if( curr_id != ARCHIVE_TABLE )
			break;
		
		Read(&tablehd.recsize, sizeof(tablehd) - sizeof(tablehd.id));

		sprintf(fname, "%s/%s", datapath, tablehd.fname);
		sprintf(objname, "table %s", tablehd.table);

		if( (fh = open(fname, O_WRONLY|O_TRUNC|O_CREAT|O_BINARY, CREATMASK)) == -1 )
		{
			printf("Cannot open file '%s'\n", fname);
			longjmp(err_jmpbuf, 1);
		}
		prev_bytecount = bytecount = 0;

		for( ;; )
		{
			Read(&curr_id, sizeof curr_id);
		
			if( curr_id != ARCHIVE_RECORD )
				break;
	
			Read(&recordhd.recid, sizeof(recordhd) - sizeof(recordhd.id));
			Read(outbuf, tablehd.recsize);

			lseek(fh, tablehd.recsize * recordhd.recno, SEEK_SET);
			write(fh, outbuf, tablehd.recsize);

			bytecount += tablehd.recsize;

			if( verbose && bytecount > prev_bytecount + 100000 )
			{
				prev_bytecount = bytecount;
				DisplayProgress(objname, bytecount);
			}
		}

		if( verbose )
		{
			DisplayProgress(objname, bytecount);
			puts("");
		}
		
		
		close(fh);
	}
}



/*--------------------------------------------------------------------------*\
 *
 * Function  : RestoreFile
 *
 * Purpose   : Read a sequential file from archive.
 *
 * Parameters: fname	- File name.
 *
 * Returns   : Nothing.
 *
 */
static void RestoreFile(fname)
char *fname;
{
	ArchiveFileDataHeader datahd;
	int					fh;
	char				buf[OUTBUF_SIZE];
	ulong				numread;
	ulong				readmax;
	ulong				bytecount=0;

	if( (fh = open(fname, O_WRONLY|O_TRUNC|O_CREAT, CREATMASK)) == -1 )
	{
		printf("Cannot open '%s'\n", fname);
		return;
	}		

	for( ;; )
	{
		Read(&curr_id, sizeof curr_id);

		if( curr_id != ARCHIVE_FILEDATA )
		{
			printf("Unexpected header id %d in middle of file \n", curr_id);
			longjmp(err_jmpbuf, 1);
		}		

		Read(&datahd.size, sizeof(datahd) - sizeof(datahd.size));

		if( !datahd.size )						/* End-of-file reached		*/
			break;

		while( datahd.size > 0 )
		{
			/* Determine number of bytes to read */
			readmax = sizeof buf;
			if( readmax > datahd.size )
				readmax = datahd.size;

			Read(buf, readmax);
			write(fh, buf, readmax);

			bytecount += readmax;

			if( verbose )
				DisplayProgress(fname, bytecount);
		
			datahd.size -= readmax;
		}
	}

	if( verbose )
		puts("");

	close(fh);
}


/*--------------------------------------------------------------------------*\
 *
 * Function  : RestoreDbdFile
 *
 * Purpose   : Restores the dbd-file from the archive.
 *
 * Parameters: None.
 *
 * Returns   : Nothing.
 *
 */
static void RestoreDbdFile()
{
	ArchiveFileHeader	filehd;

	/* Restore dbd-file */	
	Read(&filehd, sizeof filehd);
	
	if( filehd.id != ARCHIVE_FILE )
	{
		printf("Unexpected header id %d\n", filehd.id);
		longjmp(err_jmpbuf, 1);
	}

	RestoreFile(filehd.fname);
}


/*--------------------------------------------------------------------------*\
 *
 * Function  : RestoreLogFile
 *
 * Purpose   : Restores the log file from the archive.
 *
 * Parameters: None.
 *
 * Returns   : Nothing.
 *
 */
static void RestoreLogFile()
{
	ArchiveFileHeader filehd;

	if( curr_id == ARCHIVE_END )
		return;

	/* Restore the database */
	if( curr_id != ARCHIVE_FILE )
	{
		printf("Unexpected header id %ld\n", curr_id);
		longjmp(err_jmpbuf, 1);
	}	

	Read(filehd.fname, sizeof(filehd) - sizeof(filehd.id));

	RestoreFile(filehd.fname);
}



static void VerboseFn(table, records, curr_rec)
char *table;
ulong records, curr_rec;
{
	static int old_percent = 0;
	int percent = curr_rec * 100 / records;

	if( curr_rec == 0 )
	{
		printf("\nRebuilding %-32.32s    0%%", table);
		fflush(stdout);
		old_percent = 0;
	}
	else if( percent != old_percent )
	{
		printf("\b\b\b\b%3d%%", percent);
		fflush(stdout);
		old_percent = percent;
	}
}



static void help()
{
	puts("Syntax: tyrestore [option]...\n"
		 "Options:\n"
		 "    -d<device>      Backup device\n"
		 "    -f<path>        Path for data files\n"
		 "    -v              Be verbose\n");
	exit(1);
}



main(argc, argv)
int		argc;
char 	*argv[];
{
	char				dbdname[20];
	int					i;
	struct tm			*tm;

	/* The input buffer MUST be bigger than the output buffer */
	assert(OUTBUF_SIZE < INBUF_SIZE);

	printf("Typhoon Restore version %s\n", VERSION);

	if( argc < 2 )
		help();

	for( i=1; i<argc; i++ )
	{
		switch( *argv[i] )
		{
			case '-':
			case '/':
				switch( argv[i][1] )
				{
					case 'd':
						device = argv[i]+2;
						break;
					case 'f':
						datapath = argv[i]+2;
						break;
					case 'v':
						verbose = 1;
						break;
					default:
						printf("Invalid option '%c'\n", argv[i][1]);
						break;
				}
				break;
			default:
				printf("Invalid switch '%c'\n", *argv[i]);
				break;
		}
	}

	if( *datapath  )
	{
		mkdir(datapath);
		chmod(datapath, 0777);
	}
	
	if( !(inbuf = (char *)malloc(INBUF_SIZE)) )
	{
		puts("Cannot allocate output buffer");
		return;
	}
	inbytes = 0;	
	inreadpos = 0;

	if( setjmp(err_jmpbuf) )
	{
		if( verbose )
			puts("Restore aborted.");
		goto out;
	}

	if( verbose )
		printf("Restoring from %s\n", device);

	Read(&mediahd, sizeof mediahd);
	if( mediahd.id != ARCHIVE_MEDIA )
	{
		printf("Unexpected header id %d\n", mediahd.id);
		longjmp(err_jmpbuf, 1);
	}

	strcpy(dbd.name, mediahd.dbname);
	if( shm_alloc(&dbd) == -1 )
	{
		printf("Cannot allocate shared memory");
		return;
	}
	dbd.shm->restore_active = 1;

	signal(SIGINT,	SignalHandler);
	signal(SIGTERM, SignalHandler);
	signal(SIGQUIT, SignalHandler);
	signal(SIGSEGV, SignalHandler);
	signal(SIGBUS,  SignalHandler);
	signal(SIGHUP, SIG_IGN);

	if( dbd.shm->use_count > 1 )
	{
		puts("The database is currenly in use. Cannot restore");
		goto out;
	}

	/* Confirm restore */
	clock_off();
	tm = localtime(&mediahd.date);
	printf("Database '%s' from %s", mediahd.dbname, asctime(tm));
	printf("Restore? [y/n]: ");
	fflush(stdout);
	if( getchar() != 'y' )
		goto out;
	clock_on();

	RestoreDbdFile();
	RestoreDatabase();
	RestoreLogFile();
	clock_off();

	if( verbose )
	{
		ulong secs = clock_secs();
	
		/* Guard against division by zero */
		if( !secs )
			secs = 1;

		printf("\rTotal %40s bytes\n", printlong(total_read));
		printf("%s bytes/second\n", printlong(total_read / secs));
	}

	FixLog(mediahd.dbname);

	printf("Rebuilding index files");
	d_keybuild(VerboseFn);
	d_dbfpath(datapath);

	if( d_open(mediahd.dbname, "o") != S_OKAY )
		puts("Cannot open database");
	else
	{
		d_close();
		puts("");
	}

out:
	free(inbuf);
	dbd.shm->restore_active = 0;
	shm_free(&dbd);
	free(dbd.dbd);
	unlink(LOG_FNAME);

	return 0;
}

/* end-of-file */
