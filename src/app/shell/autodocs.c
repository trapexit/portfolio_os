/* $Id: autodocs.c,v 1.5 1994/09/10 02:33:25 vertex Exp $ */

/**
|||	AUTODOC PUBLIC tpg/shell/showkernelbase
|||	showkernelbase - display a few fields of interest from KernelBase.
|||
|||	  Format
|||
|||	    showkernelbase
|||
|||	  Description
|||
|||	    This command displays a few fields of interest from the system's
|||	    KernelBase data structure.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    kbprint
**/

/**
|||	AUTODOC PUBLIC tpg/shell/showavailmem
|||	showavailmem - display information about the amount of memory
|||	               currently available in the system.
|||
|||	  Format
|||
|||	    showavailmem
|||
|||	  Description
|||
|||	    This command displays information about the amount of memory
|||	    installed in the system, and the amount of memory currently
|||	    free.
|||
|||	    This command also displays the amount of memory currently consumed
|||	    by supervisor-mode code.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    availmem
|||
|||	  See Also
|||
|||	    showmemmap, showmemlist
**/

/**
|||	AUTODOC PUBLIC tpg/shell/showmemlist
|||	showmemlist - show the contents of a memory list.
|||
|||	  Format
|||
|||	    showmemlist <memory list address>
|||
|||	  Description
|||
|||	    This command displays all the chunks of memory currently
|||	    available in a memory list.
|||
|||	  Arguments
|||
|||	    <memory list address>       The address of the memory list to
|||	                                display. This address is usually
|||	                                taken from the output of another
|||	                                shell command such as showtask or
|||	                                showavailmem.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    memlist
|||
|||	  See Also
|||
|||	    showmemmap, showavailmem, showtask
**/

/**
|||	AUTODOC PUBLIC tpg/shell/showmemmap
|||	showmemmap - display a page map showing which pages of memory
|||	             are used and free in the system, and which task
|||	             owns which pages.
|||
|||	  Format
|||
|||	    showmemmap [task name | task item number]
|||
|||	  Description
|||
|||	    This command displays a page map on the debugging terminal showing
|||	    all memory pages in the system, and which task owns each
|||	    page.
|||
|||	  Arguments
|||
|||	    <task name | task item num> This specifies the name or item number
|||	                                of the task to pay special attention
|||	                                to. The item number can be in decimal,
|||	                                or in hexadecimal starting with 0x or
|||	                                $. When a task is specified, any
|||	                                pages owned by that task will be marked
|||	                                with a '*'. This makes it quicker to
|||	                                find pages used by a specific task.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    memmap
|||
|||	  See Also
|||
|||	    showavailmem, showmemlist
**/

/**
|||	AUTODOC PUBLIC tpg/shell/showerror
|||	showerror - display an error string associated with a system error
|||	            code
|||
|||	  Format
|||
|||	    showerr <error number>
|||
|||	  Description
|||
|||	    This command displays the error string associated with a
|||	    numerical system error code.
|||
|||	  Arguments
|||
|||	    <error number>              The error code to display the
|||	                                string of. This number can be in
|||	                                decimal, or in hexadecimal starting
|||	                                with 0x or $.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V21.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    err
**/

/**
|||	AUTODOC PUBLIC tpg/shell/setalias
|||	setalias - set a file path alias
|||
|||	  Format
|||
|||	    setalias <alias name> <str>
|||
|||	  Description
|||
|||	    This command lets you create a filesystem path alias. Once
|||	    created, the alias can be referenced from anywhere in the system.
|||
|||	  Arguments
|||
|||	    <alias name>                The name of the alias to create.
|||
|||	    <str>                       The string that should be substituted
|||	                                whenever the alias is encountered
|||	                                when parsing directory and file names.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
**/

/**
|||	AUTODOC PUBLIC tpg/shell/setminmem
|||	setminmem - set the amount of memory available in the system to
|||	            the minimum amount of memory guaranteed to be available
|||	            in a production environment.
|||
|||	  Format
|||
|||	    setminmem
|||
|||	  Description
|||
|||	    This command causes the shell to adjust the amount of
|||	    memory available in the system to match the minimum amount
|||	    a memory a title must be able to run in.
|||
|||	    The setmaxmem command can be used to restore the amount of
|||	    memory to the maximum available in the current system.
|||
|||	    The showshellvars command can be used to display whether
|||	    setminmem is currently active.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    minmem
|||
|||	  See Also
|||
|||	    setmaxmem, showshellvars
**/

/**
|||	AUTODOC PUBLIC tpg/shell/setmaxmem
|||	setmaxmem - set the amount of memory available in the system to
|||	            the maximum amount possible.
|||
|||	  Format
|||
|||	    setmaxmem
|||
|||	  Description
|||
|||	    This command causes the shell to adjust the amount of
|||	    memory available in the system to be the maximum supported by
|||	    the hardware. This effectively undoes a previous use of the
|||	    setminmem command.
|||
|||	    The showshellvars command can be used to display whether
|||	    setminmem is currently active.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    maxmem
|||
|||	  See Also
|||
|||	    setminmem, showshellvars
**/

/**
|||	AUTODOC PUBLIC tpg/shell/setbg
|||	setbg - set the shell's default behavior to background execution mode.
|||
|||	  Format
|||
|||	    setbg
|||
|||	  Description
|||
|||	    This command sets the shell's default execution mode to
|||	    background. This prevents the shell from waiting for tasks
|||	    to complete when they are executed. The shell returns
|||	    immediately and is ready to accept more commands.
|||
|||	    If the shell is currently in foreground mode and you wish to
|||	    to execute only a single program in background mode, you can
|||	    append a '&' at the end of the command-line.
|||
|||	    You can display the current execution mode by using the
|||	    showshellvars command.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    bg
|||
|||	  See Also
|||
|||	    setfg, showshellvars
**/

/**
|||	AUTODOC PUBLIC tpg/shell/setfg
|||	setfg - set the shell's default behavior to foreground execution mode.
|||
|||	  Format
|||
|||	    setfg
|||
|||	  Description
|||
|||	    This command sets the shell's default execution mode to
|||	    foreground. This forces the shell to wait for tasks to
|||	    complete when they are executed. The shell will not accept
|||	    new commands until the current task completes.
|||
|||	    If the shell is currently in background mode and you wish to
|||	    to execute only a single program in foreground mode, you can
|||	    append a '#' at the end of the command-line.
|||
|||	    You can display the current execution mode by using the
|||	    showshellvars command.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    fg
|||
|||	  See Also
|||
|||	    setbg, showshellvars
**/

/**
|||	AUTODOC PUBLIC tpg/shell/setverbose
|||	setverbose - control the shell's verbosity mode
|||
|||	  Format
|||
|||	    setverbose <on | off>
|||
|||	  Description
|||
|||	    This command lets you turn the shell's verbosity mode on or
|||	    off. When in verbose mode, the shell displays some extra
|||	    information when performing its work. This extra information
|||	    can sometimes be useful when debugging programs.
|||
|||	    You can display the current verbosity mode by using the
|||	    showshellvars command.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    verbose
|||
|||	  See Also
|||
|||	    showshellvars
**/

/**
|||	AUTODOC PUBLIC tpg/shell/setpri
|||	setpri - set the shell's priority
|||
|||	  Format
|||
|||	    setpri <priority>
|||
|||	  Description
|||
|||	    This command sets the priority of the shell's task. It is
|||	    sometimes desirable to boost the shell's priority to a high
|||	    number. This lets commands such as showtask or memmap work
|||	    with more accuracy.
|||
|||	    The current priority of the shell can be displayed using the
|||	    the showshellvars command.
|||
|||	  Arguments
|||
|||	    <priority>                  The new shell priority. This value
|||	                                must be in the range 10..199
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    shellpri
|||
|||	  See Also
|||
|||	    showshellvars
**/

/**
|||	AUTODOC PUBLIC tpg/shell/setcd
|||	setcd - set the shell's current directory
|||
|||	  Format
|||
|||	    setcd [directory name]
|||
|||	  Description
|||
|||	    The shell maintains the concept of a current directory.
|||	    Files within the current directory can be referenced without
|||	    a full path specification, in a relative manner.
|||
|||	    This command lets you specify the name of a directory that
|||	    should become the current directory.
|||
|||	    The current directory of the shell can be displayed using the
|||	    the showcd command, or by executing setcd with no arguments.
|||
|||	  Arguments
|||
|||	    [directory name]            The name of the directory that should
|||	                                become the new current directory. If
|||	                                this argument is not supplied, then
|||	                                the name of the current directory
|||	                                is displayed, and is not changed.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    cd
|||
|||	  See Also
|||
|||	    showcd
**/

/**
|||	AUTODOC PUBLIC tpg/shell/sleep
|||	sleep - cause the shell to pause for a number of seconds
|||
|||	  Format
|||
|||	    sleep <number of seconds>
|||
|||	  Description
|||
|||	    This command tells the shell to go to sleep for a given
|||	    number of seconds. The shell will not accept commands while
|||	    it is sleeping.
|||
|||	  Arguments
|||
|||	    <number of seconds>         The number of seconds to sleep for.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
**/

/**
|||	AUTODOC PUBLIC tpg/shell/killtask
|||	killtask - remove an executing task or thread from the system.
|||
|||	  Format
|||
|||	    killtask <task name | task item number>
|||
|||	  Description
|||
|||	    This command removes a task or thread from the system. When
|||	    removing a task, all resources used by the task are also
|||	    returned to the system.
|||
|||	  Arguments
|||
|||	    <task name | task item num> This specifies the name or item number
|||	                                of the task to remove. The item
|||	                                number can be in decimal, or in
|||	                                hexadecimal starting with 0x or $.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    kill
|||
|||	  See Also
|||
|||	    showtask
**/

/**
|||	AUTODOC PUBLIC tpg/shell/showtask
|||	showtask - display information about task in the system.
|||
|||	  Format
|||
|||	    showtask [task name | task item number]
|||
|||	  Description
|||
|||	    This command displays information about task and threads in the
|||	    system. It can also be given a specific task or thread name,
|||	    in which case a more detailled output is produced describing
|||	    the specified task exclusively.
|||
|||	  Arguments
|||
|||	    <task name | task item num> This specifies the name or item number
|||	                                of the task to display the information
|||	                                about. The item number can be in
|||	                                decimal, or in hexadecimal starting
|||	                                with 0x or $. If this argument is not
|||	                                supplied, then general information
|||	                                about all tasks in the system is
|||	                                displayed.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    ps
**/

/**
|||	AUTODOC PUBLIC tpg/shell/showshellvars
|||	showshellvars - show the state of various shell variables
|||
|||	  Format
|||
|||	    showshellvars
|||
|||	  Description
|||
|||	    This command displays the current state of various internal shell
|||	    variables. The information displayed includes:
|||
|||	      Shell Priority  (10..199)
|||	      Execution Mode  (Background or Foreground)
|||	      Memory Amount   (Maximum, Minimum)
|||	      Shell Verbosity (ON or OFF)
|||
|||	  Implementation
|||
|||	    Command implemented in shell V21.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  See Also
|||
|||	    setfg, setbg, setpri, setminmem, setmaxmem, setverbose
**/

/**
|||	AUTODOC PUBLIC tpg/shell/showcd
|||	showcd - show the name of the current directory
|||
|||	  Format
|||
|||	    showcd
|||
|||	  Description
|||
|||	    This command displays the name of the current directory
|||	    to the debugging terminal.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    pcd
|||
|||	  See Also
|||
|||	    setcd
**/

/**
|||	AUTODOC PUBLIC tpg/shell/killkprintf
|||	killkprintf - disable kernel kprintf screen output
|||
|||	  Format
|||
|||	    killkprintf
|||
|||	  Description
|||
|||	    This command disables kernel kprintf screen output.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
**/

/* keep the compiler happy... */
extern int foo;
