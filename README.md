# 3DO Opera Portfolio OS

The 3DO Opera platform ran an OS called Portfolio.  Developed internally at
NTG/3DO by several of the same people who developed the operating system for
the Amiga computer, Portfolio takes many of its design cues from Amiga.
Indeed, Amiga programmers will find Portfolio quite familiar.  However,
Portfolio addresses many of the shortcomings levelled against Amiga, making
it quite advanced for the early 1990's, particularly when compared to
mystifyingly more popular operating systems such as MS-DOS.

Among other features, Portfolio provides:

  - Preemptive threading/multitasking,
  - User/supervisor separation
  - Memory protection (MMU/fences) and process separation,
  - Resource tracking -- memory and I/O resources are released when a
    process dies,
  - Asynchronous, high-level I/O system,
  - Shared libraries,
  - Dynamically loaded libraries and device drivers,
  - Message-based IPC,
  - Custom filesystem,
  - Extensible global error codes,
  - Input handling for joypads, joysticks, mice, and light gun.

Portfolio was created to ease development on the platform, but it also acted
as an abstraction to the hardware so that manufacturers had more flexibility
in their design and provide for compatibility with future systems (which
were designed but never released to retail).

On 2022-01-07 a snapshot of the Opera's Portfolio OS from 1995-02-10 was
uploaded to Archive.org by user
[EagleSoft](https://archive.org/details/@eaglesoft).


## What is This Repo?

This repo is a copy of the Portfolio OS source code which was released by
EagleSoft on 2022-01-07.  While I wasn't responsible for the release of the
Portfolio source code, I was in contact with the individual who did so prior
to release.  I also run the site [3DO Development Repo](https://3dodev.com),
and am the primary maintainer of the libretro 3DO emulator
[Opera](https://docs.libretro.com/library/opera).  This repo is, at the very
least, a GitHub hosted backup and reference, but may be a starting place for
future development.

The initial checkin represents a snapshot of the Opera source tree from
around 1995.  Code revision was managed internally at NTG using
[RCS](https://en.wikipedia.org/wiki/Revision_Control_System), but the RCS
history is not present here.

Portfolio is *vaguely* portable, as it was successfully ported to IBM's POWER
architecture ([PowerPC
602](https://en.wikipedia.org/wiki/PowerPC_600#PowerPC_602)) as part of the
development for M2.  However, that port does not appear here.
