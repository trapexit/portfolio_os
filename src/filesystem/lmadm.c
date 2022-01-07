/*****
 $Id: lmadm.c,v 1.15 1994/09/16 22:23:09 vertex Exp $

 $Log: lmadm.c,v $
 * Revision 1.15  1994/09/16  22:23:09  vertex
 * Autodoc tweak.
 *
 * Revision 1.14  1994/09/10  03:28:17  vertex
 * Updated autodoc headers per Yvonne's request.
 *
 * Revision 1.13  1994/09/08  23:24:40  vertex
 * Updated to new release numbering scheme.
 * Updated format to match modern autodoc formatting style
 *
 * Revision 1.12  1994/08/03  17:29:19  shawn
 * Added autodoc for the -z option.
 *
 * Revision 1.11  1994/08/03  17:10:38  shawn
 * Several panasonic units showed lots of zero size files
 * just laying around when the filesystem gets full. This is due
 * to the fact that app allocates the file header first and then
 * since the filesystem is full, fails to allocate the required
 * number of blocks. Hence leaving zero size files behind. Basically
 * a 64 bytes waste.
 * lmadm now provides -z flag to remove zero size files.
 *
 *****/

/*
 *
 *      Copyright The 3DO Company Inc., 1993
 *      All Rights Reserved Worldwide.
 *      Company confidential and proprietary.
 *      Contains unpublished technical data.
 */

/*
 *      lmadm.c - Linked Memory Flat Filesystem admin.
 *
 *		  It must be called at startup as
 *		  # $boot/System/Programs/lmadm -a ram 3 0 nvram
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
/*
 *	TV display stuff
 */
#include "graphics.h"
#include "event.h"


#define TAB		"       "
#define FORMAT_CMD	"$boot/System/Programs/format"
#define MIN(a,b)        ((a < b)? a: b)
#define HOWMANY(sz, unt)        ((sz + (unt - 1)) / unt)
#define BYT2DEVBLK(byt) (HOWMANY(byt, devp.dp_devstat.ds_DeviceBlockSize))
#define DEVBLK2BYT(blk) (blk * devp.dp_devstat.ds_DeviceBlockSize)
#define BYT2FSBLK(byt)  (HOWMANY(byt, fs.fs_bsize))
#define FSBLK2BYT(blk)  (blk * fs.fs_bsize)
#define CURBLKNUM(cnt)  (mis[(cnt) % MAXBUFS].ms_blknum)
#define CURBLK(cnt)     (&(mis[(cnt) % MAXBUFS].ms_blk))
#define NXTBLKNUM(cnt)  (mis[((cnt)+1) % MAXBUFS].ms_blknum)
#define NXTBLK(cnt)     (&(mis[((cnt)+1) % MAXBUFS].ms_blk))
#define BAD_BLK(bp)             \
          (((bp)->lmb_Fingerprint != FINGERPRINT_FILEBLOCK) && \
           ((bp)->lmb_Fingerprint != FINGERPRINT_FREEBLOCK))

#define ALLBAD_BLK(bp)          \
          (((bp)->lmb_Fingerprint != FINGERPRINT_FILEBLOCK) && \
           ((bp)->lmb_Fingerprint != FINGERPRINT_FREEBLOCK) && \
           ((bp)->lmb_Fingerprint != FINGERPRINT_ANCHORBLOCK))

#define ZERO_SIZE(bp)             \
          (((bp)->lmb_Fingerprint == FINGERPRINT_FILEBLOCK) && \
           ((bp)->lmb_BlockCount == (bp)->lmb_HeaderBlockCount))

#define WAITTASK(itemid)	\
		{ \
			while (LookupItem(itemid)) { \
				WaitSignal(SIGF_DEADTASK); \
			} \
		}

#define MAX_LINKNUM	1000    /* how may times to loop before giving up */
#define MAXBUFS		2
#define CPBUFSZ		4096
#define ERRBUFSZ	100

/*
 *      trav_back search flags
 */
#define TRAV_EXACT	1
#define TRAV_BESTGUESS	2

/*
 *      qdisp flags
 */
#define QDISP_NORM	1
#define QDISP_URG	2

#define	AFLG		0x001
#define	CFLG		0x002
#define	DFLG		0x004
#define	MFLG		0x008
#define	TFLG		0x010
#define	UFLG		0x020
#define	VFLG		0x040
#define	YFLG		0x080
#define	UDFLG		0x100
#define	UTFLG		0x200
#define	HATFLG		0x400		/* system use only, not publicized */
#define	ZFLG		0x800
#define	IS_SET(bit)	(flgs & bit)
#define	SET(bit)	(flgs |= bit)

#define	MU_MOUNT	1
#define	MU_UNMOUNT	2

/*
 * 	all routine return error code
 */
#define	LM_CLEAN	0
#define	LM_SOFT		1
#define	LM_HARD		2

/*
 *	TV display stuff
 */
#define	CHARS_PER_LINE	35
#define	LINES_IN_SCREEN	25
#define	NEXT_LINE	9
#define	PIXL_START_X	20
#define	PIXL_START_Y	20
#define	BUFSZ		128

#define	WAITFORA	\
			cpad.cped_ButtonBits = 0; \
			while ((cpad.cped_ButtonBits & ControlA) == 0) \
       				GetControlPad (1, 1, &cpad)



typedef struct {
	char		*fs_mounted;
        uint32		fs_bcnt;
        uint32		fs_bsize;
        uint32		fs_rootblknum;
        LinkedMemBlock	fs_rootblk;
} fs_prop_t;

typedef struct {
        char            *dp_name;
        uint8           dp_unit;
        int             dp_offset;
        DeviceStatus    dp_devstat;
        Item            dp_item;
        Item            dp_ioreqi;
        IOReq           *dp_ioreq;
        IOInfo          dp_ioinfo;
} dev_prop_t;

typedef struct {
        LinkedMemBlock  ms_blk;
        uint32          ms_blknum;
} misc_t;

dev_prop_t      devp;
fs_prop_t       fs;
misc_t		mis[MAXBUFS];
uint32		hdrsize, hdrblk;
uint32		flgs, inited;
char            *pname;
char            errbuf[ERRBUFSZ];
char            cpbuf[CPBUFSZ], *cpbp;
/*
 *	TV display stuff
 */
Item		sgi, ScreenItem, BitmapItem;
GrafCon		gcon;
TagArg		ScreenTags[3];
Screen		*screen;
ControlPadEventData cpad;

int	parse_arg(int ac, char *av[]);
int     chknum(char *s);
int     qask(char *s);
int     rawio(uint8 cmd, uint32 offset, uint32 size, char *buf, char *errmsg);
int     validate_last(int32 lastblknum, LinkedMemBlock  *lastblk);
int     set_lastblk(uint32 wrtblk, uint32 bblk);
int     do_lmdf(void);
int     init(int verb);
int     pass_1(void);
int     pass_2(void);
int     pass_3(void);
int     do_lmck(void);
int     clear_fs(uint32 curblknum, LinkedMemBlock *curblk);
int     fix_rootblk(LinkedMemBlock *rblkp, uint32 rblknum);
int	is_mounted(char *mntdir);
int	mumount(int flg);
int	do_auto(void);
int	pass_0(DiscLabel *dlp);

uint32  copy_file(uint32 fromblknum, LinkedMemBlock *frombp,
                  uint32 toblknum, uint32 preblknum);

uint32  nxtblk_bysize(uint32 curblknum, LinkedMemBlock *curblk,
                      LinkedMemBlock *nxtblk);
uint32  nxtblk_byflink(uint32 curblknum, LinkedMemBlock *curblk,
                      LinkedMemBlock *nxtblk);
uint32  nxtblk_byfands(uint32 curblknum, LinkedMemBlock *curblk,
                      LinkedMemBlock *nxtblk);
uint32  nxtblk_byblink(uint32 curblknum, LinkedMemBlock *curblk,
                      LinkedMemBlock *nxtblk);
uint32  nxtblk_byguess(uint32 curblknum, LinkedMemBlock *curblk,
                      LinkedMemBlock *nxtblk);
uint32  trav_back(uint32 fromblk, uint32 targetblk, int flg);

char	*add_slash(char *cp);
char	*sub_slash(char *cp);

void    qdisp(int flg,char *s);
void    usage(void);
void    cleanup(void);
/*
 *	TV display stuff
 */
char	*tv_prnt_aline(char *cp);
void	tv_prnt(char *str);
void	blank_out(char *cp, int len);
void	tv_clrscr(void);

/*
 *	debug stuff
 */
int	lm_dumpfs(void);
void	lm_dumpblk(LinkedMemBlock *bp, uint32 boff);
void	lm_dumpent(LinkedMemFileEntry *ep, uint32 eoff);

#ifdef  DEBUG
void    prnt_blk(LinkedMemBlock *blk);
#endif  /* DEBUG */


/**
|||	AUTODOC PUBLIC tpg/shell/lmadm
|||	lmadm - linked Memory Filesystem Administration Utility
|||
|||	  Format
|||
|||	    lmadm -acdmtuvyz dev_name unit offset mount_dir
|||
|||	  Description
|||
|||	    This utility performs administrative tasks on a Linked Memory
|||	    filesystems. lmadm is designed to perform a variety of tasks.
|||	    It checks and repairs (-c), defragments (-d), mounts (-m),
|||	    unmounts (-u), and auto-maintains (-a) the filesystem.
|||
|||
|||	  Options
|||
|||	    -a		 Auto-maintain the filesystem. This option unmounts
|||			 the filesystem, checks/repairs, defragments,
|||			 and remounts the filesystem. In case of a failure in
|||			 the consistency check/repair or defragmentation
|||			 process, the filesystem is formatted. This option
|||			 is mainly used at system startup. This option is
|||			 mutually exclusive with all other options except
|||			 for -v and -y. This option implies -y.
|||
|||	    -c		 Consistency check and repair. Consistency
|||			 check is performed in multiple passes and all
|||			 repairs are reported. This option is mutually
|||			 exclusive with -a, -m and -u.
|||
|||	    -d		 Defragment the filesystem. As a result of adding
|||			 and deleting files, filesystems become fragmented
|||			 and free extents become scattered. Defragmentation
|||			 process gathers all scattered free extents and
|||			 combines them into a single contiguous extent.
|||			 This process not only improves system performance,
||| 			 but it also enables the filesystem to succeed in
|||			 allocating storage for larger files. This option is
|||			 mutually exclusive with -a, -m and -u.
|||
|||	    -m		 Mount the filesystem. It only mounts the filesystem
|||			 on the specified device, if the filesystem is not
|||			 already mounted. This option is mutually exclusive
|||			 with all other options except for -v and -y.
|||
|||	    -t		 Display output on the TV monitor as well
|||
|||	    -u		 Unmount the filesystem. It only unmounts the
|||			 filesystem, if the filesystem is mounted. This
|||			 option is mutually exclusive with all other
|||			 options except for -v and -y.
|||
|||	    -v		 Verbose.
|||
|||	    -y		 Perform all major/minor repairs needed to bring the
|||			 filesystem to a consistent state.
|||
|||	    -z		 Remove all zero-sized files. This is performed
|||			 during the defragmentation process, therefore it
|||			 is to be used in conjunction with -a or -d options.
|||
|||
|||	  Arguments
|||
|||	    dev_name      Name of the device the filesystem residing on.
|||
|||	    unit          Device unit number.
|||
|||	    0ffset        Offset within the device where the filesystem
|||			  resides.
|||
|||	    mount_dir     Name of the directory the filesystem is
|||			  mounted on.
|||
|||	  Examples
|||
|||	    The following line maintains the nvram filesystem for operation,
|||	    this is mainly invoked at system startup:
|||			  $boot/system/Programs/lmadm -a ram 3 0 nvram
|||
|||	    The following line performs consistency check and an
|||	    automatic repair:
|||			  $c/lmadm -cy ram 3 0 nvram
|||
|||	    The following line verbosely performs a defragmentation process
|||	    on the nvram filesystem:
|||			  $c/lmadm -dv ram 3 0 nvram
|||
|||	    The following line mounts the nvram filesystem
|||			  $c/lmadm -m ram 3 0 nvram
|||
|||	  Caveats
|||
|||	    Defragmentation process SHALL ONLY be run on a clean filesystem.
|||	    Always check/repair the filesystem first, before the
|||	    defragmentation process is started. If both -d and -c are
|||	    selected, the filesystem is first check/repaired and then
|||	    defragmented. The result of defragmenting a corrupted filesystem
|||	    is unpredictable.
|||
|||	    When -a option is sellected and the check/repair process
|||	    fails, the filesystem is automatically formatted. This means
|||	    that all existing data on the filesystem is cleared.
|||
|||	  Implementation
|||
|||	    Command implemented in V21.
|||
|||	  Location
|||
|||	    $c/lmadm
|||
|||	  See Also
|||
|||	    format, lmdump, lmfs
|||
**/
int
main(int ac, char *av[])
{
	int	ret = LM_CLEAN;
	Item	ii = 0;

	if (parse_arg(ac, av)) {
		cleanup();
		return LM_SOFT;
	}

	/*
	 *	debug stuff
	 */
	if (IS_SET(UTFLG)) {
		if ((ii = OpenDiskFile(fs.fs_mounted)) > 0) {
			sprintf(errbuf, "%s: %s is mounted\n", pname,
				fs.fs_mounted);
			qdisp(QDISP_URG, errbuf);
		} else {
			sprintf(errbuf, "%s: %s is not mounted\n", pname,
				fs.fs_mounted);
			qdisp(QDISP_URG, errbuf);
		}
		cleanup();
		return ret;
	}

	if (IS_SET(UDFLG)) {
		ret = lm_dumpfs();
		cleanup();
		return ret;
	}

	/*
	 *	real stuff
	 */
	if (IS_SET(AFLG)) {
		SET(YFLG);
		/* SET(VFLG); */
		ret = do_auto();
		cleanup();
		return ret;
	}

	if (IS_SET(CFLG)) {
		if (is_mounted(fs.fs_mounted)) {
			sprintf(errbuf, "%s: %s is mounted\n", pname, fs.fs_mounted);
			qdisp(QDISP_URG, errbuf);
			cleanup();
			return LM_SOFT;
		}
		if ((ret = do_lmck()) != LM_CLEAN) {
			cleanup();
			return ret;
		}
	}

	if (IS_SET(DFLG)) {
		if (is_mounted(fs.fs_mounted)) {
			sprintf(errbuf, "%s: %s is mounted\n", pname, fs.fs_mounted);
			qdisp(QDISP_URG, errbuf);
			cleanup();
			return LM_SOFT;
		}
		if ((ret = do_lmdf()) != LM_CLEAN) {
			cleanup();
			return ret;
		}
	}

	if (IS_SET(MFLG)) {
		if (is_mounted(fs.fs_mounted))  {
			sprintf(errbuf, "%s: %s is already mounted\n",
			       pname, fs.fs_mounted);
			qdisp(QDISP_URG, errbuf);
		} else {
			if (mumount(MU_MOUNT)) {
				sprintf(errbuf, "%s: Failed to mounted %s\n",
					 pname, fs.fs_mounted);
				qdisp(QDISP_URG, errbuf);
				cleanup();
				return LM_SOFT;
			} else {
				sprintf(errbuf, "%s: %s  mounted\n", pname,
					 fs.fs_mounted);
				qdisp(QDISP_URG, errbuf);
			}
		}
	}

	if (IS_SET(UFLG)) {
		if (is_mounted(fs.fs_mounted))  {
			if (mumount(MU_UNMOUNT)) {
				sprintf(errbuf, "%s: Failed to unmount %s\n",
					 pname, fs.fs_mounted);
				qdisp(QDISP_URG, errbuf);
				cleanup();
				return LM_SOFT;
			} else {
				sprintf(errbuf, "%s: %s unmounted\n",
					 pname, fs.fs_mounted);
				qdisp(QDISP_URG, errbuf);
			}
		} else {
			sprintf(errbuf, "%s: %s is already unmounted\n",
				pname, fs.fs_mounted);
			qdisp(QDISP_URG, errbuf);
		}
	}

	cleanup();
	return LM_CLEAN;
}


/*
 *	perform auto-maintanence
 */
int
do_auto(void)
{
	Item	result = 0;
	int	ret;

	if (is_mounted(fs.fs_mounted)) {
		if (mumount(MU_UNMOUNT))
			return LM_SOFT;
	}
	/*
	 *	first, let's check/repair the FS
	 *	second, defragment it. if FS fails
	 *	the check/repair, have no choice
	 *	but to format it.
	 */
	ret = do_lmck();
	switch (ret) {
	case LM_HARD:
		return LM_HARD;
	case LM_SOFT:		/* Bad news :-( */
		sprintf(errbuf, "%s %s %d %d %s", FORMAT_CMD,
				 devp.dp_name, devp.dp_unit,
				 devp.dp_offset, sub_slash(fs.fs_mounted));
		result = LoadProgram(errbuf);
		WAITTASK(result);
		break;
	case LM_CLEAN:
		ret = do_lmdf();
		switch (ret) {
		case LM_HARD:
			return LM_HARD;
		case LM_SOFT:	/* This should never happen  :-( */
			sprintf(errbuf, "%s %s %d %d %s", FORMAT_CMD,
				 devp.dp_name, devp.dp_unit,
				 devp.dp_offset, sub_slash(fs.fs_mounted));
			result = LoadProgram(errbuf);
			WAITTASK(result);
			break;
		case LM_CLEAN:
			break;
		default:
			sprintf(errbuf,"%s: do_lmdf: Invalid return value %d\n",
				 pname, ret);
			qdisp(QDISP_URG, errbuf);
			return LM_HARD;
		}
		break;
	default:
		sprintf(errbuf, "%s: do_lmck: Invalid return value %d\n",
			 pname, ret);
		qdisp(QDISP_URG, errbuf);
		return LM_HARD;
	}

	if (mumount(MU_MOUNT))
		return LM_SOFT;
	return LM_CLEAN;
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
	if (ac != 5) {
		sprintf(errbuf, "%s: Wrong number of arguments\n", pname);
		qdisp(QDISP_URG, errbuf);
		usage();
		return LM_SOFT;
	}
	flg = *av++;
	ac--;
	if (*flg++ != '-') {
		sprintf(errbuf, "%s: %s is not an option\n", pname, flg);
		qdisp(QDISP_URG, errbuf);
		usage();
		return LM_SOFT;
	}
	while (*flg) {
		switch(*flg) {
		case 'a':
			SET(AFLG);
			break;
		case 'c':
			SET(CFLG);
			break;
		case 'd':
			SET(DFLG);
			break;
		case 'm':
			SET(MFLG);
			break;
		case 't':
			/*
 	 		 *	TV display stuff
	 		 */
    			InitEventUtility(1, 0, LC_ISFOCUSED);
    			OpenGraphicsFolio();
    			ScreenTags[0].ta_Tag = CSG_TAG_SCREENCOUNT;
    			ScreenTags[0].ta_Arg = (void *) 1,
    			ScreenTags[1].ta_Tag = CSG_TAG_DONE;
    			ScreenTags[1].ta_Arg = 0;
    			sgi = CreateScreenGroup(&ScreenItem, ScreenTags);
			if (sgi < 0) {
				/* TFLG must not be set by now */
				sprintf(errbuf, "%s: CreateScreenGroup failed %d\n", pname, sgi);
				qdisp(QDISP_URG, errbuf);
				PrintError(0,"create screen group",0,sgi);
				return LM_SOFT;
			}
    			AddScreenGroup(sgi, NULL);
    			screen = (Screen *)LookupItem(ScreenItem);
			if (screen == 0) {
				/* TFLG must not be set by now */
				sprintf(errbuf, "%s: LookupItem failed\n", pname);
				qdisp(QDISP_URG, errbuf);
				return LM_SOFT;
			}
    			BitmapItem = screen->scr_TempBitmap->bm.n_Item;
    			DisplayScreen(ScreenItem, 0);
    			SetBGPen(&gcon, MakeRGB15(0,0,31));
    			SetFGPen(&gcon, MakeRGB15(31,31,0));
			tv_clrscr();
			SET(TFLG);
			break;
		case 'u':
			SET(UFLG);
			break;
		case 'v':
			SET(VFLG);
			break;
		case 'y':
			SET(YFLG);
			break;
		case 'z':
			SET(ZFLG);
			break;
		case 'D':
			SET(UDFLG);
			break;
		case 'T':
			SET(UTFLG);
			break;
		case '^':
			SET(HATFLG);
			SET(YFLG);
			break;
		default:
			sprintf(errbuf, "%s: Invalid option %c\n", pname, *flg);
			qdisp(QDISP_URG, errbuf);
			return LM_SOFT;
		}
		flg++;
	}

	if (IS_SET(AFLG) && (IS_SET(CFLG) || IS_SET(DFLG) || IS_SET(MFLG) ||
			     IS_SET(UFLG))) {
		sprintf(errbuf, "%s: Option a can not be used with c|d|m|u\n",
			pname);
		qdisp(QDISP_URG, errbuf);
		return LM_SOFT;
	}
	if (IS_SET(CFLG) && (IS_SET(AFLG) || IS_SET(MFLG) || IS_SET(UFLG))) {
		sprintf(errbuf, "%s: Option c can not be used with m|u\n",
			pname);
		qdisp(QDISP_URG, errbuf);
		return LM_SOFT;
	}
	if (IS_SET(DFLG) && (IS_SET(AFLG) || IS_SET(MFLG) || IS_SET(UFLG))) {
		sprintf(errbuf, "%s: Option d can not be used with m|u\n",
			pname);
		qdisp(QDISP_URG, errbuf);
		return LM_SOFT;
	}
	if (IS_SET(MFLG) && (IS_SET(AFLG) || IS_SET(CFLG) || IS_SET(DFLG) ||
			     IS_SET(UFLG))) {
		sprintf(errbuf, "%s: Option m can not be used with a|c|d|u\n",
			pname);
		qdisp(QDISP_URG, errbuf);
		return LM_SOFT;
	}
	if (IS_SET(UFLG) && (IS_SET(AFLG) || IS_SET(CFLG) || IS_SET(DFLG) ||
			     IS_SET(MFLG))) {
		sprintf(errbuf, "%s: Option u can not be used with a|c|d|m\n",
			pname);
		qdisp(QDISP_URG, errbuf);
		return LM_SOFT;
	}
	if (!IS_SET(AFLG) && !IS_SET(CFLG) && !IS_SET(DFLG) &&
	    !IS_SET(UDFLG) && !IS_SET(UTFLG) &&
	    !IS_SET(MFLG) && !IS_SET(UFLG)) {
		sprintf(errbuf, "%s: At least one of a|c|d|m|u|D|T must be specified\n",
			 pname);
		qdisp(QDISP_URG, errbuf);
		return LM_SOFT;
	}
	if (IS_SET(ZFLG) && !(IS_SET(AFLG) || IS_SET(ZFLG))) {
		sprintf(errbuf, "%s: Option z can only be used with a|d\n",
			 pname);
		qdisp(QDISP_URG, errbuf);
		return LM_SOFT;
	}

        devp.dp_name = *av++;
        ac--;
        if (chknum(*av)) {
                sprintf(errbuf, "%s: Invalid Unit Number, %s\n", pname, *av);
		qdisp(QDISP_URG, errbuf);
                return LM_SOFT;
        }
        devp.dp_unit = (uint8) strtol(*av++, NULL, 0);
        ac--;

        if (chknum(*av)) {
                sprintf(errbuf, "%s: Invalid Device Offset, %s\n", pname, *av);
		qdisp(QDISP_URG, errbuf);
                return LM_SOFT;
        }
        devp.dp_offset = (int) strtol(*av++, NULL, 0);
        ac--;

	fs.fs_mounted = add_slash(*av);
	return LM_CLEAN;
}


/*
 *	add a slash to name
 */
char *
add_slash(char *cp)
{
	static	char	mdirnm[32];

	if (*cp != '/') {
		strcpy(mdirnm, "/");
		strcat(mdirnm, cp);
		return(mdirnm);
	} else
		return(cp);
}


/*
 *	subtract a slash from name
 */
char *
sub_slash(char *cp)
{
	if (*cp == '/')
		return(++cp);
	else
		return(cp);
}


/*
 *	simple, hah.
 */
void
usage(void)
{
	qdisp(QDISP_URG,
	      "USAGE: lmadmin -acdmtuvyzDT dev_name unit offset mount_dir\n");
	qdisp(QDISP_URG, "       -a       auto-maintain\n");
	qdisp(QDISP_URG, "       -c       check/repair\n");
	qdisp(QDISP_URG, "       -d       defragment\n");
	qdisp(QDISP_URG, "       -m       mount\n");
	qdisp(QDISP_URG, "       -t       display on TV monitor\n");
	qdisp(QDISP_URG, "       -u       unmount\n");
	qdisp(QDISP_URG, "       -v       verbose\n");
	qdisp(QDISP_URG, "       -y       yes\n");
	qdisp(QDISP_URG, "       -z       remove all zero size files\n");
	qdisp(QDISP_URG, "       -D       Debug only\n");
	qdisp(QDISP_URG, "       -T       Debug only\n");
}


/*
 *	make sure the argument is a decimal number
 */
int
chknum(char *s)
{
	while (*s) {
		if (*s > '9' || *s < '0')
			return LM_SOFT;
		s++;
	}
	return LM_CLEAN;
}


/*
 *	process defragmentation
 */
int
do_lmdf(void)
{
	int		err = 0;
	LinkedMemBlock	blkbuf, *curp;
	uint32		curnum, precurnum, compnum, precompnum, first = 1;
	uint32		totcomp = 0;

#ifdef	DEBUG
	printf("PERFORMING %s, dev:%s, unit:%d, offset:%d\n",
		 pname, devp.dp_name, devp.dp_unit, devp.dp_offset);
#endif	/* DEBUG */

	if ((err = init(0)) != 0)
		return err;

	curp = &blkbuf;
	curnum = precurnum = compnum = precompnum = fs.fs_rootblknum;
	memcpy(curp, &fs.fs_rootblk, hdrsize);
	while ((curnum != fs.fs_rootblknum) || first) {
		first = 0;
#ifdef	DEBUG
		printf("LOOPING (%d): ", curnum);
		prnt_blk(curp);
#endif	/* DEBUG */
		if ((curp->lmb_Fingerprint != FINGERPRINT_FREEBLOCK) &&
		    ((IS_SET(ZFLG) && !ZERO_SIZE(curp)) ||
		     (!IS_SET(ZFLG)))) {
			if (curnum != compnum) {
				if (copy_file(curnum, curp, compnum,
					      precompnum))
					return LM_SOFT;
			}
			precompnum = compnum;
			compnum += curp->lmb_BlockCount;
		}

		/*
		 *	how much space do we gain this time
		 */
		if ((curp->lmb_Fingerprint == FINGERPRINT_FREEBLOCK) &&
		    (curp->lmb_FlinkOffset != fs.fs_rootblknum))
			totcomp += curp->lmb_BlockCount;

		precurnum = curnum;
		curnum = curp->lmb_FlinkOffset;
		if (rawio(CMD_READ, curnum, hdrsize, (char *) curp,
	          	  "read block (do_lmdf)"))
			return LM_SOFT;
	}

	if ((precurnum != compnum) && (compnum != fs.fs_bcnt))
		if (set_lastblk(compnum, precompnum))
			return LM_SOFT;
	sprintf(errbuf, "%s: Compressed (%s, %d, %d) by %d blocks\n",
		pname, devp.dp_name, devp.dp_unit, devp.dp_offset,
		totcomp);
	qdisp(QDISP_URG, errbuf);
	return LM_CLEAN;
}



/*
 *	program initialization
 */
int
init(int verb)
{
	DiscLabel	dlab;

	if (inited)
		return LM_CLEAN;
	inited++;

	if ((devp.dp_item = OpenNamedDevice(devp.dp_name, NULL)) < 0) {
		sprintf(errbuf, "%s: Failed to open device %s: ",
			pname, devp.dp_name);
		qdisp(QDISP_URG, errbuf);
                PrintError(0,"open",devp.dp_name,devp.dp_item);
		return LM_SOFT;
        }

        if ((devp.dp_ioreqi = CreateIOReq(NULL, 50, devp.dp_item, 0)) < 0) {
		sprintf(errbuf, "%s: Failed to create IOReq: ", pname);
		qdisp(QDISP_URG, errbuf);
                PrintError(0,"create IOReq",0,devp.dp_ioreqi);
                return LM_SOFT;
        }

	devp.dp_ioreq = (IOReq *) LookupItem(devp.dp_ioreqi);
	memset(&devp.dp_ioinfo, 0, sizeof(IOInfo));

	devp.dp_devstat.ds_DeviceBlockSize = 1;	/* tmp to get things rolling */
	if (rawio(CMD_STATUS, 0, sizeof(devp.dp_devstat),
		  (char *) &devp.dp_devstat, "get device status (init)"))
		return LM_SOFT;

	if (rawio(CMD_READ, devp.dp_offset, sizeof(dlab), (char *) &dlab,
	          "read device label (init)"))
		return LM_SOFT;

	if (IS_SET(HATFLG)) {
		if (pass_0(&dlab))
			return LM_SOFT;
	}
#ifdef	DEBUG
	printf("Label: bsize[%d], bcount[%d], dev: bsize[%d], bcount[%d]\n",
		dlab.dl_VolumeBlockSize, dlab.dl_VolumeBlockCount,
		devp.dp_devstat.ds_DeviceBlockSize,
		devp.dp_devstat.ds_DeviceBlockCount);
#endif	/* DEBUG */

	if (dlab.dl_RecordType != 1 ||
            dlab.dl_VolumeSyncBytes[0] != VOLUME_SYNC_BYTE ||
            dlab.dl_VolumeSyncBytes[1] != VOLUME_SYNC_BYTE ||
            dlab.dl_VolumeSyncBytes[2] != VOLUME_SYNC_BYTE ||
            dlab.dl_VolumeSyncBytes[3] != VOLUME_SYNC_BYTE ||
            dlab.dl_VolumeSyncBytes[4] != VOLUME_SYNC_BYTE ||
            dlab.dl_RootDirectoryLastAvatarIndex > ROOT_HIGHEST_AVATAR ||
            (int) dlab.dl_RootDirectoryLastAvatarIndex < 0 ||
            dlab.dl_VolumeStructureVersion != VOLUME_STRUCTURE_LINKED_MEM){
		if (IS_SET(HATFLG)) {
			return LM_SOFT;
		} else {
                	sprintf(errbuf, "%s: Not a flat linked-memory filesystem %s\n",
			pname, devp.dp_name);
			qdisp(QDISP_URG, errbuf);
			return LM_HARD;
		}
        }

	if (verb) {
		sprintf(errbuf, "Volume Id: %s\n", dlab.dl_VolumeIdentifier);
		qdisp(QDISP_URG, errbuf);
		sprintf(errbuf, "Block Siz: %d\n", dlab.dl_VolumeBlockSize);
		qdisp(QDISP_URG, errbuf);
		sprintf(errbuf, "Block Cnt: %d\n", dlab.dl_VolumeBlockCount);
		qdisp(QDISP_URG, errbuf);
		sprintf(errbuf, "Root   at: %d\n",
			dlab.dl_RootDirectoryAvatarList[0]);
		qdisp(QDISP_URG, errbuf);
	}

	fs.fs_bsize = dlab.dl_VolumeBlockSize;
	fs.fs_rootblknum = dlab.dl_RootDirectoryAvatarList[0];
	fs.fs_bcnt = dlab.dl_VolumeBlockCount;
	hdrsize = sizeof(LinkedMemBlock);
	hdrblk = HOWMANY(hdrsize, fs.fs_bsize);
	cpbp = cpbuf;
	if (rawio(CMD_READ, fs.fs_rootblknum, hdrsize, (char *) &fs.fs_rootblk,
	          "read root block (init)"))
		return LM_SOFT;
	return LM_CLEAN;
}



/*
 *	set the rest of the FS to a single FREE block
 */
int
set_lastblk(uint32 compnum, uint32 precompnum)
{
	LinkedMemBlock	lastblk;

	lastblk.lmb_Fingerprint = FINGERPRINT_FREEBLOCK;
	lastblk.lmb_FlinkOffset = fs.fs_rootblknum;
	lastblk.lmb_BlinkOffset = precompnum;
	lastblk.lmb_BlockCount = fs.fs_bcnt - compnum;
	lastblk.lmb_HeaderBlockCount = hdrblk;
#ifdef DEBUG
	printf("Last FREE block(%d):", compnum);
	prnt_blk(&lastblk);
#endif /* DEBUG */
	if (rawio(CMD_WRITE, compnum, hdrsize, (char *) &lastblk,
		  "write last block (set_lastblk)"))
		return LM_SOFT;
	fs.fs_rootblk.lmb_BlinkOffset = compnum;
#ifdef DEBUG
	printf("Last FREE root(%d):", fs.fs_rootblknum);
	prnt_blk(&fs.fs_rootblk);
#endif /* DEBUG */
	if (rawio(CMD_WRITE, fs.fs_rootblknum, hdrsize, (char *) &fs.fs_rootblk,
	          "write root block (set_lastblk)"))
		return LM_SOFT;
	return LM_CLEAN;
}


/*
 *	copy the frombp file from block number fromblknum to
 *	toblknum. preblknum is used to set for Blink.
 *	NOTE: frombp is not to be updated (READ ONLY).
 */
uint32
copy_file(uint32 fromblknum, LinkedMemBlock *frombp,
	  uint32 toblknum, uint32 preblknum)
{
	LinkedMemBlock	lmb;
	uint32 blks_done, chunk, size = FSBLK2BYT(frombp->lmb_BlockCount);
	uint32 tmpfrom = fromblknum, tmpto = toblknum;

	for (blks_done = 0; blks_done < size; blks_done += chunk,
	    tmpfrom += chunk, tmpto += chunk) {
		chunk = MIN(CPBUFSZ, (size - blks_done));

		if (rawio(CMD_READ, tmpfrom, chunk, cpbp, "read file (copy_file)"))
			return LM_SOFT;
		if (rawio(CMD_WRITE, tmpto, chunk, cpbp, "write file (copy_file)"))
			return LM_SOFT;
	}

	/*
	 * 	now, install the new block header
	 */
	lmb.lmb_Fingerprint = frombp->lmb_Fingerprint;
	lmb.lmb_FlinkOffset = tmpto;
	lmb.lmb_BlinkOffset = preblknum;
	lmb.lmb_BlockCount = frombp->lmb_BlockCount;
	lmb.lmb_HeaderBlockCount = frombp->lmb_HeaderBlockCount;
	if (rawio(CMD_WRITE, toblknum, hdrsize, (char *) &lmb,
		  "write file header (copy_file)"))
		return LM_SOFT;

	return LM_CLEAN;
}


/*
 *	process filesystem check and repair.
 */
int
do_lmck(void)
{
	int	err = 0;

	sprintf(errbuf, "%s: VALIDATING (%s, %d, %d)\n", pname, devp.dp_name,
			devp.dp_unit, devp.dp_offset);
	qdisp(QDISP_URG, errbuf);
#ifdef	DEBUG
	printf("PERFORMING fsck dev:%s, unit:%d, offset:%d\n",
		devp.dp_name, devp.dp_unit, devp.dp_offset);
#endif	/* DEBUG */

	if ((err = init(0)) != 0)
		return err;
	if ((err = pass_1()) != 0)
		return err;
	if ((err = pass_2()) != 0)
		return err;
	if ((err = pass_3()) != 0)
		return err;

	sprintf(errbuf, "%s: (%s, %d, %d) is CLEAN\n", pname, devp.dp_name,
			devp.dp_unit, devp.dp_offset);
	qdisp(QDISP_URG, errbuf);

	return err;
}


/*
 *	pass 0: check and install the default discdata for this device
 *	we already have peformed CMD_STATUS on device.
 */
int
pass_0(DiscLabel *dlp)
{
	uint32	vcnt, vsize, dcnt, dsize;

	sprintf(errbuf, "%spass 0: checking volume label information\n", TAB);
	qdisp(QDISP_URG, errbuf);

	if (dlp->dl_RecordType != 1 ||
            dlp->dl_VolumeSyncBytes[0] != VOLUME_SYNC_BYTE ||
            dlp->dl_VolumeSyncBytes[1] != VOLUME_SYNC_BYTE ||
            dlp->dl_VolumeSyncBytes[2] != VOLUME_SYNC_BYTE ||
            dlp->dl_VolumeSyncBytes[3] != VOLUME_SYNC_BYTE ||
            dlp->dl_VolumeSyncBytes[4] != VOLUME_SYNC_BYTE ||
#ifdef	RANGE_CHECK
            dlp->dl_RootDirectoryLastAvatarIndex > ROOT_HIGHEST_AVATAR ||
            (int) dlp->dl_RootDirectoryLastAvatarIndex < 0 ||
#else	/* RANGE_CHECK */
            dlp->dl_RootDirectoryLastAvatarIndex != 0 ||
#endif	/* RANGE_CHECK */
            dlp->dl_VolumeStructureVersion != VOLUME_STRUCTURE_LINKED_MEM){
		sprintf(errbuf, "%s%s Forcing volume meta data\n", TAB, TAB);
		qdisp(QDISP_URG, errbuf);
		dlp->dl_RecordType = 1;
		memset(dlp->dl_VolumeSyncBytes, VOLUME_SYNC_BYTE,
			VOLUME_SYNC_BYTE_LEN);
		dlp->dl_VolumeStructureVersion = VOLUME_STRUCTURE_LINKED_MEM;
		dlp->dl_VolumeFlags = 0;
		strncpy(dlp->dl_VolumeCommentary, "reconstructed discdata",
			VOLUME_COM_LEN);
		strncpy(dlp->dl_VolumeIdentifier, sub_slash(fs.fs_mounted),
			VOLUME_ID_LEN);
		dlp->dl_VolumeUniqueIdentifier = -1;
		dlp->dl_RootUniqueIdentifier = -2;
		dlp->dl_RootDirectoryBlockCount = 0;
		dlp->dl_RootDirectoryLastAvatarIndex = 0;
	}
	vsize = dlp->dl_VolumeBlockSize;
	vcnt = dlp->dl_VolumeBlockCount;
	dsize = devp.dp_devstat.ds_DeviceBlockSize;
	dcnt = devp.dp_devstat.ds_DeviceBlockCount;
#ifdef	RANGE_CHECK
	if ((vsize % dsize != 0) || ((int) vsize < 1) ||
	    (vsize > ((dsize * dcnt) - devp.dp_offset))) {
#else	/* RANGE_CHECK */
	if (vsize != dsize) {
#endif	/* RANGE_CHECK */
		sprintf(errbuf, "%s%s Forcing volume block size (%d, %d)\n",
			TAB, TAB, vsize, dsize);
		qdisp(QDISP_URG, errbuf);
		vsize = dlp->dl_VolumeBlockSize = dsize;

	}
#ifdef	RANGE_CHECK
	if (((vcnt * vsize) > ((dcnt * dsize) - devp.dp_offset)) ||
	    ((int) vcnt < 1))  {
#else	/* RANGE_CHECK */
	if (vcnt != (dcnt - devp.dp_offset)) {
#endif	/* RANGE_CHECK */
		sprintf(errbuf, "%s%s Forcing volume block count (%d, %d)\n",
			TAB, TAB, vcnt, dcnt);
		qdisp(QDISP_URG, errbuf);
		vcnt = dlp->dl_VolumeBlockCount = dcnt - devp.dp_offset;
	}
#ifdef	RANGE_CHECK
	if (((int) dlp->dl_RootDirectoryBlockSize < 1) ||
	    (dlp->dl_RootDirectoryBlockSize >= ((dsize*dcnt)-devp.dp_offset))) {
#else	/* RANGE_CHECK */
	if (dlp->dl_RootDirectoryBlockSize != dsize) {
#endif	/* RANGE_CHECK */
		sprintf(errbuf, "%s%s Forcing root directory block size (%d, %d)\n",
			TAB, TAB, pname, dlp->dl_RootDirectoryBlockSize, dsize);
		qdisp(QDISP_URG, errbuf);
		dlp->dl_RootDirectoryBlockSize = dsize;
	}
	if (dlp->dl_RootDirectoryAvatarList[0] !=
	    HOWMANY(sizeof(DiscLabel), dsize)) {
		sprintf(errbuf, "%s%s Forcing root directory avatar list (%d, %d)\n",
			TAB, TAB, dlp->dl_RootDirectoryAvatarList[0],
			HOWMANY(sizeof(DiscLabel), dsize));
		qdisp(QDISP_URG, errbuf);
		dlp->dl_RootDirectoryAvatarList[0] =
					      HOWMANY(sizeof(DiscLabel), dsize);
	}

	if (rawio(CMD_WRITE, devp.dp_offset, sizeof(DiscLabel), (char *) dlp,
	          "write disclabel (pass_0)"))
		return LM_SOFT;

	return LM_CLEAN;

}


/*
 *	pass 1: check filesystem and device size consistency
 */
int
pass_1(void)
{

	sprintf(errbuf, "%spass 1: checking superblock and device consistency\n", TAB);
	qdisp(QDISP_URG, errbuf);


	if ((fs.fs_bsize < devp.dp_devstat.ds_DeviceBlockSize) ||
	    (fs.fs_bsize % devp.dp_devstat.ds_DeviceBlockSize != 0)) {
		sprintf(errbuf, "%s: Filesystem block size must be a multiple of device block size %s\n", pname, devp.dp_name);
		qdisp(QDISP_URG, errbuf);
		return LM_SOFT;
	}

	if((fs.fs_bcnt * fs.fs_bsize) > (devp.dp_devstat.ds_DeviceBlockSize *
		     devp.dp_devstat.ds_DeviceBlockCount)) {
		sprintf(errbuf, "%s: Filesystem size greater that device size %s\n", pname, devp.dp_name);
		qdisp(QDISP_URG, errbuf);
		return LM_SOFT;
	}

	return LM_CLEAN;
}


/*
 *	pass 2: check forward and backward block linkes
 */
int
pass_2(void)
{
	int cnt = 0;
	LinkedMemBlock	*nxtblk, *curblk;
	uint32	curblknum, nxtblknum;

	sprintf(errbuf, "%spass 2: checking block linkage\n", TAB);
	qdisp(QDISP_URG, errbuf);

	curblk = CURBLK(cnt);
	CURBLKNUM(cnt) = fs.fs_rootblknum;
	if (rawio(CMD_READ, fs.fs_rootblknum, hdrsize, (char *) curblk,
	          "read root block (pass_2)"))
		return LM_SOFT;
	if (IS_SET(HATFLG)) {
		if (fix_rootblk(curblk, fs.fs_rootblknum))
			return LM_SOFT;
	}

	if (curblk->lmb_Fingerprint != FINGERPRINT_ANCHORBLOCK) {
		sprintf(errbuf, "%s%s Invalid root block reconstruct (%d, 0x%x) (Y,N)?",
			TAB, TAB, fs.fs_rootblknum, curblk->lmb_Fingerprint);
		if (qask(errbuf)) {
			curblk->lmb_Fingerprint = FINGERPRINT_ANCHORBLOCK;
			curblk->lmb_FlinkOffset = fs.fs_rootblknum + hdrblk;
			curblk->lmb_BlockCount = hdrblk;
			curblk->lmb_HeaderBlockCount = hdrblk;
			if (rawio(CMD_WRITE, fs.fs_rootblknum, hdrsize,
				 (char *) curblk, "write root block (pass_2)"))
				return LM_SOFT;
		} else
			return LM_SOFT;
	}

	for (; curblk->lmb_FlinkOffset != fs.fs_rootblknum;
	     NXTBLKNUM(cnt) = nxtblknum, curblk = nxtblk, cnt++) {
		curblknum = CURBLKNUM(cnt);
		if (cnt >= MAX_LINKNUM) {
			sprintf(errbuf, "%s: Infinit loop detected in forward link, %d",
				 pname, cnt);
			qdisp(QDISP_URG, errbuf);
			return LM_SOFT;
		}
#ifdef	DEBUG
		printf("BLOCK (%d): ", curblknum);
		prnt_blk(curblk);
#endif	/* DEBUG */
		nxtblk = NXTBLK(cnt);

		/*
		 *	first, the size of block determines the next link.
		 *	second, the Flink determines the next link.
		 *	third, the Blink determines the next link.
		 *	finally, we are out of luck, mark the rest of the
		 *	filesystem as free block.
		 */
		if ((nxtblknum = nxtblk_bysize(curblknum, curblk, nxtblk)) != 0)
			continue;

		if ((nxtblknum= nxtblk_byflink(curblknum, curblk, nxtblk)) != 0)
			continue;

		if ((nxtblknum= nxtblk_byfands(curblknum, curblk, nxtblk)) != 0)
			continue;

		if ((nxtblknum= nxtblk_byblink(curblknum, curblk, nxtblk)) != 0)
			continue;

		if ((nxtblknum= nxtblk_byguess(curblknum, curblk, nxtblk)) != 0)
			continue;

		return(clear_fs(curblknum, curblk));
	}
	/*
	 * 	the last wrap around block
	 */
	curblknum = CURBLKNUM(cnt);
#ifdef	DEBUG
	printf("\t\tLAST BLOCK (%d): ", curblknum);
	prnt_blk(curblk);
#endif	/* DEBUG */
	return (validate_last(curblknum, curblk));
}


/*
 *	if root block is not correct force a clean root block.
 */
int
fix_rootblk(LinkedMemBlock *rootblkp, uint32 rootblknum)
{

	if (rootblkp->lmb_Fingerprint != FINGERPRINT_ANCHORBLOCK) {
		sprintf(errbuf, "%s%s Forcing root block fingerprint (0x%x)\n",
		 	TAB, TAB, rootblkp->lmb_Fingerprint);
		qdisp(QDISP_URG, errbuf);
		rootblkp->lmb_Fingerprint = FINGERPRINT_ANCHORBLOCK;
	}

	if (rootblkp->lmb_FlinkOffset != (rootblknum + hdrblk)) {
		sprintf(errbuf, "%s%s Forcing root block link (%d)\n",
		 	TAB, TAB, rootblkp->lmb_FlinkOffset);
		qdisp(QDISP_URG, errbuf);
		rootblkp->lmb_FlinkOffset = rootblknum + hdrblk;
	}

	/*
	 *	BlinkOffset is calculated at a later time
	 */

	if (rootblkp->lmb_BlockCount != hdrblk) {
		sprintf(errbuf, "%s%s Forcing root block count (%d)\n",
		 	TAB, TAB, rootblkp->lmb_BlockCount);
		qdisp(QDISP_URG, errbuf);
		rootblkp->lmb_BlockCount = hdrblk;
	}

	if (rootblkp->lmb_HeaderBlockCount != hdrblk) {
		sprintf(errbuf, "%s%s Forcing root header block count (%d)\n",
		 	TAB, TAB, rootblkp->lmb_HeaderBlockCount);
		qdisp(QDISP_URG, errbuf);
		rootblkp->lmb_HeaderBlockCount = hdrblk;
	}

	if (rawio(CMD_WRITE, rootblknum, hdrsize, (char *) rootblkp,
	          "write root block (fix_rootblk)"))
		return LM_SOFT;
	return LM_CLEAN;
}


/*
 *	pass 3: check file and block size consistency
 */
int
pass_3(void)
{
	int	cnt;
	LinkedMemBlock	rootblk;
	LinkedMemFileEntry	buf, *entp;
	uint32	maxallocated, entnum, entsize = sizeof(LinkedMemFileEntry);

	sprintf(errbuf, "%spass 3: checking file and block size consistency\n", TAB);
	qdisp(QDISP_URG, errbuf);
	entp = &buf;
	if (rawio(CMD_READ, fs.fs_rootblknum, hdrblk, (char *) &rootblk,
		  "read root block (pass_3)"))
		return LM_SOFT;

	for (cnt = 0, entnum = rootblk.lmb_FlinkOffset;
	     entnum != fs.fs_rootblknum;
	     cnt++, entnum = entp->lmfe.lmb_FlinkOffset) {
		if (cnt >= MAX_LINKNUM) {
			sprintf(errbuf, "%s: Infinit loop detected in forward link, %d",
				 pname, cnt);
			qdisp(QDISP_URG, errbuf);
			return LM_SOFT;
		}

		if (rawio(CMD_READ, entnum, entsize, (char *) entp,
			  "read file entry (pass_3)"))
			return LM_SOFT;

		if (entp->lmfe.lmb_Fingerprint != FINGERPRINT_FILEBLOCK) {
			if (entp->lmfe.lmb_HeaderBlockCount != hdrblk) {
				sprintf(errbuf,
				  "%s%s Adjust block header size for block %d (Y,N)?",
				   TAB, TAB, entnum);
				if (!qask(errbuf))
					continue;
				entp->lmfe.lmb_HeaderBlockCount = hdrblk;
				if (rawio(CMD_WRITE, entnum, entsize,
					  (char *) entp, "write file entry (pass_3)"))
					return LM_SOFT;
			}
		} else {	/* this is a file */
			maxallocated = FSBLK2BYT(entp->lmfe.lmb_BlockCount -
					       entp->lmfe.lmb_HeaderBlockCount);
			if (entp->lmfe_ByteCount > maxallocated) {
				sprintf(errbuf,
					"%s%s Truncate file \"%s\" to %d (Y,N)?",
					TAB, TAB, entp->lmfe_FileName, maxallocated);
				if (!qask(errbuf))
					continue;
				entp->lmfe_ByteCount = maxallocated;
				if (rawio(CMD_WRITE, entnum, entsize,
					  (char *) entp, "write file entry (pass_3)"))
					return LM_SOFT;
			}
			if (entp->lmfe_ByteCount < maxallocated) {
				sprintf(errbuf, "%s%s Warning: %d bytes unused in ",
					TAB, TAB, maxallocated - entp->lmfe_ByteCount);
				qdisp(QDISP_NORM, errbuf);
				sprintf(errbuf, "\"%s\", (%d, %d)\n",
				      entp->lmfe_FileName, entp->lmfe_ByteCount,
				      entp->lmfe.lmb_BlockCount);
				qdisp(QDISP_NORM, errbuf);
			}
		}
	}
	return LM_CLEAN;
}


/*
 *	cleanup
 */
void
cleanup(void)
{
	if (IS_SET(TFLG)) {
		WAITFORA;
		KillEventUtility();
    		DeleteScreenGroup(sgi);
	}
	return;
}


/*
 *	return 1, if reply is yes. Otherwise, 0.
 */
int
qask(char *s)
{

#ifdef	notdefined
	char c = 'y';

	qdisp(QDISP_NORM, s);
	if (IS_SET(YFLG)) {
		qdisp("QDISP_NORM, Yes.\n");
		return LM_SOFT;
	}

	while (1) {
		fflush(stdin);
		c = getchar();
		switch (c) {
		case 'y':
		case 'Y':
			return LM_SOFT;
		case 'n':
		case 'N':
			return LM_CLEAN;
		default:
			qdisp(QDISP_URG, "Invalid choice (Y,N)\n");
		}
	}

	return LM_CLEAN;

#else	/* notdefined */
	qdisp(QDISP_NORM, s);
	if (IS_SET(YFLG)) {
		qdisp(QDISP_NORM, "Yes\n");
		return LM_SOFT;
	} else
		return LM_CLEAN;
#endif	/* notdefined */
}



/*
 *	disply string
 */
void
qdisp(int flg, char *s)
{
	if ((flg == QDISP_URG) || ((flg == QDISP_NORM) && (IS_SET(VFLG)))) {
		printf("%s", s);
		if (IS_SET(TFLG))
			tv_prnt(s);
	}
}


/*
 *	read from the raw device
 *	caller must provide buffer large enough to hold the data
 *	buffer must also be a multiple of device block size, otherwise
 *	rawio will write passed it.
 */
int
rawio(uint8 cmd, uint32 offset, uint32 size, char *buf, char *errmsg)
{
	Err		err;

#ifdef	DEBUG
	if (cmd == CMD_WRITE)
		printf("rawio: Write, %d, %d, %s\n", offset, size, errmsg);
#endif	/* DEBUG */
	switch (cmd) {
	case CMD_STATUS:
	case CMD_READ:
		devp.dp_ioinfo.ioi_Send.iob_Buffer = NULL;
        	devp.dp_ioinfo.ioi_Send.iob_Len = 0;
		devp.dp_ioinfo.ioi_Recv.iob_Buffer = buf;
        	devp.dp_ioinfo.ioi_Recv.iob_Len = HOWMANY(size,
		 	devp.dp_devstat.ds_DeviceBlockSize) *
			devp.dp_devstat.ds_DeviceBlockSize;
		break;

	case CMD_WRITE:
		devp.dp_ioinfo.ioi_Recv.iob_Buffer = NULL;
        	devp.dp_ioinfo.ioi_Recv.iob_Len = 0;
		devp.dp_ioinfo.ioi_Send.iob_Buffer = buf;
        	devp.dp_ioinfo.ioi_Send.iob_Len = HOWMANY(size,
		 	devp.dp_devstat.ds_DeviceBlockSize) *
			devp.dp_devstat.ds_DeviceBlockSize;
		break;
	default:
		sprintf(errbuf, "%s: rawio: Invalid command %d: ", pname, cmd);
		qdisp(QDISP_URG, errbuf);
		return LM_SOFT;
	}

        devp.dp_ioinfo.ioi_Offset = offset;
        devp.dp_ioinfo.ioi_Unit = devp.dp_unit;
        devp.dp_ioinfo.ioi_Flags = 0;
        devp.dp_ioinfo.ioi_Command = cmd;
        devp.dp_ioinfo.ioi_CmdOptions = 0;
        err = DoIO(devp.dp_ioreqi, &devp.dp_ioinfo);
        if (err < 0 || (err = (devp.dp_ioreq)->io_Error) < 0) {
		sprintf(errbuf, "%s: Failed to %s (%d, %d, 0x%x, %d, 0x%x, %d): ",
			pname, errmsg, cmd, offset,
			devp.dp_ioinfo.ioi_Send.iob_Buffer,
        		devp.dp_ioinfo.ioi_Send.iob_Len,
			devp.dp_ioinfo.ioi_Recv.iob_Buffer,
        		devp.dp_ioinfo.ioi_Recv.iob_Len);
		qdisp(QDISP_URG, errbuf);
                PrintError(0,errmsg,0,err);
                return LM_SOFT;
        }

	/*
	 *	Kludge alert
	 * 	always keep root block info updated
	 */
	if ((cmd == CMD_WRITE) && (offset == fs.fs_rootblknum) &&
	    (buf != (char *) &fs.fs_rootblk)) {
		memcpy(&fs.fs_rootblk, buf, hdrsize);
	}
	return LM_CLEAN;
}


/*
 *	traverse through the filesystem backward looking for a block
 *	entry whose backward link points to the target block number.
 *	if found such block return its block number, else 0.
 */
uint32
trav_back(uint32 fromblk, uint32 targetblk, int flg)
{
	int cnt;
	LinkedMemBlock	buf, *curblk;
	uint32	curblknum, hdrsize = sizeof(LinkedMemBlock);
	uint32	preblknum = 0;

	curblk = &buf;
	curblk->lmb_BlinkOffset = 0;
	for (cnt = 0, curblknum = fromblk; curblk->lmb_BlinkOffset != fromblk;
	     preblknum = curblknum,curblknum = curblk->lmb_BlinkOffset, cnt++) {
		if (cnt >= MAX_LINKNUM) {
			sprintf(errbuf, "%s: Infinit loop detected in backward link, %d",
				 pname, cnt);
			qdisp(QDISP_URG, errbuf);
			return LM_CLEAN;
		}

		if (rawio(CMD_READ, curblknum, hdrsize, (char *) curblk,
	          	"read block (trav_back)"))
			return ((flg == TRAV_EXACT)? 0: preblknum);

		if (ALLBAD_BLK(curblk))
			return ((flg == TRAV_EXACT)? 0: preblknum);

		if ((curblk->lmb_BlinkOffset == targetblk) ||
		    ((flg == TRAV_BESTGUESS) &&
		     (curblk->lmb_BlinkOffset < targetblk)))
			return curblknum;
	}
	return LM_CLEAN;
}


/*
 *	Validate the last block of the filesystem
 */
int
validate_last(int32 lbnum, LinkedMemBlock *lbp)
{
	LinkedMemBlock	rootblk;
	uint32	hdrsize = sizeof(LinkedMemBlock);
	uint32	hdrblk = HOWMANY(hdrsize, fs.fs_bsize);

	if (rawio(CMD_READ, fs.fs_rootblknum, hdrsize, (char *) &rootblk,
	          "read root block (validate_last)"))
		return LM_SOFT;

	if (((lbp->lmb_Fingerprint == FINGERPRINT_FILEBLOCK) ||
	     (lbp->lmb_Fingerprint == FINGERPRINT_FREEBLOCK)) &&
	    (lbp->lmb_FlinkOffset == fs.fs_rootblknum) &&
	    (lbp->lmb_BlockCount == fs.fs_bcnt - lbnum) &&
	    (rootblk.lmb_BlinkOffset == lbnum))
		return LM_CLEAN;

	sprintf(errbuf, "%s%s Salvage last block (Y,N)? ", TAB, TAB);
	if (!qask(errbuf))
		return LM_SOFT;

	if (lbp->lmb_Fingerprint == FINGERPRINT_FILEBLOCK)
		lbp->lmb_HeaderBlockCount = sizeof(LinkedMemFileEntry);
	else {
		lbp->lmb_Fingerprint = FINGERPRINT_FREEBLOCK;
		lbp->lmb_HeaderBlockCount = hdrblk;
	}

	lbp->lmb_FlinkOffset = fs.fs_rootblknum;
	lbp->lmb_BlockCount = fs.fs_bcnt - lbnum;
	rootblk.lmb_BlinkOffset = lbnum;

	if (rawio(CMD_WRITE, fs.fs_rootblknum, hdrsize, (char *) &rootblk,
	  	  "write root block (last block)"))
		return LM_SOFT;

	if (rawio(CMD_WRITE, lbnum, hdrsize, (char *) lbp,
		  "write curr block (clearfs)"))
		return LM_SOFT;

	return LM_CLEAN;
}


/*
 *	determine the next block from size of this block
 */
uint32
nxtblk_bysize(uint32 curblknum, LinkedMemBlock *curblk, LinkedMemBlock *nxtblk)
{
	uint32	nxtblknum;

	if ((curblk->lmb_BlockCount < hdrblk) ||
	    (curblk->lmb_BlockCount > fs.fs_bcnt - curblknum))
		return LM_CLEAN;
	nxtblknum = curblknum + curblk->lmb_BlockCount;
	if (rawio(CMD_READ, nxtblknum, hdrsize, (char *) nxtblk,
          	  "read block (nxtblk_bysize)"))
		return LM_CLEAN;
	if (BAD_BLK(nxtblk))
		return LM_CLEAN;

	/* adjust links, if needed */
	if ((curblk->lmb_FlinkOffset != nxtblknum) ||
	    (nxtblk->lmb_BlinkOffset != curblknum)) {
		sprintf(errbuf, "%s%s Connect Flink/Blink (%d, %d) (Y,N)?",
			TAB, TAB, curblknum, nxtblknum);
		if (qask(errbuf)) {
			curblk->lmb_FlinkOffset = nxtblknum;
			nxtblk->lmb_BlinkOffset= curblknum;
			rawio(CMD_WRITE, curblknum, hdrsize, (char *) curblk,
			      "write block (nxtblk_bysize)");
			rawio(CMD_WRITE, nxtblknum, hdrsize, (char *) nxtblk,
			      "write block (nxtblk_bysize)");
		}
	}
	return nxtblknum;
}


/*
 *	determine the next block from the forward link of this block
 */
uint32
nxtblk_byflink(uint32 curblknum, LinkedMemBlock *curblk, LinkedMemBlock *nxtblk)
{
	uint32	nxtblknum, blkcnt;

	if ((curblk->lmb_FlinkOffset < curblknum + hdrblk) ||
	    (curblk->lmb_FlinkOffset > fs.fs_bcnt - hdrblk))
		return LM_CLEAN;
	nxtblknum = curblk->lmb_FlinkOffset;
	if (rawio(CMD_READ, nxtblknum, hdrsize, (char *) nxtblk,
          	  "read block (nxtblk_byflink)"))
		return LM_CLEAN;
	if (BAD_BLK(nxtblk))
		return LM_CLEAN;

	blkcnt = nxtblknum - curblknum;
	/* adjust size, if needed */
	if ((curblk->lmb_BlockCount != blkcnt) ||
	    (nxtblk->lmb_BlinkOffset != curblknum)) {
		sprintf(errbuf,"%s%s Adjust block size/Blink (%d, %d) (Y,N)?",
			TAB, TAB, curblknum, nxtblknum);
		if (qask(errbuf)) {
			curblk->lmb_BlockCount = blkcnt;
			nxtblk->lmb_BlinkOffset = curblknum;
			rawio(CMD_WRITE, curblknum, hdrsize, (char *) curblk,
			      "write block (nxtblk_byflink)");
			rawio(CMD_WRITE, nxtblknum, hdrsize, (char *) nxtblk,
			      "write block (nxtblk_byflink)");
		}
	}
	return nxtblknum;
}


/*
 *	if flink and size point to the same location, then that location
 *	must be a valid block that got corrupted. since we already know by
 *	now that nxtblk does not have a valid fingerprint. Force a block there
 *	and let's continue from that point on. If it is a bad block it will be
 *	forced to a free block anyway.
 */
uint32
nxtblk_byfands(uint32 curblknum, LinkedMemBlock *curblk, LinkedMemBlock *nxtblk)
{
	uint32	nxtblknum;

	if ((curblk->lmb_FlinkOffset != curblk->lmb_BlockCount + curblknum) ||
	    (curblk->lmb_BlockCount < hdrblk) ||
	    (curblk->lmb_BlockCount > fs.fs_bcnt - curblknum))
		return LM_CLEAN;

	nxtblknum = curblk->lmb_FlinkOffset;
	if (rawio(CMD_READ, nxtblknum, hdrsize, (char *) nxtblk,
          	  "read block (nxtblk_byfands)"))
		return LM_CLEAN;

	sprintf(errbuf,"%s%s Force a FREE block on %d (Y,N)?",
		TAB, TAB, nxtblknum);
	if (qask(errbuf)) {
		nxtblk->lmb_Fingerprint = FINGERPRINT_FREEBLOCK;
		nxtblk->lmb_BlinkOffset = curblknum;
		nxtblk->lmb_HeaderBlockCount = hdrblk;
		rawio(CMD_WRITE, nxtblknum, hdrsize, (char *) nxtblk,
		      "write block (nxtblk_byfands)");
	}
	return nxtblknum;
}


/*
 *	determine the next block from the backward links
 */
uint32
nxtblk_byblink(uint32 curblknum, LinkedMemBlock *curblk, LinkedMemBlock *nxtblk)
{
	uint32	nxtblknum, blkcnt;

	if ((nxtblknum = trav_back(fs.fs_rootblknum, curblknum, TRAV_EXACT)) == 0)
		return LM_CLEAN;
#ifdef	DEBUG
	printf("BYBLINK: trav_back (%d): ret(%d)\n", curblknum, nxtblknum);
#endif	/* DEBUG */
	if (rawio(CMD_READ, nxtblknum, hdrsize, (char *) nxtblk,
          	  "read block (nxtblk_byblink)"))
		return LM_CLEAN;
	if (BAD_BLK(nxtblk))
		return LM_CLEAN;

	blkcnt = nxtblknum - curblknum;
	/* adjust size and link */
	sprintf(errbuf, "%s%s Adjust block size/Flink (%d) (Y,N)?",
		TAB, TAB, curblknum);
	if (qask(errbuf)) {
		curblk->lmb_BlockCount = blkcnt;
		curblk->lmb_FlinkOffset = nxtblknum;
		rawio(CMD_WRITE, curblknum, hdrsize, (char *) curblk,
			"write block (nxtblk_byblink)");
	}
	return nxtblknum;
}


/*
 *	determine the next block from the backward link using best guess
 */
uint32
nxtblk_byguess(uint32 curblknum, LinkedMemBlock *curblk, LinkedMemBlock *nxtblk)
{
	uint32	nxtblknum, blkcnt;

	if ((nxtblknum = trav_back(fs.fs_rootblknum, curblknum,
				   TRAV_BESTGUESS)) == 0)
		return LM_CLEAN;
#ifdef	DEBUG
	printf("BYGUESS: trav_back (%d): ret(%d)\n", curblknum, nxtblknum);
#endif	/* DEBUG */
	if (rawio(CMD_READ, nxtblknum, hdrsize, (char *) nxtblk,
          	  "read block (nxtblk_byguess)"))
		return LM_CLEAN;
	if (BAD_BLK(nxtblk))		/* trav_back may return ANCHOR */
		return LM_CLEAN;

	blkcnt = nxtblknum - curblknum;
	/* adjust size and link */
	sprintf(errbuf, "%s%s Set block FREE (%d, %d) (Y,N)?",
		TAB, TAB, curblknum, blkcnt);
	if (qask(errbuf)) {
		curblk->lmb_Fingerprint = FINGERPRINT_FREEBLOCK;
		curblk->lmb_HeaderBlockCount = hdrblk;
		curblk->lmb_BlockCount = blkcnt;
		curblk->lmb_FlinkOffset = nxtblknum;
		rawio(CMD_WRITE, curblknum, hdrsize, (char *) curblk,
			"write block (nxtblk_byguess)");
		nxtblk->lmb_BlinkOffset = curblknum;
		rawio(CMD_WRITE, nxtblknum, hdrsize, (char *) nxtblk,
			"write block (nxtblk_byguess)");
	}
	return nxtblknum;
}


/*
 *	mark the rest of the filesystem blocks as FREE
 */
int
clear_fs(uint32 curblknum, LinkedMemBlock *curblk)
{
	LinkedMemBlock rootblk;

	sprintf(errbuf, "%s%SClear filesystem from block %d (Y,N)?",
		TAB, TAB, curblknum);
	if (!qask(errbuf))
		return LM_CLEAN;

	curblk->lmb_Fingerprint = FINGERPRINT_FREEBLOCK;
	curblk->lmb_FlinkOffset = fs.fs_rootblknum;
	curblk->lmb_BlockCount = fs.fs_bcnt - curblknum;
	curblk->lmb_HeaderBlockCount = hdrblk;

	if (rawio(CMD_READ, fs.fs_rootblknum, hdrsize, (char *) &rootblk,
          	  "read root block (clear_fs)"))
		return LM_SOFT;

	rootblk.lmb_BlinkOffset = curblknum;
	if (rawio(CMD_WRITE, fs.fs_rootblknum, hdrsize, (char *) &rootblk,
	      "write root block (clear_fs)"))
		return LM_SOFT;
	if (rawio(CMD_WRITE, curblknum, hdrsize, (char *) curblk,
	      "write block (clear_fs)"))
		return LM_SOFT;
	return LM_CLEAN;
}


/*
 *	is the FS mounted
 */
int
is_mounted(char *mntdir)
{
	Item	ii;

	if ((ii = OpenDiskFile(mntdir)) > 0) {
		CloseDiskFile(ii);
		return LM_SOFT;
	} else
		return LM_CLEAN;
}


/*
 *	mount/umount FS
 */
int
mumount(int flg)
{
	Item	ii;
	int32	err;

	if (flg == MU_MOUNT) {
                if ((ii = OpenNamedDevice(devp.dp_name, NULL)) < 0) {
                        sprintf(errbuf, "%s: Failed to open %s: ",
				pname, devp.dp_name);
			qdisp(QDISP_URG, errbuf);
                        PrintError(0,"open",devp.dp_name,ii);
			return LM_SOFT;
                }
                if ((err = MountFileSystem(ii, devp.dp_unit,
					       devp.dp_offset)) < 0) {
                        sprintf(errbuf, "%s: Failed to mount %s: ",
				pname, devp.dp_name);
			qdisp(QDISP_URG, errbuf);
                        PrintError(0,"mount",devp.dp_name,err);
			return LM_SOFT;
                }
	} else if (flg == MU_UNMOUNT) {
                if ((err = DismountFileSystem(fs.fs_mounted)) < 0) {
                        sprintf(errbuf, "%s: Failed to unmount %s: ",
				 pname, fs.fs_mounted);
			qdisp(QDISP_URG, errbuf);
                        PrintError(0,"unmount",fs.fs_mounted,err);
			return LM_SOFT;
                }
	} else {
		sprintf(errbuf, "%s: mumount invalid flg %d\n", pname, flg);
		qdisp(QDISP_URG, errbuf);
		return LM_SOFT;
	}
	return LM_CLEAN;
}


/*
 *	debug stuff
 */
int
lm_dumpfs(void)
{
	int	cnt, err;
	LinkedMemBlock	rootblk;
	LinkedMemFileEntry	buf, *entp;
	uint32	entnum, entsize = sizeof(LinkedMemFileEntry);

	if ((err = init(1)) != 0)
		return err;
	entp = &buf;
	if (rawio(CMD_READ, fs.fs_rootblknum, hdrblk, (char *) &rootblk,
		  "read root block (lm_dumpfs)"))
		return LM_SOFT;
	lm_dumpblk(&rootblk, fs.fs_rootblknum), printf("\n");

	for (cnt = 0, entnum = rootblk.lmb_FlinkOffset;
	     entnum != fs.fs_rootblknum;
	     cnt++, entnum = entp->lmfe.lmb_FlinkOffset) {
		if (cnt >= MAX_LINKNUM) {
			sprintf(errbuf, "%s: Infinit loop detected in forward link, %d",
				 pname, cnt);
			qdisp(QDISP_URG, errbuf);
			return LM_SOFT;
		}

		if (rawio(CMD_READ, entnum, entsize, (char *) entp,
			  "read file entry (lm_fumpfs)"))
			return LM_SOFT;
		lm_dumpent(entp, entnum);
	}
	return LM_CLEAN;
}


/*
 *	TV display stuff
 */
void
tv_prnt(char *str)
{
	char *cp;

	if (!IS_SET(TFLG))
		return;
	cp = str;
	while (cp)
		cp = tv_prnt_aline(cp);
}


char *
tv_prnt_aline(char *cp)
{
	char line[CHARS_PER_LINE + 1];
	int len = (int) strlen(cp);

	if (gcon.gc_PenY > (NEXT_LINE * LINES_IN_SCREEN)) {
		WAITFORA;
		tv_clrscr();
	}

    	gcon.gc_PenX = PIXL_START_X;
	if (len > CHARS_PER_LINE) {
		(void) strncpy(line, cp, CHARS_PER_LINE);
    		DrawText8(&gcon, BitmapItem, line);
  		gcon.gc_PenY += NEXT_LINE;
		return (cp + CHARS_PER_LINE);
	} else {
    		DrawText8(&gcon, BitmapItem, cp);
  		gcon.gc_PenY += NEXT_LINE;
		return NULL;
	}
}


void
blank_out(char *cp, int len)
{
	int i;

	for (i = 0; i < len; i++, cp++)
		*cp = ' ';
	cp = '\0';
}


void
tv_clrscr()
{
	int i;
	char tmp[CHARS_PER_LINE+10];

	blank_out(tmp, CHARS_PER_LINE + 9);
	gcon.gc_PenY = 0;
	for (i = 0; i < (LINES_IN_SCREEN + 8); i++) {
    		gcon.gc_PenX = 0;
    		DrawText8(&gcon, BitmapItem, tmp);
		gcon.gc_PenY += (NEXT_LINE - 1);
	}
	gcon.gc_PenX = PIXL_START_X;
	gcon.gc_PenY = PIXL_START_Y;
}


/*
 *	debug stuff: print a block
 */
void
lm_dumpblk(LinkedMemBlock *bp, uint32 boff)
{
	char tmpbf[32];

	switch(bp->lmb_Fingerprint) {
	case FINGERPRINT_ANCHORBLOCK:
		sprintf(tmpbf, "ANCHOR");
		break;
	case FINGERPRINT_FREEBLOCK:
		sprintf(tmpbf, "FREE");
		break;
	case FINGERPRINT_FILEBLOCK:
		sprintf(tmpbf, "FILE");
		break;
	default:
		sprintf(tmpbf, "BOGUS (0x%x)\n", bp->lmb_Fingerprint);
	}

	sprintf(errbuf, "(%d) %d (%d) %d, %s ",
		bp->lmb_BlinkOffset,
		boff,
		bp->lmb_FlinkOffset,
		bp->lmb_BlockCount,
		tmpbf);
	qdisp(QDISP_URG, errbuf);
}


/*
 *	debug stuff: print an entry
 */
void
lm_dumpent(LinkedMemFileEntry *ep, uint32 eoff)
{
	lm_dumpblk(&ep->lmfe, eoff);

	if (ep->lmfe.lmb_Fingerprint == FINGERPRINT_FILEBLOCK) {
		sprintf(errbuf, "%d, %s\n", ep->lmfe_ByteCount,
			ep->lmfe_FileName);
		qdisp(QDISP_URG, errbuf);
	} else	/* this must remain just a printf */
		printf("\n");
}


#ifdef	DEBUG
/*
 *	print debug info for a block
 */
void
prnt_blk(LinkedMemBlock	*blk)
{
	printf("Flink[%d], Blink[%d], Bcount[%d], Bhcount[%d]\n",
		blk->lmb_FlinkOffset, blk->lmb_BlinkOffset,
		blk->lmb_BlockCount, blk->lmb_HeaderBlockCount);
}
#endif	/* DEBUG */
