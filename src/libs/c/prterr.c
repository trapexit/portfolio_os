/* $Id: prterr.c,v 1.10 1994/09/10 02:52:22 vertex Exp $ */

#include <types.h>
#include <stdio.h>
#include <kernel.h>
#include <operror.h>

/**
|||	AUTODOC PUBLIC spg/clib/printerror
|||	PrintError - Print the error string for an error
|||
|||	  Synopsis
|||
|||	    void PrintError( char *who, char *what, char *whom, Err err )
|||
|||	  Description
|||
|||	    On DEVELOPMENT builds, this macro calls a procedure that
|||	    prints the error string for an error to the Macintosh
|||	    console for a 3DO development system.  This has the same
|||	    effect as using printf() to print the who, what and whom
|||	    strings with the string constructed by GetSysErr().
|||
|||	    On non-DEVELOPMENT builds, this macro expands to an empty
|||	    statement, reducing code size and avoiding pulling in
|||	    lots of extraneous junk from the support libraries.
|||
|||	    To copy an error string into a buffer instead of sending
|||	    it the console, use GetSysErr().
|||
|||	    In the task called "shell", the C statement
|||
|||	        PrintError((char *)0, "open", filename, errno)
|||
|||	    might result in something like this:
|||
|||	        +================ Terminal ================
|||	        | FFS-Severe-System-extended-No such file
|||	        | shell: unable to open "badfile"
|||
|||
|||	  Arguments
|||
|||	    who                         a string that identifies the task,
|||	                                folio, or module that encountered the
|||	                                error; if a NULL pointer or a
|||	                                zero-length string is passed, the name
|||	                                of the current task is used (or, if
|||	                                there is no current task, "notask").
|||
|||	    what                        a string that describes what action
|||	                                was being taken that failed. Normally,
|||	                                the words "unable to" are printed
|||	                                before this string to save client data
|||	                                space; if the first character of the
|||	                                "what" string is a backslash, these
|||	                                words are not printed.
|||
|||	    whom                        a string that describes the object to
|||	                                which the described action is being
|||	                                taken (for instance, the name of a
|||	                                file). It is printed surrounded by
|||	                                quotes; if a NULL pointer or zero
|||	                                length string is passed, the quotes
|||	                                are not printed.
|||
|||	    err                         a (normally negative) number denoting
|||	                                the error that occurred, which will be
|||	                                decoded with GetSysErr. If not
|||	                                negative, then no decoded error will
|||	                                be printed.
|||
|||	  Implementation
|||
|||	    Macro implemented in the operror.h V21.
|||
|||	  Associated Files
|||
|||	    operror.h                   ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  See Also
|||
|||	    PrintfSysErr(), GetSysErr()
|||
**/

void
clib_PrintError(char *who, char *what, char *whom, Err err)
{
#ifdef	DEVELOPMENT
	Task *ct;

	if (!who || !*who) {
		if ((ct = KernelBase->kb_CurrentTask) != (Task *)0)
			who = ct->t.n_Name;
		else
			who = "notask";
	}
	printf("%s:", who);
	if (!what || !*what) {
		if (whom && *whom)
			what = "\\error from";
		else
			what = "\\error";
	}
	if (*what != '\\')
		printf(" unable to");
	else
		what++;
	printf(" %s", what);
	if (whom && *whom)
		printf(" '%s'", whom);
	printf("\n");
	if (err < 0)
		PrintfSysErr(err);
#endif
}
