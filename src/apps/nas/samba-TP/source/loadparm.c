/* 
   Unix SMB/Netbios implementation.
   Version based 3.0.
   Parameter loading functions
   Copyright (C) Karl Auer 1993,1997

   Largely re-written by Andrew Tridgell, September 1994
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
 *  Load parameters.
 *
 *  This module provides suitable callback functions for the params
 *  module. It builds the internal table of service details which is
 *  then used by the rest of the server.
 *
 * To add a parameter:
 *
 * 1) add it to the global or service structure definition
 * 2) add it to the parm_table
 * 3) add it to the list of available functions (eg: using FN_GLOBAL_STRING())
 * 4) If it's a global then initialise it in init_globals. If a local
 *    (ie. service) parameter then initialise it in the sDefault structure
 *  
 *
 * Notes:
 *   The configuration file is processed sequentially for speed. It is NOT
 *   accessed randomly as happens in 'real' Windows. For this reason, there
 *   is a fair bit of sequence-dependent code here - ie., code which assumes
 *   that certain things happen before others. In particular, the code which
 *   happens at the boundary between sections is delicately poised, so be
 *   careful!
 *
 */

#include "includes.h"

BOOL bLoaded = False;

extern int DEBUGLEVEL;
extern pstring user_socket_options;
extern pstring myname;

#ifndef GLOBAL_NAME
#define GLOBAL_NAME "global"
#endif

#ifndef PRINTERS_NAME
#define PRINTERS_NAME "printers"
#endif

#ifndef HOMES_NAME
#define HOMES_NAME "homes"
#endif

/* some helpful bits */
#define pSERVICE(i) ServicePtrs[i]
#define iSERVICE(i) (*pSERVICE(i))
#define LP_SNUM_OK(iService) (((iService) >= 0) && ((iService) < iNumServices) && iSERVICE(iService).valid)
#define VALID(i) iSERVICE(i).valid

/* these are the types of parameter we have */
typedef enum
{
  P_BOOL,P_BOOLREV,P_CHAR,P_INTEGER,P_OCTAL,
  P_STRING,P_USTRING,P_GSTRING,P_UGSTRING,P_ENUM
} parm_type;

typedef enum
{
  P_LOCAL,P_GLOBAL,P_NONE
} parm_class;

int keepalive=0;
extern BOOL use_getwd_cache;

extern int extra_time_offset;

/* 
 * This structure describes global (ie., server-wide) parameters.
 */
typedef struct
{
//  char *szPrintcapname;
  char *szLockDir;
  char *szRootdir;
  char *szDefaultService;
//  char *szDfree;
//  char *szMsgCommand;
  char *szHostsEquiv;
  char *szServerString;
  char *szAutoServices;
  char *szPasswdProgram;
  char *szPasswdChat;
  char *szLogFile;
  char *szConfigFile;
  char *szSMBPasswdFile;
  char *szPasswordServer;
  char *szSocketOptions;
  char *szValidChars;
  char *szWorkGroup;
  char *szDomainAdminUsers;
  char *szDomainGuestUsers;
  char *szDomainHostsallow; 
  char *szDomainHostsdeny;
  char *szUsernameMap;
  char *szCharacterSet;
  char *szLogonScript;
  char *szLogonPath;
  char *szLogonDrive;
  char *szLogonHome;
//  char *szSmbrun;
  char *szWINSserver;
  char *szCodingSystem;
  char *szInterfaces;
  char *szRemoteAnnounce;
  char *szRemoteBrowseSync;
  char *szSocketAddress;
  char *szNISHomeMapName;
  char *szAnnounceVersion; /* This is initialised in init_globals */
  char *szNetbiosAliases;
  char *szDomainSID;
  char *szDomainOtherSIDs;
  char *szDomainGroups;
//  char *szDriverFile;
  int max_log_size;
  int mangled_stack;
  int max_xmit;
  int max_mux;
  int max_packet;
  int pwordlevel;
  int unamelevel;
  int deadtime;
  int maxprotocol;
  int minprotocol;
  int security;
  int maxdisksize;
  int lpqcachetime;
  int syslog;
  int os_level;
  int max_ttl;
  int max_wins_ttl;
  int min_wins_ttl;
  int ReadSize;
  int lm_announce;
  int lm_interval;
  int shmem_size;
//  int client_code_page;
  int announce_as;   /* This is initialised in init_globals */
  BOOL bDNSproxy;
  BOOL bWINSsupport;
  BOOL bWINSproxy;
  BOOL bLocalMaster;
  BOOL bPreferredMaster;
  BOOL bDomainController;
  BOOL bDomainMaster;
  BOOL bDomainLogons;
  BOOL bEncryptPasswords;
  BOOL bStripDot;
  BOOL bNullPasswords;
  BOOL bLoadPrinters;
  BOOL bUseRhosts;
  BOOL bReadRaw;
  BOOL bWriteRaw;
//  BOOL bReadPrediction;
  BOOL bReadbmpx;
  BOOL bSyslogOnly;
  BOOL bBrowseList;
  BOOL bUnixRealname;
  BOOL bNISHomeMap;
  BOOL bTimeServer;
  BOOL bBindInterfacesOnly;
} global;

static global Globals;

/* 
 * This structure describes a single service. 
 */
typedef struct
{
  BOOL valid;
  char *szService;
  char *szPath;
  char *szUsername;
  char *szGuestaccount;
  char *szInvalidUsers;
  char *szValidUsers;
  char *szAdminUsers;
  char *szCopy;
  char *szInclude;
  char *szPreExec;
//  char *szPostExec;
//  char *szRootPreExec;
//  char *szRootPostExec;
//  char *szPrintcommand;
  char *szLpqcommand;
  char *szLprmcommand;
  char *szLppausecommand;
  char *szLpresumecommand;
//  char *szPrintername;
//  char *szPrinterDriver;
//  char *szPrinterDriverLocation;
  char *szDontdescend;
  char *szHostsallow;
  char *szHostsdeny;
//  char *szMagicScript;
//  char *szMagicOutput;
  char *szMangledMap;
  char *szVetoFiles;
  char *szHideFiles;
  char *szVetoOplockFiles;
  char *comment;
  char *force_user;
  char *force_group;
  char *readlist;
  char *writelist;
  char *volume;
//  int  iMinPrintSpace;
  int  iCreate_mask;
  int  iCreate_force_mode;
  int  iDir_mask;
  int  iDir_force_mode;
  int  iMaxConnections;
  int  iDefaultCase;
//  int  iPrinting;
  BOOL bAlternatePerm;
  BOOL bRevalidate;
  BOOL bCaseSensitive;
  BOOL bCasePreserve;
  BOOL bShortCasePreserve;
  BOOL bCaseMangle;
  BOOL status;
  BOOL bHideDotFiles;
  BOOL bBrowseable;
  BOOL bAvailable;
  BOOL bRead_only;
  BOOL bNo_set_dir;
  BOOL bGuest_only;
  BOOL bGuest_ok;
//  BOOL bPrint_ok;
  BOOL bPostscript;
  BOOL bMap_system;
  BOOL bMap_hidden;
  BOOL bMap_archive;
  BOOL bLocking;
  BOOL bStrictLocking;
  BOOL bShareModes;
  BOOL bOpLocks;
  BOOL bOnlyUser;
  BOOL bMangledNames;
  BOOL bWidelinks;
  BOOL bSymlinks;
//  BOOL bSyncAlways;
  char magic_char;
  BOOL *copymap;
  BOOL bDeleteReadonly;
  BOOL bFakeOplocks;
  BOOL bDeleteVetoFiles;
  BOOL bDosFiletimes;
  char dummy[3]; /* for alignment */
} service;


/* This is a default service used to prime a services structure */
static service sDefault = 
{
  True,   /* valid */
  NULL,    /* szService */
  NULL,    /* szPath */
  NULL,    /* szUsername */
  NULL,    /* szGuestAccount  - this is set in init_globals() */
  NULL,    /* szInvalidUsers */
  NULL,    /* szValidUsers */
  NULL,    /* szAdminUsers */
  NULL,    /* szCopy */
  NULL,    /* szInclude */
  NULL,    /* szPreExec */
//  NULL,    /* szPostExec */
//  NULL,    /* szRootPreExec */
//  NULL,    /* szRootPostExec */
//  NULL,    /* szPrintcommand */
  NULL,    /* szLpqcommand */
  NULL,    /* szLprmcommand */
  NULL,    /* szLppausecommand */
  NULL,    /* szLpresumecommand */
//  NULL,    /* szPrintername */
//  NULL,    /* szPrinterDriver - this is set in init_globals() */
//  NULL,    /* szPrinterDriverLocation */
  NULL,    /* szDontdescend */
  NULL,    /* szHostsallow */
  NULL,    /* szHostsdeny */
//  NULL,    /* szMagicScript */
//  NULL,    /* szMagicOutput */
  NULL,    /* szMangledMap */
  NULL,    /* szVetoFiles */
  NULL,    /* szHideFiles */
  NULL,    /* szVetoOplockFiles */
  NULL,    /* comment */
  NULL,    /* force user */
  NULL,    /* force group */
  NULL,    /* readlist */
  NULL,    /* writelist */
  NULL,    /* volume */
//  0,       /* iMinPrintSpace */
  0744,    /* iCreate_mask */
  0000,    /* iCreate_force_mode */
  0755,    /* iDir_mask */
  0000,    /* iDir_force_mode */
  0,       /* iMaxConnections */
  CASE_LOWER, /* iDefaultCase */
//  DEFAULT_PRINTING, /* iPrinting */
  False,   /* bAlternatePerm */
  False,   /* revalidate */
/*##DO NOT MODIFY THE VALUE OF THE FOLLOWING THREE PARAMETERS!!!##*/
  False,   /* case sensitive */
  True,   /* case preserve */
  True,   /* short case preserve */
/*################################################*/
  False,  /* case mangle */
  True,  /* status */
  True,  /* bHideDotFiles */
  True,  /* bBrowseable */
  True,  /* bAvailable */
  True,  /* bRead_only */
  True,  /* bNo_set_dir */
  False, /* bGuest_only */
  False, /* bGuest_ok */
//  False, /* bPrint_ok */
  False, /* bPostscript */
  False, /* bMap_system */
  False, /* bMap_hidden */
  True,  /* bMap_archive */
  True,  /* bLocking */
  False,  /* bStrictLocking */
  True,  /* bShareModes */
 /* bOpLocks */
#ifdef OPLOCK_ENABLE
  True, 
#else
  False,
#endif
//bOpLocks
  False, /* bOnlyUser */
  True,  /* bMangledNames */
  False,  /* bWidelinks */
  False,  /* bSymlinks */
//  False, /* bSyncAlways */
  '~',   /* magic char */
  NULL,  /* copymap */
  False, /* bDeleteReadonly */
  False, /* bFakeOplocks */
  False, /* bDeleteVetoFiles */
  False, /* bDosFiletimes */
  ""     /* dummy */
};



/* local variables */
static service **ServicePtrs = NULL;
static int iNumServices = 0;
static int iServiceIndex = 0;
static BOOL bInGlobalSection = True;
static BOOL bGlobalOnly = False;
static int default_server_announce;

#define NUMPARAMETERS (sizeof(parm_table) / sizeof(struct parm_struct))

/* prototypes for the special type handlers */
static BOOL handle_valid_chars(char *pszParmValue, char **ptr);
static BOOL handle_include(char *pszParmValue, char **ptr);
static BOOL handle_copy(char *pszParmValue, char **ptr);
static BOOL handle_character_set(char *pszParmValue,char **ptr);
static BOOL handle_coding_system(char *pszParmValue,char **ptr);

static void set_default_server_announce_type(void);

struct enum_list {
	int value;
	char *name;
};

static struct enum_list enum_protocol[] = {{PROTOCOL_NT1, "NT1"}, {PROTOCOL_LANMAN2, "LANMAN2"}, 
					   {PROTOCOL_LANMAN1, "LANMAN1"}, {PROTOCOL_CORE,"CORE"}, 
					   {PROTOCOL_COREPLUS, "COREPLUS"}, 
					   {PROTOCOL_COREPLUS, "CORE+"}, {-1, NULL}};

static struct enum_list enum_security[] = {{SEC_SHARE, "SHARE"},  {SEC_USER, "USER"}, 
					   {SEC_SERVER, "SERVER"}, {-1, NULL}};

static struct enum_list enum_announce_as[] = {{ANNOUNCE_AS_NT, "NT"}, {ANNOUNCE_AS_WIN95, "win95"},
					      {ANNOUNCE_AS_WFW, "WfW"}, {-1, NULL}};

static struct enum_list enum_case[] = {{CASE_LOWER, "lower"}, {CASE_UPPER, "upper"}, {-1, NULL}};

static struct enum_list enum_lm_announce[] = {{0, "False"}, {1, "True"}, {2, "Auto"}, {-1, NULL}};

static struct parm_struct
{
	char *label;
	parm_type type;
	parm_class class;
	void *ptr;
	BOOL (*special)();
	struct enum_list *enum_list;
} parm_table[] =
{
  {"debuglevel",       P_INTEGER, P_GLOBAL, &DEBUGLEVEL,                NULL,   NULL},
  {"log level",        P_INTEGER, P_GLOBAL, &DEBUGLEVEL,                NULL,   NULL},
  {"syslog",           P_INTEGER, P_GLOBAL, &Globals.syslog,            NULL,   NULL},
  {"syslog only",      P_BOOL,    P_GLOBAL, &Globals.bSyslogOnly,       NULL,   NULL},
  {"protocol",         P_ENUM,    P_GLOBAL, &Globals.maxprotocol,       NULL,   enum_protocol},
  {"security",         P_ENUM,    P_GLOBAL, &Globals.security,          NULL,   enum_security},
  {"max disk size",    P_INTEGER, P_GLOBAL, &Globals.maxdisksize,       NULL,   NULL},
  {"lpq cache time",   P_INTEGER, P_GLOBAL, &Globals.lpqcachetime,      NULL,   NULL},
  {"announce as",      P_ENUM,    P_GLOBAL, &Globals.announce_as,       NULL,   enum_announce_as},
  {"encrypt passwords",P_BOOL,    P_GLOBAL, &Globals.bEncryptPasswords, NULL,   NULL},
  {"getwd cache",      P_BOOL,    P_GLOBAL, &use_getwd_cache,           NULL,   NULL},
//  {"read prediction",  P_BOOL,    P_GLOBAL, &Globals.bReadPrediction,   NULL,   NULL},
  {"read bmpx",        P_BOOL,    P_GLOBAL, &Globals.bReadbmpx,         NULL,   NULL},
  {"read raw",         P_BOOL,    P_GLOBAL, &Globals.bReadRaw,          NULL,   NULL},
  {"write raw",        P_BOOL,    P_GLOBAL, &Globals.bWriteRaw,         NULL,   NULL},
  {"use rhosts",       P_BOOL,    P_GLOBAL, &Globals.bUseRhosts,        NULL,   NULL},
  {"load printers",    P_BOOL,    P_GLOBAL, &Globals.bLoadPrinters,     NULL,   NULL},
  {"null passwords",   P_BOOL,    P_GLOBAL, &Globals.bNullPasswords,    NULL,   NULL},
  {"strip dot",        P_BOOL,    P_GLOBAL, &Globals.bStripDot,         NULL,   NULL},
  {"interfaces",       P_STRING,  P_GLOBAL, &Globals.szInterfaces,      NULL,   NULL},
  {"bind interfaces only", P_BOOL,P_GLOBAL, &Globals.bBindInterfacesOnly,NULL,   NULL},
  {"password server",  P_STRING,  P_GLOBAL, &Globals.szPasswordServer,  NULL,   NULL},
  {"socket options",   P_GSTRING, P_GLOBAL, user_socket_options,        NULL,   NULL},
  {"netbios name",     P_UGSTRING,P_GLOBAL, myname,                     NULL,   NULL},
  {"netbios aliases",  P_STRING,  P_GLOBAL, &Globals.szNetbiosAliases,  NULL,   NULL},
//  {"smbrun",           P_STRING,  P_GLOBAL, &Globals.szSmbrun,          NULL,   NULL},
  {"log file",         P_STRING,  P_GLOBAL, &Globals.szLogFile,         NULL,   NULL},
  {"config file",      P_STRING,  P_GLOBAL, &Globals.szConfigFile,      NULL,   NULL},
  {"smb passwd file",  P_STRING,  P_GLOBAL, &Globals.szSMBPasswdFile,   NULL,   NULL},
  {"hosts equiv",      P_STRING,  P_GLOBAL, &Globals.szHostsEquiv,      NULL,   NULL},
  {"preload",          P_STRING,  P_GLOBAL, &Globals.szAutoServices,    NULL,   NULL},
  {"auto services",    P_STRING,  P_GLOBAL, &Globals.szAutoServices,    NULL,   NULL},
  {"server string",    P_STRING,  P_GLOBAL, &Globals.szServerString,    NULL,   NULL},
//  {"printcap name",    P_STRING,  P_GLOBAL, &Globals.szPrintcapname,    NULL,   NULL},
//  {"printcap",         P_STRING,  P_GLOBAL, &Globals.szPrintcapname,    NULL,   NULL},
  {"lock dir",         P_STRING,  P_GLOBAL, &Globals.szLockDir,         NULL,   NULL},
  {"lock directory",   P_STRING,  P_GLOBAL, &Globals.szLockDir,         NULL,   NULL},
  {"root directory",   P_STRING,  P_GLOBAL, &Globals.szRootdir,         NULL,   NULL},
  {"root dir",         P_STRING,  P_GLOBAL, &Globals.szRootdir,         NULL,   NULL},
  {"root",             P_STRING,  P_GLOBAL, &Globals.szRootdir,         NULL,   NULL},
  {"default service",  P_STRING,  P_GLOBAL, &Globals.szDefaultService,  NULL,   NULL},
  {"default",          P_STRING,  P_GLOBAL, &Globals.szDefaultService,  NULL,   NULL},
//  {"message command",  P_STRING,  P_GLOBAL, &Globals.szMsgCommand,      NULL,   NULL},
//  {"dfree command",    P_STRING,  P_GLOBAL, &Globals.szDfree,           NULL,   NULL},
  {"passwd program",   P_STRING,  P_GLOBAL, &Globals.szPasswdProgram,   NULL,   NULL},
  {"passwd chat",      P_STRING,  P_GLOBAL, &Globals.szPasswdChat,      NULL,   NULL},
  {"valid chars",      P_STRING,  P_GLOBAL, &Globals.szValidChars,      handle_valid_chars, NULL},
  {"workgroup",        P_USTRING, P_GLOBAL, &Globals.szWorkGroup,       NULL,   NULL},
#ifdef NTDOMAIN
  {"domain sid",       P_USTRING, P_GLOBAL, &Globals.szDomainSID,       NULL,   NULL},
  {"domain other sids",P_STRING,  P_GLOBAL, &Globals.szDomainOtherSIDs, NULL,   NULL},
  {"domain groups",    P_STRING,  P_GLOBAL, &Globals.szDomainGroups,    NULL,   NULL},
  {"domain controller",P_BOOL  ,  P_GLOBAL, &Globals.bDomainController,NULL,   NULL},
  {"domain admin users",P_STRING, P_GLOBAL, &Globals.szDomainAdminUsers, NULL,   NULL},
  {"domain guest users",P_STRING, P_GLOBAL, &Globals.szDomainGuestUsers, NULL,   NULL},
  {"domain hosts allow",P_STRING, P_GLOBAL, &Globals.szDomainHostsallow, NULL,   NULL},
  {"domain allow hosts",P_STRING, P_GLOBAL, &Globals.szDomainHostsallow, NULL,   NULL},
  {"domain hosts deny", P_STRING, P_GLOBAL, &Globals.szDomainHostsdeny,  NULL,   NULL},
  {"domain deny hosts", P_STRING, P_GLOBAL, &Globals.szDomainHostsdeny,  NULL,   NULL},
#endif /* NTDOMAIN */
  {"username map",     P_STRING,  P_GLOBAL, &Globals.szUsernameMap,     NULL,   NULL},
  {"character set",    P_STRING,  P_GLOBAL, &Globals.szCharacterSet,    handle_character_set, NULL},
  {"logon script",     P_STRING,  P_GLOBAL, &Globals.szLogonScript,     NULL,   NULL},
  {"logon path",       P_STRING,  P_GLOBAL, &Globals.szLogonPath,       NULL,   NULL},
  {"logon drive",      P_STRING,  P_GLOBAL, &Globals.szLogonDrive,      NULL,   NULL},
  {"logon home",       P_STRING,  P_GLOBAL, &Globals.szLogonHome,       NULL,   NULL},
  {"remote announce",  P_STRING,  P_GLOBAL, &Globals.szRemoteAnnounce,  NULL,   NULL},
  {"remote browse sync",P_STRING, P_GLOBAL, &Globals.szRemoteBrowseSync,NULL,   NULL},
  {"socket address",   P_STRING,  P_GLOBAL, &Globals.szSocketAddress,   NULL,   NULL},
  {"homedir map",      P_STRING,  P_GLOBAL, &Globals.szNISHomeMapName,  NULL,   NULL},
  {"announce version", P_STRING,  P_GLOBAL, &Globals.szAnnounceVersion, NULL,   NULL},
  {"max log size",     P_INTEGER, P_GLOBAL, &Globals.max_log_size,      NULL,   NULL},
  {"mangled stack",    P_INTEGER, P_GLOBAL, &Globals.mangled_stack,     NULL,   NULL},
  {"max mux",          P_INTEGER, P_GLOBAL, &Globals.max_mux,           NULL,   NULL},
  {"max xmit",         P_INTEGER, P_GLOBAL, &Globals.max_xmit,          NULL,   NULL},
  {"max packet",       P_INTEGER, P_GLOBAL, &Globals.max_packet,        NULL,   NULL},
  {"packet size",      P_INTEGER, P_GLOBAL, &Globals.max_packet,        NULL,   NULL},
  {"password level",   P_INTEGER, P_GLOBAL, &Globals.pwordlevel,        NULL,   NULL},
  {"username level",   P_INTEGER, P_GLOBAL, &Globals.unamelevel,        NULL,   NULL},
  {"keepalive",        P_INTEGER, P_GLOBAL, &keepalive,                 NULL,   NULL},
  {"deadtime",         P_INTEGER, P_GLOBAL, &Globals.deadtime,          NULL,   NULL},
  {"time offset",      P_INTEGER, P_GLOBAL, &extra_time_offset,         NULL,   NULL},
  {"read size",        P_INTEGER, P_GLOBAL, &Globals.ReadSize,          NULL,   NULL},
  {"shared mem size",  P_INTEGER, P_GLOBAL, &Globals.shmem_size,        NULL,   NULL},
  {"coding system",    P_STRING,  P_GLOBAL, &Globals.szCodingSystem,    handle_coding_system, NULL},
//  {"client code page", P_INTEGER, P_GLOBAL, &Globals.client_code_page,	NULL,   NULL},
  {"os level",         P_INTEGER, P_GLOBAL, &Globals.os_level,          NULL,   NULL},
  {"max ttl",          P_INTEGER, P_GLOBAL, &Globals.max_ttl,           NULL,   NULL},
  {"max wins ttl",     P_INTEGER, P_GLOBAL, &Globals.max_wins_ttl,      NULL,   NULL},
  {"min wins ttl",     P_INTEGER, P_GLOBAL, &Globals.min_wins_ttl,      NULL,   NULL},
  {"lm announce",      P_ENUM,    P_GLOBAL, &Globals.lm_announce,       NULL,   enum_lm_announce},
  {"lm interval",      P_INTEGER, P_GLOBAL, &Globals.lm_interval,       NULL,   NULL},
  {"dns proxy",        P_BOOL,    P_GLOBAL, &Globals.bDNSproxy,         NULL,   NULL},
  {"wins support",     P_BOOL,    P_GLOBAL, &Globals.bWINSsupport,      NULL,   NULL},
  {"wins proxy",       P_BOOL,    P_GLOBAL, &Globals.bWINSproxy,        NULL,   NULL},
  {"wins server",      P_STRING,  P_GLOBAL, &Globals.szWINSserver,      NULL,   NULL},
  {"preferred master", P_BOOL,    P_GLOBAL, &Globals.bPreferredMaster,  NULL,   NULL},
  {"prefered master",  P_BOOL,    P_GLOBAL, &Globals.bPreferredMaster,  NULL,   NULL},
  {"local master",     P_BOOL,    P_GLOBAL, &Globals.bLocalMaster,      NULL,   NULL},
  {"domain master",    P_BOOL,    P_GLOBAL, &Globals.bDomainMaster,     NULL,   NULL},
  {"domain logons",    P_BOOL,    P_GLOBAL, &Globals.bDomainLogons,     NULL,   NULL},
  {"browse list",      P_BOOL,    P_GLOBAL, &Globals.bBrowseList,       NULL,   NULL},
  {"unix realname",    P_BOOL,    P_GLOBAL, &Globals.bUnixRealname,     NULL,   NULL},
  {"NIS homedir",      P_BOOL,    P_GLOBAL, &Globals.bNISHomeMap,       NULL,   NULL},
  {"time server",      P_BOOL,    P_GLOBAL, &Globals.bTimeServer,	NULL,   NULL},
//  {"printer driver file", P_STRING,  P_GLOBAL, &Globals.szDriverFile,   NULL,   NULL},
  {"-valid",           P_BOOL,    P_LOCAL,  &sDefault.valid,            NULL,   NULL},
  {"comment",          P_STRING,  P_LOCAL,  &sDefault.comment,          NULL,   NULL},
  {"copy",             P_STRING,  P_LOCAL,  &sDefault.szCopy,           handle_copy, NULL},
  {"include",          P_STRING,  P_LOCAL,  &sDefault.szInclude,        handle_include, NULL},
  {"exec",             P_STRING,  P_LOCAL,  &sDefault.szPreExec,        NULL,   NULL},
  {"preexec",          P_STRING,  P_LOCAL,  &sDefault.szPreExec,        NULL,   NULL},
//  {"postexec",         P_STRING,  P_LOCAL,  &sDefault.szPostExec,       NULL,   NULL},
//  {"root preexec",     P_STRING,  P_LOCAL,  &sDefault.szRootPreExec,    NULL,   NULL},
//  {"root postexec",    P_STRING,  P_LOCAL,  &sDefault.szRootPostExec,   NULL,   NULL},
  {"alternate permissions",P_BOOL,P_LOCAL,  &sDefault.bAlternatePerm,   NULL,   NULL},
  {"revalidate",       P_BOOL,    P_LOCAL,  &sDefault.bRevalidate,      NULL,   NULL},
  {"default case",     P_ENUM, P_LOCAL,  &sDefault.iDefaultCase,        NULL,   enum_case},
  {"case sensitive",   P_BOOL,    P_LOCAL,  &sDefault.bCaseSensitive,   NULL,   NULL},
  {"casesignames",     P_BOOL,    P_LOCAL,  &sDefault.bCaseSensitive,   NULL,   NULL},
  {"preserve case",    P_BOOL,    P_LOCAL,  &sDefault.bCasePreserve,    NULL,   NULL},
  {"short preserve case",P_BOOL,  P_LOCAL,  &sDefault.bShortCasePreserve,NULL,   NULL},
  {"mangle case",      P_BOOL,    P_LOCAL,  &sDefault.bCaseMangle,      NULL,   NULL},
  {"mangling char",    P_CHAR,    P_LOCAL,  &sDefault.magic_char,       NULL,   NULL},
  {"browseable",       P_BOOL,    P_LOCAL,  &sDefault.bBrowseable,      NULL,   NULL},
  {"browsable",        P_BOOL,    P_LOCAL,  &sDefault.bBrowseable,      NULL,   NULL},
  {"available",        P_BOOL,    P_LOCAL,  &sDefault.bAvailable,       NULL,   NULL},
  {"path",             P_STRING,  P_LOCAL,  &sDefault.szPath,           NULL,   NULL},
  {"directory",        P_STRING,  P_LOCAL,  &sDefault.szPath,           NULL,   NULL},
  {"username",         P_STRING,  P_LOCAL,  &sDefault.szUsername,       NULL,   NULL},
  {"user",             P_STRING,  P_LOCAL,  &sDefault.szUsername,       NULL,   NULL},
  {"users",            P_STRING,  P_LOCAL,  &sDefault.szUsername,       NULL,   NULL},
  {"guest account",    P_STRING,  P_LOCAL,  &sDefault.szGuestaccount,   NULL,   NULL},
  {"invalid users",    P_STRING,  P_LOCAL,  &sDefault.szInvalidUsers,   NULL,   NULL},
  {"valid users",      P_STRING,  P_LOCAL,  &sDefault.szValidUsers,     NULL,   NULL},
  {"admin users",      P_STRING,  P_LOCAL,  &sDefault.szAdminUsers,     NULL,   NULL},
  {"read list",        P_STRING,  P_LOCAL,  &sDefault.readlist,         NULL,   NULL},
  {"write list",       P_STRING,  P_LOCAL,  &sDefault.writelist,        NULL,   NULL},
  {"volume",           P_STRING,  P_LOCAL,  &sDefault.volume,           NULL,   NULL},
  {"force user",       P_STRING,  P_LOCAL,  &sDefault.force_user,       NULL,   NULL},
  {"force group",      P_STRING,  P_LOCAL,  &sDefault.force_group,      NULL,   NULL},
  {"group",            P_STRING,  P_LOCAL,  &sDefault.force_group,      NULL,   NULL},
  {"read only",        P_BOOL,    P_LOCAL,  &sDefault.bRead_only,       NULL,   NULL},
  {"write ok",         P_BOOLREV, P_LOCAL,  &sDefault.bRead_only,       NULL,   NULL},
  {"writeable",        P_BOOLREV, P_LOCAL,  &sDefault.bRead_only,       NULL,   NULL},
  {"writable",         P_BOOLREV, P_LOCAL,  &sDefault.bRead_only,       NULL,   NULL},
  {"max connections",  P_INTEGER, P_LOCAL,  &sDefault.iMaxConnections,  NULL,   NULL},
//  {"min print space",  P_INTEGER, P_LOCAL,  &sDefault.iMinPrintSpace,   NULL,   NULL},
  {"create mask",      P_OCTAL,   P_LOCAL,  &sDefault.iCreate_mask,     NULL,   NULL},
  {"create mode",      P_OCTAL,   P_LOCAL,  &sDefault.iCreate_mask,     NULL,   NULL},
  {"force create mode",P_OCTAL,   P_LOCAL,  &sDefault.iCreate_force_mode,     NULL,   NULL},
  {"directory mask",   P_OCTAL,   P_LOCAL,  &sDefault.iDir_mask,        NULL,   NULL},
  {"directory mode",   P_OCTAL,   P_LOCAL,  &sDefault.iDir_mask,        NULL,   NULL},
  {"force directory mode",   P_OCTAL,   P_LOCAL,  &sDefault.iDir_force_mode,        NULL,   NULL},
  {"set directory",    P_BOOLREV, P_LOCAL,  &sDefault.bNo_set_dir,      NULL,   NULL},
  {"status",           P_BOOL,    P_LOCAL,  &sDefault.status,           NULL,   NULL},
  {"hide dot files",   P_BOOL,    P_LOCAL,  &sDefault.bHideDotFiles,    NULL,   NULL},
  {"delete veto files",P_BOOL,    P_LOCAL,  &sDefault.bDeleteVetoFiles, NULL,   NULL},
  {"veto files",       P_STRING,  P_LOCAL,  &sDefault.szVetoFiles,      NULL,   NULL},
  {"hide files",       P_STRING,  P_LOCAL,  &sDefault.szHideFiles,      NULL,   NULL},
  {"veto oplock files",P_STRING,  P_LOCAL,  &sDefault.szVetoOplockFiles,NULL,   NULL},
  {"guest only",       P_BOOL,    P_LOCAL,  &sDefault.bGuest_only,      NULL,   NULL},
  {"only guest",       P_BOOL,    P_LOCAL,  &sDefault.bGuest_only,      NULL,   NULL},
  {"guest ok",         P_BOOL,    P_LOCAL,  &sDefault.bGuest_ok,        NULL,   NULL},
  {"public",           P_BOOL,    P_LOCAL,  &sDefault.bGuest_ok,        NULL,   NULL},
//  {"print ok",         P_BOOL,    P_LOCAL,  &sDefault.bPrint_ok,        NULL,   NULL},
//  {"printable",        P_BOOL,    P_LOCAL,  &sDefault.bPrint_ok,        NULL,   NULL},
  {"postscript",       P_BOOL,    P_LOCAL,  &sDefault.bPostscript,      NULL,   NULL},
  {"map system",       P_BOOL,    P_LOCAL,  &sDefault.bMap_system,      NULL,   NULL},
  {"map hidden",       P_BOOL,    P_LOCAL,  &sDefault.bMap_hidden,      NULL,   NULL},
  {"map archive",      P_BOOL,    P_LOCAL,  &sDefault.bMap_archive,     NULL,   NULL},
  {"locking",          P_BOOL,    P_LOCAL,  &sDefault.bLocking,         NULL,   NULL},
  {"strict locking",   P_BOOL,    P_LOCAL,  &sDefault.bStrictLocking,   NULL,   NULL},
  {"share modes",      P_BOOL,    P_LOCAL,  &sDefault.bShareModes,      NULL,   NULL},
  {"oplocks",          P_BOOL,    P_LOCAL,  &sDefault.bOpLocks,         NULL,   NULL},
  {"only user",        P_BOOL,    P_LOCAL,  &sDefault.bOnlyUser,        NULL,   NULL},
//  {"wide links",       P_BOOL,    P_LOCAL,  &sDefault.bWidelinks,       NULL,   NULL},
//  {"follow symlinks",  P_BOOL,    P_LOCAL,  &sDefault.bSymlinks,        NULL,   NULL},
//  {"sync always",      P_BOOL,    P_LOCAL,  &sDefault.bSyncAlways,      NULL,   NULL},
  {"mangled names",    P_BOOL,    P_LOCAL,  &sDefault.bMangledNames,    NULL,   NULL},
  {"fake oplocks",     P_BOOL,    P_LOCAL,  &sDefault.bFakeOplocks,     NULL,   NULL},
//  {"printing",         P_ENUM,    P_LOCAL,  &sDefault.iPrinting,        NULL,   enum_printing},
//  {"print command",    P_STRING,  P_LOCAL,  &sDefault.szPrintcommand,   NULL,   NULL},
  {"lpq command",      P_STRING,  P_LOCAL,  &sDefault.szLpqcommand,     NULL,   NULL},
  {"lprm command",     P_STRING,  P_LOCAL,  &sDefault.szLprmcommand,    NULL,   NULL},
  {"lppause command",  P_STRING,  P_LOCAL,  &sDefault.szLppausecommand, NULL,   NULL},
  {"lpresume command", P_STRING,  P_LOCAL,  &sDefault.szLpresumecommand,NULL,   NULL},
//  {"printer",          P_STRING,  P_LOCAL,  &sDefault.szPrintername,    NULL,   NULL},
//  {"printer name",     P_STRING,  P_LOCAL,  &sDefault.szPrintername,    NULL,   NULL},
//  {"printer driver",   P_STRING,  P_LOCAL,  &sDefault.szPrinterDriver,  NULL,   NULL},
//  {"printer driver location",   P_STRING,  P_LOCAL,  &sDefault.szPrinterDriverLocation,  NULL,   NULL},
  {"hosts allow",      P_STRING,  P_LOCAL,  &sDefault.szHostsallow,     NULL,   NULL},
  {"allow hosts",      P_STRING,  P_LOCAL,  &sDefault.szHostsallow,     NULL,   NULL},
  {"hosts deny",       P_STRING,  P_LOCAL,  &sDefault.szHostsdeny,      NULL,   NULL},
  {"deny hosts",       P_STRING,  P_LOCAL,  &sDefault.szHostsdeny,      NULL,   NULL},
  {"dont descend",     P_STRING,  P_LOCAL,  &sDefault.szDontdescend,    NULL,   NULL},
//  {"magic script",     P_STRING,  P_LOCAL,  &sDefault.szMagicScript,    NULL,   NULL},
//  {"magic output",     P_STRING,  P_LOCAL,  &sDefault.szMagicOutput,    NULL,   NULL},
  {"mangled map",      P_STRING,  P_LOCAL,  &sDefault.szMangledMap,     NULL,   NULL},
  {"delete readonly",  P_BOOL,    P_LOCAL,  &sDefault.bDeleteReadonly,  NULL,   NULL},
  {"dos filetimes",    P_BOOL,    P_LOCAL,  &sDefault.bDosFiletimes,    NULL,   NULL},

  {NULL,               P_BOOL,    P_NONE,   NULL,                       NULL,   NULL}
};



/***************************************************************************
Initialise the global parameter structure.
***************************************************************************/
static void init_globals(void)
{
  static BOOL done_init = False;
  pstring s;

  if (!done_init)
    {
      int i;
      bzero((void *)&Globals,sizeof(Globals));

      for (i = 0; parm_table[i].label; i++) 
	if ((parm_table[i].type == P_STRING ||
	     parm_table[i].type == P_USTRING) && 
	    parm_table[i].ptr)
	  string_init(parm_table[i].ptr,"");

      string_set(&sDefault.szGuestaccount, GUEST_ACCOUNT);
//      string_set(&sDefault.szPrinterDriver, "NULL");

      done_init = True;
    }


  DEBUG(3,("Initialising global parameters\n"));

#ifdef SMB_PASSWD_FILE
  string_set(&Globals.szSMBPasswdFile, SMB_PASSWD_FILE);
#endif
  string_set(&Globals.szPasswdChat,"*old*password* %o\\n *new*password* %n\\n *new*password* %n\\n *changed*");
  string_set(&Globals.szWorkGroup, WORKGROUP);
  string_set(&Globals.szPasswdProgram, SMB_PASSWD);
//  string_set(&Globals.szPrintcapname, PRINTCAP_NAME);
//  string_set(&Globals.szDriverFile, DRIVERFILE);
  string_set(&Globals.szLockDir, LOCKDIR);
//#########################################################
//##DO NOT EDIT THE FOLLOWING LINE UNLESS YOU KNOW WHAT YOU ARE DOING!###
//  string_set(&Globals.szDefaultService, "volume1");
//#########################################################
  string_set(&Globals.szRootdir, "/");
//  string_set(&Globals.szSmbrun, SMBRUN);
  string_set(&Globals.szSocketAddress, "0.0.0.0");
  sprintf(s,"SMB %s",VERSION);
  string_set(&Globals.szServerString,s);
  sprintf(s,"%d.%d", DEFAULT_MAJOR_VERSION, DEFAULT_MINOR_VERSION);
  string_set(&Globals.szAnnounceVersion,s);

  string_set(&Globals.szLogonDrive, "");
  /* %N is the NIS auto.home server if -DAUTOHOME is used, else same as %L */
  string_set(&Globals.szLogonHome, "\\\\%N\\%U");
  string_set(&Globals.szLogonPath, "\\\\%N\\%U\\profile");

  Globals.bLoadPrinters = False;
  Globals.bUseRhosts = False;
  Globals.max_packet = 0x4104;
  Globals.mangled_stack = 50;
  Globals.max_xmit = 0x4104;
  Globals.max_mux = 50; /* This is *needed* for profile support. */
  Globals.lpqcachetime = 10;
  Globals.pwordlevel = 0;
  Globals.unamelevel = 0;
  Globals.deadtime = 0;
  Globals.max_log_size = 5000;
  Globals.maxprotocol = PROTOCOL_NT1;
  Globals.minprotocol = PROTOCOL_CORE;
  Globals.security = SEC_USER;
  Globals.bEncryptPasswords = True;
  Globals.bReadRaw = True;
  Globals.bWriteRaw = True;
//  Globals.bReadPrediction = False;
  Globals.bReadbmpx = True;
  Globals.bNullPasswords = False;
  Globals.bStripDot = False;
  Globals.syslog = 1;
  Globals.bSyslogOnly = False;
  Globals.os_level = 0;
  Globals.max_ttl = 60*60*4; /* 4 hours default */
  Globals.max_wins_ttl = 60*60*24*3; /* 3 days default */
  Globals.min_wins_ttl = 60*60*6; /* 6 hours default */
  Globals.ReadSize = 65536;
  Globals.lm_announce = 2;   /* = Auto: send only if LM clients found */
  Globals.lm_interval = 60;
  Globals.shmem_size = SHMEM_SIZE;
  Globals.announce_as = ANNOUNCE_AS_NT;
  Globals.bUnixRealname = False;
  
#if (defined(NETGROUP) && defined(AUTOMOUNT))
  Globals.bNISHomeMap = False;
  string_set(&Globals.szNISHomeMapName, "auto.home");
#endif

  
//  Globals.client_code_page = DEFAULT_CLIENT_CODE_PAGE;
  Globals.bTimeServer = False;
  Globals.bBindInterfacesOnly = False;

/* these parameters are set to defaults that are more appropriate
   for the increasing samba install base:

   as a member of the workgroup, that will possibly become a
   _local_ master browser (lm = True).  this is opposed to a forced
   local master browser startup (pm = True).

   doesn't provide WINS server service by default (wsupp = False),
   and doesn't provide domain master browser services by default, either.

*/

  Globals.bPreferredMaster = False;
  Globals.bLocalMaster = True;
  Globals.bDomainMaster = False;
  Globals.bDomainLogons = False;
  Globals.bBrowseList = True;
  Globals.bWINSsupport = False;
  Globals.bWINSproxy = False;

  Globals.bDNSproxy = True;

//  interpret_coding_system("utf8");
}

/******************************************************************* a
convenience routine to grab string parameters into a rotating buffer,
and run standard_sub_basic on them. The buffers can be written to by
callers without affecting the source string.
********************************************************************/
char *lp_string(char *s)
{
  static char *bufs[10];
  static int buflen[10];
  static int next = -1;  
  char *ret;
  int i;
  int len = s?strlen(s):0;

  if (next == -1) {
    /* initialisation */
    for (i=0;i<10;i++) {
      bufs[i] = NULL;
      buflen[i] = 0;
    }
    next = 0;
  }

  len = MAX(len+100,sizeof(pstring)); /* the +100 is for some
					 substitution room */

  if (buflen[next] != len) {
    buflen[next] = len;
    if (bufs[next]) free(bufs[next]);
    bufs[next] = (char *)malloc(len);
    if (!bufs[next]) {
      DEBUG(0,("out of memory in lp_string()"));
      exit(1);
    }
  } 

  ret = &bufs[next][0];
  next = (next+1)%10;

  if (!s) 
    *ret = 0;
  else
    StrCpy(ret,s);

  trim_string(ret, "\"", "\"");

  standard_sub_basic(ret);
  return(ret);
}


/*
   In this section all the functions that are used to access the 
   parameters from the rest of the program are defined 
*/

#define FN_GLOBAL_STRING(fn_name,ptr) \
 char *fn_name(void) {return(lp_string(*(char **)(ptr) ? *(char **)(ptr) : ""));}
#define FN_GLOBAL_BOOL(fn_name,ptr) \
 BOOL fn_name(void) {return(*(BOOL *)(ptr));}
#define FN_GLOBAL_CHAR(fn_name,ptr) \
 char fn_name(void) {return(*(char *)(ptr));}
#define FN_GLOBAL_INTEGER(fn_name,ptr) \
 int fn_name(void) {return(*(int *)(ptr));}

#define FN_LOCAL_STRING(fn_name,val) \
 char *fn_name(int i) {return(lp_string((LP_SNUM_OK(i)&&pSERVICE(i)->val)?pSERVICE(i)->val : sDefault.val));}
#define FN_LOCAL_BOOL(fn_name,val) \
 BOOL fn_name(int i) {return(LP_SNUM_OK(i)? pSERVICE(i)->val : sDefault.val);}
#define FN_LOCAL_CHAR(fn_name,val) \
 char fn_name(int i) {return(LP_SNUM_OK(i)? pSERVICE(i)->val : sDefault.val);}
#define FN_LOCAL_INTEGER(fn_name,val) \
 int fn_name(int i) {return(LP_SNUM_OK(i)? pSERVICE(i)->val : sDefault.val);}

FN_GLOBAL_STRING(lp_logfile,&Globals.szLogFile)
//FN_GLOBAL_STRING(lp_smbrun,&Globals.szSmbrun)
FN_GLOBAL_STRING(lp_configfile,&Globals.szConfigFile)
FN_GLOBAL_STRING(lp_smb_passwd_file,&Globals.szSMBPasswdFile)
FN_GLOBAL_STRING(lp_serverstring,&Globals.szServerString)
//FN_GLOBAL_STRING(lp_printcapname,&Globals.szPrintcapname)
FN_GLOBAL_STRING(lp_lockdir,&Globals.szLockDir)
FN_GLOBAL_STRING(lp_rootdir,&Globals.szRootdir)
FN_GLOBAL_STRING(lp_defaultservice,&Globals.szDefaultService)
//FN_GLOBAL_STRING(lp_msg_command,&Globals.szMsgCommand)
//FN_GLOBAL_STRING(lp_dfree_command,&Globals.szDfree)
FN_GLOBAL_STRING(lp_hosts_equiv,&Globals.szHostsEquiv)
FN_GLOBAL_STRING(lp_auto_services,&Globals.szAutoServices)
FN_GLOBAL_STRING(lp_passwd_program,&Globals.szPasswdProgram)
FN_GLOBAL_STRING(lp_passwd_chat,&Globals.szPasswdChat)
FN_GLOBAL_STRING(lp_passwordserver,&Globals.szPasswordServer)
FN_GLOBAL_STRING(lp_workgroup,&Globals.szWorkGroup)
FN_GLOBAL_STRING(lp_username_map,&Globals.szUsernameMap)
FN_GLOBAL_STRING(lp_character_set,&Globals.szCharacterSet) 
FN_GLOBAL_STRING(lp_logon_script,&Globals.szLogonScript) 
FN_GLOBAL_STRING(lp_logon_path,&Globals.szLogonPath) 
FN_GLOBAL_STRING(lp_logon_drive,&Globals.szLogonDrive) 
FN_GLOBAL_STRING(lp_logon_home,&Globals.szLogonHome) 
FN_GLOBAL_STRING(lp_remote_announce,&Globals.szRemoteAnnounce) 
FN_GLOBAL_STRING(lp_remote_browse_sync,&Globals.szRemoteBrowseSync) 
FN_GLOBAL_STRING(lp_wins_server,&Globals.szWINSserver)
FN_GLOBAL_STRING(lp_interfaces,&Globals.szInterfaces)
FN_GLOBAL_STRING(lp_socket_address,&Globals.szSocketAddress)
FN_GLOBAL_STRING(lp_nis_home_map_name,&Globals.szNISHomeMapName)
FN_GLOBAL_STRING(lp_announce_version,&Globals.szAnnounceVersion)
FN_GLOBAL_STRING(lp_netbios_aliases,&Globals.szNetbiosAliases)
//FN_GLOBAL_STRING(lp_driverfile,&Globals.szDriverFile)

#ifdef NTDOMAIN
FN_GLOBAL_STRING(lp_domain_sid,&Globals.szDomainSID)
FN_GLOBAL_STRING(lp_domain_other_sids,&Globals.szDomainOtherSIDs)
FN_GLOBAL_STRING(lp_domain_groups,&Globals.szDomainGroups)
FN_GLOBAL_STRING(lp_domain_admin_users,&Globals.szDomainAdminUsers)
FN_GLOBAL_STRING(lp_domain_guest_users,&Globals.szDomainGuestUsers)
FN_GLOBAL_STRING(lp_domain_hostsallow,&Globals.szDomainHostsallow)
FN_GLOBAL_STRING(lp_domain_hostsdeny,&Globals.szDomainHostsdeny)
#endif /* NTDOMAIN */

FN_GLOBAL_BOOL(lp_dns_proxy,&Globals.bDNSproxy)
FN_GLOBAL_BOOL(lp_wins_support,&Globals.bWINSsupport)
FN_GLOBAL_BOOL(lp_we_are_a_wins_server,&Globals.bWINSsupport)
FN_GLOBAL_BOOL(lp_wins_proxy,&Globals.bWINSproxy)
FN_GLOBAL_BOOL(lp_local_master,&Globals.bLocalMaster)
FN_GLOBAL_BOOL(lp_domain_controller,&Globals.bDomainController)
FN_GLOBAL_BOOL(lp_domain_master,&Globals.bDomainMaster)
FN_GLOBAL_BOOL(lp_domain_logons,&Globals.bDomainLogons)
FN_GLOBAL_BOOL(lp_preferred_master,&Globals.bPreferredMaster)
FN_GLOBAL_BOOL(lp_load_printers,&Globals.bLoadPrinters)
FN_GLOBAL_BOOL(lp_use_rhosts,&Globals.bUseRhosts)
FN_GLOBAL_BOOL(lp_getwdcache,&use_getwd_cache)
//FN_GLOBAL_BOOL(lp_readprediction,&Globals.bReadPrediction)
FN_GLOBAL_BOOL(lp_readbmpx,&Globals.bReadbmpx)
FN_GLOBAL_BOOL(lp_readraw,&Globals.bReadRaw)
FN_GLOBAL_BOOL(lp_writeraw,&Globals.bWriteRaw)
FN_GLOBAL_BOOL(lp_null_passwords,&Globals.bNullPasswords)
FN_GLOBAL_BOOL(lp_strip_dot,&Globals.bStripDot)
FN_GLOBAL_BOOL(lp_encrypted_passwords,&Globals.bEncryptPasswords)
FN_GLOBAL_BOOL(lp_syslog_only,&Globals.bSyslogOnly)
FN_GLOBAL_BOOL(lp_browse_list,&Globals.bBrowseList)
FN_GLOBAL_BOOL(lp_unix_realname,&Globals.bUnixRealname)
FN_GLOBAL_BOOL(lp_nis_home_map,&Globals.bNISHomeMap)
FN_GLOBAL_BOOL(lp_time_server,&Globals.bTimeServer)
FN_GLOBAL_BOOL(lp_bind_interfaces_only,&Globals.bBindInterfacesOnly)

FN_GLOBAL_INTEGER(lp_os_level,&Globals.os_level)
FN_GLOBAL_INTEGER(lp_max_ttl,&Globals.max_ttl)
FN_GLOBAL_INTEGER(lp_max_wins_ttl,&Globals.max_wins_ttl)
FN_GLOBAL_INTEGER(lp_min_wins_ttl,&Globals.max_wins_ttl)
FN_GLOBAL_INTEGER(lp_max_log_size,&Globals.max_log_size)
FN_GLOBAL_INTEGER(lp_mangledstack,&Globals.mangled_stack)
FN_GLOBAL_INTEGER(lp_maxxmit,&Globals.max_xmit)
FN_GLOBAL_INTEGER(lp_maxmux,&Globals.max_mux)
FN_GLOBAL_INTEGER(lp_maxpacket,&Globals.max_packet)
FN_GLOBAL_INTEGER(lp_keepalive,&keepalive)
FN_GLOBAL_INTEGER(lp_passwordlevel,&Globals.pwordlevel)
FN_GLOBAL_INTEGER(lp_usernamelevel,&Globals.unamelevel)
FN_GLOBAL_INTEGER(lp_readsize,&Globals.ReadSize)
FN_GLOBAL_INTEGER(lp_shmem_size,&Globals.shmem_size)
FN_GLOBAL_INTEGER(lp_deadtime,&Globals.deadtime)
FN_GLOBAL_INTEGER(lp_maxprotocol,&Globals.maxprotocol)
FN_GLOBAL_INTEGER(lp_minprotocol, &Globals.minprotocol)
FN_GLOBAL_INTEGER(lp_security,&Globals.security)
FN_GLOBAL_INTEGER(lp_maxdisksize,&Globals.maxdisksize)
FN_GLOBAL_INTEGER(lp_lpqcachetime,&Globals.lpqcachetime)
FN_GLOBAL_INTEGER(lp_syslog,&Globals.syslog)
//FN_GLOBAL_INTEGER(lp_client_code_page,&Globals.client_code_page)
FN_GLOBAL_INTEGER(lp_announce_as,&Globals.announce_as)
FN_GLOBAL_INTEGER(lp_lm_announce,&Globals.lm_announce)
FN_GLOBAL_INTEGER(lp_lm_interval,&Globals.lm_interval)

FN_LOCAL_STRING(lp_preexec,szPreExec)
//FN_LOCAL_STRING(lp_postexec,szPostExec)
//FN_LOCAL_STRING(lp_rootpreexec,szRootPreExec)
//FN_LOCAL_STRING(lp_rootpostexec,szRootPostExec)
FN_LOCAL_STRING(lp_servicename,szService)
FN_LOCAL_STRING(lp_pathname,szPath)
FN_LOCAL_STRING(lp_dontdescend,szDontdescend)
FN_LOCAL_STRING(lp_username,szUsername)
FN_LOCAL_STRING(lp_guestaccount,szGuestaccount)
FN_LOCAL_STRING(lp_invalid_users,szInvalidUsers)
FN_LOCAL_STRING(lp_valid_users,szValidUsers)
FN_LOCAL_STRING(lp_admin_users,szAdminUsers)
//FN_LOCAL_STRING(lp_printcommand,szPrintcommand)
FN_LOCAL_STRING(lp_lpqcommand,szLpqcommand)
FN_LOCAL_STRING(lp_lprmcommand,szLprmcommand)
FN_LOCAL_STRING(lp_lppausecommand,szLppausecommand)
FN_LOCAL_STRING(lp_lpresumecommand,szLpresumecommand)
//FN_LOCAL_STRING(lp_printername,szPrintername)
//FN_LOCAL_STRING(lp_printerdriver,szPrinterDriver)
FN_LOCAL_STRING(lp_hostsallow,szHostsallow)
FN_LOCAL_STRING(lp_hostsdeny,szHostsdeny)
//FN_LOCAL_STRING(lp_magicscript,szMagicScript)
//FN_LOCAL_STRING(lp_magicoutput,szMagicOutput)
FN_LOCAL_STRING(lp_comment,comment)
FN_LOCAL_STRING(lp_force_user,force_user)
FN_LOCAL_STRING(lp_force_group,force_group)
FN_LOCAL_STRING(lp_readlist,readlist)
FN_LOCAL_STRING(lp_writelist,writelist)
FN_LOCAL_STRING(lp_volume,volume)
FN_LOCAL_STRING(lp_mangled_map,szMangledMap)
FN_LOCAL_STRING(lp_veto_files,szVetoFiles)
FN_LOCAL_STRING(lp_hide_files,szHideFiles)
FN_LOCAL_STRING(lp_veto_oplocks,szVetoOplockFiles)
//FN_LOCAL_STRING(lp_driverlocation,szPrinterDriverLocation)

FN_LOCAL_BOOL(lp_alternate_permissions,bAlternatePerm)
FN_LOCAL_BOOL(lp_revalidate,bRevalidate)
FN_LOCAL_BOOL(lp_casesensitive,bCaseSensitive)
FN_LOCAL_BOOL(lp_preservecase,bCasePreserve)
FN_LOCAL_BOOL(lp_shortpreservecase,bShortCasePreserve)
FN_LOCAL_BOOL(lp_casemangle,bCaseMangle)
FN_LOCAL_BOOL(lp_status,status)
FN_LOCAL_BOOL(lp_hide_dot_files,bHideDotFiles)
FN_LOCAL_BOOL(lp_browseable,bBrowseable)
FN_LOCAL_BOOL(lp_readonly,bRead_only)
FN_LOCAL_BOOL(lp_no_set_dir,bNo_set_dir)
FN_LOCAL_BOOL(lp_guest_ok,bGuest_ok)
FN_LOCAL_BOOL(lp_guest_only,bGuest_only)
//FN_LOCAL_BOOL(lp_print_ok,bPrint_ok)
FN_LOCAL_BOOL(lp_postscript,bPostscript)
FN_LOCAL_BOOL(lp_map_hidden,bMap_hidden)
FN_LOCAL_BOOL(lp_map_archive,bMap_archive)
FN_LOCAL_BOOL(lp_locking,bLocking)
FN_LOCAL_BOOL(lp_strict_locking,bStrictLocking)
FN_LOCAL_BOOL(lp_share_modes,bShareModes)
FN_LOCAL_BOOL(lp_oplocks,bOpLocks)
FN_LOCAL_BOOL(lp_onlyuser,bOnlyUser)
FN_LOCAL_BOOL(lp_manglednames,bMangledNames)
FN_LOCAL_BOOL(lp_widelinks,bWidelinks)
FN_LOCAL_BOOL(lp_symlinks,bSymlinks)
//FN_LOCAL_BOOL(lp_syncalways,bSyncAlways)
FN_LOCAL_BOOL(lp_map_system,bMap_system)
FN_LOCAL_BOOL(lp_delete_readonly,bDeleteReadonly)
FN_LOCAL_BOOL(lp_fake_oplocks,bFakeOplocks)
FN_LOCAL_BOOL(lp_recursive_veto_delete,bDeleteVetoFiles)
FN_LOCAL_BOOL(lp_dos_filetimes,bDosFiletimes)

FN_LOCAL_INTEGER(lp_create_mode,iCreate_mask)
FN_LOCAL_INTEGER(lp_force_create_mode,iCreate_force_mode)
FN_LOCAL_INTEGER(lp_dir_mode,iDir_mask)
FN_LOCAL_INTEGER(lp_force_dir_mode,iDir_force_mode)
FN_LOCAL_INTEGER(lp_max_connections,iMaxConnections)
FN_LOCAL_INTEGER(lp_defaultcase,iDefaultCase)
//FN_LOCAL_INTEGER(lp_minprintspace,iMinPrintSpace)
//FN_LOCAL_INTEGER(lp_printing,iPrinting)

FN_LOCAL_CHAR(lp_magicchar,magic_char)



/* local prototypes */
static int    strwicmp( char *psz1, char *psz2 );
static int    map_parameter( char *pszParmName);
static BOOL   set_boolean( BOOL *pb, char *pszParmValue );
static int    getservicebyname(char *pszServiceName, service *pserviceDest);
static void   copy_service( service *pserviceDest, 
                            service *pserviceSource,
                            BOOL *pcopymapDest );
static BOOL   service_ok(int iService);
static BOOL   do_parameter(char *pszParmName, char *pszParmValue);
static BOOL   do_section(char *pszSectionName);
static void init_copymap(service *pservice);

/***************************************************************************
initialise a service to the defaults
***************************************************************************/
static void init_service(service *pservice)
{
  bzero((char *)pservice,sizeof(service));
  copy_service(pservice,&sDefault,NULL);
}


/***************************************************************************
free the dynamically allocated parts of a service struct
***************************************************************************/
static void free_service(service *pservice)
{
  int i;
  if (!pservice)
     return;

  for (i=0;parm_table[i].label;i++)
    if ((parm_table[i].type == P_STRING ||
	 parm_table[i].type == P_STRING) &&
	parm_table[i].class == P_LOCAL)
      string_free((char **)(((char *)pservice) + PTR_DIFF(parm_table[i].ptr,&sDefault)));
}

/***************************************************************************
add a new service to the services array initialising it with the given 
service
***************************************************************************/
static int add_a_service(service *pservice, char *name)
{
  int i;
  service tservice;
  int num_to_alloc = iNumServices+1;

  tservice = *pservice;

  /* it might already exist */
  if (name) 
    {
      i = getservicebyname(name,NULL);
      if (i >= 0)
	return(i);
    }

  /* find an invalid one */
  for (i=0;i<iNumServices;i++)
    if (!pSERVICE(i)->valid)
      break;

  /* if not, then create one */
  if (i == iNumServices)
    {
      ServicePtrs = (service **)Realloc(ServicePtrs,sizeof(service *)*num_to_alloc);
      if (ServicePtrs)
	pSERVICE(iNumServices) = (service *)malloc(sizeof(service));

      if (!ServicePtrs || !pSERVICE(iNumServices))
	return(-1);

      iNumServices++;
    }
  else
    free_service(pSERVICE(i));

  pSERVICE(i)->valid = True;

  init_service(pSERVICE(i));
  copy_service(pSERVICE(i),&tservice,NULL);
  if (name)
    string_set(&iSERVICE(i).szService,name);  

  return(i);
}

/***************************************************************************
add a new home service, with the specified home directory, defaults coming 
from service ifrom
***************************************************************************/
BOOL lp_add_home(char *pszHomename, int iDefaultService, char *pszHomedir)
{
  int i = add_a_service(pSERVICE(iDefaultService),pszHomename);

  if (i < 0)
    return(False);

  if (!(*(iSERVICE(i).szPath)) || strequal(iSERVICE(i).szPath,lp_pathname(-1)))
    string_set(&iSERVICE(i).szPath,pszHomedir);
  if (!(*(iSERVICE(i).comment)))
    {
      pstring comment;
      sprintf(comment,"Home directory of %s",pszHomename);
      string_set(&iSERVICE(i).comment,comment);
    }
  iSERVICE(i).bAvailable = sDefault.bAvailable;
  iSERVICE(i).bBrowseable = sDefault.bBrowseable;

  DEBUG(3,("adding home directory %s at %s\n", pszHomename, pszHomedir));

  return(True);
}

/***************************************************************************
add a new service, based on an old one
***************************************************************************/
int lp_add_service(char *pszService, int iDefaultService)
{
  return(add_a_service(pSERVICE(iDefaultService),pszService));
}

/***************************************************************************
add the IPC service
***************************************************************************/
static BOOL lp_add_ipc(void)
{
  pstring comment;
  int i = add_a_service(&sDefault,"IPC$");

  if (i < 0)
    return(False);

  sprintf(comment,"IPC Service (%s)",lp_serverstring());

  string_set(&iSERVICE(i).szPath,tmpdir());
  string_set(&iSERVICE(i).szUsername,"");
  string_set(&iSERVICE(i).comment,comment);
  iSERVICE(i).status = False;
  iSERVICE(i).iMaxConnections = 0;
  iSERVICE(i).bAvailable = True;
  iSERVICE(i).bRead_only = True;
  iSERVICE(i).bGuest_only = False;
  iSERVICE(i).bGuest_ok = True;
  iSERVICE(i).bBrowseable = sDefault.bBrowseable;

  DEBUG(3,("adding IPC service\n"));

  return(True);
}


/***************************************************************************
Do a case-insensitive, whitespace-ignoring string compare.
***************************************************************************/
static int strwicmp(char *psz1, char *psz2)
{
   /* if BOTH strings are NULL, return TRUE, if ONE is NULL return */
   /* appropriate value. */
   if (psz1 == psz2)
      return (0);
   else
      if (psz1 == NULL)
         return (-1);
      else
          if (psz2 == NULL)
              return (1);

   /* sync the strings on first non-whitespace */
   while (1)
   {
      while (isspace(*psz1))
         psz1++;
      while (isspace(*psz2))
         psz2++;
      if (toupper(*psz1) != toupper(*psz2) || *psz1 == '\0' || *psz2 == '\0')
         break;
      psz1++;
      psz2++;
   }
   return (*psz1 - *psz2);
}

/***************************************************************************
Map a parameter's string representation to something we can use. 
Returns False if the parameter string is not recognised, else TRUE.
***************************************************************************/
static int map_parameter(char *pszParmName)
{
   int iIndex;

   if (*pszParmName == '-')
     return(-1);

   for (iIndex = 0; parm_table[iIndex].label; iIndex++) 
      if (strwicmp(parm_table[iIndex].label, pszParmName) == 0)
         return(iIndex);

   DEBUG(0,( "Unknown parameter encountered: \"%s\"\n", pszParmName));
   return(-1);
}


/***************************************************************************
Set a boolean variable from the text value stored in the passed string.
Returns True in success, False if the passed string does not correctly 
represent a boolean.
***************************************************************************/
static BOOL set_boolean(BOOL *pb, char *pszParmValue)
{
   BOOL bRetval;

   bRetval = True;
   if (strwicmp(pszParmValue, "yes") == 0 ||
       strwicmp(pszParmValue, "true") == 0 ||
       strwicmp(pszParmValue, "1") == 0)
      *pb = True;
   else
      if (strwicmp(pszParmValue, "no") == 0 ||
          strwicmp(pszParmValue, "False") == 0 ||
          strwicmp(pszParmValue, "0") == 0)
         *pb = False;
      else
      {
         DEBUG(0,( "Badly formed boolean in configuration file: \"%s\".\n",
               pszParmValue));
         bRetval = False;
      }
   return (bRetval);
}

/***************************************************************************
Find a service by name. Otherwise works like get_service.
***************************************************************************/
static int getservicebyname(char *pszServiceName, service *pserviceDest)
{
   int iService;

   for (iService = iNumServices - 1; iService >= 0; iService--)
      if (VALID(iService) &&
	  strwicmp(iSERVICE(iService).szService, pszServiceName) == 0) 
      {
         if (pserviceDest != NULL)
	   copy_service(pserviceDest, pSERVICE(iService), NULL);
         break;
      }

   return (iService);
}



/***************************************************************************
Copy a service structure to another

If pcopymapDest is NULL then copy all fields
***************************************************************************/
static void copy_service(service *pserviceDest, 
                         service *pserviceSource,
                         BOOL *pcopymapDest)
{
  int i;
  BOOL bcopyall = (pcopymapDest == NULL);

  for (i=0;parm_table[i].label;i++)
    if (parm_table[i].ptr && parm_table[i].class == P_LOCAL && 
	(bcopyall || pcopymapDest[i]))
      {
	void *def_ptr = parm_table[i].ptr;
	void *src_ptr = 
	  ((char *)pserviceSource) + PTR_DIFF(def_ptr,&sDefault);
	void *dest_ptr = 
	  ((char *)pserviceDest) + PTR_DIFF(def_ptr,&sDefault);

	switch (parm_table[i].type)
	  {
	  case P_BOOL:
	  case P_BOOLREV:
	    *(BOOL *)dest_ptr = *(BOOL *)src_ptr;
	    break;

	  case P_INTEGER:
	  case P_ENUM:
	  case P_OCTAL:
	    *(int *)dest_ptr = *(int *)src_ptr;
	    break;

	  case P_CHAR:
	    *(char *)dest_ptr = *(char *)src_ptr;
	    break;

	  case P_STRING:
	    string_set(dest_ptr,*(char **)src_ptr);
	    break;

	  case P_USTRING:
	    string_set(dest_ptr,*(char **)src_ptr);
	    strupper_m(*(char **)dest_ptr);
	    break;
	  default:
	    break;
	  }
      }

  if (bcopyall)
    {
      init_copymap(pserviceDest);
      if (pserviceSource->copymap)
	memcpy((void *)pserviceDest->copymap,
	       (void *)pserviceSource->copymap,sizeof(BOOL)*NUMPARAMETERS);
    }
}

/***************************************************************************
Check a service for consistency. Return False if the service is in any way
incomplete or faulty, else True.
***************************************************************************/
static BOOL service_ok(int iService)
{
   BOOL bRetval;

   bRetval = True;
   if (iSERVICE(iService).szService[0] == '\0')
   {
      DEBUG(0,( "The following message indicates an internal error:\n"));
      DEBUG(0,( "No service name in service entry.\n"));
      bRetval = False;
   }

#if 0
   /* The [printers] entry MUST be printable. I'm all for flexibility, but */
   /* I can't see why you'd want a non-printable printer service...        */
   if (strwicmp(iSERVICE(iService).szService,PRINTERS_NAME) == 0)
      if (!iSERVICE(iService).bPrint_ok)
      {
         DEBUG(0,( "WARNING: [%s] service MUST be printable!\n",
               iSERVICE(iService).szService));
	 iSERVICE(iService).bPrint_ok = True;
      }
#endif

   if (iSERVICE(iService).szPath[0] == '\0' &&
       strwicmp(iSERVICE(iService).szService,HOMES_NAME) != 0)
   {
      DEBUG(0,("No path in service %s - using %s\n",iSERVICE(iService).szService,tmpdir()));
      string_set(&iSERVICE(iService).szPath,tmpdir());      
   }

   /* If a service is flagged unavailable, log the fact at level 0. */
   if (!iSERVICE(iService).bAvailable) 
      DEBUG(1,( "NOTE: Service %s is flagged unavailable.\n",
            iSERVICE(iService).szService));

   return (bRetval);
}

static struct file_lists {
  struct file_lists *next;
  char *name;
  time_t modtime;
} *file_lists = NULL;

/*******************************************************************
keep a linked list of all config files so we know when one has changed 
it's date and needs to be reloaded
********************************************************************/
static void add_to_file_list(char *fname)
{
  struct file_lists *f=file_lists;

  while (f) {
    if (f->name && !strcmp(f->name,fname)) break;
    f = f->next;
  }

  if (!f) {
    f = (struct file_lists *)malloc(sizeof(file_lists[0]));
    if (!f) return;
    f->next = file_lists;
    f->name = strdup(fname);
    if (!f->name) {
      free(f);
      return;
    }
    file_lists = f;
  }

  {
    pstring n2;
    pstrcpy(n2,fname);
    standard_sub_basic(n2);
    f->modtime = file_modtime(n2);
  }

}

/*******************************************************************
check if a config file has changed date
********************************************************************/
BOOL lp_file_list_changed(void)
{
  struct file_lists *f = file_lists;
  DEBUG(6,("lp_file_list_changed()\n"));

  while (f)
  {
    pstring n2;
    time_t mod_time;

    pstrcpy(n2,f->name);
    standard_sub_basic(n2);

    DEBUG(6,("file %s -> %s  last mod_time: %s\n",
             f->name, n2, ctime(&f->modtime)));

    mod_time = file_modtime(n2);

    if (f->modtime != mod_time) {
	    DEBUG(6,("file %s modified: %s\n", n2, ctime(&mod_time)));
	    f->modtime = mod_time;
	    return(True);
    }
    f = f->next;   
  }
  return(False);
}

/***************************************************************************
  handle the interpretation of the coding system parameter
  *************************************************************************/
static BOOL handle_coding_system(char *pszParmValue,char **ptr)
{
	string_set(ptr,pszParmValue);
//	interpret_coding_system(pszParmValue);
	return(True);
}

/***************************************************************************
handle the interpretation of the character set system parameter
***************************************************************************/
static BOOL handle_character_set(char *pszParmValue,char **ptr)
{
	string_set(ptr,pszParmValue);
//	interpret_character_set(pszParmValue);
	return(True);
}


/***************************************************************************
handle the valid chars lines
***************************************************************************/
static BOOL handle_valid_chars(char *pszParmValue,char **ptr)
{ 
  string_set(ptr,pszParmValue);

  /* A dependency here is that the parameter client code page must be
     set before this is called - as calling codepage_initialise()
     would overwrite the valid char lines.
   */
//  codepage_initialise(lp_client_code_page());

//  add_char_string(pszParmValue);
  return(True);
}


/***************************************************************************
handle the include operation
***************************************************************************/
static BOOL handle_include(char *pszParmValue,char **ptr)
{ 
  pstring fname;
  pstrcpy(fname,pszParmValue);

  add_to_file_list(fname);

  standard_sub_basic(fname);

  string_set(ptr,fname);


  if (file_exist(fname,NULL)){
    	return(pm_process(fname, do_section, do_parameter));  
  }

  DEBUG(2,("Can't find include file %s\r\n",fname));

  return(False);
}


/***************************************************************************
handle the interpretation of the copy parameter
***************************************************************************/
static BOOL handle_copy(char *pszParmValue,char **ptr)
{
   BOOL bRetval;
   int iTemp;
   service serviceTemp;

   string_set(ptr,pszParmValue);

   init_service(&serviceTemp);

   bRetval = False;
   
   DEBUG(3,("Copying service from service %s\n",pszParmValue));

   if ((iTemp = getservicebyname(pszParmValue, &serviceTemp)) >= 0)
     {
       if (iTemp == iServiceIndex)
	 {
	   DEBUG(0,("Can't copy service %s - unable to copy self!\n",
		    pszParmValue));
	 }
       else
	 {
	   copy_service(pSERVICE(iServiceIndex), 
			&serviceTemp,
			iSERVICE(iServiceIndex).copymap);
	   bRetval = True;
	 }
     }
   else
     {
       DEBUG(0,( "Unable to copy service - source not found: %s\n",
		pszParmValue));
       bRetval = False;
     }

   free_service(&serviceTemp);
   return (bRetval);
}


/***************************************************************************
initialise a copymap
***************************************************************************/
static void init_copymap(service *pservice)
{
  int i;
  if (pservice->copymap) free(pservice->copymap);
  pservice->copymap = (BOOL *)malloc(sizeof(BOOL)*NUMPARAMETERS);
  if (!pservice->copymap)
    DEBUG(0,("Couldn't allocate copymap!! (size %d)\n",NUMPARAMETERS));

  for (i=0;i<NUMPARAMETERS;i++)
    pservice->copymap[i] = True;
}


/***************************************************************************
Process a parameter for a particular service number. If snum < 0
then assume we are in the globals
***************************************************************************/
BOOL lp_do_parameter(int snum, char *pszParmName, char *pszParmValue)
{
   int parmnum, i;
   void *parm_ptr=NULL; /* where we are going to store the result */
   void *def_ptr=NULL;

   parmnum = map_parameter(pszParmName);

   if (parmnum < 0)
     {
       DEBUG(0,( "Ignoring unknown parameter \"%s\"\n", pszParmName));
       return(True);
     }

   def_ptr = parm_table[parmnum].ptr;

   /* we might point at a service, the default service or a global */
   if (snum < 0) {
     parm_ptr = def_ptr;
   } else {
       if (parm_table[parmnum].class == P_GLOBAL) {
	   DEBUG(0,( "Global parameter %s found in service section!\n",pszParmName));
	   return(True);
	 }
       parm_ptr = ((char *)pSERVICE(snum)) + PTR_DIFF(def_ptr,&sDefault);
   }

   if (snum >= 0) {
	   if (!iSERVICE(snum).copymap)
		   init_copymap(pSERVICE(snum));
	   
	   /* this handles the aliases - set the copymap for other entries with
	      the same data pointer */
	   for (i=0;parm_table[i].label;i++)
		   if (parm_table[i].ptr == parm_table[parmnum].ptr)
			   iSERVICE(snum).copymap[i] = False;
   }

   /* if it is a special case then go ahead */
   if (parm_table[parmnum].special) {
	   parm_table[parmnum].special(pszParmValue,parm_ptr);
	   return(True);
   }

   /* now switch on the type of variable it is */
   switch (parm_table[parmnum].type)
     {
     case P_BOOL:
       set_boolean(parm_ptr,pszParmValue);
       break;

     case P_BOOLREV:
       set_boolean(parm_ptr,pszParmValue);
       *(BOOL *)parm_ptr = ! *(BOOL *)parm_ptr;
       break;

     case P_INTEGER:
       *(int *)parm_ptr = atoi(pszParmValue);
       break;

     case P_CHAR:
       *(char *)parm_ptr = *pszParmValue;
       break;

     case P_OCTAL:
       sscanf(pszParmValue,"%o",(int *)parm_ptr);
       break;

     case P_STRING:
       string_set(parm_ptr,pszParmValue);
       break;

     case P_USTRING:
       string_set(parm_ptr,pszParmValue);
       strupper_m(*(char **)parm_ptr);
       break;

     case P_GSTRING:
       strcpy((char *)parm_ptr,pszParmValue);
       break;

     case P_UGSTRING:
       strcpy((char *)parm_ptr,pszParmValue);
       strupper_m((char *)parm_ptr);
       break;

     case P_ENUM:
	     for (i=0;parm_table[parmnum].enum_list[i].name;i++) {
		     if (strequal(pszParmValue, parm_table[parmnum].enum_list[i].name)) {
			 	DEBUG(0, ("(%s) get parm: %s\n", parm_table[parmnum].enum_list[i].name));
			     *(int *)parm_ptr = parm_table[parmnum].enum_list[i].value;
			     break;
		     }
	     }
	     break;
     }

   return(True);
}

/***************************************************************************
Process a parameter.
***************************************************************************/
static BOOL do_parameter(char *pszParmName, char *pszParmValue)
{
   if (!bInGlobalSection && bGlobalOnly) return(True);

   DEBUG(3,("doing parameter %s = %s\n",pszParmName,pszParmValue));

   return lp_do_parameter(bInGlobalSection?-2:iServiceIndex, pszParmName, pszParmValue);
}

/***************************************************************************
Process a new section (service). At this stage all sections are services.
Later we'll have special sections that permit server parameters to be set.
Returns True on success, False on failure.
***************************************************************************/
static BOOL do_section(char *pszSectionName)
{
   BOOL bRetval;
   BOOL isglobal = ((strwicmp(pszSectionName, GLOBAL_NAME) == 0) || 
		    (strwicmp(pszSectionName, GLOBAL_NAME2) == 0));
   bRetval = False;

   /* if we've just struck a global section, note the fact. */
   bInGlobalSection = isglobal;   

   /* check for multiple global sections */
   if (bInGlobalSection)
   {
     DEBUG(3,( "Processing section \"[%s]\"\n", pszSectionName));
     return(True);
   }

   if (!bInGlobalSection && bGlobalOnly) return(True);

   /* if we have a current service, tidy it up before moving on */
   bRetval = True;

   if (iServiceIndex >= 0)
     bRetval = service_ok(iServiceIndex);

   /* if all is still well, move to the next record in the services array */
   if (bRetval)
     {
       /* We put this here to avoid an odd message order if messages are */
       /* issued by the post-processing of a previous section. */
       DEBUG(2,( "Processing section \"[%s]\"\n", pszSectionName));

       if ((iServiceIndex=add_a_service(&sDefault,pszSectionName)) < 0)
	 {
	   DEBUG(0,("Failed to add a new service\n"));
	   return(False);
	 }
     }

   return (bRetval);
}

/***************************************************************************
Return TRUE if the passed service number is within range.
***************************************************************************/
BOOL lp_snum_ok(int iService)
{
   return (LP_SNUM_OK(iService) && iSERVICE(iService).bAvailable);
}


/***************************************************************************
auto-load some homes and printer services
***************************************************************************/
static void lp_add_auto_services(char *str)
{
  char *s;
  char *p;
  int homes = lp_servicenumber(HOMES_NAME);
//  int printers = lp_servicenumber(PRINTERS_NAME);

  if (!str)
    return;

  s = strdup(str);
  if (!s) return;

  for (p=strtok(s,LIST_SEP);p;p=strtok(NULL,LIST_SEP))
    {
      char *home = get_home_dir(p);

      if (lp_servicenumber(p) >= 0) continue;

      if (home && homes >= 0)
	{
	  lp_add_home(p,homes,home);
	  continue;
	}

//      if (printers >= 0 && pcap_printername_ok(p,NULL))
//	lp_add_printer(p,printers);
    }
  free(s);
}

/***************************************************************************
have we loaded a services file yet?
***************************************************************************/
BOOL lp_loaded(void)
{
  return(bLoaded);
}

/***************************************************************************
unload unused services
***************************************************************************/
void lp_killunused(BOOL (*snumused)(int ))
{
  int i;
  for (i=0;i<iNumServices;i++)
    if (VALID(i) && (!snumused || !snumused(i)))
      {
	iSERVICE(i).valid = False;
	free_service(pSERVICE(i));
      }
}

/***************************************************************************
Load the services array from the services file. Return True on success, 
False on failure.
***************************************************************************/
BOOL lp_load(char *pszFname,BOOL global_only)
{
  pstring n2;
  BOOL bRetval;
 
  add_to_file_list(pszFname);

  bRetval = False;

  bInGlobalSection = True;
  bGlobalOnly = global_only;
  
  init_globals();
  
  pstrcpy(n2,pszFname);
  standard_sub_basic(n2);

  /* We get sections first, so have to start 'behind' to make up */
  iServiceIndex = -1;
  bRetval = pm_process(n2, do_section, do_parameter);
  
  /* finish up the last section */
  DEBUG(3,("pm_process() returned %s\n", BOOLSTR(bRetval)));
  if (bRetval)
    if (iServiceIndex >= 0)
      bRetval = service_ok(iServiceIndex);	   

  lp_add_auto_services(lp_auto_services());
  
//  if (lp_load_printers())
//    lp_add_all_printers();

  lp_add_ipc();

  set_default_server_announce_type();

  bLoaded = True;

  init_iconv();

  return (bRetval);
}


/***************************************************************************
return the max number of services
***************************************************************************/
int lp_numservices(void)
{
  return(iNumServices);
}

/***************************************************************************
Return the number of the service with the given name, or -1 if it doesn't
exist. Note that this is a DIFFERENT ANIMAL from the internal function
getservicebyname()! This works ONLY if all services have been loaded, and
does not copy the found service.
***************************************************************************/
int lp_servicenumber(char *pszServiceName)
{
   int iService;

   for (iService = iNumServices - 1; iService >= 0; iService--)
      if (VALID(iService) &&
	  strequal(lp_servicename(iService), pszServiceName)) 
         break;

   if (iService < 0)
     DEBUG(7,("lp_servicenumber: couldn't find %s\n",pszServiceName));
   
   return (iService);
}

/*******************************************************************
  a useful volume label function
  ******************************************************************/
char *volume_label(int snum)
{
  char *ret = lp_volume(snum);
  if (!*ret) return(lp_servicename(snum));
  return(ret);
}


/*******************************************************************
 Set the server type we will announce as via nmbd.
********************************************************************/
static void set_default_server_announce_type()
{
  default_server_announce = (SV_TYPE_WORKSTATION | SV_TYPE_SERVER |
                              		SV_TYPE_SERVER_UNIX | SV_TYPE_PRINTQ_SERVER);
  
  if(lp_announce_as() == ANNOUNCE_AS_NT)
    	default_server_announce |= (SV_TYPE_SERVER_NT | SV_TYPE_NT);
  else if(lp_announce_as() == ANNOUNCE_AS_WIN95)
    	default_server_announce |= SV_TYPE_WIN95_PLUS;
  else if(lp_announce_as() == ANNOUNCE_AS_WFW)
    	default_server_announce |= SV_TYPE_WFW;
  
  default_server_announce |= (lp_time_server() ? SV_TYPE_TIME_SOURCE : 0);
/*
 * nmbd only loads the [global] section. There seems to be no way to
 * determine exactly if any service is printable by only looking at the
 * [global] section so for now always announce as a print server. This
 * will need looking at in the future. Jeremy (jallison@whistle.com).
 */
#if 0
  default_server_announce |= (lp_printer_services() ? SV_TYPE_PRINTQ_SERVER : 0);
#endif
}

