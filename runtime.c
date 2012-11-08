/***************************************************************************
 *  Title: Runtime environment 
 * -------------------------------------------------------------------------
 *    Purpose: Runs commands
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.1 $
 *    Last Modification: $Date: 2005/10/13 05:24:59 $
 *    File: $RCSfile: runtime.c,v $
 *    Copyright: (C) 2002 by Stefan Birrer
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: runtime.c,v $
 *    Revision 1.1  2005/10/13 05:24:59  sbirrer
 *    - added the skeleton files
 *
 *    Revision 1.6  2002/10/24 21:32:47  sempi
 *    final release
 *
 *    Revision 1.5  2002/10/23 21:54:27  sempi
 *    beta release
 *
 *    Revision 1.4  2002/10/21 04:49:35  sempi
 *    minor correction
 *
 *    Revision 1.3  2002/10/21 04:47:05  sempi
 *    Milestone 2 beta
 *
 *    Revision 1.2  2002/10/15 20:37:26  sempi
 *    Comments updated
 *
 *    Revision 1.1  2002/10/15 20:20:56  sempi
 *    Milestone 1
 *
 ***************************************************************************/
	#define __RUNTIME_IMPL__

  /************System include***********************************************/
	#include <assert.h>
	#include <errno.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <sys/wait.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <signal.h>

  /************Private include**********************************************/
	#include "runtime.h"
	#include "io.h"

  /************Defines and Typedefs*****************************************/
    /*  #defines and typedefs should have their names in all caps.
     *  Global variables begin with g. Global constants with k. Local
     *  variables should be in all lower case. When initializing
     *  structures and arrays, line everything up in neat columns.
     */

  /************Global Variables*********************************************/

	#define NBUILTINCOMMANDS (sizeof BuiltInCommands / sizeof(char*))

	typedef struct bgjob_l {
		pid_t pid;
		struct bgjob_l* next;
	} bgjobL;

	/* the pids of the background processes */
	bgjobL *bgjobs = NULL;

  /************Function Prototypes******************************************/
	/* run command */
	static void RunCmdFork(commandT*, bool);
	/* runs an external program command after some checks */
	static void RunExternalCmd(commandT*, bool);
	/* resolves the path and checks for exutable flag */
	static bool ResolveExternalCmd(commandT*);
	/* forks and runs a external program */
	static void Exec(commandT*, bool);
	/* runs a builtin command */
	static void RunBuiltInCmd(commandT*);
	/* checks whether a command is a builtin command */
	static bool IsBuiltIn(char*);
  /************External Declaration*****************************************/

/**************Implementation***********************************************/

	void RunCmd(commandT* cmd)
	{
		RunCmdFork(cmd, TRUE);
	}

	void RunCmdFork(commandT* cmd, bool fork)
	{
		if (cmd->argc<=0)
			return;
		if (IsBuiltIn(cmd->argv[0]))
		{
			RunBuiltInCmd(cmd);
		}
		else
		{
			RunExternalCmd(cmd, fork);
		}
	}

	void RunCmdBg(commandT* cmd)
	{
		// TODO
	}

	void RunCmdPipe(commandT* cmd1, commandT* cmd2)
	{
	}

	void RunCmdRedirOut(commandT* cmd, char* file)
	{
	}

	void RunCmdRedirIn(commandT* cmd, char* file)
	{
	}

        static void RunExternalCmd(commandT* cmd, bool fork)
        {
		 if (ResolveExternalCmd(cmd))
                        Exec(cmd, fork);
        }

        static bool ResolveExternalCmd(commandT* cmd)
        {
          	return FALSE;
	}

	static void Exec(commandT* cmd, bool forceFork)
	{
	}

        static bool IsBuiltIn(char* cmd)
        {
        	return FALSE;     
        }

   
	static void RunBuiltInCmd(commandT* cmd)
	{
	}

        void CheckJobs()
	{
 	}

