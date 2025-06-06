############################################################################
# apps/graphics/libyuv/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

include $(APPDIR)/Make.defs

SRCDIR = libyuv

CXXEXT := .cc
CXXFLAGS += -Wno-shadow -Wno-undef
CXXSRCS = $(wildcard $(SRCDIR)/source/*.cc)

libyuv.tar.gz:
	$(Q) curl -L https://chromium.googlesource.com/libyuv/libyuv/+archive/refs/heads/$(CONFIG_LIBYUV_BRANCH).tar.gz -o libyuv.tar.gz
	$(Q) mkdir -p $(SRCDIR)
	$(Q) tar zxf libyuv.tar.gz --directory $(SRCDIR)
ifeq ($(CONFIG_LIBYUV_BRANCH),"stable")
	$(Q) patch -p0 < 0001-include-libyuv-fix-strict-prototype-warning.patch
endif

# Download and unpack tarball if no git repo found
ifeq ($(wildcard libyuv/.git),)
context:: libyuv.tar.gz

distclean::
	$(call DELDIR, libyuv)
	$(call DELFILE, libyuv.tar.gz)
endif

include $(APPDIR)/Application.mk
