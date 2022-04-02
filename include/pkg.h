/*-
 * Copyright (c) 2011-2016 Baptiste Daroussin <bapt@FreeBSD.org>
 * Copyright (c) 2011-2012 Julien Laffaye <jlaffaye@FreeBSD.org>
 * Copyright (c) 2011 Will Andrews <will@FreeBSD.org>
 * Copyright (c) 2011 Philippe Pepiot <phil@philpep.org>
 * Copyright (c) 2011-2012 Marin Atanasov Nikolov <dnaeon@gmail.com>
 * Copyright (c) 2013-2014 Matthew Seaman <matthew@FreeBSD.org>
 * Copyright (c) 2014-2016 Vsevolod Stakhov <vsevolod@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _PKG_H
#define _PKG_H

#ifdef __cplusplus
extern "C" {
#define restrict
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/cdefs.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

/* The expected name of the pkg(8) binary executable. */
#ifndef PKG_EXEC_NAME
#define PKG_EXEC_NAME	"pkg"
#endif

/* The expected name of the pkg-static(8) binary */
#ifndef PKG_STATIC_NAME
#define PKG_STATIC_NAME	"pkg-static"
#endif

#define PKGVERSION "1.12.0"

/* PORTVERSION equivalent for proper pkg-static->ports-mgmt/pkg
 * version comparison in pkgdb_query_newpkgversion() */

#define PKG_PORTVERSION "1.12.0"

/* The OS major version at the time of compilation */
#ifdef __FreeBSD__
#define OSMAJOR	__FreeBSD__
#endif

/* Not supported under DragonFly */
#ifdef __DragonFly__
#undef OSMAJOR
#endif

#ifdef __NetBSD_Version__
#define OSMAJOR ((__NetBSD_Version__ + 1000000) / 100000000)
#endif

#ifndef __DECONST
#define __DECONST(type, var)    ((type)(uintptr_t)(const void *)(var))
#endif

#ifndef NELEM
#define NELEM(array)    (sizeof(array) / sizeof((array)[0]))
#endif

/* Special exit status for worker processes indicating that a restart
 * is desired -- eg. after a child has updated pkg(8) itself.  Don't
 * clash with any of the sysexits values */

#define EX_NEEDRESTART	(EX__MAX + 1)

struct pkg;
struct pkg_dep;
struct pkg_conflict;
struct pkg_file;
struct pkg_dir;
struct pkg_option;
struct pkg_license;
struct pkg_config_file;

struct pkgdb;
struct pkgdb_it;

struct pkg_jobs;
struct pkg_solve_problem;

struct pkg_repo;

struct pkg_plugin;

struct pkg_manifest_key;
struct pkg_manifest_parser;

typedef struct ucl_object_s pkg_object;
typedef void * pkg_iter;

struct pkg_kv {
	char *key;
	char *value;
	struct pkg_kv *next, *prev;
};

/**
 * The system-wide pkg(8) status: ie. is it a) installed or otherwise
 * available on the sysem, b) database (local.sqlite) initialised and
 * c) has at least one package installed (which should be pkg
 * itself). PKG_STATUS_UNINSTALLED logically cannot be returned by
 * pkg(8) itself, but it can be useful for the pkg bootstrapper
 * /usr/bin/pkg or for applications that link against libpkg.so
 */

typedef enum {
	PKG_STATUS_ACTIVE = 0,	/* pkg in use */
	PKG_STATUS_NOPACKAGES,	/* local.sqlite empty */
	PKG_STATUS_NODB,	/* local.sqlite not found, unreadable or not initialised */
	PKG_STATUS_UNINSTALLED,	/* pkg not argv[0] or not on $PATH */
} pkg_status_t;

typedef enum {
	/**
	 * The license logic is OR (dual in the ports)
	 */
	LICENSE_OR = '|',
	/**
	 * The license logic is AND (multi in the ports)
	 */
	LICENSE_AND = '&',
	/**
	 * The license logic un single (default in the ports)
	 */
	LICENSE_SINGLE = 1U
} lic_t;

typedef enum {
	PKGDB_DEFAULT = 0,
	PKGDB_REMOTE,
	PKGDB_MAYBE_REMOTE
} pkgdb_t;

typedef enum {
	PKG_INIT_FLAG_USE_IPV4 = (1U << 0),
	PKG_INIT_FLAG_USE_IPV6 = (1U << 1)
} pkg_init_flags;

/**
 * Specify how an argument should be used by query functions.
 */
typedef enum {
	/**
	 * The argument does not matter, all items will be matched.
	 */
	MATCH_ALL,
	/**
	 * The argument is the exact pattern.  Match will be case
	 * sensitive or case insensitive according to
	 * pkgdb_case_sensitive()
	 */
	MATCH_EXACT,
	/**
	 * The argument is an exact pattern except that matches will
	 * be made case insensitively.  Match is always case sensitive
	 */
	MATCH_GLOB,
	/**
	 * The argument is a regular expression ('modern' style
	 * according to re_format(7).  Match will be case sensitive or
	 * case insensitive according to pkgdb_case_sensitive()
	 */
	MATCH_REGEX,
	/**
	 * The argument is a WHERE clause to use as condition
	 */
	MATCH_CONDITION,
} match_t;

/**
 * Specify on which field the pattern will be matched uppon.
 */

typedef enum {
	FIELD_NONE,
	FIELD_ORIGIN,
	FIELD_NAME,
	FIELD_NAMEVER,
	FIELD_COMMENT,
	FIELD_DESC
} pkgdb_field;

/**
 * The type of package.
 */
typedef enum {
	/**
	 * The pkg type can not be determined.
	 */
	PKG_NONE = 0,

	/**
	 * The pkg refers to a local file archive.
	 */
	PKG_FILE = (1U << 0),
	/**
	 * The pkg refers to data read from a non-regular file
	 * (device, pipeline, unix dmain socket etc.)
	 */
	PKG_STREAM = (1U << 1),
	/**
	 * The pkg refers to a package available on the remote repository.
	 * @todo Document which attributes are available.
	 */
	PKG_REMOTE = (1U << 2),
	/**
	 * The pkg refers to a localy installed package.
	 */
	PKG_INSTALLED = (1U << 3),
	/**
	 * The pkg refers to a local file old archive.
	 */
	PKG_OLD_FILE = (1U << 4),
} pkg_t;

/**
 * Contains keys to refer to a string attribute.
 * Used by pkg_get() and pkg_set()
 */
typedef enum {
	PKG_ORIGIN = 1U,
	PKG_NAME,
	PKG_VERSION,
	PKG_COMMENT,
	PKG_DESC,
	PKG_MTREE,
	PKG_MESSAGE,
	PKG_ARCH,
	PKG_ABI,
	PKG_MAINTAINER,
	PKG_WWW,
	PKG_PREFIX,
	PKG_REPOPATH,
	PKG_CKSUM,
	PKG_OLD_VERSION,
	PKG_REPONAME,
	PKG_REPOURL,
	PKG_DIGEST,
	PKG_REASON,
	PKG_FLATSIZE,
	PKG_OLD_FLATSIZE,
	PKG_PKGSIZE,
	PKG_LICENSE_LOGIC,
	PKG_AUTOMATIC,
	PKG_LOCKED,
	PKG_ROWID,
	PKG_TIME,
	PKG_ANNOTATIONS,
	PKG_UNIQUEID,
	PKG_OLD_DIGEST,
	PKG_DEP_FORMULA,
	PKG_VITAL,
	PKG_NUM_FIELDS,		/* end of fields */
} pkg_attr;

typedef enum {
	PKG_SET_FLATSIZE = 1U,
	PKG_SET_AUTOMATIC,
	PKG_SET_LOCKED,
	PKG_SET_DEPORIGIN,
	PKG_SET_ORIGIN,
	PKG_SET_DEPNAME,
	PKG_SET_NAME,
	PKG_SET_VITAL,
	PKG_SET_MAX
} pkg_set_attr;

/**
 * contains keys to refer to a string attribute
 * Used by pkg_dep_get()
 */
typedef enum {
	PKG_DEP_NAME = 0,
	PKG_DEP_ORIGIN,
	PKG_DEP_VERSION
} pkg_dep_attr;

typedef enum {
	PKG_DEPS = 0,
	PKG_RDEPS,
	PKG_OPTIONS,
	PKG_FILES,
	PKG_DIRS,
	PKG_USERS,
	PKG_GROUPS,
	PKG_SHLIBS_REQUIRED,
	PKG_SHLIBS_PROVIDED,
	PKG_CONFLICTS,
	PKG_PROVIDES,
	PKG_CONFIG_FILES,
	PKG_REQUIRES,
	PKG_CATEGORIES,
	PKG_LICENSES
} pkg_list;

typedef enum {
	SRV,
	HTTP,
	NOMIRROR,
} mirror_t;

typedef enum {
	SIG_NONE = 0,
	SIG_PUBKEY,
	SIG_FINGERPRINT
} signature_t;

/**
 * Determine the type of a pkg_script.
 */
typedef enum {
	PKG_SCRIPT_PRE_INSTALL = 0,
	PKG_SCRIPT_POST_INSTALL,
	PKG_SCRIPT_PRE_DEINSTALL,
	PKG_SCRIPT_POST_DEINSTALL,
	PKG_SCRIPT_PRE_UPGRADE,
	PKG_SCRIPT_POST_UPGRADE,
	PKG_SCRIPT_INSTALL,
	PKG_SCRIPT_DEINSTALL,
	PKG_SCRIPT_UPGRADE,
	PKG_SCRIPT_UNKNOWN
} pkg_script;

/**
 * Determine the type of a pkg_lua_script.
 */
typedef enum {
	PKG_LUA_PRE_INSTALL = 0,
	PKG_LUA_POST_INSTALL,
	PKG_LUA_PRE_DEINSTALL,
	PKG_LUA_POST_DEINSTALL,
	PKG_LUA_UNKNOWN
} pkg_lua_script;

typedef enum _pkg_jobs_t {
	PKG_JOBS_INSTALL,
	PKG_JOBS_DEINSTALL,
	PKG_JOBS_FETCH,
	PKG_JOBS_AUTOREMOVE,
	PKG_JOBS_UPGRADE,
} pkg_jobs_t;

typedef enum _pkg_flags {
	PKG_FLAG_NONE = 0,
	PKG_FLAG_DRY_RUN = (1U << 0),
	PKG_FLAG_FORCE = (1U << 1),
	PKG_FLAG_RECURSIVE = (1U << 2),
	PKG_FLAG_AUTOMATIC = (1U << 3),
	PKG_FLAG_WITH_DEPS = (1U << 4),
	PKG_FLAG_NOSCRIPT = (1U << 5),
	PKG_FLAG_PKG_VERSION_TEST = (1U << 6),
	PKG_FLAG_UPGRADES_FOR_INSTALLED = (1U << 7),
	PKG_FLAG_SKIP_INSTALL = (1U << 8),
	PKG_FLAG_FORCE_MISSING = (1U << 9),
	PKG_FLAG_FETCH_MIRROR = (1U << 10),
	PKG_FLAG_USE_IPV4 = (1U << 11),
	PKG_FLAG_USE_IPV6 = (1U << 12),
	PKG_FLAG_UPGRADE_VULNERABLE = (1U << 13)
} pkg_flags;

typedef enum _pkg_stats_t {
	PKG_STATS_LOCAL_COUNT = 0,
	PKG_STATS_LOCAL_SIZE,
	PKG_STATS_REMOTE_COUNT,
	PKG_STATS_REMOTE_UNIQUE,
	PKG_STATS_REMOTE_SIZE,
	PKG_STATS_REMOTE_REPOS,
} pkg_stats_t;

typedef enum {
	PKG_STRING = 0,
	PKG_BOOL,
	PKG_INT,
	PKG_ARRAY,
	PKG_OBJECT,
	PKG_NULL
} pkg_object_t;

/**
 * Keys for accessing pkg plugin data
 */
typedef enum _pkg_plugin_key {
	PKG_PLUGIN_NAME = 0,
	PKG_PLUGIN_DESC,
	PKG_PLUGIN_VERSION,
	PKG_PLUGIN_PLUGINFILE
} pkg_plugin_key;

/**
 * Keys for hooking into the library
 */
typedef enum _pkg_plugin_hook_t {
	PKG_PLUGIN_HOOK_PRE_INSTALL = 1,
	PKG_PLUGIN_HOOK_POST_INSTALL,
	PKG_PLUGIN_HOOK_PRE_DEINSTALL,
	PKG_PLUGIN_HOOK_POST_DEINSTALL,
	PKG_PLUGIN_HOOK_PRE_FETCH,
	PKG_PLUGIN_HOOK_POST_FETCH,
	PKG_PLUGIN_HOOK_EVENT,
	PKG_PLUGIN_HOOK_PRE_UPGRADE,
	PKG_PLUGIN_HOOK_POST_UPGRADE,
	PKG_PLUGIN_HOOK_PRE_AUTOREMOVE,
	PKG_PLUGIN_HOOK_POST_AUTOREMOVE,
	PKG_PLUGIN_HOOK_PKGDB_CLOSE_RW,
} pkg_plugin_hook_t;

/**
 * Error type used everywhere by libpkg.
 */
typedef enum {
	EPKG_OK = 0,
	/**
	 * No more items available (end of the loop).
	 */
	EPKG_END,
	EPKG_WARN,
	/**
	 * The function encountered a fatal error.
	 */
	EPKG_FATAL,
	/**
	 * Can not delete the package because it is required by
	 * another package.
	 */
	EPKG_REQUIRED,
	/**
	 * Can not install the package because it is already installed.
	 */
	EPKG_INSTALLED,
	/**
	 * Can not install the package because some dependencies are
	 * unresolved.
	 */
	EPKG_DEPENDENCY,
	/**
	 * Can not operate on package because it is locked
	 */
	EPKG_LOCKED,
	/**
	 * Can not create local database or database non-existent
	 */
	EPKG_ENODB,
	/**
	 * local file newer than remote
	 */
	EPKG_UPTODATE,
	/**
	 * unkown keyword
	 */
	EPKG_UNKNOWN,
	/**
	 * repo DB schema incompatible version
	 */
	EPKG_REPOSCHEMA,
	/**
	 * Insufficient privilege for action
	 */
	EPKG_ENOACCESS,
	/**
	 * Insecure permissions on any component of
	 * $PKGDB_DIR/local.sqlite or any of the repo database bits
	 */
	EPKG_INSECURE,
	/**
	 * A conflict between packages found
	 */
	EPKG_CONFLICT,
	/**
	 * Need to repeat operation
	 */
	EPKG_AGAIN,
	/**
	 * Not installed
	 */
	EPKG_NOTINSTALLED,
	/**
	 * Can not delete the package because it is vital, i.e. a kernel
	 */
	EPKG_VITAL
} pkg_error_t;

/**
 * Upgrade, downgrade or reinstall?
 */

typedef enum {
	PKG_DOWNGRADE = 0,
	PKG_REINSTALL,
	PKG_UPGRADE,
} pkg_change_t;

/**
 * Locking types for database:
 * `PKGDB_LOCK_READONLY`: lock for read only queries (can be nested)
 * `PKGDB_LOCK_ADVISORY`: write to DB inside a transaction (allows `PKGDB_LOCK_READONLY`)
 * `PKGDB_LOCK_EXCLUSIVE`: possibly destructive operations (does not allow other locks)
 */
typedef enum {
	PKGDB_LOCK_READONLY,
	PKGDB_LOCK_ADVISORY,
	PKGDB_LOCK_EXCLUSIVE
} pkgdb_lock_t;

typedef enum {
	PKG_SOLVED_INSTALL,
	PKG_SOLVED_DELETE,
	PKG_SOLVED_UPGRADE,
	PKG_SOLVED_UPGRADE_REMOVE,
	PKG_SOLVED_FETCH,
	PKG_SOLVED_UPGRADE_INSTALL
} pkg_solved_t;

#define PKG_OPEN_MANIFEST_ONLY 0x1
#define PKG_OPEN_MANIFEST_COMPACT (0x1 << 1)
#define PKG_OPEN_TRY (0x1 << 2)

/**
 * test if pkg is installed and activated.
 * @param count	If all the tests pass, and count is non-NULL,
 * write the number of installed packages into *count
 */
pkg_status_t pkg_status(int *count);

/**
 * Allocate a new pkg.
 * Allocated pkg must be deallocated by pkg_free().
 */
int pkg_new(struct pkg **, pkg_t type);

/**
 * Deallocate a pkg
 */
void pkg_free(struct pkg *);

/**
 * Check if a package is valid according to its type.
 */
int pkg_is_valid(const struct pkg * restrict);

/**
 * Open a package file archive and retrive informations.
 * @param p A pointer to pkg allocated by pkg_new(), or if it points to a
 * NULL pointer, the function allocate a new pkg using pkg_new().
 * @param path The path to the local package archive.
 * @param keys manifest keys that should be initialised
 * @param flags open flags
 */
int pkg_open(struct pkg **p, const char *path, struct pkg_manifest_key *keys, int flags);
int pkg_open_fd(struct pkg **p, int fd, struct pkg_manifest_key *keys, int flags);

/**
 * @return the type of the package.
 * @warning returns PKG_NONE on error.
 */
pkg_t pkg_type(const struct pkg * restrict);

/**
 * Generic getter for simple attributes.
 * @return NULL-terminated string.
 * @warning May return a NULL pointer.
 */
int pkg_get2(struct pkg const *const, ...);
#define pkg_get(pkg, ...) pkg_get2(pkg, __VA_ARGS__, -1)

int pkg_list_count(const struct pkg *, pkg_list);

/**
 * Iterates over the dependencies of the package.
 * @param dep Must be set to NULL for the first call.
 * @return An error code.
 */
int pkg_deps(const struct pkg *, struct pkg_dep **dep);

/**
 * Iterates over the reverse dependencies of the package.
 * That is, the packages which require this package.
 * @param dep Must be set to NULL for the first call.
 * @return An error code.
 */
int pkg_rdeps(const struct pkg *, struct pkg_dep **dep);

/**
 * Iterates over the files of the package.
 * @param file Must be set to NULL for the first call.
 * @return An error code.
 */
int pkg_files(const struct pkg *, struct pkg_file **file);

/**
 * Iterates over the directories of the package.
 * @param Must be set to NULL for the first call.
 * @return An error code.
 */
int pkg_dirs(const struct pkg *pkg, struct pkg_dir **dir);

/**
 * Iterates over the users of the package.
 * @param Must be set to NULL for the first call.
 * @return An error code.
 */
int pkg_users(const struct pkg *pkg, char **user);

/**
 * Iterates over the groups of the package.
 * @param Must be set to NULL for the first call.
 * @return An error code.
 */
int pkg_groups(const struct pkg *pkg, char **group);

/**
 * Iterates over the options of the package.
 * @param  option Must be set to NULL for the first call.
 * @return An error code.
 */
int pkg_options(const struct pkg *, struct pkg_option **option);

/**
 * Iterates over the shared libraries used by the package.
 * @param shlib must be set to NULL for the first call.
 * @return An error code
 */
int pkg_shlibs_required(const struct pkg *pkg, char **shlib);

/**
 * Iterates over the shared libraries provided by the package.
 * @param shlib must be set to NULL for the first call.
 * @return An error code
 */
int pkg_shlibs_provided(const struct pkg *pkg, char **shlib);

/**
 * Iterates over the conflicts registered in the package.
 * @param conflict must be set to NULL for the first call.
 * @return An error code
 */
int pkg_conflicts(const struct pkg *pkg, struct pkg_conflict **conflict);

/**
 * Iterates over the provides registered in the package.
 * @param provide must be set to NULL for the first call.
 * @return An error code
 */
int pkg_provides(const struct pkg *pkg, char **provide);
int pkg_requires(const struct pkg *pkg, char **require);

int pkg_categories(const struct pkg *pkg, char **category);
int pkg_licenses(const struct pkg *pkg, char **licenses);

/**
 * Iterates over the config files registered in the package.
 * @param provide must be set to NULL for the first call.
 * @return An error code
 */
int pkg_config_files(const struct pkg *pkg, struct pkg_config_file **cf);

/**
 * Iterate over all of the files within the package pkg, ensuring the
 * dependency list contains all applicable packages providing the
 * shared objects used by pkg.
 * Also add all the shared object into the shlibs.
 * It respects the SHLIBS options from configuration
 * @return An error code
 */

 /* Don't conflict with PKG_LOAD_* q.v. */
#define PKG_CONTAINS_ELF_OBJECTS	(1U << 24)
#define PKG_CONTAINS_STATIC_LIBS	(1U << 25)
#define PKG_CONTAINS_H_OR_LA		(1U << 26)

int pkg_analyse_files(struct pkgdb *, struct pkg *, const char *);

/**
 * Suggest if a package could be marked architecture independent or
 * not.
 */
int pkg_suggest_arch(struct pkg *, const char *, bool);

/**
 * Generic setter for simple attributes.
 */
int pkg_set2(struct pkg *pkg, ...);
#define pkg_set(pkg, ...) pkg_set2(pkg, __VA_ARGS__, -1)

int pkgdb_set2(struct pkgdb *db, struct pkg *pkg, ...);
#define pkgdb_set(db, pkg, ...) pkgdb_set2(db, pkg, __VA_ARGS__, -1)

/**
 * Set a new debug level used inside of pkg.
 * @param debug_level Debug level between 0 (no debugging) and 4 (max debugging).
 * @return Previous debug level.
 */
int64_t pkg_set_debug_level(int64_t debug_level);
int pkg_set_rootdir(const char *rootdir);

/**
 * Allocate a new struct pkg and add it to the deps of pkg.
 * @return An error code.
 */
int pkg_adddep(struct pkg *pkg, const char *name, const char *origin, const
			   char *version, bool locked);
int pkg_addrdep(struct pkg *pkg, const char *name, const char *origin, const
			   char *version, bool locked);


/**
 * Helper which call pkg_addscript() with the content of the file and
 * with the correct type.
 */
int pkg_addscript_fileat(int fd, struct pkg *pkg, const char *path);
int pkg_addluascript_fileat(int fd, struct pkg *pkg, const char *path);

/**
 * Parse a manifest and set the attributes of pkg accordingly.
 * @param buf An NULL-terminated buffer containing the manifest data.
 * @return An error code.
 */
int pkg_parse_manifest(struct pkg *pkg, char *buf, size_t len, struct pkg_manifest_key *key);
int pkg_parse_manifest_file(struct pkg *pkg, const char *, struct pkg_manifest_key *key);
int pkg_parse_manifest_fileat(int fd, struct pkg *pkg, const char *, struct pkg_manifest_key *key);
int pkg_manifest_keys_new(struct pkg_manifest_key **k);
void pkg_manifest_keys_free(struct pkg_manifest_key *k);
int pkg_manifest_parser_new(struct pkg_manifest_parser **p);
void pkg_manifest_parser_free(struct pkg_manifest_parser *p);

#define PKG_MANIFEST_EMIT_COMPACT 0x1
#define PKG_MANIFEST_EMIT_NOFILES (0x1 << 1)
#define PKG_MANIFEST_EMIT_PRETTY (0x1 << 2)
#define PKG_MANIFEST_EMIT_JSON (0x1 << 3)
#define PKG_MANIFEST_EMIT_UCL (0x1 << 4)

/**
 * Emit a manifest according to the attributes of pkg.
 * @param buf A pointer which will hold the allocated buffer containing the
 * manifest. To be free'ed.
 * @param flags Flags for manifest emitting.
 * @param pdigest A pointer that will hold digest of manifest produced, ignored
 * if NULL. To be free'ed if not NULL.
 * @return An error code.
 */
int pkg_emit_manifest(struct pkg *pkg, char **buf, short flags, char **pdigest);
int pkg_emit_manifest_file(struct pkg*, FILE *, short, char **pdigest);

/* pkg_dep */
const char *pkg_dep_get(struct pkg_dep const * const , const pkg_dep_attr);
#define pkg_dep_name(d) pkg_dep_get(d, PKG_DEP_NAME)
#define pkg_dep_origin(d) pkg_dep_get(d, PKG_DEP_ORIGIN)
#define pkg_dep_version(d) pkg_dep_get(d, PKG_DEP_VERSION)
bool pkg_dep_is_locked(struct pkg_dep const * const);

bool pkg_has_dir(struct pkg *, const char *);
bool pkg_has_file(struct pkg *, const char *);

struct pkg_file *pkg_get_file(struct pkg *p, const char *path);
struct pkg_dir *pkg_get_dir(struct pkg *p, const char *path);

/* pkg_license */
const char *pkg_license_name(struct pkg_license const * const);

/* pkg_script */
const char *pkg_script_get(struct pkg const * const, pkg_script);

/**
 * @param db A pointer to a struct pkgdb object
 * @param origin Package origin
 * @param pkg An allocated struct pkg or a pointer to a NULL pointer. In the
 * last case, the function take care of the allocation.
 * @param flags OR'ed PKG_LOAD_*
 * @return EPKG_OK if the package is installed,
 * and != EPKG_OK if the package is not installed or an error occurred
 * Match will be case sensitive or insensitive depending on
 * pkgdb_case_sensitive()
 */
int pkg_try_installed(struct pkgdb *db, const char *origin,
		struct pkg **pkg, unsigned flags);

/**
 * @param db A pointer to a struct pkgdb object
 * @param origin Package origin
 * @return EPKG_OK if the package is installed,
 * and != EPKG_OK if the package is not installed or an error occurred
 * Match will be case sensitive or insensitive depending on
 * pkgdb_case_sensitive()
 */
int pkg_is_installed(struct pkgdb *db, const char *name);

/**
 * Create a repository database.
 * @param path The path where the repository live.
 * @param output_dir The path where the package repository should be created.
 * @param force If true, rebuild the repository catalogue from scratch
 * @param filesite If true, create a list of all files in repo
 * @param metafile Open meta from the specified file
 * @param legacy Create legacy (1.2 compatible) repo
 */
typedef int(pkg_password_cb)(char *, int, int, void*);
int pkg_create_repo(char *path, const char *output_dir, bool filelist,
	const char *metafile);
int pkg_finish_repo(const char *output_dir, pkg_password_cb *cb, char **argv,
    int argc, bool filelist);


/**
 * Test if the EUID has sufficient privilege to carry out some
 * operation (mode is a bitmap indicating READ, WRITE, CREATE) on the
 * databases indicated in the database bitmap.
 */
#define PKGDB_MODE_READ		(0x1<<0)
#define PKGDB_MODE_WRITE	(0x1<<1)
#define PKGDB_MODE_CREATE	(0x1<<2)

#define PKGDB_DB_LOCAL		(0x1<<0)
#define PKGDB_DB_REPO		(0x1<<1)
int pkgdb_access(unsigned mode, unsigned database);

/**
 * Open the local package database.
 * The db must be free'ed with pkgdb_close().
 * @return An error code.
 */
int pkgdb_open(struct pkgdb **db, pkgdb_t type);

/**
 * Open the local package database and repositories, possibly
 * overriding configured repositories and replacing with the given
 * reponame if not NULL
 * @return An error code
 */
int pkgdb_open_all(struct pkgdb **db, pkgdb_t type, const char *reponame);

/**
 * Locking functions
 */
int pkgdb_obtain_lock(struct pkgdb *db, pkgdb_lock_t type);
int pkgdb_upgrade_lock(struct pkgdb *db, pkgdb_lock_t old_type, pkgdb_lock_t new_type);
int pkgdb_downgrade_lock(struct pkgdb *db, pkgdb_lock_t old_type,
    pkgdb_lock_t new_type);
int pkgdb_release_lock(struct pkgdb *db, pkgdb_lock_t type);

/**
 * Transaction/savepoint handling.
 * @param savepoint -- if NULL or an empty string, use BEGIN, ROLLBACK, COMMIT
 * otherwise use SAVEPOINT, ROLLBACK TO, RELEASE.
 * @return an error code.
 */
int pkgdb_transaction_begin(struct pkgdb *db, const char *savepoint);
int pkgdb_transaction_commit(struct pkgdb *db, const char *savepoint);
int pkgdb_transaction_rollback(struct pkgdb *db, const char *savepoint);

/**
 * Close and free the struct pkgdb.
 */
void pkgdb_close(struct pkgdb *db);

/**
 * Initialize the local cache of the remote database with indicies
 */
int pkgdb_remote_init(struct pkgdb *db, const char *reponame);

/**
 * Dump to or load from a backup copy of the main database file
 * (local.sqlite)
 */

int pkgdb_dump(struct pkgdb *db, const char *dest);
int pkgdb_load(struct pkgdb *db, const char *src);

/**
 * Register a ports to the database.
 * @return An error code.
 */

int pkgdb_register_ports(struct pkgdb *db, struct pkg *pkg);

/**
 * Set the case sensitivity flag on or off.  Defaults to
 * true (case_sensitive)
 */
void pkgdb_set_case_sensitivity(bool);

/**
 * Query the state of the case sensitity setting.
 */
bool pkgdb_case_sensitive(void);

/**
 * Query the local package database.
 * @param type Describe how pattern should be used.
 * @warning Returns NULL on failure.
 */
struct pkgdb_it * pkgdb_query(struct pkgdb *db, const char *pattern,
    match_t type);
struct pkgdb_it * pkgdb_repo_query(struct pkgdb *db, const char *pattern,
    match_t type, const char *reponame);
struct pkgdb_it * pkgdb_repo_search(struct pkgdb *db, const char *pattern,
    match_t type, pkgdb_field field, pkgdb_field sort, const char *reponame);

/**
 * @todo Return directly the struct pkg?
 */
struct pkgdb_it * pkgdb_query_which(struct pkgdb *db, const char *path, bool glob);

struct pkgdb_it * pkgdb_query_shlib_require(struct pkgdb *db, const char *shlib);
struct pkgdb_it * pkgdb_query_shlib_provide(struct pkgdb *db, const char *shlib);
struct pkgdb_it * pkgdb_query_require(struct pkgdb *db, const char *req);
struct pkgdb_it * pkgdb_query_provide(struct pkgdb *db, const char *req);

struct pkgdb_it * pkgdb_rquery_provide(struct pkgdb *db,
    const char *provide, const char *repo);

/**
 * Add/Modify/Delete an annotation for a package
 * @param tag -- tag for the annotation
 * @param value -- text of the annotation
 * @return An error code
 */
int pkgdb_add_annotation(struct pkgdb *db, struct pkg *pkg,
	const char *tag, const char *value);
int pkgdb_modify_annotation(struct pkgdb *db, struct pkg *pkg,
	const char *tag, const char *value);
int pkgdb_delete_annotation(struct pkgdb *db, struct pkg *pkg,
	const char *tag);

#define PKG_LOAD_BASIC			0
#define PKG_LOAD_DEPS			(1U << 0)
#define PKG_LOAD_RDEPS			(1U << 1)
#define PKG_LOAD_FILES			(1U << 2)
#define PKG_LOAD_SCRIPTS		(1U << 3)
#define PKG_LOAD_OPTIONS		(1U << 4)
#define PKG_LOAD_DIRS			(1U << 5)
#define PKG_LOAD_CATEGORIES		(1U << 6)
#define PKG_LOAD_LICENSES		(1U << 7)
#define PKG_LOAD_USERS			(1U << 8)
#define PKG_LOAD_GROUPS			(1U << 9)
#define PKG_LOAD_SHLIBS_REQUIRED	(1U << 10)
#define PKG_LOAD_SHLIBS_PROVIDED	(1U << 11)
#define PKG_LOAD_ANNOTATIONS		(1U << 12)
#define PKG_LOAD_CONFLICTS		(1U << 13)
#define PKG_LOAD_PROVIDES		(1U << 14)
#define PKG_LOAD_REQUIRES		(1U << 15)
#define PKG_LOAD_LUA_SCRIPTS		(1u << 16)
/* Make sure new PKG_LOAD don't conflict with PKG_CONTAINS_* */

/**
 * Get the next pkg.
 * @param pkg An allocated struct pkg or a pointer to a NULL pointer. In the
 * last case, the function take care of the allocation.
 * @param flags OR'ed PKG_LOAD_*
 * @return An error code.
 */
int pkgdb_it_next(struct pkgdb_it *, struct pkg **pkg, unsigned flags);

/**
 * Reset the pkgdb_it iterator, allowing for re-iterating over it
 */
void pkgdb_it_reset(struct pkgdb_it *);

/**
 * Return the number of rows found.
 * @return -1 on error
 */
int pkgdb_it_count(struct pkgdb_it *);

/**
 * Free a struct pkgdb_it.
 */
void pkgdb_it_free(struct pkgdb_it *);

/**
 * Compact the database to save space.
 * Note that the function will really compact the database only if some
 * internal criterias are met.
 * @return An error code.
 */
int pkgdb_compact(struct pkgdb *db);

/**
 * Install and register a new package.
 * @param db An opened pkgdb
 * @param path The path to the package archive file on the local disk
 * @return An error code.
 */
int pkg_add(struct pkgdb *db, const char *path, unsigned flags,
    struct pkg_manifest_key *keys, const char *location);

int pkg_add_from_remote(struct pkgdb *db, const char *path, unsigned flags,
    struct pkg_manifest_key *keys, const char *location, struct pkg *rp);

#define PKG_ADD_UPGRADE			(1U << 0)
#define PKG_ADD_USE_UPGRADE_SCRIPTS	(1U << 1)
#define PKG_ADD_AUTOMATIC		(1U << 2)
#define PKG_ADD_FORCE			(1U << 3)
#define PKG_ADD_NOSCRIPT		(1U << 4)
#define PKG_ADD_FORCE_MISSING		(1U << 5)
#define PKG_ADD_SPLITTED_UPGRADE	(1U << 6)

/**
 * Allocate a new pkg_jobs.
 * @param db A pkgdb open with PKGDB_REMOTE.
 * @return An error code.
 */
int pkg_jobs_new(struct pkg_jobs **jobs, pkg_jobs_t type, struct pkgdb *db);

/**
 * Free a pkg_jobs
 */
void pkg_jobs_free(struct pkg_jobs *jobs);

/**
 * Add a pkg to the jobs queue.
 * @return An error code.
 */
int pkg_jobs_add(struct pkg_jobs *j, match_t match, char **argv, int argc);
int pkg_jobs_solve(struct pkg_jobs *j);
int pkg_jobs_set_repository(struct pkg_jobs *j, const char *name);
const char* pkg_jobs_destdir(struct pkg_jobs *j);
int pkg_jobs_set_destdir(struct pkg_jobs *j, const char *name);
void pkg_jobs_set_flags(struct pkg_jobs *j, pkg_flags f);
pkg_jobs_t pkg_jobs_type(struct pkg_jobs *j);

/**
 * Returns the number of elements in the job queue
 */
int pkg_jobs_count(struct pkg_jobs *jobs);

/**
 * Returns the number of total elements in pkg universe
 */
int pkg_jobs_total(struct pkg_jobs *jobs);

/**
 * Iterates over the packages in the jobs queue.
 * @param iter Must be set to NULL for the first call.
 * @return A next pkg or NULL.
 */
bool pkg_jobs_iter(struct pkg_jobs *jobs, void **iter, struct pkg **n,
	struct pkg **o, int *type);

/**
 * Apply the jobs in the queue (fetch and install).
 * @return An error code.
 */
int pkg_jobs_apply(struct pkg_jobs *jobs);

/**
 * Emit CUDF spec to a file for a specified jobs request
 * @return error code
 */
int pkg_jobs_cudf_emit_file(struct pkg_jobs *, pkg_jobs_t , FILE *);

/**
 * Parse the output of an external CUDF solver
 * @return error code
 */
int pkg_jobs_cudf_parse_output(struct pkg_jobs *j, FILE *f);

/**
 * Check if there are locked packages
 * @return true if locked packages exist, false if not
 */
bool pkg_jobs_has_lockedpkgs(struct pkg_jobs *j);

/**
 * Iterate through the locked packages, calling the passed in function pointer
 * on each.
 */
typedef int(*locked_pkgs_cb)(struct pkg *, void *);
void pkg_jobs_iter_lockedpkgs(struct pkg_jobs *j, locked_pkgs_cb, void *);

/**
 * Solve a SAT problem
 * @return true if a problem is solvable
 */
int pkg_solve_sat_problem(struct pkg_solve_problem *problem);

/**
 * Export SAT problem to a dot graph description
 */
void pkg_solve_dot_export(struct pkg_solve_problem *problem, FILE *file);

/**
 * Convert package jobs to a SAT problem
 * @return SAT problem or NULL if failed
 */
struct pkg_solve_problem * pkg_solve_jobs_to_sat(struct pkg_jobs *j);

/**
 * Export sat problem to the DIMACS format
 * @return error code
 */
int pkg_solve_dimacs_export(struct pkg_solve_problem *problem, FILE *f);

/**
 * Move solved problem to the jobs structure
 * @return error code
 */
int pkg_solve_sat_to_jobs(struct pkg_solve_problem *problem);

/**
 * Parse SAT solver output and convert it to jobs
 * @return error code
 */
int pkg_solve_parse_sat_output(FILE *f, struct pkg_solve_problem *problem);

/**
 * Free a SAT problem structure
 */
void pkg_solve_problem_free(struct pkg_solve_problem *problem);

/**
 * Archive formats options.
 */
typedef enum pkg_formats { TAR, TGZ, TBZ, TXZ, TZS } pkg_formats;

/**
 * Create package from an installed & registered package
 */
int pkg_create_installed(const char *, pkg_formats, struct pkg *);

/**
 * Create package from stage install using just the manifest --
 * no old pkg_tools compatibility stuff
 */
int pkg_create_from_manifest(const char *, pkg_formats, const char *,
    const char *, const char *);

/**
 * Create package from stage install with a metadata directory
 */
int pkg_create_staged(const char *, pkg_formats, const char *, const char *, char *, bool);
int pkg_load_metadata(struct pkg *, const char *, const char *, const char *, const char *, bool);

/**
 * Download the latest repo db file and checks its signature if any
 * @param force Always download the repo catalogue
 */
int pkg_update(struct pkg_repo *repo, bool force);

/**
 * Get statistics information from the package database(s)
 * @param db A valid database object as returned by pkgdb_open()
 * @param type Type of statistics to be returned
 * @return The statistic information requested
 */
int64_t pkgdb_stats(struct pkgdb *db, pkg_stats_t type);

/**
 * pkg plugin functions
 * @todo Document
 */
int pkg_plugins_init(void);
void pkg_plugins_shutdown(void);
int pkg_plugins(struct pkg_plugin **plugin);
int pkg_plugin_set(struct pkg_plugin *p, pkg_plugin_key key, const char *str);
const char *pkg_plugin_get(struct pkg_plugin *p, pkg_plugin_key key);
void *pkg_plugin_func(struct pkg_plugin *p, const char *func);

int pkg_plugin_conf_add(struct pkg_plugin *p, pkg_object_t type, const char *key, const char *def);
const pkg_object *pkg_plugin_conf(struct pkg_plugin *p);

int pkg_plugin_parse(struct pkg_plugin *p);
void pkg_plugin_errno(struct pkg_plugin *p, const char *func, const char *arg);
void pkg_plugin_error(struct pkg_plugin *p, const char *fmt, ...);
void pkg_plugin_info(struct pkg_plugin *p, const char *fmt, ...);
/**
 * This is where plugin hook into the library using pkg_plugin_hook()
 * @todo: Document
 */
typedef int(*pkg_plugin_callback)(void *data, struct pkgdb *db);
int pkg_plugins_hook_run(pkg_plugin_hook_t hook, void *data, struct pkgdb *db);
int pkg_plugin_hook_register(struct pkg_plugin *p, pkg_plugin_hook_t hook, pkg_plugin_callback callback);

/**
 * Get the value of a configuration key
 */

const pkg_object *pkg_config_get(const char *);
pkg_object_t pkg_object_type(const pkg_object *);
const pkg_object *pkg_object_find(const pkg_object *o, const char *key);
int64_t pkg_object_int(const pkg_object *o);
bool pkg_object_bool(const pkg_object *o);
const char *pkg_object_string(const pkg_object *o);
void pkg_object_free(pkg_object *o);
const char *pkg_object_key(const pkg_object *);
const pkg_object *pkg_object_iterate(const pkg_object *, pkg_iter *);
unsigned pkg_object_count(const pkg_object *);
const char *pkg_object_dump(const pkg_object *o);
const char *pkg_config_dump(void);

/**
 * @todo Document
 */
int pkg_version_cmp(const char * const , const char * const);
pkg_change_t pkg_version_change(const struct pkg * restrict);
pkg_change_t pkg_version_change_between(const struct pkg * pkg1, const struct pkg *pkg2);

/**
 * Fetch a file.
 * @return An error code.
 */
int pkg_fetch_file(struct pkg_repo *repo, const char *url, char *dest, time_t t,
    ssize_t offset, int64_t size);
/**
 * Fetch a file to temporary destination
 */
int pkg_fetch_file_tmp(struct pkg_repo *repo, const char *url, char *dest,
	time_t t);

/**
 * Get cached name of a package
 */
int pkg_repo_cached_name(struct pkg *pkg, char *dest, size_t destlen);

/* glue to deal with ports */
int ports_parse_plist(struct pkg *, const char *, const char *);

/**
 * Special structure to report about conflicts
 */
struct pkg_event_conflict {
	char *uid;
	struct pkg_event_conflict *next;
};

/*
 * Capsicum sandbox callbacks
 */
typedef int (*pkg_sandbox_cb)(int fd, void *user_data);

/**
 * Event type used to report progress or problems.
 */
typedef enum {
	/* informational */
	PKG_EVENT_INSTALL_BEGIN = 0,
	PKG_EVENT_INSTALL_FINISHED,
	PKG_EVENT_DEINSTALL_BEGIN,
	PKG_EVENT_DEINSTALL_FINISHED,
	PKG_EVENT_UPGRADE_BEGIN,
	PKG_EVENT_UPGRADE_FINISHED,
	PKG_EVENT_EXTRACT_BEGIN,
	PKG_EVENT_EXTRACT_FINISHED,
	PKG_EVENT_DELETE_FILES_BEGIN,
	PKG_EVENT_DELETE_FILES_FINISHED,
	PKG_EVENT_ADD_DEPS_BEGIN,
	PKG_EVENT_ADD_DEPS_FINISHED,
	PKG_EVENT_FETCHING,
	PKG_EVENT_FETCH_BEGIN,
	PKG_EVENT_FETCH_FINISHED,
	PKG_EVENT_UPDATE_ADD,
	PKG_EVENT_UPDATE_REMOVE,
	PKG_EVENT_INTEGRITYCHECK_BEGIN,
	PKG_EVENT_INTEGRITYCHECK_FINISHED,
	PKG_EVENT_INTEGRITYCHECK_CONFLICT,
	PKG_EVENT_NEWPKGVERSION,
	PKG_EVENT_NOTICE,
	PKG_EVENT_DEBUG,
	PKG_EVENT_INCREMENTAL_UPDATE,
	PKG_EVENT_QUERY_YESNO,
	PKG_EVENT_QUERY_SELECT,
	PKG_EVENT_SANDBOX_CALL,
	PKG_EVENT_SANDBOX_GET_STRING,
	PKG_EVENT_PROGRESS_START,
	PKG_EVENT_PROGRESS_TICK,
	PKG_EVENT_BACKUP,
	PKG_EVENT_RESTORE,
	/* errors */
	PKG_EVENT_ERROR,
	PKG_EVENT_ERRNO,
	PKG_EVENT_ARCHIVE_COMP_UNSUP = 65536,
	PKG_EVENT_ALREADY_INSTALLED,
	PKG_EVENT_FAILED_CKSUM,
	PKG_EVENT_CREATE_DB_ERROR,
	PKG_EVENT_LOCKED,
	PKG_EVENT_REQUIRED,
	PKG_EVENT_MISSING_DEP,
	PKG_EVENT_NOREMOTEDB,
	PKG_EVENT_NOLOCALDB,
	PKG_EVENT_FILE_MISMATCH,
	PKG_EVENT_DEVELOPER_MODE,
	PKG_EVENT_PLUGIN_ERRNO,
	PKG_EVENT_PLUGIN_ERROR,
	PKG_EVENT_PLUGIN_INFO,
	PKG_EVENT_NOT_FOUND,
	PKG_EVENT_NEW_ACTION,
	PKG_EVENT_MESSAGE,
	PKG_EVENT_FILE_MISSING,
	PKG_EVENT_CLEANUP_CALLBACK_REGISTER,
	PKG_EVENT_CLEANUP_CALLBACK_UNREGISTER,
	PKG_EVENT_CONFLICTS,
} pkg_event_t;

struct pkg_event {
	pkg_event_t type;
	union {
		struct {
			const char *func;
			const char *arg;
			int no;
		} e_errno;
		struct {
			char *msg;
		} e_pkg_error;
		struct {
			char *msg;
		} e_pkg_notice;
		struct {
			int total;
			int done;
		} e_upd_add;
		struct {
			int total;
			int done;
		} e_upd_remove;
		struct {
			const char *url;
		} e_fetching;
		struct {
			struct pkg *pkg;
		} e_already_installed;
		struct {
			struct pkg *pkg;
		} e_install_begin;
		struct {
			struct pkg *pkg;
			struct pkg *old;
		} e_install_finished;
		struct {
			struct pkg *pkg;
		} e_deinstall_begin;
		struct {
			struct pkg *pkg;
		} e_deinstall_finished;
		struct {
			struct pkg *n;
			struct pkg *o;
		} e_upgrade_begin;
		struct {
			struct pkg *n;
			struct pkg *o;
		} e_upgrade_finished;
		struct {
			struct pkg *pkg;
		} e_extract_begin;
		struct {
			struct pkg *pkg;
		} e_extract_finished;
		struct {
			struct pkg *pkg;
		} e_delete_files_begin;
		struct {
			struct pkg *pkg;
		} e_delete_files_finished;
		struct {
			struct pkg *pkg;
		} e_add_deps_begin;
		struct {
			struct pkg *pkg;
		} e_add_deps_finished;
		struct {
			struct pkg *pkg;
			struct pkg_dep *dep;
		} e_missing_dep;
		struct {
			struct pkg *pkg;
		} e_locked;
		struct {
			struct pkg *pkg;
			int force;
		} e_required;
		struct {
			const char *repo;
		} e_remotedb;
		struct {
			struct pkg *pkg;
			struct pkg_file *file;
			const char *newsum;
		} e_file_mismatch;
		struct {
			struct pkg_plugin *plugin;
			char *msg;
		} e_plugin_info;
		struct {
			struct pkg_plugin *plugin;
			const char *func;
			const char *arg;
			int no;
		} e_plugin_errno;
		struct {
			struct pkg_plugin *plugin;
			char *msg;
		} e_plugin_error;
		struct {
			const char *pkg_name;
		} e_not_found;
		struct {
			const char *pkg_uid;
			const char *pkg_path;
			struct pkg_event_conflict *conflicts;
		} e_integrity_conflict;
		struct {
			int conflicting;
		} e_integrity_finished;
		struct {
			const char *reponame;
			int processed;
		} e_incremental_update;
		struct {
			int level;
			char *msg;
		} e_debug;
		struct {
			const char *msg;
			int deft;
		} e_query_yesno;
		struct {
			const char *msg;
			const char **items;
			int ncnt;
			int deft;
		} e_query_select;
		struct {
			pkg_sandbox_cb call;
			int fd;
			void *userdata;
		} e_sandbox_call;
		struct {
			pkg_sandbox_cb call;
			void *userdata;
			char **result;
			int64_t *len;
		} e_sandbox_call_str;
		struct {
			char *msg;
		} e_progress_start;
		struct {
			int64_t current;
			int64_t total;
		} e_progress_tick;
		struct {
			const char *msg;
		} e_pkg_message;
		struct {
			struct pkg *pkg;
			struct pkg_file *file;
		} e_file_missing;
		struct {
			void *data;
			void (*cleanup_cb)(void *data);
		} e_cleanup_callback;
		struct {
			struct pkg *p1;
			struct pkg *p2;
			const char *path;
		} e_conflicts;
	};
};

/**
 * Event callback mechanism.  Events will be reported using this callback,
 * providing an event identifier and up to two event-specific pointers.
 */
typedef int(*pkg_event_cb)(void *, struct pkg_event *);

void pkg_event_register(pkg_event_cb cb, void *data);

bool pkg_compiled_for_same_os_major(void);
int pkg_ini(const char *, const char *, pkg_init_flags);
int pkg_init(const char *, const char *);
int pkg_initialized(void);
void pkg_shutdown(void);

int pkg_test_filesum(struct pkg *);
int pkg_recompute(struct pkgdb *, struct pkg *);
int pkgdb_reanalyse_shlibs(struct pkgdb *, struct pkg *);

void pkgdb_cmd(int argc, char **argv);
int pkg_sshserve(int fd);

int pkg_repos_total_count(void);
int pkg_repos_activated_count(void);
int pkg_repos(struct pkg_repo **);
const char *pkg_repo_url(struct pkg_repo *r);
const char *pkg_repo_name(struct pkg_repo *r);
const char *pkg_repo_key(struct pkg_repo *r);
const char *pkg_repo_fingerprints(struct pkg_repo *r);
signature_t pkg_repo_signature_type(struct pkg_repo *r);
bool pkg_repo_enabled(struct pkg_repo *r);
mirror_t pkg_repo_mirror_type(struct pkg_repo *r);
unsigned pkg_repo_priority(struct pkg_repo *r);
unsigned pkg_repo_ip_version(struct pkg_repo *r);
struct pkg_repo *pkg_repo_find(const char *name);

/**
 * pkg_printf() and friends.  These parallel the similarly named libc
 * functions printf(), fprintf() etc.
 */

/**
 * print to stdout data from pkg as indicated by the format code format
 * @param ... Varargs list of struct pkg etc. supplying the data
 * @param format String with embedded %-escapes indicating what to print
 * @return count of the number of characters printed
 */
int pkg_printf(const char * restrict format, ...);

/**
 * print to stdout data from pkg as indicated by the format code format
 * @param ap Varargs list of struct pkg etc. supplying the data
 * @param format String with embedded %-escapes indicating what to print
 * @return count of the number of characters printed
 */
int pkg_vprintf(const char * restrict format, va_list ap);

/**
 * print to named stream from pkg as indicated by the format code format
 * @param ... Varargs list of struct pkg etc. supplying the data
 * @param format String with embedded %-escapes indicating what to output
 * @return count of the number of characters printed
 */
int pkg_fprintf(FILE * restrict stream, const char * restrict format, ...);

/**
 * print to named stream from pkg as indicated by the format code format
 * @param ap varargs arglist
 * @param format String with embedded %-escapes indicating what to output
 * @return count of the number of characters printed
 */
int pkg_vfprintf(FILE * restrict stream, const char * restrict format,
	va_list ap);

/**
 * print to file descriptor fd data from pkg as indicated by the format
 * code format
 * @param fd Previously opened file descriptor to print to
 * @param ... Varargs list of struct pkg etc. supplying the data
 * @param format String with embedded %-escapes indicating what to print
 * @return count of the number of characters printed
 */
int pkg_dprintf(int fd, const char * restrict format, ...);

/**
 * print to file descriptor fd data from pkg as indicated by the format
 * code format
 * @param fd Previously opened file descriptor to print to
 * @param ap Varargs list of struct pkg etc. supplying the data
 * @param format String with embedded %-escapes indicating what to print
 * @return count of the number of characters printed
 */
int pkg_vdprintf(int fd, const char * restrict format, va_list ap);

/**
 * print to buffer str of given size data from pkg as indicated by the
 * format code format as a NULL-terminated string
 * @param str Character array buffer to receive output
 * @param size Length of the buffer str
 * @param ... Varargs list of struct pkg etc. supplying the data
 * @param format String with embedded %-escapes indicating what to output
 * @return count of the number of characters that would have been output
 * disregarding truncation to fit size
 */
int pkg_snprintf(char * restrict str, size_t size,
	const char * restrict format, ...);

/**
 * print to buffer str of given size data from pkg as indicated by the
 * format code format as a NULL-terminated string
 * @param str Character array buffer to receive output
 * @param size Length of the buffer str
 * @param ap Varargs list of struct pkg etc. supplying the data
 * @param format String with embedded %-escapes indicating what to output
 * @return count of the number of characters that would have been output
 * disregarding truncation to fit size
 */
int pkg_vsnprintf(char * restrict str, size_t size,
	const char * restrict format,va_list ap);

/**
 * Allocate a string buffer ret sufficiently big to contain formatted
 * data data from pkg as indicated by the format code format
 * @param ret location of pointer to be set to point to buffer containing
 * result
 * @param ... Varargs list of struct pkg etc. supplying the data
 * @param format String with embedded %-escapes indicating what to output
 * @return count of the number of characters printed
 */
int pkg_asprintf(char **ret, const char * restrict format, ...);

/**
 * Allocate a string buffer ret sufficiently big to contain formatted
 * data data from pkg as indicated by the format code format
 * @param ret location of pointer to be set to point to buffer containing
 * result
 * @param ap Varargs list of struct pkg etc. supplying the data
 * @param format String with embedded %-escapes indicating what to output
 * @return count of the number of characters printed
 */
int pkg_vasprintf(char **ret, const char * restrict format, va_list ap);

#if defined(UTSTRING_H)
/**
 * store data from pkg into sbuf as indicated by the format code format.
 * @param sbuf contains the result
 * @param ... Varargs list of struct pkg etc. supplying the data
 * @param format String with embedded %-escapes indicating what to output
 * @return count of the number of characters in the result
 */
UT_string *pkg_utstring_printf(UT_string * restrict sbuf,
	const char * restrict format, ...);

/**
 * store data from pkg into sbuf as indicated by the format code format.
 * This is the core function called by all the other pkg_printf() family.
 * @param sbuf contains the result
 * @param ap Arglist with struct pkg etc. supplying the data
 * @param format String with embedded %-escapes indicating what to output
 * @return count of the number of characters in the result
 */
UT_string *pkg_utstring_vprintf(UT_string * restrict sbuf,
	const char * restrict format, va_list ap);
#endif

bool pkg_has_message(struct pkg *p);
bool pkg_is_locked(const struct pkg * restrict p);


/**
 * Defines how many chars of checksum are there in a package's name
 * We define this number as sufficient for 24k packages.
 * To find out probability of collision it is possible to use the following
 * python function to calculate 'birthday paradox' probability:
 *  def bp(m, n):
 *      power = -(n * n) / (2. * m)
 *      return 1. - exp(power)
 *
 * For our choice of 2^40 (or 10 hex characters) it is:
 *  >>> bp(float(2 ** 40), 24500.)
 *  0.00027292484660568217
 *
 * And it is negligible probability
 */
#define PKG_FILE_CKSUM_CHARS 10

struct pkg_audit;

/**
 * Creates new pkg_audit structure
 */
struct pkg_audit * pkg_audit_new(void);

/**
 * Fetch and extract audit file from url `src` to the file `dest`
 * If no update is required then this function returns `EPKG_UPTODATE`
 * @return error code
 */
int pkg_audit_fetch(const char *src, const char *dest);

/**
 * Load audit file into memory
 * @return error code
 */
int pkg_audit_load(struct pkg_audit *audit, const char *fname);

/**
 * Process loaded audit structure.
 * Can and should be executed after cap_enter(3) or another sandboxing call
 * @return error code
 */
int pkg_audit_process(struct pkg_audit *audit);

#if defined(UTSTRING_H)
/**
 * Check whether `pkg` is vulnerable against processed `audit` structure.
 * If a package is vulnerable, then `result` is set to sbuf describing the
 * vulnerability. If `quiet` is true, then this function produces reduced output
 * just returning a name of vulnerable package.
 * It's caller responsibility to free `result` after use
 * @return true and `*result` is set if a package is vulnerable
 */
bool pkg_audit_is_vulnerable(struct pkg_audit *audit, struct pkg *pkg,
		bool quiet, UT_string **result, int *affected);
#endif

void pkg_audit_free (struct pkg_audit *audit);
char *pkg_utils_tokenize(char **);
int pkg_utils_count_spaces(const char *);
int pkg_add_port(struct pkgdb *db, struct pkg *pkg, const char *root, \
    const char *locationn, bool testing);
char *pkg_absolutepath(const char *src, char *dest, size_t dest_len, bool fromroot);

void pkg_cache_full_clean(void);

int pkg_get_cachedirfd(void);
int pkg_get_dbdirfd(void);

#ifdef __cplusplus
}
#endif
#endif
