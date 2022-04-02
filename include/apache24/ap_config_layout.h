/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file  ap_config_layout.h
 * @brief Apache Config Layout
 */

#ifndef AP_CONFIG_LAYOUT_H
#define AP_CONFIG_LAYOUT_H

/* Configured Apache directory layout */
#define DEFAULT_PREFIX "/usr/local"
#define DEFAULT_EXP_EXEC_PREFIX "/usr/local"
#define DEFAULT_REL_EXEC_PREFIX ""
#define DEFAULT_EXP_BINDIR "/usr/local/bin"
#define DEFAULT_REL_BINDIR "bin"
#define DEFAULT_EXP_SBINDIR "/usr/local/sbin"
#define DEFAULT_REL_SBINDIR "sbin"
#define DEFAULT_EXP_LIBEXECDIR "/usr/local/libexec/apache24"
#define DEFAULT_REL_LIBEXECDIR "libexec/apache24"
#define DEFAULT_EXP_MANDIR "/usr/local/man"
#define DEFAULT_REL_MANDIR "man"
#define DEFAULT_EXP_SYSCONFDIR "/usr/local/etc/apache24"
#define DEFAULT_REL_SYSCONFDIR "etc/apache24"
#define DEFAULT_EXP_DATADIR "/usr/local/www/apache24"
#define DEFAULT_REL_DATADIR "www/apache24"
#define DEFAULT_EXP_INSTALLBUILDDIR "/usr/local/share/apache24/build"
#define DEFAULT_REL_INSTALLBUILDDIR "share/apache24/build"
#define DEFAULT_EXP_ERRORDIR "/usr/local/www/apache24/error"
#define DEFAULT_REL_ERRORDIR "www/apache24/error"
#define DEFAULT_EXP_ICONSDIR "/usr/local/www/apache24/icons"
#define DEFAULT_REL_ICONSDIR "www/apache24/icons"
#define DEFAULT_EXP_HTDOCSDIR "/usr/local/www/apache24/data"
#define DEFAULT_REL_HTDOCSDIR "www/apache24/data"
#define DEFAULT_EXP_MANUALDIR "/usr/local/share/doc/apache24"
#define DEFAULT_REL_MANUALDIR "share/doc/apache24"
#define DEFAULT_EXP_CGIDIR "/usr/local/www/apache24/cgi-bin"
#define DEFAULT_REL_CGIDIR "www/apache24/cgi-bin"
#define DEFAULT_EXP_INCLUDEDIR "/usr/local/include/apache24"
#define DEFAULT_REL_INCLUDEDIR "include/apache24"
#define DEFAULT_EXP_LOCALSTATEDIR "/var"
#define DEFAULT_REL_LOCALSTATEDIR "/var"
#define DEFAULT_EXP_RUNTIMEDIR "/var/run"
#define DEFAULT_REL_RUNTIMEDIR "/var/run"
#define DEFAULT_EXP_LOGFILEDIR "/var/log"
#define DEFAULT_REL_LOGFILEDIR "/var/log"
#define DEFAULT_EXP_PROXYCACHEDIR "/usr/local/www/apache24/proxy"
#define DEFAULT_REL_PROXYCACHEDIR "www/apache24/proxy"

#endif /* AP_CONFIG_LAYOUT_H */
