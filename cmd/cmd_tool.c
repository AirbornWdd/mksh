#include "command.h"
#include "vtysh_config.h"
#include <ctype.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/if_ether.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <sys/vfs.h>    /* or <sys/statfs.h> */
#include <assert.h>
#include "ldsec_nlclient.h"
#include "utils.h"

#include "mk.h"

DEFUN(config_ping,
        config_ping_cmd,
        "ping WORD",
        "Send echo messages\n"
        "Ping destination address or hostname\n")
{
    cmd_execute_system_command ("ping", 1, argv);
    return CMD_SUCCESS;
}

DEFUN(system_password,
        system_password_cmd,
        "password",
        "Change password of myself.\n")
{
    return cmd_execute_system_command2("passwd");
}

DEFUN(config_ping_count,
        config_ping_cmd_count,
        "ping WORD NUM",
        "send count echo messages\n"
        "Ping destination address or hostname\n"
        "Stop after sending NUM ECHO_REQUEST packets\n")
{
    char *myargv[5];

    if(!isdigit(argv[1][0])){
        vty_out(vty, "Invalid number '%s', Please input the count number\n", 
			argv[1]);
        return CMD_ERR_NOTHING_TODO;
    }
    myargv[0] = argv[0];
    myargv[1] = "-c";
    myargv[2] = argv[1];
    cmd_execute_system_command ("ping", 3, myargv);
    return CMD_SUCCESS;
}

DEFUN(config_traceroute,
        config_traceroute_cmd,
        "traceroute WORD",
        "Trace route to destination\n"
        "Trace route to destination address or hostname\n")
{
    cmd_execute_system_command ("traceroute", 1, argv);
    return CMD_SUCCESS;
}

DEFUN(net_netstat_info,
        net_netstat_info_cmd,
        "show netstat",
        SHOW_STR
        "Show netstat\n")

{ 
    char *myargv[4] = {NULL};
    
    myargv[0] = "-n";
    myargv[1] = "-a";
    myargv[2] = "-p";	
    return cmd_execute_system_command ("netstat", 2, myargv);
} 

DEFUN(show_version,
        show_version_cmd,
        "show version",
        SHOW_STR
        "Show all versions\n")
{ 
    versions_show(vty);
    
    return CMD_SUCCESS;
} 

DEFUN(show_version_detail,
        show_version_detail_cmd,
        "show version detail",
        SHOW_STR
        "Version details\n"
        "Show all versions\n")
{ 
    versions_show_detail(vty);
    
    return CMD_SUCCESS;
} 

DEFUN(show_project,
        show_project_cmd,
        "show project",
        SHOW_STR
        "Show all projects\n")
{ 
    struct version_inf *vi = 
        (struct version_inf *)vty->index;
    
    projects_show(vty, vi);
    
    return CMD_SUCCESS;
} 

DEFUN(get_version,
        get_version_cmd,
        "version WORD (soho|themis|sag|x86_32|x86_64|arm_32) [refresh]",
        "Enter the version node\n"
        "Version name\n"
        "soho version\n"
        "themis version\n"
        "sag version\n"
        "x86 architecture\n"
        "x64 architecture\n"
        "arm architecture\n"
        "Refresh the version.\n")
{ 
    int ret, refresh = 0;
    struct version_inf *vi;
    char *name;

    name = XSTRDUP(MTYPE_TMP, argv[0]);
    mk_fix_name(name);
    
    if ((vi = version_get(name)) == NULL) {
        vty_out(vty, "No such version: %s.\n", argv[0]);
        goto out;
    }

    if (argc == 3 && !strcmp(argv[2], "refresh")) {
        refresh = 1;
    }

    if ((ret = vi->init(vi, argv[1])) != 0) {
        vty_out(vty, "Version init error: %s.\n", 
            mk_strerror(ret));
        goto out;
    }

    if (refresh || !dir_or_file_exist(PROJECT_BUILD_NAME)) {
        if ((ret = vi->refresh(vi)) != 0) {

            vi->finit(vi);
            
            vty_out(vty, "Version refresh error: %s.\n", 
                mk_strerror(ret));
            goto out;
        }
    }
    
    vty->index = vi;
    vty->node = VERSION_NODE;

out:
    XFREE(MTYPE_TMP, name);
    return CMD_SUCCESS;
} 

DEFUN(get_project,
        get_project_cmd,
        "project WORD [refresh]",
        "Enter the project node\n"
        "Project name\n"
        "Refresh the project.\n")
{ 
    int ret, refresh = 0;
    struct version_inf *vi = (struct version_inf *)vty->index;
    struct project_inf *pi;
    
    if ((pi = project_get(argv[0], vi)) == NULL) {
        vty_out(vty, "No such project: %s\n", argv[0]);
        return CMD_WARNING;
    }

    if (argc == 2 && !strcmp(argv[1], "refresh")) {
        refresh = 1;
    }
    
    if ((ret = pi->init(pi, refresh)) != 0) {
        vty_out(vty, "Project init error: %s.\n", 
            mk_strerror(ret));
        return CMD_WARNING;
    }

    vty->index = pi;
    vty->node = PROJECT_NODE;
    
    return CMD_SUCCESS;
} 

DEFUN (version_exit,
       version_exit_cmd,
       "exit",
       "Exit current mode and down to previous mode\n")
{
    struct version_inf *vi = (struct version_inf *)vty->index;

    vi->finit(vi);
    
    vty->index = NULL;
    vty->node = ENABLE_NODE;
    
	return CMD_SUCCESS;
}

DEFUN (version_cmd_refresh,
       version_cmd_refresh_cmd,
       "refresh",
       "Refresh the version.\n")
{
    struct version_inf *vi = (struct version_inf *)vty->index;

    return vi->refresh(vi);
}

DEFUN (project_exit,
       project_exit_cmd,
       "exit",
       "Exit current mode and down to previous mode\n")
{
    struct project_inf *pi = (struct project_inf *)vty->index;
    struct version_inf *vi = pi->version;    
    
    pi->finit(pi);
    
    vty->index = vi;
    vty->node = VERSION_NODE;
    
    return CMD_SUCCESS;
}

DEFUN (project_end,
       project_end_cmd,
       "end",
       "End current mode and change to enable mode.")
{
    struct project_inf *pi = (struct project_inf *)vty->index;
    struct version_inf *vi = pi->version;

    pi->finit(pi);
    vi->finit(vi);
    
    vty->index = NULL;
    vty->node = ENABLE_NODE;
    
	return CMD_SUCCESS;
}

DEFUN (system_pwd,
       system_pwd_cmd,
       "pwd",
       "Show current directory\n")
{
    return cmd_execute_system_command ("pwd", 0, NULL);
}

DEFUN (system_env,
       system_env_cmd,
       "env",
       "Show system environment\n")
{
    return cmd_execute_system_command ("env", 0, NULL);
}

DEFUN (system_ls3,
       system_ls3_cmd,
       "ls WORD WORD",
       "List current directories.\n"
       "Options\n"
       "Directories\n")
{
    char *myargv[] = {argv[0], argv[1]};
    
    return cmd_execute_system_command ("ls", 2, myargv);
}

DEFUN (system_ls2,
       system_ls2_cmd,
       "ls WORD",
       "List current directories.\n"
       "Directories\n")
{
    char *myargv[] = {argv[0]};
    
    return cmd_execute_system_command ("ls", 1, myargv);
}

DEFUN (system_ls,
       system_ls_cmd,
       "ls",
       "List current directories.\n")
{
    return cmd_execute_system_command ("ls", 0, NULL);
}

DEFUN (system_mkdir,
       system_mkdir_cmd,
       "mkdir -p WORD",
       "Make directory.\n"
       "Argument of the command.\n"
       "Directory name.\n")
{
    return mkdir_cmd(argv[0]);
}

DEFUN (project_make,
       project_make_cmd,
       "make",
       "Compile the project.\n")
{
    struct project_inf *pi = (struct project_inf *)vty->index;

    pi->compile(0, NULL);
    
    return CMD_SUCCESS;
}

DEFUN (project_make_install_or_clean,
       project_make_install_or_clean_cmd,
       "make (install|clean)",
       "Compile the project.\n"
       "Export the binary files.\n"
       "Clean the binary files.\n")
{
    struct project_inf *pi = (struct project_inf *)vty->index;

    if (!strcmp(argv[0], "install")) {
        pi->install(pi, 0, NULL);
    } else {
        pi->clean(0, NULL);
    }

    return CMD_SUCCESS;
}

DEFUN (project_make2,
       project_make2_cmd,
       "make WORD",
       "Compile the project.\n"
       "Argument of make.\n")
{
    return make_cmd(argc, argv);
}

DEFUN (project_make_c,
       project_make_c_cmd,
       "make -C WORD",
       "Compile the project.\n"       
       "Change to directory.\n"
       "Directory name.\n")
{
    char *myargv[2] = {"-C", argv[0]};

    return make_cmd(2, myargv);
}

DEFUN (project_make_c_2,
       project_make_c_2_cmd,
       "make -C WORD WORD",
       "Compile the project.\n"       
       "Change to directory.\n"
       "Directory name.\n"
       "Argument of make.\n")
{
    char *myargv[3] = {"-C", argv[0], argv[1]};

    return make_cmd(3, myargv);
}

DEFUN (project_cmd_refresh,
       project_cmd_refresh_cmd,
       "refresh",
       "Refresh the project.\n")
{
    struct project_inf *pi = (struct project_inf *)vty->index;

    chdir("..");
    pi->refresh(pi);
    chdir(pi->name);

    return CMD_SUCCESS;
}

DEFUN (project_conf,
       project_conf_cmd,
       "configure-project",
       "Enter the project-configure node\n")
{
    vty->node = PRO_CONF_NODE;
    
    return CMD_SUCCESS;
}

DEFUN (pro_conf_exit,
       pro_conf_exit_cmd,
       "exit",
       "Exit current mode and down to previous mode\n")
{
    vty->node = PROJECT_NODE;
    
    return CMD_SUCCESS;
}

DEFUN (pro_conf_kernel_conf_file,
       pro_conf_kernel_conf_file_cmd,
       "kernel-config-file WORD",
       "Configuration file of kernel\n"
       "File name\n")
{
    struct project_inf *pi = (struct project_inf *)vty->index;
    char *myargv[] = {argv[0], ".config"};

    if (!strstr(pi->name, "kernel")) {
        vty_out(vty, "Not kernel project.\n");
        return CMD_WARNING;
    }
    
    if (!strstr(argv[0], "config") || 
            !dir_or_file_exist(argv[0])) {
        vty_out(vty, "File name invalid.\n");
        return CMD_WARNING;
    }
    
    return cmd_execute_system_command ("cp", 2, myargv);
}

int cmd_tool_init()
{
    /* Each node's basic commands. */
    cmd_install_element (ENABLE_NODE, &config_ping_cmd);
    cmd_install_element (ENABLE_NODE, &config_ping_cmd_count);
    cmd_install_element (ENABLE_NODE, &config_traceroute_cmd);
    cmd_install_element (ENABLE_NODE, &net_netstat_info_cmd);
    cmd_install_element (ENABLE_NODE, &system_env_cmd);    
    cmd_install_element (ENABLE_NODE, &show_version_cmd);
    cmd_install_element (ENABLE_NODE, &show_version_detail_cmd);
    cmd_install_element (ENABLE_NODE, &get_version_cmd);
    cmd_install_element (ENABLE_NODE, &system_pwd_cmd);    
    cmd_install_element (ENABLE_NODE, &system_password_cmd);
    
    cmd_install_element (VERSION_NODE, &get_project_cmd);
    cmd_install_element (VERSION_NODE, &version_exit_cmd);
    cmd_install_element (VERSION_NODE, &system_pwd_cmd);    
    cmd_install_element (VERSION_NODE, &system_env_cmd);    
    cmd_install_element (VERSION_NODE, &system_ls_cmd);    
    cmd_install_element (VERSION_NODE, &system_ls2_cmd);    
    cmd_install_element (VERSION_NODE, &system_ls3_cmd);    
    cmd_install_element (VERSION_NODE, &system_mkdir_cmd);    
    cmd_install_element (VERSION_NODE, &show_version_cmd);
    cmd_install_element (VERSION_NODE, &show_version_detail_cmd);
    cmd_install_element (VERSION_NODE, &show_project_cmd);
    cmd_install_element (VERSION_NODE, &version_cmd_refresh_cmd);
    
    cmd_install_element (PROJECT_NODE, &project_exit_cmd);
    cmd_install_element (PROJECT_NODE, &project_end_cmd);
    cmd_install_element (PROJECT_NODE, &project_conf_cmd);
    cmd_install_element (PROJECT_NODE, &system_pwd_cmd);    
    cmd_install_element (PROJECT_NODE, &system_env_cmd);    
    cmd_install_element (PROJECT_NODE, &system_ls_cmd);    
    cmd_install_element (PROJECT_NODE, &system_ls2_cmd);    
    cmd_install_element (PROJECT_NODE, &system_ls3_cmd);    
    cmd_install_element (PROJECT_NODE, &system_mkdir_cmd);    
    cmd_install_element (PROJECT_NODE, &project_make_install_or_clean_cmd);    
    cmd_install_element (PROJECT_NODE, &project_make_c_cmd);    
    cmd_install_element (PROJECT_NODE, &project_make_c_2_cmd);    
    cmd_install_element (PROJECT_NODE, &project_make2_cmd);    
    cmd_install_element (PROJECT_NODE, &project_make_cmd);    
    cmd_install_element (PROJECT_NODE, &project_cmd_refresh_cmd);    

    cmd_install_element (PRO_CONF_NODE, &pro_conf_exit_cmd);    
    cmd_install_element (PRO_CONF_NODE, &pro_conf_kernel_conf_file_cmd);    
    
    return 0;
}
