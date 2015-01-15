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
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <linklist.h>
#include <utime.h>

#include "mk.h"

struct list *versions;

int
illegal_patch_file(char *filename)
{
    char cmd[MK_MAX_STR_LEN] = "";

    sprintf(cmd, 
        "grep '^\\(+\\{3\\}\\|-\\{3\\}\\) [a-zA-Z0-9/]*\\([Mm]akefile\\|configure\\)' "
        "%s &>/dev/null", filename);

    return (cmd_execute_system_command2(cmd) == 0);
}

int
hack_in_file(char *filename)
{
    int found = 0;
    char cmd[MK_MAX_STR_LEN] = "";

    /*1. If the `ftp-server-path' found in the file, 
         *    then we could absolutely know the illegal operation. 
         */
    sprintf(cmd, "grep '"FTP_SERVER_DIR"' %s &>/dev/null", filename);
    
    if (cmd_execute_system_command2(cmd) == 0) {
        found = 1;
        goto out;
    }
    
out:    
    return found;
}

int
check_single_file_w(char *file)
{
    struct stat sb;
    
    if (stat(file, &sb) == -1) {
        return -MK_ERR_SYSTEM;
    }

    if (sb.st_mode & (S_IWOTH|S_IWGRP|S_IWUSR)) {
        return 1;
    }

    return 0;
}

int
change_single_file_rx(char *file)
{
    char cmd[MK_MAX_STR_LEN] = "";

    sprintf(cmd, "chmod 555 %s &>/dev/null", file);

    return cmd_execute_system_command2(cmd);
}

int
change_all_files_ro(char *dir)
{
    char cmd[MK_MAX_STR_LEN] = "";

    sprintf(cmd, 
        "chmod 444 %s/* -R &>/dev/null", dir);

    return cmd_execute_system_command2(cmd);
}

int
change_all_makefiles_ro(char *dir)
{
    char cmd[MK_MAX_STR_LEN] = "";

    sprintf(cmd, 
        "find %s -name Makefile | "
        "xargs --no-run-if-empty chmod 444 &>/dev/null", 
        dir);

    return cmd_execute_system_command2(cmd);
}

int
dir_or_file_exist(char *name)
{
    char cmd[MK_MAX_STR_LEN] = "ls ";

    strcat(cmd, name);
    strcat(cmd, " &>/dev/null");
    
    return !cmd_execute_system_command2(cmd);
}

int
file_same(char *file1, char *file2)
{
    char cmd[MK_MAX_STR_LEN] = "diff ";

    strcat(cmd, file1);
    strcat(cmd, " ");
    strcat(cmd, file2);
    strcat(cmd, " &>/dev/null");
    
    return !cmd_execute_system_command2(cmd);
}

char *
mk_string_to_upper(char *string)
{
    char *p = string;

    while (*p != '\0') {
        if (islower(*p))
            *p = toupper(*p);
        p++;
    }

    return string;
}

int
handle_configure_file(void)
{
    struct stat sb;
    struct utimbuf ub;    
    
    if (hack_in_file("configure")) {
        return -MK_ERR_HACK;
    }

    if (stat("configure", &sb) == -1) {
        return -MK_ERR_SYSTEM;
    }

    if (sb.st_mode & (S_IWOTH|S_IWGRP|S_IWUSR)) {
        if (change_single_file_rx("configure")) {
            return -MK_ERR_SYSTEM;
        }
        
        ub.actime = sb.st_atime;
        ub.modtime = sb.st_mtime;
        if (utime("configure", &ub) == -1) {
            return -MK_ERR_SYSTEM;
        }
    }

    return 0;
}

char *
mk_strerror(int mk_errno)
{
    switch (mk_errno) {
        case -MK_ERR_VER_ARCH_INVAL:
            return "version and arch not match";
        case -MK_ERR_SVN:
            return "svn commond error";
        case -MK_ERR_SYSTEM:
            return "system commond error";
        case -MK_ERR_OOM:
            return "out of memory";
        case -MK_ERR_HACK:
            return "illegal operation";
        default:
            return "unknown";
    }
}

int 
make_cmd(int argc, char **argv)
{
    return cmd_execute_system_command("make", argc, argv);
}

int 
make_common_cmd(int argc, char **argv)
{
    return cmd_execute_system_command("make", argc, argv);
}

int 
mkdir_cmd(char *dir)
{
    char *argv[2] = {"-p", dir};
    
    return cmd_execute_system_command("mkdir", 2, argv);
}

int
svn_co_cmd(char *url)
{
    char *argv[2] = {"co", url};
 
    return cmd_execute_system_command("svn", 2, argv);
}

int 
svn_co_cmd2(char *url)
{
    char cmd[MK_MAX_STR_LEN] = "svn co ";

    strcat(cmd, url);
    strcat(cmd, " &>/dev/null");
    
    return cmd_execute_system_command2(cmd);
}

int 
svn_ls_cmd(char *url)
{
    char cmd[MK_MAX_STR_LEN] = "svn ls ";

    strcat(cmd, url);
    strcat(cmd, " &>/dev/null");

    return cmd_execute_system_command2(cmd);
}

int 
svn_common_cmd(int argc, char **argv)
{
    return cmd_execute_system_command("svn", argc, argv);
}

void
mk_fix_name(char *name)
{
    int len = strlen(name);
    
    while(name[--len] == '/') {
        name[len] = '\0';
    }
}

static int
project_fcms_init_env(void *this)
{
    char dir[MK_MAX_STR_LEN];
    struct project_inf *pi = (struct project_inf *)this;

    setenv("CLI_ROOT", pi->install_dir(pi, dir), 1);

    return 0;
}

static void
project_fcms_finit_env(void *this)
{
    unsetenv("CLI_ROOT");
}

static int
project_themis_kernel_init_env(void *this)
{
    int ret, version;
    char cmd[MK_MAX_STR_LEN] = "";
    struct project_inf *pi = (struct project_inf *)this;

    if (dir_or_file_exist(".config")) {
        return 0;
    }

    version = pi->version->type(pi->version);
    if (version == MK_VERSION_3602) {
        char config[MK_MAX_STR_LEN] = "config-2.6.35.6";
        
        sprintf(cmd, "cp %s .config", config);
    } else if (version == MK_VERSION_3609) {
        char config[MK_MAX_STR_LEN] = "config.";
        
        strcat(config, pi->version->arch);
        sprintf(cmd, "cp %s .config", config);
    } else {
        ret = -MK_ERR_MAX;
        goto out;
    }

    if ((ret = cmd_execute_system_command2(cmd)) != 0) {
        ret = -MK_ERR_SYSTEM;
        goto out;
    }

out:
    return ret;
}

static inline char *
project_themis_arch_dir(struct project_inf *pi)
{
    if (!strcmp(pi->version->arch, "x86_32") || 
        !strcmp(pi->version->arch, "x86_64") ||
        !strcmp(pi->version->arch, "themis") ||
        !strcmp(pi->version->arch, "sag")) {
        return "x86";
    } else if (!strcmp(pi->version->arch, "arm_32")) {
        return "arm";
    } else {
        return "null";
    }
}

static int
project_themis_kernel_install(void *this, int argc, char **argv)
{
    char cmd[MK_MAX_STR_LEN] = "make modules_install";
    char *install_dir;
    struct project_inf *pi = (struct project_inf *)this;
    
    if (((install_dir = getenv("INSTALL_ROOT")) == NULL)) {
        return -MK_ERR_SYSTEM;
    }
    
    if (cmd_execute_system_command2(cmd)) {
        return -MK_ERR_SYSTEM;
    }

    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "cp arch/%s/boot/bzImage %s", 
        project_themis_arch_dir(pi), install_dir);
    
    if (cmd_execute_system_command2(cmd)) {
        return -MK_ERR_SYSTEM;
    }

    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "cp -a install/lib/* %s/lib/", install_dir);
    
    if (cmd_execute_system_command2(cmd)) {
        return -MK_ERR_SYSTEM;
    }

    return 0;
}

int
project_library_install(void *this, int argc, char **argv)
{
    char cmd[MK_MAX_STR_LEN];
    char *install_dir;
    
    if (((install_dir = getenv("INSTALL_ROOT")) == NULL)) {
        return -MK_ERR_SYSTEM;
    }
    
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "cp -f utmsiglib.pkg %s/tmp", install_dir);
    
    if (cmd_execute_system_command2(cmd)) {
        return -MK_ERR_SYSTEM;
    }

    return 0;
}

int
project_ntop_init_env(void *this)
{
    return change_single_file_rx("ntopmakeinstall.sh");
}

int
project_ntop_install(void *this, int argc, char **argv)
{
    return cmd_execute_system_command2("./ntopmakeinstall.sh");
}

int
project_iptables_init_env(void *this)
{
    int ret;

    if ((ret = handle_configure_file()) != 0) {
        return ret;
    }

    if (cmd_execute_system_command2("./configure --enable-static --disable-shared")) {
        return -MK_ERR_SYSTEM;
    }

    return change_all_makefiles_ro("./");
}

int
project_iptables_install(void *this, int argc, char **argv)
{
    struct project_inf *pi = (struct project_inf *)this;
    struct version_inf *vi = pi->version;
    int version, ret = 0;
    char *dir_ptr, 
        cmd[MK_MAX_STR_LEN], 
        install_sbin_dir[MK_MAX_STR_LEN],
        cwd[PATH_MAX+1];

    if (!getcwd(cwd, PATH_MAX+1)) {
        ret = -MK_ERR_SYSTEM;
        goto out;
    }
    
    if (((dir_ptr = getenv("INSTALL_ROOT")) == NULL)) {
        ret = -MK_ERR_SYSTEM;
        goto out;
    }

    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "cp -au iptables-multi ip6tables-multi %s/sbin", dir_ptr);
    if (cmd_execute_system_command2(cmd)) {
        ret = -MK_ERR_SYSTEM;
        goto out;
    }

    strcpy(install_sbin_dir, dir_ptr);
    strcat(install_sbin_dir, "/sbin");
    chdir(install_sbin_dir);

    strcpy(cmd, "ln -fs iptables-multi iptables && "
        "ln -fs iptables-multi iptables-save && "
        "ln -fs ip6tables-multi ip6tables && "
        "ln -fs ip6tables-multi ip6tables-save");
    if (cmd_execute_system_command2(cmd)) {
        chdir(cwd);
        ret = -MK_ERR_SYSTEM;
        goto out;
    }
    chdir(cwd);
    
    version = vi->type(vi);
    if (version != MK_VERSION_3609) {
        ret = -MK_ERR_MAX;
        goto out;
    }
    
    if ((dir_ptr = getenv("COMPILE_INCLUDE_DIR")) == NULL) {
        ret = -MK_ERR_SYSTEM;
        goto out;
    }
    
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "cp -f include/xtables.h %s", dir_ptr);
    if (cmd_execute_system_command2(cmd)) {
        ret = -MK_ERR_SYSTEM;
        goto out;
    }
    
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "cp -rf include/libiptc %s", dir_ptr);
    if (cmd_execute_system_command2(cmd)) {
        ret = -MK_ERR_SYSTEM;
        goto out;
    }
    
out:    
    return ret;
}

int
project_quagga_install(void *this, int argc, char **argv)
{
    int ret = 0;
    char *dir, cmd[MK_MAX_STR_LEN];

    if (((dir = getenv("INSTALL_ROOT")) == NULL)) {
        ret = -MK_ERR_SYSTEM;
        goto out;
    }

    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "make install DESTDIR=%s", dir);

    if (cmd_execute_system_command2(cmd)) {
        ret = -MK_ERR_SYSTEM;
        goto out;
    }
out:
    return ret;
}

int
project_quagga_init_env(void *this)
{
    int ret;

    if ((ret = handle_configure_file()) != 0) {
        return ret;
    }
    
    if (cmd_execute_system_command2("./configure --enable-vtysh --enable-user='root' "
        "--enable-group='root' --enable-vty-group='root' --enable-isisd")) {
        return -MK_ERR_SYSTEM;
    }

    return change_all_makefiles_ro("./");
}

int
project_has_configure_compile(void *this, int argc, char **argv)
{
    int ret = 0;
    struct stat sb;
    struct utimbuf ub;    
    
    if (!dir_or_file_exist("configure")) {
        return make_cmd(argc, argv);
    }

    if (hack_in_file("configure")) {
        return -MK_ERR_HACK;
    }

    if (stat("configure", &sb) == -1) {
        return -MK_ERR_SYSTEM;
    }

    if (cmd_execute_system_command2("chmod 777 configure &>/dev/null")) {
        return -MK_ERR_SYSTEM;
    }

    if (make_cmd(argc, argv)) {
        ret = -MK_ERR_SYSTEM;
        goto __rollback;
    }

__rollback:
    cmd_execute_system_command2("chmod 555 configure &>/dev/null");    
    ub.actime = sb.st_atime;
    ub.modtime = sb.st_mtime;
    if (utime("configure", &ub) == -1) {
        return -MK_ERR_SYSTEM;
    }
    return ret;
}

struct project_attr project_attrs[] = 
{
    {
         "fcms", 
         project_fcms_init_env,
         project_fcms_finit_env,
         NULL,
         NULL
    },
    {
         "themis.kernel", 
         project_themis_kernel_init_env,
         NULL,
         project_themis_kernel_install,
         NULL
    },
    {
         "library", 
         NULL,
         NULL,
         project_library_install,
         NULL
    },
    {
         "ntop", 
         project_ntop_init_env,
         NULL,
         project_ntop_install,
         NULL
    },
    {
         "iptables", 
         project_iptables_init_env,
         NULL,
         project_iptables_install,
         project_has_configure_compile
    },
    {
         "quagga", 
         project_quagga_init_env,
         NULL,
         project_quagga_install,
         project_has_configure_compile
    },
};

static char *
project_url(void *this, char *url)
{
    struct project_inf *pi = (struct project_inf *)this;
    
    strcpy(url, SVN_PREFIX);
    strcat(url, "/");
    strcat(url, pi->version->name);
    strcat(url, "/");
    strcat(url, pi->name);

    return url;
}

static char *
project_install_dir(void *this, char *dir)
{
    struct project_inf *pi = (struct project_inf *)this;
    
    strcpy(dir, DEST_DIR);
    strcat(dir, "/");
    strcat(dir, pi->version->name);
    strcat(dir, "/");
    strcat(dir, pi->name);

    return dir;
}

static int
project_refresh(void *this)
{
    struct project_inf *pi = (struct project_inf *)this;
    char url[MK_MAX_STR_LEN];
    
    if (svn_co_cmd(pi->url(pi, url)))
        return -MK_ERR_SVN;
    
    if (change_all_makefiles_ro(pi->name)) {
        return -MK_ERR_SYSTEM;
    }

    return 0;
}

static int
project_init(void *this, int refresh)
{
    int ret;
    struct project_inf *pi = (struct project_inf *)this;

    if (refresh || !dir_or_file_exist(pi->name)) {
        if ((ret = pi->refresh(pi)) != 0) {
            goto out;
        }
    }
    
    chdir(pi->name);
    
    if (!pi->attribute) {
        ret = 0;
        goto out;
    }

    ret = pi->attribute->init_env ? 
        pi->attribute->init_env(pi) : 0;

    if (ret) {
        chdir("..");
    }

out:
    return ret;
}

static void
project_finit(void *this)
{
    struct project_inf *pi = (struct project_inf *)this;
    
    chdir("..");
    
    if (pi->attribute && pi->attribute->finit_env) {
        pi->attribute->finit_env(pi);
    }
}

int
project_compile(void *this, int argc, char **argv)
{
    struct project_inf *pi = (struct project_inf *)this;

    if (pi->attribute && pi->attribute->compile2) {
        return pi->attribute->compile2(pi, argc, argv);
    }

    return make_cmd(argc, argv);
}

int
project_install(void *this, int argc, char **argv)
{
    struct project_inf *pi = (struct project_inf *)this;

    if (pi->attribute && pi->attribute->install2) {
        return pi->attribute->install2(pi, argc, argv);
    }

    return make_cmd(argc, argv);
}

int
project_clean(int argc, char **argv)
{
    return make_cmd(argc, argv);
}

void
project_free(struct project_inf *pi)
{
    if (pi->name)
        XFREE(MTYPE_PROJECT_NAME, pi->name);
        
    XFREE(MTYPE_PROJECT, pi);
}

void *
project_create_finish(struct project_inf *pi)
{
    int i;

    pi->init = project_init;
    pi->refresh = project_refresh;
    pi->compile = project_compile;
    pi->clean = project_clean;
    pi->finit = project_finit;
    pi->install_dir = project_install_dir;
    pi->url = project_url;
    
    for (i = 0; i < ARRAY_SIZE(project_attrs); i++) {
        if (!strcmp(pi->name, project_attrs[i].name)) {
            pi->attribute = &project_attrs[i];
            if (project_attrs[i].install2)
                pi->install = project_attrs[i].install2;
            break;
        }
    }

    if (!pi->install)
        pi->install = project_install;

    return pi;
}

void *
project_create(char *name, void *father)
{
    struct project_inf *pi;
    struct version_inf *vi = (struct version_inf *)father;

    pi = XCALLOC(MTYPE_PROJECT, sizeof(struct project_inf));

    pi->name = XSTRDUP(MTYPE_PROJECT_NAME, name);
    pi->version = vi;
    
    listnode_add(vi->projects, pi);

    return project_create_finish(pi);
}

struct project_inf *
project_find(char *name, struct version_inf *vi)
{
    struct listnode *nn;
    struct project_inf *pi;
    
    LIST_LOOP (vi->projects, pi, nn) {
        if (!strcmp(name, pi->name))
            return pi;
    }

    return NULL;
}

struct project_inf *
project_get(char *name, struct version_inf *vi)
{
    struct project_inf *pi;
    
    if ((pi = project_find(name, vi)) != NULL)
        return pi;

    if (!vi->has_project(vi, name))
        return NULL;

    return project_create(name, vi);
}

int
version_type(void *this)
{
    struct version_inf *vi = (struct version_inf *)this;
    
    if (strstr(VERSION_3602_ARGS, vi->arch)) {
        return MK_VERSION_3602;
    } else if (strstr(VERSION_3609_ARGS, vi->arch)) {
        return MK_VERSION_3609;
    } else {
        return MK_VERSION_NON;
    }
}

static char *
version_url(void *this, char *url)
{
    struct version_inf *vi = (struct version_inf *)this;
    
    strcpy(url, SVN_PREFIX);
    strcat(url, "/");
    strcat(url, vi->name);

    return url;
}

static char *
version_install_dir(void *this, char *dir)
{
    struct version_inf *vi = (struct version_inf *)this;
    
    strcpy(dir, DEST_DIR);
    strcat(dir, "/");
    strcat(dir, vi->name);

    return dir;
}

static char *
version_src_root_dir(void *this, char *dir)
{
    struct version_inf *vi = (struct version_inf *)this;
    
    strcpy(dir, ROOT_DIR);
    strcat(dir, "/");
    strcat(dir, vi->dir_name);

    return dir;
}

int
version_3602_refresh(void *this)
{
    struct version_inf *vi = (struct version_inf *)this;
    char arch[MK_ARCH_STR_LEN];
    char url[MK_MAX_STR_LEN];
    char cmd[MK_MAX_STR_LEN];
    char *install_dir;

    strcpy(arch, vi->arch);
    strcat(arch, ".os");
    vi->url(vi, url);
    strcat(url, "/");
    strcat(url, arch);

    if (svn_co_cmd(url)) {
        return -MK_ERR_SVN;
    }

    if (((install_dir = getenv("INSTALL_ROOT")) == NULL)) {
        return -MK_ERR_SYSTEM;
    }

    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "rsync -ua --exclude '.svn' %s/rootdir/* %s", arch, install_dir);
    
    return cmd_execute_system_command2(cmd);
}

void    
version_3602_finit_env(void *this)
{
    unsetenv("SRC_ROOT");
    unsetenv("TARGET_ROOT");
    unsetenv("INSTALL_ROOT");
    unsetenv("SNMPLIB");
    unsetenv("INCLUDEDIR");
    unsetenv("FW_PLATFORM");
    unsetenv("THEMIS_KERNEL_DIR");
}

int
version_3602_init_env(void *this)
{
    char dir[MK_MAX_STR_LEN];
    struct version_inf *vi = (struct version_inf *)this;

    setenv("SRC_ROOT", vi->src_root_dir(vi, dir), 1);
    strcat(dir, "/themis.kernel");
    setenv("THEMIS_KERNEL_DIR", dir, 1);
    setenv("TARGET_ROOT", vi->install_dir(vi, dir), 1);
    setenv("INSTALL_ROOT", dir, 1);
    setenv("SNMPLIB", "net-snmp", 1);
    setenv("INCLUDEDIR", "/opt/crossppc/powerpc-linux/usr/local/ssl/include", 1);

    if (!strcmp(vi->arch, "soho")) {
        setenv("FW_PLATFORM", "PPC_SMART", 1);
    } else {
        setenv("FW_PLATFORM", "IA86_POWER", 1);
    }

    return 0;
}

int
version_appctl_refresh(void *this)
{
    struct version_inf *vi = (struct version_inf *)this;
    char arch[MK_ARCH_STR_LEN] = "os.";
    char url[MK_MAX_STR_LEN];
    char cmd[MK_MAX_STR_LEN];
    char *install_dir;

    strcat(arch, vi->arch);
    vi->url(vi, url);
    strcat(url, "/");
    strcat(url, arch);

    if (svn_co_cmd(url)) {
        return -MK_ERR_SVN;
    }

    if (((install_dir = getenv("INSTALL_ROOT")) == NULL)) {
        return -MK_ERR_SYSTEM;
    }

    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "rsync -ua --exclude '.svn' %s/rootdir/* %s", arch, install_dir);
    
    return cmd_execute_system_command2(cmd);
}

int
version_appctl_init_env(void *this)
{
    char dir[MK_MAX_STR_LEN];
    char compile_root[MK_MAX_STR_LEN];
    char cflags[MK_MAX_STR_LEN] = "";
    char arch[MK_ARCH_STR_LEN];
    char platform[MK_ARCH_STR_LEN] = "PLATFORM_";
    struct version_inf *vi = (struct version_inf *)this;

    strncpy(arch, vi->arch, MK_ARCH_STR_LEN-1);
    arch[MK_ARCH_STR_LEN-1] = '\0';
    mk_string_to_upper(arch);
    strcat(platform, arch);
    
    setenv("GW_PLATFORM", platform, 1);
    setenv("SRC_ROOT", vi->src_root_dir(vi, compile_root), 1);
    
    strcat(compile_root, "/public/compile/");
    strcat(compile_root, vi->arch);
    setenv("COMPILE_ROOT", compile_root, 1);

    strcpy(dir, compile_root);
    strcat(dir, "/include");
    setenv("COMPILE_INCLUDE_DIR", dir, 1);
    
    strcpy(dir, compile_root);
    strcat(dir, "/lib");
    setenv("COMPILE_LIB_DIR", dir, 1);
    setenv("LD_LIBRARY_PATH", dir, 1);

    sprintf(cflags, 
        "-D%s -Wall -I%s/public/include -I%s/include -L%s/lib", 
        platform, vi->src_root_dir(vi, dir), 
        compile_root, compile_root);
    setenv("CFLAGS", cflags, 1);

    vi->install_dir(vi, dir);
    strcat(dir, "/install/");
    strcat(dir, vi->arch);
    setenv("TARGET_ROOT", dir, 1);
    setenv("INSTALL_ROOT", dir, 1);
    
    if (!strcmp(vi->arch, "arm_32")) {
        char *env;

        if ((env = getenv("PATH")) != NULL) {
            char path[MK_MAX_STR_LEN];

            strcpy(path, env);
            strcat(path, ":/opt/crossarm/bin");
            setenv("PATH", path, 1);
        }
        setenv("CROSS", "arm-mv5sft-linux-gnueabi-", 1);
        setenv("CROSS_HOST", "arm-mv5sft-linux-gnueabi", 1);
    }

    setenv("SNMPLIB", "net-snmp", 1);
    
    return 0;
}

void
version_appctl_finit_env(void *this)
{
    struct version_inf *vi = (struct version_inf *)this;

    if (!strcmp(vi->arch, "arm_32")) {
        char *env;
        
        if ((env = getenv("PATH")) != NULL) {
            char path[MK_MAX_STR_LEN];
            char *p;

            strcpy(path, env);
            if ((p = strstr(path, ":/opt/crossarm/bin")) != NULL) {
                *p = '\0';
                setenv("PATH", path, 1);
            }
        }
    }
    
    unsetenv("SRC_ROOT");
    unsetenv("TARGET_ROOT");
    unsetenv("INSTALL_ROOT");
    unsetenv("COMPILE_ROOT");
    unsetenv("COMPILE_INCLUDE_DIR");
    unsetenv("COMPILE_LIB_DIR");
    unsetenv("LD_LIBRARY_PATH");
    unsetenv("CFLAGS");
    unsetenv("CROSS");
    unsetenv("CROSS_HOST");
    unsetenv("GW_PLATFORM");
    unsetenv("SNMPLIB");
}

struct version_attr version_attrs[] = 
{
    {
         VERSION_2_6_BRANCHES
         "/Branch_3.6.0.2_Maintain_20131011", 
         version_3602_init_env,
         version_3602_refresh,
         version_3602_finit_env
    },
    {
         VERSION_2_6_BRANCHES
         "/Branch_AppControl_20140103", 
         version_appctl_init_env,
         version_appctl_refresh,
         version_appctl_finit_env
    },    
};

void
version_change_name(struct version_inf *vi)
{
    char *p;

    p = vi->name;
    while ((p = strchr(p, '+')) != NULL) {
        *p = '/';
    }

    p = vi->dir_name;
    while ((p = strchr(p, '/')) != NULL) {
        *p = '+';
    }
}

static int 
version_refresh(void *this)
{
    struct version_inf *vi = (struct version_inf *)this;
    char dir[MK_MAX_STR_LEN];
    
    vi->url(vi, dir);
    strcat(dir, "/"PROJECT_BUILD_NAME);
    
    if (svn_co_cmd(dir))
        return -MK_ERR_SVN;
    
    if (change_all_files_ro(PROJECT_BUILD_NAME))
        return -MK_ERR_SYSTEM;

    if (!vi->attribute)
        return 0;

    return vi->attribute->refresh ? 
        vi->attribute->refresh(vi) : 0;
}

int
version_init(void *this, char *arch)
{
    char dir[MK_MAX_STR_LEN] = ROOT_DIR;
    struct version_inf *vi = (struct version_inf *)this;

    strcat(dir, "/");
    strcat(dir, vi->dir_name);
    
    mkdir_cmd(dir);
    
    if (chdir(dir) < 0)
        return -1;
    
    if (vi->arch_invalid(arch))
        return -MK_ERR_VER_ARCH_INVAL;
    
    strncpy(vi->arch, arch, MK_ARCH_STR_LEN-1);
    vi->arch[MK_ARCH_STR_LEN-1] = '\0';

    mkdir_cmd(vi->install_dir(vi, dir));

    if (!vi->attribute)
        return 0;

    return vi->attribute->init_env ? 
        vi->attribute->init_env(vi) : 0;
}

void
version_finit(void *this)
{
    struct version_inf *vi = (struct version_inf *)this;

    if (vi->attribute && vi->attribute->finit_env) {
        vi->attribute->finit_env(vi);
    }
    
    vi->arch[0] = '\0';
    chdir("..");
}

int
version_has_project(void *this, char *name)
{
    struct version_inf *vi = (struct version_inf *)this;
    char url[MK_MAX_STR_LEN] = SVN_PREFIX;

    strcat(url, "/");
    strcat(url, vi->name);
    strcat(url, "/");
    strcat(url, name);
    
    return !svn_ls_cmd(url);
}

int
version_arch_invalid(char *arch)
{
    if (strstr(VERSION_3602_ARGS, arch)) {
        return cmd_execute_system_command2("grep '"VERSION_3602_ARGS"' build/fwbuild &>/dev/null");
    } else if (strstr(VERSION_3609_ARGS, arch)) {
        return cmd_execute_system_command2("grep '"VERSION_3609_ARGS"' build/fwbuild &>/dev/null");
    } else {
        return -MK_ERR_MAX;
    }
}

int
version_exists(char *name)
{
    char url[MK_MAX_STR_LEN] = SVN_PREFIX;

    strcat(url, "/");
    strcat(url, name);
    
    return !svn_ls_cmd(url);
}

void
version_free(struct version_inf *vi)
{
    if (vi->name)
        XFREE(MTYPE_VERSION_NAME, vi->name);
    
    if (vi->dir_name)
        XFREE(MTYPE_VERSION_NAME, vi->dir_name);
        
    list_delete(vi->projects);

    XFREE(MTYPE_VERSION, vi);
}

void *
version_create_finish(struct version_inf *vi)
{
    int i;

    vi->init = version_init;
    vi->finit = version_finit;
    vi->refresh = version_refresh;
    vi->has_project = version_has_project;
    vi->arch_invalid = version_arch_invalid;
    vi->url = version_url;
    vi->install_dir = version_install_dir;
    vi->src_root_dir = version_src_root_dir;
    vi->type = version_type;
    
    for (i = 0; i < ARRAY_SIZE(version_attrs); i++) {
        if (!strcmp(vi->name, version_attrs[i].name)) {
            vi->attribute = &version_attrs[i];
            break;
        }
    }

    return vi;
}

void *
version_create(char *name, void *father)
{
    struct version_inf *vi;
    struct list *vis = (struct list *)father;    

    vi = XCALLOC(MTYPE_VERSION, sizeof(struct version_inf));

    vi->name = XSTRDUP(MTYPE_VERSION_NAME, name);
    vi->dir_name = XSTRDUP(MTYPE_VERSION_NAME, name);

    vi->projects = list_new();
    vi->projects->del = (void (*) (void *))project_free;

    version_change_name(vi);
    
    listnode_add(vis, vi);
    
    return version_create_finish(vi);
}

struct version_inf *
version_find(char *name)
{
    struct listnode *nn;
    struct version_inf *vi;
    
    LIST_LOOP (versions, vi, nn) {
        if (!strcmp(name, vi->name))
            return vi;
    }

    return NULL;
}

struct version_inf *
version_get(char *name)
{
    struct version_inf *vi;
    
    if ((vi = version_find(name)) != NULL)
        return vi;

    if (!version_exists(name))
        return NULL;

    return version_create(name, versions);
}

void
versions_show_detail(struct vty *vty)
{
	struct listnode *nn;
    struct version_inf *vi;
    
	LIST_LOOP (versions, vi, nn) {
        struct listnode *nnn;
        struct project_inf *pi;
        
        vty_out(vty, "Version: %s\n", vi->name);
        
        LIST_LOOP (vi->projects, pi, nnn) {
            vty_out(vty, "  Project: %s\n", pi->name);
        }
    }
}

void
versions_show(struct vty *vty)
{
	struct listnode *nn;
    struct version_inf *vi;
    
    vty_out(vty, "Total versions: %u\n", versions->count);
    
	LIST_LOOP (versions, vi, nn) {
        vty_out(vty, "  projects: %-3u @ %s\n", 
            vi->projects->count, vi->name);
    }
}

void
projects_show(struct vty *vty, struct version_inf *vi)
{
	struct listnode *nn;
    struct project_inf *pi;
    
    vty_out(vty, "Total projects: %u\n", vi->projects->count);
    
	LIST_LOOP (vi->projects, pi, nn) {
        vty_out(vty, "    %s\n", pi->name);
    }
}

void *(*init_callbacks[])(char *, void *) = 
    {version_create, project_create, NULL};

int
for_all_dirs(const char *path, void *father, 
        int depth, void *(*callback[])(char *, void *))
{
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    if ((dp = opendir(path)) == NULL) {
        return -1;
    }

    /* cd path */
    chdir(path);

    while ((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name, &statbuf);
        if (S_ISDIR(statbuf.st_mode)) {
            void *son;
            
            if (entry->d_name[0] == '.')
                continue;

            if (!callback[depth]) {
                break;
            }
                
            if ((son = callback[depth](entry->d_name, 
                    father)) == NULL) {
                break;
            }

            for_all_dirs(entry->d_name, son, depth+1, init_callbacks);
        }
    }

    /* cd back */
    chdir("..");
    closedir(dp);

    return 0;
}

int 
all_versions_init(void)
{
    versions = list_new();
    versions->del = (void (*) (void *))version_free;

    for_all_dirs(ROOT_DIR, versions, 0, init_callbacks);

    return 0;
}

void 
all_versions_finit(void)
{
    list_delete(versions);
}

static struct cmd_node version_node =
{
  VERSION_NODE,
  "%s@%s(version)# ",
  1
};

static struct cmd_node project_node =
{
  PROJECT_NODE,
  "%s@%s(version-project)# ",
  1
};

static struct cmd_node pro_kernel_node =
{
  PRO_CONF_NODE,
  "%s@%s(version-project-conf)# ",
  1
};

int mk_init(void)
{
    cmd_install_node (&version_node, NULL);
    cmd_install_node (&project_node, NULL);
    cmd_install_node (&pro_kernel_node, NULL);

    all_versions_init();
    
    return 0;
}
