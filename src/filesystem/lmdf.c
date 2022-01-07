/* $Id: lmdf.c,v 1.5 1994/09/16 22:24:28 vertex Exp $ */

/*
 *
 *      Copyright The 3DO Company Inc., 1993
 *      All Rights Reserved Worldwide.
 *      Company confidential and proprietary.
 *      Contains unpublished technical data.
 */

/*
 *      lmdf.c - Linked Memory Flat Filesystem df.
 *
 */

#include 	"types.h"
#include 	"item.h"
#include 	"mem.h"
#include 	"nodes.h"
#include 	"debug.h"
#include 	"list.h"
#include 	"device.h"
#include 	"driver.h"
#include 	"msgport.h"
#include 	"kernel.h"
#include 	"kernelnodes.h"
#include 	"io.h"
#include 	"discdata.h"
#include 	"filesystem.h"
#include 	"filesystemdefs.h"
#include 	"operror.h"
#include 	"time.h"

#include 	"filefunctions.h"
#include 	"stdio.h"
#include 	"strings.h"

#define		MNT_POINT	"/nvram"
#define		DEV_NAME	"ram"
#define		DEV_UNIT	3
#define		DEV_OFFSET	0

#define		ONEK		1024

#define		ROUND(i, r)	((i / r) + ((i & (r >> 1))? 1: 0))
#define		IS_SET(bit)	(flgs & bit)
#define		SET(bit)	(flgs |= bit)

#define		KFLG		0x1

typedef struct {
        DeviceStatus    dp_devstat;
        Item            dp_item;
        Item            dp_ioreqi;
        IOReq           *dp_ioreq;
        IOInfo          dp_ioinfo;
} dev_prop_t;

typedef struct {
        Item            fp_item;
        Item            fp_ioreqi;
        IOReq           *fp_ioreq;
        IOInfo          fp_ioinfo;
} file_prop_t;

uint32	flgs, kbyte;
char	*pname;

int	parse_arg(int ac, char *av[]);
void	sleep(int sec);
void    usage(void);

/**
|||	AUTODOC PUBLIC tpg/shell/lmdf
|||	lmdf - Report free space for Linked Memory Filesystem
|||
|||	  Synopsis
|||
|||	    lmdf [-k]
|||
|||	  Description
|||
|||	    lmdf displays the amount of disk space occupied on
|||	    nvram, the amount of used and available space, and how
|||	    much of the filesystem's total capacity has been used
|||	    and fragmented. lmdf produces an output similar to
|||	    the following:
|||
|||
|||		    Device  Bytes  used  avail  capacity  fragment  Mounted on
|||		    ram.3.0 32768 16408  16040       50%       27%  /nvram
|||
|||
|||	    Note: used+avail is less than the amount of space in the
|||	    filesystem (Bytes|Kbytes); this is due to filesystem overhead.
|||	    Furthermore, the amount of space available in the filesystem
|||	    may not be allocated all to a single file due to fragmentation.
|||	    If the filesystem is fragmented, use lmadm to defragment
|||	    the filesystem. However, if there is no fragmentation
|||	    (fragment reports 0%), then the available space in its
|||	    entirely can be allocated to a single file.
|||
|||	    If nvram is not mounted, no data is reported.
|||
|||	  Options
|||
|||	    -k		 All ouput data is rounded to 1024 bytes.
|||
|||	  Implementation
|||
|||	    Command implemented in V21.
|||
|||	  Location
|||
|||	    $c/lmdf
|||
|||	  See Also
|||
|||	    lmadm, format, lmdump, lmfs
|||
**/
int
main(int ac, char *av[])
{
	uint32		totbytes, used, avail, maxf, frag;
	Err		err;
	dev_prop_t     	devp;
	file_prop_t     filep;
	FileSystemStat	fstat;

	if (parse_arg(ac, av))
		return 1;

	totbytes = used = avail = maxf = frag = 0;
	/*
	 * set up device stuff.
	 */
        if ((devp.dp_item = OpenNamedDevice(DEV_NAME, NULL)) < 0) {
		printf("%s: Failed to open %s (0x%x)\n", pname,
			DEV_NAME, devp.dp_item);
                PrintError(0,"",DEV_NAME,devp.dp_item);
                return 1;
        }

        if ((devp.dp_ioreqi = CreateIOReq(NULL, 50, devp.dp_item, 0)) < 0) {
                printf("%s: Failed to create dev IOReq: ", pname);
                PrintError(0,"",0,devp.dp_ioreqi);
                return 1;
        }

        if ((devp.dp_ioreq = (IOReq *) LookupItem(devp.dp_ioreqi)) == 0) {
		PrintError(0,"lookup dev IOReq",0,devp.dp_ioreq);
		return 1;
	}
        memset(&devp.dp_ioinfo, 0, sizeof(IOInfo));

	devp.dp_ioinfo.ioi_Command = CMD_STATUS;
        devp.dp_ioinfo.ioi_Unit = (uint8) DEV_UNIT;
        devp.dp_ioinfo.ioi_Offset = DEV_OFFSET;
	devp.dp_ioinfo.ioi_Recv.iob_Buffer = &devp.dp_devstat;
	devp.dp_ioinfo.ioi_Recv.iob_Len = sizeof(DeviceStatus);
	err = DoIO(devp.dp_ioreqi, &devp.dp_ioinfo);
	if (err < 0 || (err = devp.dp_ioreq->io_Error) < 0) {
		printf("%s: Failed to get status of %s (0x%x)\n", pname,
			DEV_NAME, err);
		PrintError(0, "", 0, err);
		return 1;

	}

	totbytes = devp.dp_devstat.ds_DeviceBlockSize *
		   devp.dp_devstat.ds_DeviceBlockCount;

	/*
	 * set up mount-point stuff.
	 */
	if ((filep.fp_item = OpenDiskFile(MNT_POINT)) < 0) {
		printf("%s: \"%s\" not mounted\n", pname, MNT_POINT);
                PrintError(0,"",MNT_POINT,filep.fp_item);
 		return 1;
	}

	if ((filep.fp_ioreqi = CreateIOReq(NULL, 0, filep.fp_item, 0)) < 0) {
                printf("%s: Failed to create file IOReq: ", pname);
		PrintError(0,"",0,filep.fp_ioreqi);
 		return 1;
	}

	if ((filep.fp_ioreq = (IOReq *) LookupItem(filep.fp_ioreqi)) == 0) {
		PrintError(0,"lookup file IOReq",0,filep.fp_ioreq);
		return 1;
	}

	memset(&filep.fp_ioinfo, 0, sizeof(IOInfo));

	filep.fp_ioinfo.ioi_Command = FILECMD_FSSTAT;
	filep.fp_ioinfo.ioi_Recv.iob_Buffer = &fstat;
	filep.fp_ioinfo.ioi_Recv.iob_Len = sizeof(FileSystemStat);
	err = DoIO(filep.fp_ioreqi, &filep.fp_ioinfo);
	if (err < 0 || (err = filep.fp_ioreq->io_Error) < 0) {
		PrintError(0,"get file system status",0,err);
		return 1;
	}

	sleep(1);
	printf("\n%-9s%-9s%-7s%-8s%-11s%-11s%s\n", "Device", (kbyte == ONEK)? "Kbytes": " Bytes", "used",
		"avail", "capacity","fragment", "Mounted on");
	printf("%s.%d.%d%8d",
		DEV_NAME, DEV_UNIT, DEV_OFFSET,
		ROUND(totbytes, kbyte));
	if (FSSTAT_ISSET(fstat.fst_BitMap, FSSTAT_USED)) {
		used = fstat.fst_Used * devp.dp_devstat.ds_DeviceBlockSize;
		printf("%7d", ROUND(used, kbyte));
	} else
		printf("%7s", "****");

	if (FSSTAT_ISSET(fstat.fst_BitMap, FSSTAT_FREE)) {
		avail = fstat.fst_Free * devp.dp_devstat.ds_DeviceBlockSize;
		printf("%8d", ROUND(avail, kbyte));
	} else
		printf("%8s", "*****");

	if (FSSTAT_ISSET(fstat.fst_BitMap, FSSTAT_FREE) &&
	    FSSTAT_ISSET(fstat.fst_BitMap, FSSTAT_USED)) {
		printf("%10d%%",  (100 * used) / totbytes);
	} else
		printf("%11s", "********");

	if (FSSTAT_ISSET(fstat.fst_BitMap, FSSTAT_FREE) &&
	    FSSTAT_ISSET(fstat.fst_BitMap, FSSTAT_MAXFILESIZE)) {
		maxf = fstat.fst_MaxFileSize*devp.dp_devstat.ds_DeviceBlockSize;
		printf("%10d%%", (100 * (avail - maxf)) / totbytes);
	} else
		printf("%11s", "********");

	printf("%9s\n", MNT_POINT);
	return 0;
}

void
sleep(int sec)
{
	Item            timer;
	Item            io;
	IOReq          *ior;
	IOInfo          ioInfo;
	Err             ioerr;
	struct timeval  tv;

    	tv.tv_sec  = sec;
    	tv.tv_usec = 0;

    	timer = OpenNamedDevice("timer",0);
    	if (timer >= 0) {
        	io = CreateIOReq(0,0,timer,0);
        	if (io >= 0) {
            	ior = (IOReq *)LookupItem(io);

            	/* initialize ioInfo */
            	memset(&ioInfo,0,sizeof(ioInfo));
            	ioInfo.ioi_Command         = TIMERCMD_DELAY;
            	ioInfo.ioi_Unit            = TIMER_UNIT_USEC;
            	ioInfo.ioi_Send.iob_Buffer = &tv;
            	ioInfo.ioi_Send.iob_Len    = sizeof(tv);

            	ioerr = DoIO(io,&ioInfo);

            	if (ioerr < 0) {
                	PrintError(NULL,"\\(Sleep) unable to perform DoIO() to","timer",ioerr);
            	} else if (ior->io_Error) {
            	    PrintError(NULL,"\\(Sleep) unable to complete DoIO() to","timer",ior->io_Error);
            	}

            	DeleteIOReq(io);
        	} else {
            	PrintError(NULL,"\\(Sleep) unable to create IO request for","timer",io);
        	}
        	CloseItem(timer);
    	} else {
    	    PrintError(NULL,"\\(Sleep) unable to open device","timer",timer);
    	}
}

/*
 *	argument parser
 */
int
parse_arg(int ac, char *av[])
{
	char	*flg;

	pname = *av;
	ac--, av++;

	if (ac == 0)
		goto flgsdone;
	if (ac > 1) {
		printf("%s: Wrong number of arguments\n", pname);
		usage();
		return 1;
	}
	flg = *av++;
	ac--;
	if (*flg++ != '-') {
		printf("%s: %s is not an option\n", pname, flg);
		usage();
		return 1;
	}
	while (*flg) {
		switch(*flg) {
		case 'k':
			SET(KFLG);
			kbyte = ONEK;
			break;
		default:
			printf("%s: Invalid option %c\n", pname, *flg);
			usage();
			return 1;
		}
		flg++;
	}

flgsdone:
	if (!IS_SET(KFLG))
		kbyte = 1;

	return 0;
}


/*
 *	simple, hah.
 */
void
usage(void)
{
	printf("USAGE: %s [-k]\n", pname);
	printf("       -k       round off to KByte\n");
}


