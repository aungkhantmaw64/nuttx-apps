/****************************************************************************
 * apps/wireless/ieee802154/libmac/sixlowpan_assocresp.c
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

#include <sys/ioctl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <nuttx/fs/ioctl.h>

#include <nuttx/wireless/ieee802154/ieee802154_mac.h>
#include "wireless/ieee802154.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int sixlowpan_assoc_resp(int sock, FAR const char *ifname,
                         FAR struct ieee802154_assoc_resp_s *resp)
{
  struct ieee802154_netmac_s arg;
  int ret;

  strlcpy(arg.ifr_name, ifname, IFNAMSIZ);
  memcpy(&arg.u.assocresp, resp, sizeof(struct ieee802154_assoc_resp_s));

  ret = ioctl(sock, MAC802154IOC_MLME_ASSOC_RESPONSE,
              (unsigned long)((uintptr_t)&arg));
  if (ret < 0)
    {
      ret = -errno;
      fprintf(stderr, "MAC802154IOC_MLME_ASSOC_RESPONSE failed: %d\n", ret);
      return ret;
    }

  memcpy(resp, &arg.u.assocresp, sizeof(struct ieee802154_assoc_resp_s));
  return OK;
}
