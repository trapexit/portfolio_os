/*****
$Id: lmfs.c,v 1.12 1994/08/05 21:41:59 shawn Exp $

$Log: lmfs.c,v $
 * Revision 1.12  1994/08/05  21:41:59  shawn
 * Added log recording for rcs.
 *
*****/

/*
 *	Copyright The 3DO Company Inc., 1993
 *	All Rights Reserved Worldwide.
 *	Company confidential and proprietary.
 *	Contains unpublished technical data.
 */

/*
 *	lmfs.c - create/delete files on the nvram.
 */

#include "types.h"
#include "item.h"
#include "mem.h"
#include "nodes.h"
#include "debug.h"
#include "list.h"
#include "device.h"
#include "driver.h"
#include "msgport.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "io.h"
#include "discdata.h"
#include "filesystem.h"
#include "filesystemdefs.h"
#include "operror.h"
#include "time.h"

#include "filefunctions.h"
#include "stdio.h"
#include "strings.h"

#define	DEBUG		1
#define	BUFSZ		64
#define	DIR_NAME	"/nvram"
#define	DEV_NAME	"ram"
#define	DEV_UNIT	3
#define	DEV_OFFSET	0

#define	PRNT_LINE_LEN	40

typedef struct {
        DeviceStatus    dp_devstat;
        Item            dp_item;
        Item            dp_ioreqi;
        IOReq           *dp_ioreq;
        IOInfo          dp_ioinfo;
} dev_prop_t;
dev_prop_t     	devp;

char	buf[BUFSZ];
char	*pname;

void	usage(void);
void	create_file(int cnt, char *vec[], int flg);
void	set_eof(int cnt, char *vec[]);
void	corrupt_fs(int cnt, char *vec[], char flg);
void	rawdevio(int cnt, char *vec[], char flg);
void	rdwr_file(int cnt, char *vec[], char flg);
void	delete_file(int cnt, char *vec[]);
void	getfs_stat(int cnt, char *vec[]);
void	do_create_file(char *pathnm, int fsize, int flg);
void	do_set_eof(char *pathnm, int fsize);
void	do_delete_file(char *pathnm);
void	do_rawdevio(uint32 offset, uint32 size, char flg, uint32 val);
void	do_rdwr_file(char *pathnm, uint32 fsize, char flg, char val);
void	mumount(int cnt, char flg);
#ifdef	DEBUG
void	prnt_ioinf(IOInfo *ioinf);
#endif	/* DEBUG */
int	chknum(char *s);
int	rawio(uint8 cmd, int32 offset, int32 size, char *buf, char *errmsg);
char	*mkpathnm(char *fname);

int
main (ac, av)
int	ac;
char	*av[];
{
	char *flg;
	
	pname = av[0];
	av++, ac--;

	if (ac < 1) {
		printf("%s: Invalid number of arguments\n", pname);
		usage();
		return -1;
	}
	flg = *av;
	if (*flg != '-') {
		printf("%s: First argument must be a flag %c\n", pname, *flg);
		usage();
		return -1;
	}

	switch (flg[1]) {
	case 'B':
	case 'C':
	case 'F':
	case 'P':
	case 'Y':
	case 'Z':
		ac--; 
		av++;
		corrupt_fs(ac, av, flg[1]);
		break;
	case 'R':
	case 'W':
		ac--; 
		av++;
		rawdevio(ac, av, flg[1]);
		break;
	case 'a':
		ac--; 
		av++;
		create_file(ac, av, 0);
		break;
	case 'c':
		ac--; 
		av++;
		create_file(ac, av, 1);
		break;
	case 'd':
		ac--; 
		av++;
		delete_file(ac, av);
		break;
	case 'e':
		ac--; 
		av++;
		set_eof(ac, av);
		break;
	case 'r':
	case 'w':
		ac--; 
		av++;
		rdwr_file(ac, av, flg[1]);
		break;
	case 's':
		ac--; 
		av++;
		getfs_stat(ac, av);
		break;
	case 'm':
	case 't':
	case 'u':
		ac--; 
		av++;
		mumount(ac, flg[1]);
		break;
	default:
		printf("%s: Invalid flag %c\n", pname, flg[1]);
		usage();
		return -1;
	}
	return 0;
}


/*
 *	simple hah?
 */
void
usage(void)
{
	printf("USAGE: lmfs [-a|c file_name size] [-d file_name] ");
	printf("[-e file_name size] [-m]\n            [-r file_name nbytes] [-s file_name] ");
	printf("[-t] [-u] [-w file_name nbytes val]\n            ");
	printf("[-B blknum val] [-C blknum val] [-F blknum val]\n            ");
	printf("[-H blknum val] [-P blknum val] [-R offset size]\n            ");
	printf("[-W offset size val] [-Y blknum val]\n\n");
	printf("            -a:\tAllocate size bytes for file_name\n");
	printf("            -c:\tCreate file_name of size bytes long\n");
	printf("            -d:\tDetele file_name\n");
	printf("            -e:\tset Eof for file_name to size\n");
	printf("            -m:\tMount nvram\n");
	printf("            -r:\tread nbytes from file_name \n");
	printf("            -s:\tget Status of filesystem of file_name \n");
	printf("            -t:\tTest if nvram is mounted\n");
	printf("            -u:\tUnmount nvram\n");
	printf("            -w:\twrite nbytes of val to file_name \n");
	printf("            -B:\tset BlinkOffset of blknum to val\n");
	printf("            -C:\tset BlockCount of blknum to val\n");
	printf("            -F:\tset FlinkOffset of blknum to val\n");
	printf("            -H:\tset HeaderBlockCount of blknum to val\n");
	printf("            -P:\tset fingerPrint of blknum to val\n");
	printf("            -R:\tRead size bytes from raw device at offset\n");
	printf("            -W:\tWrite size bytes of val to raw device at offset\n");
	printf("            -Y:\tset bYteCount of file at blknum to val\n");
	printf("            -Z:\tZap blknum\n");
}




/*
 *	set eof for file_name to seize specified.
 */
void
set_eof(cnt, vec)
int	cnt;
char	*vec[];
{
	char	*fname;
	int	fsize;

	if (cnt != 2) {
		printf("%s: set_eof: Invalid number of arguments, %d\n",
			 pname, cnt);
		usage();
		return;
	}
	fname = *vec;
	if (chknum(vec[1])) {
		printf("%s: set_eof: Invalid file size, %s\n",
			 pname, vec[1]);
		return;
	}
	fsize = (int) strtol(vec[1], NULL, 0);
	if (fsize <= 0) {
		printf("%s: set_eof: Invalid file size, %s\n",
			 pname, vec[1]);
		return;
	}

	do_set_eof(mkpathnm(fname), fsize);
}

/*
 *	the real set eof
 */
void
do_set_eof(pathnm, fsize)
char	*pathnm;
int	fsize;
{
	Err	err;
	Item	ofi, iori;
	IOReq	*ior;
	IOInfo	ioinf;

	if ((ofi = OpenDiskFile(pathnm)) < 0) {
		PrintError(0,"open",pathnm,ofi);
 		return;
	}
	if ((iori = CreateIOReq(NULL, 50, ofi, 0)) < 0) {
		PrintError(0,"create IOReq",0,iori);
		CloseDiskFile(ofi);
 		return;
	}

	ior = (IOReq *) LookupItem(iori);
	memset(&ioinf, 0, sizeof ioinf);

	ioinf.ioi_Command = FILECMD_SETEOF;
	ioinf.ioi_Recv.iob_Buffer = NULL;
	ioinf.ioi_Recv.iob_Len = 0;
	ioinf.ioi_Offset = fsize;
	DoIO(iori, &ioinf);
	if ((err = ior->io_Error) < 0) {
		PrintError(0,"set eof",0,err);
		CloseDiskFile(ofi);
 		return;
	}
	CloseDiskFile(ofi);
}


/*
 *	create/allocate blocks for a file in the filesystem
 */
void
create_file(cnt, vec, flg)
int	cnt;
char	*vec[];
int	flg;
{
	char	*fname;
	int	fsize;

	if (cnt != 2) {
		printf("%s: create_file: Invalid number of arguments, %d\n",
			 pname, cnt);
		usage();
		return;
	}
	fname = *vec;
	if (chknum(vec[1])) {
		printf("%s: create_file: Invalid file size, %s\n",
			 pname, vec[1]);
		return;
	}
	fsize = (int) strtol(vec[1], NULL, 0);
	if (fsize < 0) {
		printf("%s: create_file: Invalid file size, %s\n",
			 pname, vec[1]);
		return;
	}

	do_create_file(mkpathnm(fname), fsize, flg);
}


/*
 *	the real create file
 */
void
do_create_file(pathnm, fsize, flg)
char	*pathnm;
int	fsize;
int	flg;
{
	Err	err;
	Item	ofi, iori;
	IOReq	*ior;
	IOInfo	ioinf;

	if (flg) {
		if ((err = CreateFile(pathnm)) < 0) {
			PrintError(0,"create",pathnm,err);
                	return;
		}
        }

	if (fsize == 0)		/* for zero size file, no alloc block needed */
		return;
	if ((ofi = OpenDiskFile(pathnm)) < 0) {
		PrintError(0,"open",pathnm,ofi);
 		return;
	}
	if ((iori = CreateIOReq(NULL, 50, ofi, 0)) < 0) {
		PrintError(0,"create IOReq",0,iori);
		CloseDiskFile(ofi);
 		return;
	}

	ior = (IOReq *) LookupItem(iori);
	memset(&ioinf, 0, sizeof ioinf);

	ioinf.ioi_Command = FILECMD_ALLOCBLOCKS;
	ioinf.ioi_Recv.iob_Buffer = NULL;
	ioinf.ioi_Recv.iob_Len = 0;
	ioinf.ioi_Offset = fsize;
	DoIO(iori, &ioinf);
	if ((err = ior->io_Error) < 0) {
		PrintError(0,"allocate blocks",0,err);
		CloseDiskFile(ofi);
 		return;
	}

	CloseDiskFile(ofi);
}


/*
 *	get status of the filesystem file_name resides on
 */
void
getfs_stat(cnt, vec)
int	cnt;
char	*vec[];
{
	Err	err;
	Item	ofi, iori;
	IOReq	*ior;
	IOInfo	ioinf;
	FileSystemStat	fstat;

	if (cnt != 1) {
		printf("%s: getfs_stat: Invalid number of arguments, %d\n",
			 pname, cnt);
		usage();
		return;
	}

	if ((ofi = OpenDiskFile(*vec)) < 0) {
		PrintError(0,"open",*vec,ofi);
 		return;
	}
	if ((iori = CreateIOReq(NULL, 50, ofi, 0)) < 0) {
		PrintError(0,"create IOReq",0,iori);
		CloseDiskFile(ofi);
 		return;
	}

	ior = (IOReq *) LookupItem(iori);
	memset(&ioinf, 0, sizeof ioinf);

	ioinf.ioi_Command = FILECMD_FSSTAT;
	ioinf.ioi_Recv.iob_Buffer = &fstat;
	ioinf.ioi_Recv.iob_Len = sizeof(FileSystemStat);
	DoIO(iori, &ioinf);
	if ((err = ior->io_Error) < 0) {
		PrintError(0,"get file system status",0,ofi);
		CloseDiskFile(ofi);
 		return;
	}
	printf("BitMap: 0x%x, CreateTime: %d, BlockSize: %d\n",
		fstat.fst_BitMap, fstat.fst_CreateTime,
		fstat.fst_BlockSize);
	printf("Size: %d, MaxFileSize: %d, Free: %d, Used: %d\n",
		fstat.fst_Size, fstat.fst_MaxFileSize,
		fstat.fst_Free, fstat.fst_Used);

	if (FSSTAT_ISSET(fstat.fst_BitMap, FSSTAT_CREATETIME))
		printf("CreateTime ");
	if (FSSTAT_ISSET(fstat.fst_BitMap, FSSTAT_BLOCKSIZE))
		printf("BlockSize ");
	if (FSSTAT_ISSET(fstat.fst_BitMap, FSSTAT_SIZE))
		printf("Size ");
	if (FSSTAT_ISSET(fstat.fst_BitMap, FSSTAT_MAXFILESIZE))
		printf("MaxFileSize ");
	if (FSSTAT_ISSET(fstat.fst_BitMap, FSSTAT_FREE))
		printf("Free ");
	if (FSSTAT_ISSET(fstat.fst_BitMap, FSSTAT_USED))
		printf("Used ");

	printf("\n");

	CloseDiskFile(ofi);
}


/*
 *	delete a file from filesystem
 */
void
delete_file(cnt, vec)
int	cnt;
char	*vec[];
{
	char *fname;
	if (cnt != 1) {
		printf("%s: delete_file: Invalid number of arguments, %d\n",
			 pname, cnt);
		usage();
		return;
	}
	fname = *vec;
	do_delete_file(mkpathnm(fname));
}


/*
 *	the real delete file
 */
void
do_delete_file(pathnm)
char	*pathnm;
{
	Err	err;

	if ((err = DeleteFile(pathnm)) < 0) {
		PrintError(0,"delete file",pathnm,err);
 		return;
	}	
}


/*
 *	make sure the argument is a decimal number
 */
int
chknum(s)
char	*s;
{
	while (*s) {
		if (*s > '9' || *s < '0')
			return (1);
		s++;
	}
	return (0);
}


/*
 *	add DIR_NAME to the file name specified
 */
char	*
mkpathnm(fname)
char 	*fname;
{
	memset(buf, 0, BUFSZ);
	strcpy(buf, DIR_NAME);
	strcat(buf, "/");
	strcat(buf, fname);
	return buf;
}


/*
 *	Corrupt fields of the filesystem on device.
 */
void
corrupt_fs(int cnt, char *vec[], char flg)
{
	uint32 		val = 0, blknum, hdrsize = sizeof(LinkedMemFileEntry);
	LinkedMemFileEntry	fe;

	
#ifdef	DEBUG
	printf("%s: cnt:[%d], vec[0]:[%s], flg:[%c]\n", pname, cnt, vec[0],flg);
#endif	/* DEBUG */
	if (((cnt != 2) && (flg != 'Z')) || ((cnt != 1) && (flg == 'Z'))) {
		printf("%s: corrupt_fs: Invalid number of arguments, %d\n",
			 pname, cnt);
		usage();
		return;
	}

        if ((devp.dp_item = OpenNamedDevice(DEV_NAME, NULL)) < 0) {
                PrintError(0,"open device",DEV_NAME,devp.dp_item);
                return;
        }

        if ((devp.dp_ioreqi = CreateIOReq(NULL, 50, devp.dp_item, 0)) < 0) {
                printf("%s: Failed to create IOReq: ", pname);
                PrintError(0,"create IOReq",0,devp.dp_ioreqi);
                return;
        }

        devp.dp_ioreq = (IOReq *) LookupItem(devp.dp_ioreqi);
        memset(&devp.dp_ioinfo, 0, sizeof(IOInfo));

	if (chknum(vec[0])) {
		printf("%s: corrupt_fs: Invalid blknum, %s\n",
			 pname, vec[0]);
		return;
	}
	blknum = (uint32) strtol(vec[0], NULL, 0);

	if (flg != 'Z') {
		if (chknum(vec[1])) {
			printf("%s: corrupt_fs: Invalid int value, %s\n",
			 	pname, vec[1]);
			return;
		}
		val = (uint32) strtol(vec[1], NULL, 0);
	}
	
	if (rawio(CMD_READ, blknum, hdrsize, (char *) &fe, "read file entry"))
               	return;	
#ifdef	DEBUG
	printf("%s:BEFORE(%d): P:0x%x, F:%d, B:%d, C:%d, H:%d, Y:%d\n ",
		pname, blknum, fe.lmfe.lmb_Fingerprint,
		fe.lmfe.lmb_FlinkOffset, fe.lmfe.lmb_BlinkOffset,
		fe.lmfe.lmb_BlockCount, fe.lmfe.lmb_HeaderBlockCount,
		fe.lmfe_ByteCount);
#endif	/* DEBUG */

	switch (flg) {
	case 'P':
		fe.lmfe.lmb_Fingerprint = val;
		break;
	case 'F':
		fe.lmfe.lmb_FlinkOffset = val;
		break;
	case 'B':
		fe.lmfe.lmb_BlinkOffset = val;
		break;
	case 'C':
		fe.lmfe.lmb_BlockCount = val;
		break;
	case 'H':
		fe.lmfe.lmb_HeaderBlockCount = val;
		break;
	case 'Y':
		fe.lmfe_ByteCount = val;
		break;
	case 'Z':
        	memset(&fe, 0, sizeof(LinkedMemBlock));
		break;
	default:
		printf("%s: corrupt_fs: Invalid flg, %c\n",
			 pname, flg);
		return;
	}
	if (rawio(CMD_WRITE, blknum, hdrsize, (char *) &fe, "write file entry"))
                return;	
#ifdef	DEBUG
	printf("%s:AFTER(%d): P:0x%x, F:%d, B:%d, C:%d, H:%d, Y:%d\n ",
		pname, blknum, fe.lmfe.lmb_Fingerprint, fe.lmfe.lmb_FlinkOffset,
		fe.lmfe.lmb_BlinkOffset, fe.lmfe.lmb_BlockCount,
		fe.lmfe.lmb_HeaderBlockCount, fe.lmfe_ByteCount);
#endif	/* DEBUG */

}



/*
 *	real read/write file routine
 */
void
rdwr_file(int cnt, char *vec[], char flg)
{
	char	*fname, val = 'X';
	int	fsize;

	if ((cnt < 2) || 
	    ((cnt != 2) && (flg == 'r')) || 
	    ((cnt != 3) && (flg == 'w'))) {
		printf("%s: rdwr_file: Invalid number of arguments, %d\n",
			 pname, cnt);
		usage();
		return;
	}
	fname = *vec;
	if (chknum(vec[1])) {
		printf("%s: rdwr_file: Invalid file size, %s\n",
			 pname, vec[1]);
		return;
	}
	fsize = (int) strtol(vec[1], NULL, 0);
	if (fsize <= 0) {
		printf("%s: rdwr_file: Invalid file size, %s\n",
			 pname, vec[1]);
		return;
	}

	if (flg == 'w')
		val = vec[2][0];

	do_rdwr_file(mkpathnm(fname), fsize, flg, val);
}


/*
 *	real read/write file routine
 */
void
do_rdwr_file(char *pathnm, uint32 fsize, char flg, char val)
{
	Err	err;
	Item	ofi, iori;
	IOReq	*ior;
	IOInfo	ioinf;
	int	i;
	char	*bp;

	if ((ofi = OpenDiskFile(pathnm)) < 0) {
		PrintError(0,"open",pathnm,ofi);
 		return;
	}
	if ((iori = CreateIOReq(NULL, 50, ofi, 0)) < 0) {
		PrintError(0,"create IOReq",0,iori);
		CloseDiskFile(ofi);
 		return;
	}

	if ((bp = (char *) ALLOCMEM(fsize, MEMTYPE_DMA)) == NULL) {
		printf("%s: do_rdwr_file: Failed to ALLOCMEM  %d\n",
			 pname, fsize);
		CloseDiskFile(ofi);
 		return;
	}

	ior = (IOReq *) LookupItem(iori);
	memset(&ioinf, 0, sizeof(IOInfo));

	if (flg == 'w') {
		memset(bp, val, fsize);
		ioinf.ioi_Command = CMD_WRITE;
		ioinf.ioi_Recv.iob_Buffer = NULL;
        	ioinf.ioi_Recv.iob_Len = 0;
		ioinf.ioi_Send.iob_Buffer = bp;
        	ioinf.ioi_Send.iob_Len = fsize;
	} else {	/* read */
		ioinf.ioi_Command = CMD_READ;
		ioinf.ioi_Send.iob_Buffer = NULL;
        	ioinf.ioi_Send.iob_Len = 0;
		ioinf.ioi_Recv.iob_Buffer = bp;
        	ioinf.ioi_Recv.iob_Len = fsize;
	}
	ioinf.ioi_Offset = 0;
	err = DoIO(iori, &ioinf);
	if (err < 0 || (err = ior->io_Error) < 0) {
		PrintError(0,(flg=='w')?"write":"read",pathnm,err);
#ifdef	DEBUG
		prnt_ioinf(&ioinf);
#endif	/* DEBUG */
		CloseDiskFile(ofi);
 		return;
	}

	if (flg == 'r') {
		for (i = 0; i < fsize; i++, bp++)
			putchar(*bp);
		putchar('\n');
	} else {	/* write */
#ifdef notdef
		memset(&ioinf, 0, sizeof(IOInfo));
		ioinf.ioi_Command = FILECMD_SETEOF;
		ioinf.ioi_Offset = fsize;
		err = DoIO(iori, &ioinf);
		if (err < 0 || (err = ior->io_Error) < 0) {
			PrintError(0,"set length of",pathnm,err);
#ifdef	DEBUG
			prnt_ioinf(&ioinf);
#endif	/* DEBUG */
			CloseDiskFile(ofi);
 			return;
		}
#endif /* notdef */
		printf("EOF not set\n");
	}

	CloseDiskFile(ofi);
}


/*
 *	read/write to raw device
 */
void
rawdevio(int cnt, char *vec[], char flg)
{
	int32	size, offset;
	uint32	val = 0;

	if ((cnt < 2) || 
	    ((cnt != 2) && (flg == 'R')) || 
	    ((cnt != 3) && (flg == 'W'))) {
		printf("%s: rawdevio: Invalid number of arguments, %d\n",
			 pname, cnt);
		usage();
		return;
	}

	if (chknum(vec[0])) {
		printf("%s: rawdevio: Invalid offset, %s\n",
			 pname, vec[0]);
		return;
	}
	offset = (int) strtol(vec[0], NULL, 0);
	if (offset < 0) {
		printf("%s: rawdevio: Invalid offset, %s\n",
			 pname, vec[0]);
		return;
	}

	if (chknum(vec[1])) {
		printf("%s: rawdevio: Invalid size, %s\n",
			 pname, vec[1]);
		return;
	}
	size = (int) strtol(vec[1], NULL, 0);
	if (size <= 0) {
		printf("%s: rawdevio: Invalid size, %s\n",
			 pname, vec[1]);
		return;
	}

	if (flg == 'W')
		val = (int) strtol(vec[2], NULL, 0);

	do_rawdevio(offset, size, flg, val);
}

/*
 *	do io on the rawdev
 */
void
do_rawdevio(uint32 offset, uint32 size, char flg, uint32 val)
{
	int	i;
	char	*bp, *tmp;

        if ((devp.dp_item = OpenNamedDevice(DEV_NAME, NULL)) < 0) {
                PrintError(0,"open device",DEV_NAME,devp.dp_item);
                return;
        }

        if ((devp.dp_ioreqi = CreateIOReq(NULL, 50, devp.dp_item, 0)) < 0) {
                printf("%s: Failed to create IOReq: ", pname);
                PrintError(0,"create IOReq",0,devp.dp_ioreqi);
                return;
        }

        devp.dp_ioreq = (IOReq *) LookupItem(devp.dp_ioreqi);
        memset(&devp.dp_ioinfo, 0, sizeof(IOInfo));

	if ((bp = (char *) ALLOCMEM(size, MEMTYPE_DMA)) == NULL) {
		printf("%s: do_rawdevio: Failed to ALLOCMEM  %d\n",
			 pname, size);
 		return;
	}
        memset(bp, (int) val, (int) size);

	if (flg == 'W') {
		rawio(CMD_WRITE, offset, size, (char *) bp, "do_rawdevio write");
	} else {
		rawio(CMD_READ, offset, size, (char *) bp, "do_rawdevio read");
		tmp = bp;
		for (i = 0; i < size; i++, tmp++) {
			printf("%.2x%s", *tmp, (!(i % (PRNT_LINE_LEN /2)))? "\n": "");
		}
		printf("%s", (!(--i % (PRNT_LINE_LEN / 2)))? "": "\n");
	}
}

/*
 *	mount/unmount nvram
 */
void
mumount(int cnt, char flg)
{
	Item	ii = 0;
	int32	err;

	if (cnt) {
		printf("%s: mumount: Invalid number of arguments, %d\n",
			 pname, cnt);
		usage();
		return;
	}

	switch(flg) {
	case 'm':
		if ((ii = OpenNamedDevice(DEV_NAME, NULL)) < 0) {
			PrintError(0,"open",pname,ii);
		}
		if ((err = MountFileSystem(ii, DEV_UNIT, DEV_OFFSET)) < 0) {
			PrintError(0,"mount",DEV_NAME,err);
		}
		break;
	case 't':
		if ((ii = OpenDiskFile(DIR_NAME)) > 0)
			printf("%s: %s is mounted\n", pname, DIR_NAME);
		else 
			printf("%s: %s is not mounted\n", pname, DIR_NAME);
		break;
	case 'u':
		if ((err = DismountFileSystem(DIR_NAME)) < 0) {
			PrintError(0,"unmount",DIR_NAME,err);
		}
		break;
	}
	return;
}


/*
 *	io from/to the raw device
 *	caller must provide buffer large enough to hold the data
 *	buffer must also be a multiple of device block size, otherwise
 *	rawio will write pass it.
 */
int
rawio(uint8 cmd, int32 offset, int32 size, char *buf, char *errmsg)
{
	Err		err;

	switch (cmd) {
	case CMD_STATUS:
	case CMD_READ:
		devp.dp_ioinfo.ioi_Send.iob_Buffer = NULL;
        	devp.dp_ioinfo.ioi_Send.iob_Len = 0;
		devp.dp_ioinfo.ioi_Recv.iob_Buffer = buf;
        	devp.dp_ioinfo.ioi_Recv.iob_Len = size;
		break;

	case CMD_WRITE:
		devp.dp_ioinfo.ioi_Recv.iob_Buffer = NULL;
        	devp.dp_ioinfo.ioi_Recv.iob_Len = 0;
		devp.dp_ioinfo.ioi_Send.iob_Buffer = buf;
        	devp.dp_ioinfo.ioi_Send.iob_Len = size;
		break;
	default:
		printf("%s: rawio: Invalid command %d: ", pname, cmd);
		return 1;
	}

        devp.dp_ioinfo.ioi_Offset = offset;
        devp.dp_ioinfo.ioi_Unit = DEV_UNIT;
        devp.dp_ioinfo.ioi_Flags = 0;
        devp.dp_ioinfo.ioi_Command = cmd;
        devp.dp_ioinfo.ioi_CmdOptions = 0;
        err = DoIO(devp.dp_ioreqi, &devp.dp_ioinfo);
        if (err < 0 || (err = (devp.dp_ioreq)->io_Error) < 0) {
#if 0
		printf("%s: Failed to %s (%d, %d, 0x%x, %d, 0x%x, %d): ",
			pname, errmsg, cmd, offset, 
			devp.dp_ioinfo.ioi_Send.iob_Buffer,
        		devp.dp_ioinfo.ioi_Send.iob_Len,
			devp.dp_ioinfo.ioi_Recv.iob_Buffer,
        		devp.dp_ioinfo.ioi_Recv.iob_Len);
#endif
                PrintError(0,errmsg,0,err);
                return 1;
        }
	return 0;
}



#ifdef	DEBUG
void
prnt_ioinf(IOInfo *ioinf)
{
	printf("Info (%u, %u, %u, %u, %d, 0x%x, %d, 0x%x, %d, 0x%x, %d)\n",
                        pname,
                        ioinf->ioi_Command,
                        ioinf->ioi_Flags,
                        ioinf->ioi_Unit,
                        ioinf->ioi_Flags2,
                        ioinf->ioi_CmdOptions,
                        ioinf->ioi_User,
                        ioinf->ioi_Offset,
                        ioinf->ioi_Send.iob_Buffer,
                        ioinf->ioi_Send.iob_Len,
                        ioinf->ioi_Recv.iob_Buffer,
                        ioinf->ioi_Recv.iob_Len);
}

#endif	/* DEBUG */
