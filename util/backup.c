/*----------------------------------------------------------------------------
 * File    : backup.c
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
 *   Contains the tybackup utility.
 *
 * Functions:
 *
 *--------------------------------------------------------------------------*/

static char rcsid[] = "$Id: backup.c,v 1.2 1999/10/03 23:36:49 kaz Exp $";

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
#define OUTBUF_SIZE		(BLOCK_SIZE * 512)
#define INBUF_SIZE		(64 * 1024)

/*-------------------------- Function prototypes ---------------------------*/
static void		SignalHandler			PRM( (int); )
static void		Write					PRM( (void *, ulong); )
static void		Flush					PRM( (void); )
static void		DisplayProgress			PRM( (char *, ulong); )
static void		BackupDatabase			PRM( (void); )
static void 	BackupFile				PRM( (char *); )
static void		help					PRM( (void); )

/*---------------------------- Global variables ----------------------------*/
static ArchiveMediaHeader	mediahd;	/* Archive media header				*/
static jmp_buf	err_jmpbuf;				/* Jumped to on error				*/
static Dbentry	dbd;					/* DBD anchor						*/
static int		verbose = 0;			/* Be verbose?						*/
static int		archive_fh = -1;		/* Archive file handle				*/
static char		*dbname = "";			/* Database name					*/
static char		*datapath = "";			/* Path for database files			*/
static char		*device = "/dev/null";	/* Backup device					*/
static char		*outbuf;				/* Output buffer					*/
static ulong	outbytes;				/* Number of bytes in outbuf		*/
static ulong	total_written = 0;		/* Total number of bytes written	*/
	   int		db_status;				/* Required by ../read_dbd.c		*/


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
	if( sig == SIGSEGV )
		puts("Segmentation violation");
	else if( sig == SIGBUS )
		puts("Bus error");

	dbd.shm->backup_active = 0;
	shm_free(&dbd);
	unlink(LOG_FNAME);
	puts("Restore aborted.");
	exit(1);
}



/*--------------------------------------------------------------------------*\
 *
 * Function  : Write
 *
 * Purpose   : Write a block of data to the archive. If data cannot be
 *			   written because the backup media is full, the user will
 *			   be prompted to insert a new media.
 *
 * Parameters: buf		- Buffer to write.
 *			   size		- Number of bytes in buffer.
 *
 * Returns   : Nothing.
 *
 */
static void Write(buf, size)
void *buf;
ulong size;
{
	ArchiveBlockHeader	blockhd;
	char				s[20];
	ulong				copymax;
	long				rc;

	/* Copy as much as possible to the output buffer */
	copymax = size;
	if( copymax > OUTBUF_SIZE-outbytes )
		copymax = OUTBUF_SIZE-outbytes;

	memcpy(outbuf+outbytes, buf, copymax);
	outbytes += copymax;

retry:
	while( archive_fh == -1 )
	{
		clock_off();
		printf("Insert backup media no %d [enter, q]", mediahd.seqno);
		fflush(stdout);
		gets(s);
		clock_on();
		
		if( s[0] == 'q' )
			return;
			
		if( (archive_fh = open(device, O_WRONLY)) == -1 )
			printf("Cannot open '%s' (errno %d)\n", device, errno);
	}

	if( outbytes == OUTBUF_SIZE )
	{
		rc = write(archive_fh, outbuf, OUTBUF_SIZE);
		
		if( rc == -1 )
		{
			printf("Write error (errno %d)\n", errno);
			longjmp(err_jmpbuf, 1);
		}
		else if( rc != OUTBUF_SIZE )
		{
			close(archive_fh);
			goto retry;
		}

		/* Now copy the rest of the buffer */
		outbytes = size - copymax;
		memcpy(outbuf, (char *)buf + copymax, outbytes);

		total_written += rc;
	}
}


static void Flush()
{
	long rc;

	/* Ensure that a complete block is written to archive */
	outbytes += (BLOCK_SIZE - (outbytes % BLOCK_SIZE));

	if( (rc = write(archive_fh, outbuf, outbytes)) != outbytes )
	{
		printf("Write error (errno %d, rc %ld)\n", errno, rc);
		longjmp(err_jmpbuf, 1);
	}

	total_written += rc;
	outbytes = 0;
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
	printf("\rReading %-26.26s  %10s bytes", table, printlong(bytes));
	fflush(stdout);
}


/*--------------------------------------------------------------------------*\
 *
 * Function  : BackupDatabase
 *
 * Purpose   : Controls the backup.
 *
 * Parameters: None.
 *
 * Returns   : Nothing.
 *
 */
static void BackupDatabase()
{
	ArchiveRecordHeader recordhd;
	ArchiveTableHeader	tablehd;
	Record		*rec;
	ulong		recid;				/* Record ID of current table			*/
	ulong		recno;				/* Current record in current table		*/
	ulong		recsize;			/* Record size in current table			*/
	ulong		bytecount;			/* Number of bytes read in current table*/
	ulong		numread;			/* Number of bytes read					*/
	ulong		timeout;
	int			fh;
	int			maxread, i, rc;
	char		fname[128], objname[128];
	char		buf[INBUF_SIZE];
	RECORD		filehd;
	
	dbd.shm->backup_active = 1;

	for( rec=dbd.record, recid=0; recid<dbd.header.records; recid++, rec++ )
	{
		dbd.shm->curr_recid = recid;

		sprintf(fname, "%s/%s", datapath, dbd.file[rec->fileid].name);
		sprintf(objname, "table %s", rec->name);
		
		if( (fh = os_open(fname, O_RDONLY|O_BINARY, 0)) == -1 )
		{
			printf("Cannot open '%s'\n", fname);
			return;
		}

		/* Because of referential integrity data, the record size may
		 * be different from rec->size, so we read the header for H.recsize.
		 */
		read(fh, &filehd.H, sizeof filehd.H);

		recsize		= filehd.H.recsize;
		maxread		= INBUF_SIZE / recsize;
		bytecount	= 0;
		recno		= 0;

		/* Write table header */
		strcpy(tablehd.fname, dbd.file[rec->fileid].name);
		strcpy(tablehd.table, rec->name);
		tablehd.id		= ARCHIVE_TABLE;
		tablehd.recsize	= recsize;
		Write(&tablehd, sizeof tablehd);

		if( verbose )
			DisplayProgress(objname, 0);

		do
		{
			for( i=0; i<maxread; i++ )
			{
				dbd.shm->curr_recno = recno;

				lseek(fh, recno * recsize, SEEK_SET);
				numread = read(fh, buf, recsize);

				if( numread != recsize )
					break;

				recordhd.id		= ARCHIVE_RECORD;
				recordhd.recid	= recid;
				recordhd.recno	= recno;

				Write(&recordhd, sizeof recordhd);
				Write(buf, numread);

				bytecount += recsize;
				recno++;
			}			

			if( verbose )
				DisplayProgress(objname, bytecount);
		}
		while( i == maxread );

		if( verbose )
			puts("");

		close(fh);
	}

	/* Set curr_recid to max value, so that all changes are logged */
	dbd.shm->curr_recid = 0xffffffff;
	dbd.shm->backup_active = 0;
	
	/* Wait for last transaction to complete */
	timeout = 120;
	while( dbd.shm->num_trans_active > 0 && timeout-- )
		sleep(1);
}



/*--------------------------------------------------------------------------*\
 *
 * Function  : BackupFile
 *
 * Purpose   : Write a sequential file to archive.
 *
 * Parameters: fname	- File name.
 *
 * Returns   : Nothing.
 *
 */
static void BackupFile(fname)
char *fname;
{
	ArchiveFileHeader	filehd;
	ArchiveFileDataHeader datahd;
	int					fh;
	char				buf[INBUF_SIZE];
	ulong				numread;
	ulong				bytecount=0;

	if( (fh = open(fname, O_RDONLY)) == -1 )
	{
		printf("Cannot open '%s'\n", fname);
		return;
	}		

	datahd.id = ARCHIVE_FILEDATA;
	filehd.id = ARCHIVE_FILE;
	strcpy(filehd.fname, fname);
	Write(&filehd, sizeof filehd);

	while( numread = read(fh, buf, INBUF_SIZE) )
	{
		datahd.size = numread;
		Write(&datahd, sizeof datahd);
		Write(buf, numread);

		bytecount += numread;

		if( verbose )
			DisplayProgress(fname, bytecount);
	}

	datahd.size = 0;
	Write(&datahd, sizeof datahd);

	if( verbose )
		puts("");

	close(fh);
}




static void help()
{
	puts("Syntax: tybackup database [option]...\n"
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
	ArchiveEnd			end;
	char				dbdname[20];
	int					i;

	/* The output buffer MUST be bigger than the input buffer */
	assert(INBUF_SIZE < OUTBUF_SIZE);

	printf("Typhoon Online Backup version %s\n", VERSION);

	if( argc < 3 )
		help();

	for( i=2; i<argc; i++ )
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
				exit(1);
		}
	}

	dbname = argv[1];
	sprintf(dbdname, "%s.dbd", dbname);
	if( read_dbdfile(&dbd, dbdname) != S_OKAY )
	{
		printf("Invalid dbd-file '%s'\n", dbdname);
		return;
	}

	if( !(outbuf = (char *)malloc(OUTBUF_SIZE)) )
	{
		puts("Cannot allocate output buffer");
		return;
	}
	outbytes = 0;	
	
	strcpy(dbd.name, dbname);
	if( shm_alloc(&dbd) == -1 )
	{
		printf("Cannot allocate shared memory");
		return;
	}

	signal(SIGINT,	SignalHandler);
	signal(SIGTERM, SignalHandler);
	signal(SIGQUIT, SignalHandler);
	signal(SIGSEGV, SignalHandler);
	signal(SIGBUS,  SignalHandler);
	signal(SIGHUP, SIG_IGN);

	if( setjmp(err_jmpbuf) )
	{
		if( verbose )
			puts("Backup aborted.");
		goto out;
	}

	/* Initialize media header */
	memset(&mediahd, 0, sizeof mediahd);
	strcpy(mediahd.dbname, dbname);
	mediahd.id		= ARCHIVE_MEDIA;
	mediahd.date	= time(NULL);
	mediahd.seqno	= 1;

	if( verbose )
		printf("Backing up to %s\n", device);

	Write(&mediahd, sizeof mediahd);

	BackupFile(dbdname);
	BackupDatabase();
	BackupFile(LOG_FNAME);

	end.id = ARCHIVE_END;
	Write(&end, sizeof end);
	Flush();
	close(archive_fh);
	clock_off();

	if( verbose )
	{
		ulong secs = clock_secs();
	
		/* Guard against division by zero */
		if( !secs )
			secs = 1;
	
		printf("\rTotal %40s bytes\n", printlong(total_written));
		printf("%s bytes/second\n", printlong(total_written / secs));
		puts("Done.");
	}

out:
	free(outbuf);
	shm_free(&dbd);
	free(dbd.dbd);

	unlink(LOG_FNAME);

	return 0;
}

/* end-of-file */
