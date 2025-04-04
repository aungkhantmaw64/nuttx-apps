/****************************************************************************
 * apps/nshlib/nsh_builtin.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#ifdef CONFIG_SCHED_WAITPID
#  include <sys/ioctl.h>
#  include <sys/wait.h>
#endif

#include <stdbool.h>
#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <string.h>

#include <nuttx/lib/builtin.h>
#include "builtin/builtin.h"

#include "nsh.h"
#include "nsh_console.h"

#ifdef CONFIG_NSH_BUILTIN_APPS

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_builtin
 *
 * Description:
 *    Attempt to execute the application task whose name is 'cmd'
 *
 * Returned Value:
 *   <0          If exec_builtin() fails, then the negated errno value
 *               is returned.
 *   -1 (ERROR)  if the application task corresponding to 'cmd' could not
 *               be started (possibly because it doesn not exist).
 *    0 (OK)     if the application task corresponding to 'cmd' was
 *               and successfully started.  If CONFIG_SCHED_WAITPID is
 *               defined, this return value also indicates that the
 *               application returned successful status (EXIT_SUCCESS)
 *    1          If CONFIG_SCHED_WAITPID is defined, then this return value
 *               indicates that the application task was spawned successfully
 *               but returned failure exit status.
 *
 ****************************************************************************/

int nsh_builtin(FAR struct nsh_vtbl_s *vtbl, FAR const char *cmd,
                FAR char **argv,
                FAR const struct nsh_param_s *param)
{
#if !defined(CONFIG_NSH_DISABLEBG) && defined(CONFIG_SCHED_CHILD_STATUS)
  struct sigaction act;
  struct sigaction old;
#endif
  int ret = OK;

  /* Lock the scheduler in an attempt to prevent the application from
   * running until waitpid() has been called.
   */

  sched_lock();

#if !defined(CONFIG_NSH_DISABLEBG) && defined(CONFIG_SCHED_CHILD_STATUS)
  /* Ignore the child status if run the application on background. */

  if (vtbl->np.np_bg == true)
    {
      act.sa_handler = SIG_DFL;
      act.sa_flags = SA_NOCLDWAIT;
      sigemptyset(&act.sa_mask);

      sigaction(SIGCHLD, &act, &old);
    }

#endif /* CONFIG_NSH_DISABLEBG */

  /* Try to find and execute the command within the list of builtin
   * applications.
   */

  ret = exec_builtin(cmd, argv, param);
  if (ret >= 0)
    {
      /* The application was successfully started with pre-emption disabled.
       * In the simplest cases, the application will not have run because the
       * the scheduler is locked.  But in the case where I/O was redirected,
       * a proxy task ran and broke our lock.  As result, the application may
       * have aso ran if its priority was higher than than the priority of
       * this thread.
       *
       * If the application did not run to completion and if the application
       * was not backgrounded, then we need to wait here for the application
       * to exit.  This only works works with the following options:
       *
       * - CONFIG_NSH_DISABLEBG - Do not run commands in background
       * - CONFIG_SCHED_WAITPID - Required to run external commands in
       *     foreground
       */

       vtbl->np.np_lastpid = ret;
#ifdef CONFIG_SCHED_WAITPID

      /* CONFIG_SCHED_WAITPID is selected, so we may run the command in
       * foreground unless we were specifically requested to run the command
       * in background (and running commands in background is enabled).
       */

#  ifndef CONFIG_NSH_DISABLEBG
      if (vtbl->np.np_bg == false)
#  endif /* CONFIG_NSH_DISABLEBG */
        {
          int rc = 0;
          int tc = 0;

          if (vtbl->isctty)
            {
              /* Setup up to receive SIGINT if control-C entered. */

              tc = nsh_ioctl(vtbl, TIOCSCTTY, ret);
            }

          /* Wait for the application to exit.  We did lock the scheduler
           * above, but that does not guarantee that the application did not
           * already run to completion in the case where I/O was redirected.
           * Here the scheduler will be unlocked while waitpid is waiting
           * and if the application has not yet run, it will now be able to
           * do so.
           *
           * Also, if CONFIG_SCHED_HAVE_PARENT is defined waitpid() might
           * fail even if task is still active:  If the I/O was re-directed
           * by a proxy task, then the ask is a child of the proxy, and not
           * this task. waitpid() fails with ECHILD in either case.
           *
           * NOTE: WUNTRACED does nothing in the default case, but in the
           * case the where CONFIG_SIG_SIGSTOP_ACTION=y, the built-in app
           * may also be stopped.  In that case WUNTRACED will force
           * waitpid() to return with ECHILD.
           */

          ret = waitpid(ret, &rc, WUNTRACED);
          if (ret < 0)
            {
              /* If the child thread does not exist, waitpid() will return
               * the error ECHLD.  Since we know that the task was
               * successfully started, this must be one of the cases
               * described above; we have to assume that the task already
               * exit'ed. In this case, we have no idea if the application
               * ran successfully or not (because NuttX does not retain exit
               * status of child tasks). Let's assume that is did run
               * successfully.
               */

              int errcode = errno;
              if (errcode == ECHILD)
                {
                  ret = OK;
                }
              else
                {
                  nsh_error(vtbl, g_fmtcmdfailed, cmd, "waitpid",
                            NSH_ERRNO_OF(errcode));
                }
            }

          /* Waitpid completed the wait successfully */

          else
            {
              /* We can't return the exact status (nsh has nowhere to put it)
               * so just pass back zero/nonzero in a fashion that doesn't
               * look like an error.
               */

              ret = (rc == 0) ? OK : 1;

              /* TODO:  Set the environment variable '?' to a string
               * corresponding to WEXITSTATUS(rc) so that $? will expand to
               * the exit status of the most recently executed task.
               */
            }

          if (vtbl->isctty && tc == 0)
            {
              nsh_ioctl(vtbl, TIOCNOTTY, 0);
            }
        }
#  ifndef CONFIG_NSH_DISABLEBG
      else
#  endif /* CONFIG_NSH_DISABLEBG */
#endif /* CONFIG_SCHED_WAITPID */

      /* We get here if either:
       *
       * - CONFIG_SCHED_WAITPID is not selected meaning that all commands
       *   have to be run in background, or
       * - CONFIG_SCHED_WAITPID and CONFIG_NSH_DISABLEBG are both selected,
       *   but the user requested to run the command in background.
       *
       * NOTE that the case of a) CONFIG_SCHED_WAITPID is not selected and
       * b) CONFIG_NSH_DISABLEBG selected cannot be supported.  In that
       * event, all commands will have to run in background.  The waitpid()
       * API must be available to support running the command in foreground.
       */

#if !defined(CONFIG_SCHED_WAITPID) || !defined(CONFIG_NSH_DISABLEBG)
        {
#if !defined(CONFIG_NSH_DISABLEBG) && defined(CONFIG_SCHED_CHILD_STATUS)

          /* Restore the old actions */

#  ifndef CONFIG_SCHED_WAITPID
          if (vtbl->np.np_bg == true)
#  endif
            {
              sigaction(SIGCHLD, &old, NULL);
            }

#  endif
          struct sched_param sched;
          sched_getparam(ret, &sched);
          nsh_output(vtbl, "%s [%d:%d]\n", cmd, ret, sched.sched_priority);

          /* Backgrounded commands always 'succeed' as long as we can start
           * them.
           */

          ret = OK;
        }
#endif /* !CONFIG_SCHED_WAITPID || !CONFIG_NSH_DISABLEBG */
    }

  sched_unlock();

  /* If exec_builtin() or waitpid() failed, then return -1 (ERROR) with the
   * errno value set appropriately.
   */

  if (ret < 0)
    {
      return ERROR;
    }

  return ret;
}

#endif /* CONFIG_NSH_BUILTIN_APPS */
