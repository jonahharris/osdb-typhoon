#ifndef TYPHOON_CATALOG_H
#define TYPHOON_CATALOG_H

/*---------- headerfile for catalog.ddl ----------*/
/* alignment is 4 */

/*---------- structures ----------*/
struct sys_dissite {  /* size 166 */
    unsigned long    id;
    long    last_call;
    unsigned short   tables;
    char    name[21];
    char    nua[17];
    char    udata_len;
    char    udata[16];
    char    up_to_date;
    char    in_error;
    char    spare[99];
};

struct sys_distab {  /* size 24 */
    long    id;
    long    site_id;
    long    last_seqno;
    char    tag;
    char    up_to_date;
    char    spare[10];
};

struct sys_distab_key {  /* size 8 */
    long    site_id;
    long    id;
};

struct sys_dischange {  /* size 53 */
    unsigned long    seqno;
    long    table_id;
    long    recno;
    unsigned short   sites;
    unsigned char    prog_id;
    char    action;
    unsigned char    site[37];
};

struct sys_dischange_key {  /* size 8 */
    long    table_id;
    long    recno;
};

struct sys_disprkey {  /* size 257 */
    unsigned short   size;
    unsigned char    buf[255];
};

struct sys_disfulltab {  /* size 12 */
    long    site_id;
    unsigned long    table_id;
    unsigned long    curr_rec;
};

struct fulltab_key {  /* size 8 */
    long    site_id;
    unsigned long    table_id;
};

/*---------- record names ----------*/
#define SYS_DISSITE 1000L
#define SYS_DISTAB 2000L
#define SYS_DISCHANGE 3000L
#define SYS_DISPRKEY 4000L
#define SYS_DISFULLTAB 5000L

/*---------- field names ----------*/
#define SYS_DISSITE_ID 1001L

/*---------- key constants ----------*/
#define SYS_DISTAB_KEY 1
#define SYS_DISCHANGE_KEY 2
#define FULLTAB_KEY 3

/*---------- integer constants ----------*/
#define SITES_MAX 296
#define SITEBITMAP_SIZE 37
#define SITENAME_LEN 20
#define NUA_LEN 16
#define UDATA_LEN 16
#define KEYSIZE_MAX 255

#endif
