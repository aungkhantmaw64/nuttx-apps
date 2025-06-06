/****************************************************************************
 * apps/netutils/libcurl4nx/curl4nx_easy_cleanup.c
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
#include <nuttx/compiler.h>
#include <debug.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <nuttx/version.h>

#include "netutils/netlib.h"
#include "netutils/curl4nx.h"
#include "curl4nx_private.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: curl4nx_easy_cleanup()
 ****************************************************************************/

void curl4nx_easy_cleanup(FAR struct curl4nx_s *handle)
{
  curl4nx_info("handle=%p\n", handle);

  /* Reset custom options in the context, effectively freeing the headers */

  curl4nx_easy_reset(handle);

  /* And release allocated memory */

  if (handle->rxbuf != NULL)
    {
      free(handle->rxbuf);
      handle->rxbufsize = 0;
    }

  free(handle);
}
