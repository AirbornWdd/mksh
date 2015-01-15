#ifndef _ZEBRA_MK_H
#define _ZEBRA_MK_H

#define FTP_SERVER_DIR      "Bane_ftp"
#define SRC_DIR             "Bane/src"
#define ROOT_DIR            "/home/"SRC_DIR
#define DEST_DIR            "/home/"FTP_SERVER_DIR

#define SVN_PREFIX              "https://10.50.10.30/svn"
#define SVN_PREFIX_VERSION2_6   "2.6"
#define SVN_PREFIX_BRANCHES     "branches"

#define VERSION_2_6_BRANCHES    SVN_PREFIX_VERSION2_6"/"SVN_PREFIX_BRANCHES
#define VERSION_3602_ARGS       "(soho/themis/sag)"
#define VERSION_3609_ARGS       "(arm_32/x86_32/x86_64)"

#define PROJECT_BUILD_NAME      "build"

#define MK_MAX_STR_LEN          1024
#define MK_ARCH_STR_LEN         32

enum MK_ERROR_NUM {
    MK_ERR_VER_ARCH_INVAL = 100,
    MK_ERR_SVN,
    MK_ERR_SYSTEM,
    MK_ERR_OOM,
    MK_ERR_HACK,
    MK_ERR_MAX
};

enum MK_VERSION_NUM {
    MK_VERSION_3602,
    MK_VERSION_3609,
    MK_VERSION_NON
};

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

struct version_attr {
    char *name;
    int (*init_env)(void *);
    int (*refresh)(void *);
    void (*finit_env)(void *);
};

struct version_inf {
    char arch[MK_ARCH_STR_LEN];
    char *name;
    char *dir_name;
    int (*init)(void *, char *);
    void (*finit)(void *);
    int (*refresh)(void *);
    int (*has_project)(void *, char *);
    int (*arch_invalid)(char *);
    char *(*install_dir)(void *, char *);
    char *(*src_root_dir)(void *, char *);
    char *(*url)(void *, char *);
    int (*type)(void *);
    struct version_attr *attribute;
    struct list *projects;
};

struct project_attr {
    char *name;
    int (*init_env)(void *);
    void (*finit_env)(void *);
    int (*install2)(void *, int, char **);
    int (*compile2)(void *, int, char **);
};

struct project_inf {
    char *name;
    int (*init)(void *, int);
    int (*refresh)(void *);
    int (*compile)(void *, int, char **);
    int (*install)(void *, int, char **);
    int (*clean)(int, char **);
    void (*finit)(void *);
    char *(*install_dir)(void *, char *);
    char *(*url)(void *, char *);
    struct project_attr *attribute;
    struct version_inf *version;
    struct list *conf_args;
};

extern int mk_init(void);
extern int all_versions_init(void);
extern void all_versions_finit(void);
extern void versions_show(struct vty *vty);
extern void versions_show_detail(struct vty *vty);
extern void projects_show(struct vty * vty,struct version_inf * vi);
extern struct version_inf *version_find(char * name);
extern struct version_inf *version_get(char * name);
extern struct project_inf *project_find(char * name, struct version_inf *vi);
extern struct project_inf *project_get(char * name, struct version_inf *vi);
extern void mk_fix_name(char *name);
extern char *mk_strerror(int mk_errno);
extern int mkdir_cmd(char *dir);
extern int make_cmd(int argc, char **argv);
extern int dir_or_file_exist(char *name);
extern int hack_in_file(char *filename);
extern int illegal_patch_file(char *filename);

#endif /*_ZEBRA_MK_H*/
