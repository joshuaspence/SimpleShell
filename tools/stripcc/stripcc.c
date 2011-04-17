/**
 *  File: stripcc.c
 *                                                                          
 *  Copyright (C) 2007-2008 Du XiaoGang <dugang@188.com>                         
 *                                                                          
 *  -- Modified By harite <Harite.K@gmail.com> in 14/09/2007
 *         please read "CHANGELOG" to know what changed
 *  -- Modified By Du XiaoGang<dugang@188.com> in 20/09/2007
 *         please read "CHANGELOG" to know what changed
 *  -- Modified By Du XiaoGang<dugang@188.com> in 12/12/2008
 *         please read "CHANGELOG" to know what changed
 *
 *  This file is part of stripcc.                                          
 *                                                                          
 *  stripcc is free software: you can redistribute it and/or modify        
 *  it under the terms of the GNU General Public License as                 
 *  published by the Free Software Foundation, either version 3 of the      
 *  License, or (at your option) any later version.                         
 *                                                                          
 *  stripcc is distributed in the hope that it will be useful,             
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           
 *  GNU General Public License for more details.                            
 *                                                                          
 *  You should have received a copy of the GNU General Public License       
 *  along with stripcc.  If not, see <http://www.gnu.org/licenses/>.       
 */
/****************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "list.h"
#include "ringbuf.h"

#define CONF_FILE           "./stripcc.conf"
#define ERR_FILE            "./stripcc.err"
#define LINE_MAX            4096
#define PATH_MAX            4096
#define MAGIC               "eecbfb859094a362907dfb2f2cd3a8c8"
#define TMPFILE             "eecbfb859094a362907dfb2f2cd3a8c8.tmp"
#define CC_NEST_MAX         64
#define PHYLINE_COUNT_MAX   1024
#define RINGBUF_LEN         16384
#define GCC_LACK_MAIN       "undefined reference to `main'"
#define MAKE_LEAVE_DIR      "Leaving directory `"
#define MAKE_LEAVE_DIR_LEN  19
#define MAIN_FUNC           "int main(int argc, char *argv[]) {return 0;}"

#define ERRExit \
    do { \
        fprintf(stderr, "\033[31m  o \033[0mUnexpected error(%s: %d), errno = %d\n", \
                __FILE__, __LINE__, errno); \
        exit(1); \
    } while (0)
#define _ERRExit \
    do { \
        fprintf(stderr, "\033[31m  o \033[0mUnexpected error(%s: %d), errno = %d\n", \
                __FILE__, __LINE__, errno); \
        _exit(1); \
    } while (0)
#define OutOfMemory \
    do { \
        fprintf(stderr, "\033[31m  o \033[0mInsufficient memory.\n"); \
        exit(1); \
    } while (0)

static void
show_help(void)
{
    printf("Usage: stripcc <option(s)>\n"
           "Remove CCs(Conditional Compilation Branchs) which won't be compiled from c/c++ source files under current directory.\n"
           " The options are:\n"
           "  -c <str> Commands for build target program, \"make\" is default\n"
           "  -m <str> Directory to run build commands, current directory is default\n"
           "  -f       Working in fast mode (Experimental, NOT RECOMMENDED!)\n"
           "  -n       Don't verify after strip\n"
           "  -v       Display this program's version number\n"
           "  -h       Display this output\n");
}

static void
show_ver(void)
{
    printf("stripcc 0.2.0 20081212\n"
           "Copyright 2007-2008 Du XiaoGang <dugang@188.com>\n"
           "Modified By harite <harite.k@gmail.com>\n"
           "This program is free software; you may redistribute it under the terms of\n"
           "the GNU General Public License.  This program has absolutely no warranty.\n");
}

static void 
item_free(struct list_t *item)
{
    free(item->data);
    free(item);
}

static struct list_t *
_list_need_strip_files(struct list_t *exts, struct list_t *dirs, struct list_t *files, 
                       struct list_t *exc_dirs, struct list_t *exc_files)
{
    DIR *dir;
    struct dirent *entry;
    char path[PATH_MAX], *p;
    int ret, len, ever_found;
    struct stat st;
    struct list_t *item, *ext, *rets_tmp, *prev;

    struct list_t *rets = NULL;

    /* add dirs */
    while (dirs != NULL) {
        /* open directory */
        dir = opendir((const char *)dirs->data);
        if (dir == NULL) {
            fprintf(stderr, "\033[31m  o \033[0mFailed to open directory(%s), errno = %d.\n", 
                    (char *)dirs->data, errno);
            exit(1);
        }
        /* read directory */
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                /* "."/"..", skip */
                continue;
            }
            /* build path */
            p = (char *)dirs->data;
            if (p[strlen(p) - 1] == '/') {
                ret = snprintf(path, sizeof(path), "%s%s", (char *)dirs->data, 
                               entry->d_name);
            } else {
                ret = snprintf(path, sizeof(path), "%s/%s", (char *)dirs->data, 
                               entry->d_name);
            }
            if (ret >= sizeof(path)) {
                fprintf(stderr, "\033[31m  o \033[0mPath or file name is too long.\n");
                exit(1);
            }
            if (lstat(path, &st) == -1)
                ERRExit;
            /* subdir or source file */
            if (S_ISDIR(st.st_mode)) {
                /* process later */
                item = malloc(sizeof(struct list_t));
                if (item == NULL)
                    OutOfMemory;
                item->data = strdup(path);
                if (item->data == NULL)
                    OutOfMemory;
                /* insert item after dirs */
                item->next = dirs->next;
                dirs->next = item;
            } else if (S_ISREG(st.st_mode)) {
                /* need strip? */
                p = strrchr(path, '.');
                if (p == NULL)
                    continue;
                ext = exts;
                while (ext != NULL) {
                    if (strcmp(p, (char *)ext->data) == 0)
                        break;
                    /* next */
                    ext = ext->next;
                }
                if (ext == NULL)
                    continue;
                /* new item */
                item = malloc(sizeof(struct list_t));
                if (item == NULL)
                    OutOfMemory;
                item->data = strdup(path);
                if (item->data == NULL)
                    OutOfMemory;
                /* insert to rets */
                rets_tmp = sorted_list_insert_item(rets, item, (data_cmp_func)strcmp);
                if (rets_tmp == NULL) {
                    /* item is already exist */
                    free(item->data);
                    free(item);
                } else {
                    rets = rets_tmp;
                }
            }
        }
        closedir(dir);
        /* next */
        dirs = dirs->next;
    }

    /* remove exc_dirs */
    while (exc_dirs != NULL) {
        len = strlen((char *)exc_dirs->data);
        /* small --> big */
        item = rets;
        prev = NULL;
        ever_found = 0;
        while (item != NULL) {
            ret = strncmp((char *)item->data, (char *)exc_dirs->data, len);
            if (ret == 0 && ((char *)item->data)[len] == '/') {
                ever_found = 1;
                /* remove */
                if (prev == NULL) {
                    rets = item->next;
                    item_free(item);
                    /* next */
                    item = rets;
                    continue;
                } else {
                    prev->next = item->next;
                    item_free(item);
                    /* next */
                    item = prev->next;
                    continue;
                }
            } else {
                if (ever_found) {
                    /* rets is a sorted list */
                    break;
                }
            }
            /* next */
            prev = item;
            item = item->next;
        }
        /* next */
        exc_dirs = exc_dirs->next;
    }

    /* add files */
    while (files != NULL) {
        /* new item */
        item = malloc(sizeof(struct list_t));
        if (item == NULL)
            OutOfMemory;
        item->data = strdup((char *)files->data);
        if (item->data == NULL)
            OutOfMemory;
        /* insert to rets */
        rets_tmp = sorted_list_insert_item(rets, item, (data_cmp_func)strcmp);
        if (rets_tmp == NULL) {
            /* item is already exist */
            free(item->data);
            free(item);
        } else {
            rets = rets_tmp;
        }
        /* next */
        files = files->next;
    }

    /* remove exc_files */
    while (exc_files != NULL) {
        /* remove */
        rets = sorted_list_remove_and_free_item_by_data(rets, 
                                                        exc_files->data, 
                                                        (data_cmp_func)strcmp, 
                                                        item_free);
        /* next */
        exc_files = exc_files->next;
    }

    return rets;
}

static struct list_t *
list_need_strip_files(void)
{
    FILE *fp;
    int lineno;
    char linebuf[LINE_MAX], *p, *q;
    struct list_t *item, *rets;

    struct list_t exts_c = {NULL, ".c"}, exts_h = {&exts_c, ".h"}, dir_cwd = {NULL, "."};
    enum {
        none, 
        strip_exts, 
        strip_dirs, 
        strip_files, 
        dont_strip_dirs, 
        dont_strip_files
    } cur_section = none;
    struct list_t *exts = NULL, *dirs = NULL, *files = NULL, *exc_dirs = NULL, 
                  *exc_files = NULL;

    fp = fopen(CONF_FILE, "r");
    if (fp == NULL) {
        fprintf(stderr, "\033[31m  o \033[0mFailed to open config file(%s), use the default values.\n", 
                CONF_FILE);
        return _list_need_strip_files(&exts_h, &dir_cwd, NULL, NULL, NULL);
    }

    /* parse config file */
    lineno = 0;
    while (fgets(linebuf, sizeof(linebuf), fp) != NULL) {
        lineno++;
        /* line's length check */
        if (strlen(linebuf) == sizeof(linebuf) - 1 
            && linebuf[sizeof(linebuf) - 2] != '\n') 
        {
            fprintf(stderr, "\033[31m  o \033[0mThe config line(%s: %d) is too long to parse.\n", 
                    CONF_FILE, lineno);
            exit(1);
        }
        /* empty line? */
        p = linebuf;
        p += strspn(p, " \t");
        if (*p == '\r' || *p == '\n' || *p == '\0')
            continue;
        /* comments line? */
        if (*p == '#')
            continue;
        /* section header or data line */
        if (*p == '[') {
            /* section header line */
            q = strpbrk(p, " \t\r\n");
            if (q != NULL)
                *q = '\0';
            if (strcmp(p, "[strip_exts]") == 0) {
                cur_section = strip_exts;
            } else if (strcmp(p, "[strip_dirs]") == 0) {
                cur_section = strip_dirs;
            } else if (strcmp(p, "[strip_files]") == 0) {
                cur_section = strip_files;
            } else if (strcmp(p, "[dont_strip_dirs]") == 0) {
                cur_section = dont_strip_dirs;
            } else if (strcmp(p, "[dont_strip_files]") == 0) {
                cur_section = dont_strip_files;
            }
        } else {
            /* data line */
            /* right trim */
            q = p + strlen(p);
            do {
                q--;
            } while (*q == ' ' || *q == '\t' || *q == '\r' || *q == '\n');
            *(++q) = '\0';
            /* new list item */
            item = malloc(sizeof(struct list_t));
            if (item == NULL)
                OutOfMemory;
            item->data = strdup(p);
            if (item->data == NULL)
                OutOfMemory;
            /* insert into list */
            if (cur_section == strip_exts) {
                item->next = exts;
                exts = item;
            } else if (cur_section == strip_dirs) {
                item->next = dirs;
                dirs = item;
            } else if (cur_section == strip_files) {
                item->next = files;
                files = item;
            } else if (cur_section == dont_strip_dirs) {
                item->next = exc_dirs;
                exc_dirs = item;
            } else if (cur_section == dont_strip_files) {
                item->next = exc_files;
                exc_files = item;
            }
        }
    }
    fclose(fp);

    rets = _list_need_strip_files(exts, dirs, files, exc_dirs, exc_files);
    list_free(exts, item_free);
    list_free(dirs, item_free);
    list_free(files, item_free);
    list_free(exc_dirs, item_free);
    list_free(exc_files, item_free);
    return rets;
}

static void
backup_files(struct list_t *file_list)
{
    int ret;
    char path[PATH_MAX];

    /* for each file */
    while (file_list != NULL) {
        /* backup */
        ret = snprintf(path, sizeof(path), "%s.%s", (char *)file_list->data, MAGIC);
        if (ret >= sizeof(path)) {
            fprintf(stderr, "\033[31m  o \033[0mPath or file name is too long.\n");
            exit(1);
        }
        unlink(path);
        if (link((char *)file_list->data, path) == -1)
            ERRExit;
        /* next */
        file_list = file_list->next;
    }
}

struct logiline_t {
    char *linebuf;
    size_t linebuf_size;
    size_t linebuf_idx;
    size_t phyline_count;
    size_t phyline_idx[PHYLINE_COUNT_MAX];
};

static int
is_unterminate_line(const char *line)
{
    int i, len;

    /* the line must be terminated by '\n' */
    len = strlen(line);
    if (line[len - 1] != '\n')
        return 0;
    for (i = len - 2; i >= 0; i--) {
        if (line[i] == '\\') {
            return 1;
        } else if (line[i] == ' ' || line[i] == '\t' || line[i] == '\r') {
            /* gcc extends */
            continue;
        } else {
            return 0;
        }
    }
    return 0;
}

#define MALLOC_STEP 16384
static struct logiline_t *
build_logiline(struct logiline_t *logiline, const char *phyline)
{
    int len, add;

    if (logiline == NULL) {
        /* create */
        logiline = malloc(sizeof(struct logiline_t));
        if (logiline == NULL)
            OutOfMemory;
        /* alloc linebuf */
        logiline->linebuf_size = MALLOC_STEP;
        len = strlen(phyline);
        while (logiline->linebuf_size <= len)
            logiline->linebuf_size += MALLOC_STEP;
        logiline->linebuf = malloc(logiline->linebuf_size);
        if (logiline->linebuf == NULL)
            OutOfMemory;
        strcpy(logiline->linebuf, phyline);
        logiline->linebuf_idx = len;
        logiline->phyline_count = 1;
        logiline->phyline_idx[0] = 0;
    } else {
        /* join */
        len = strlen(phyline);
        add = 0;
        /* check free space */
        while (logiline->linebuf_size + add - logiline->linebuf_idx <= len)
            add += MALLOC_STEP;
        if (add > 0) {
            logiline->linebuf = realloc(logiline->linebuf, 
                                        logiline->linebuf_size + add);
            if (logiline->linebuf == NULL)
                OutOfMemory;
            logiline->linebuf_size += add;
        }
        /* join */
        strcat(logiline->linebuf, phyline);
        logiline->phyline_count++;
        if (logiline->phyline_count == PHYLINE_COUNT_MAX) {
            fprintf(stderr, "\033[31m  o \033[0mToo many physical line in a logical line.\n");
            exit(1);
        }
        logiline->phyline_idx[logiline->phyline_count - 1] = logiline->linebuf_idx;
        logiline->linebuf_idx += len;
    }
    return logiline;
}

static int
is_in_quote(/* const */char *line, char *pos)
{
    char c, *p, *q, *r;
    int quotes_num = 0;

    /* replace the end */
    c = *pos;
    *pos = '\0';

    p = line;
    while (1) {
        q = strchr(p, '"');
        if (q == NULL)
            break;
        if (q == p) {
            quotes_num++;
            /* search next quotes */
            p = q + 1;
        } else {
            /* q > p */
            if (q[-1] == '\\') {
                /* \" */
                r = &q[-1];
                while (r >= p && *r == '\\')
                    r--;
                if ((q - 1 - r) % 2 == 0)
                    quotes_num++;
                p = q + 1;
            } else if (q[-1] == '\'' && q[1] == '\'') {
                /* bug? \'"' */
                p = q + 2;
            } else {
                quotes_num++;
                /* search next quotes */
                p = q + 1;
            }
        }
    }
    /* search complete */
    *pos = c;
    if (quotes_num % 2 == 0) {
        return 0;
    } else {
        return 1;
    }
}

static char *
find_first_quote(const char *str)
{
    char *p, *q, *r;

    p = (char*)str;
    while (1) {
        q = strchr(p, '"');
        if (q == NULL)
            return NULL;
        if (q == p) {
            return q;
        } else {
            /* q > p */
            if (q[-1] == '\\') {
                /* \" */
                r = &q[-1];
                while (r >= p && *r == '\\')
                    r--;
                if ((q - 1 - r) % 2 == 0)
                    return q;
                p = q + 1;
            } else if (q[-1] == '\'' && q[1] == '\'') {
                p = q + 2;
            } else {
                return q;
            }
        }
    }
    return NULL;
}

/* source line type */
#define NOTCC               0
#define NOTCC_MAIN          1
#define IFCC                2
#define ELIFCC              3
#define ELSECC              4
#define ENDCC               5
#define OTHERCC             6

static int
get_line_type(const char *line)
{
    const char *p;

    /* search "[ \t]*#" */
    p = line;
    p += strspn(p, " \t");
    if (*p == '#') {
        p++;
        p += strspn(p, " \t");
        if (strncmp(p, "if", 2) == 0) {
            /* "[ \t]*#[ \t]*if" */
            return IFCC;
        } else if (strncmp(p, "elif", 4) == 0) {
            /* "[ \t]*#[ \t]*elif" */
            return ELIFCC;
        } else if (strncmp(p, "else", 4) == 0) {
            /* "[ \t]*#[ \t]*else" */
            return ELSECC;
        } else if (strncmp(p, "endif", 5) == 0) {
            /* "[ \t]*#[ \t]*endif" */
            return ENDCC;
        } else {
            return OTHERCC;
        }
    }
    /* have "main"? */
    p = strstr(line, "main");
    if (p == NULL)
        return NOTCC;
    if (p > line && !isspace(p[-1])) 
        return NOTCC;
    p += 4;
    p += strspn(p, " \t\r\n");
    if (*p != '(' && *p != '\0')
        return NOTCC;
    return NOTCC_MAIN;
}

static void
fputs2(const char *s, FILE *stream)
{
    if(fputs(s, stream) == EOF)
        ERRExit;
}

static void
logiline_free(struct logiline_t *logiline)
{
    free(logiline->linebuf);
    free(logiline);
}

static void
discovered_main(struct list_t **main_list, const char *path)
{
    struct list_t *item, *tmp_list;

    item = malloc(sizeof(struct list_t));
    if (item == NULL)
        OutOfMemory;
    item->data = strdup(path);
    if (item->data == NULL)
        OutOfMemory;
    /* insert */
    tmp_list = sorted_list_insert_item(*main_list, item, (data_cmp_func)strcmp);
    if (tmp_list == NULL) {
        /* item is already exist */
        free(item->data);
        free(item);
    } else {
        *main_list = tmp_list;
    }
}

static int
add_warning_cc_to_files(struct list_t *file_list, int fast_mode, 
                        struct list_t **invalid_file_list, struct list_t **main_list)
{
    FILE *sfp, *tfp;
    struct logiline_t *logiline;
    char *p, *q, *r, *s, *comment_begin, linebuf[LINE_MAX], *srcfile;
    int cc_type, dont_output, comment_begin_in_this_line, whole_file_cc_id,
        cc_id[CC_NEST_MAX], cc_branch[CC_NEST_MAX], have_else[CC_NEST_MAX], 
        cur_nest, lineno, is_in_comment, invalid_file, warning_cc_id = 0;
    struct list_t *item;

    while (file_list != NULL) {
        /* initial setup */
        comment_begin = NULL;
        cur_nest = 0;
        lineno = 0;
        is_in_comment = 0;
        invalid_file = 0;
        
        /* open source and temprary file */
        srcfile = (char *)file_list->data;
        sfp = fopen(srcfile, "r");
        if (sfp == NULL) {
            fprintf(stderr, "\033[31m  o \033[0mFailed to open the source file(%s), errno = %d.\n", 
                    srcfile, errno);
            exit(1);
        }
        tfp = fopen(TMPFILE, "w");
        if (tfp == NULL) {
            fprintf(stderr, "\033[31m  o \033[0mFailed to open the temprary file(%s), errno = %d.\n", 
                    TMPFILE, errno);
            exit(1);
        }

        /* add a whole file CC(#if) */
        cc_id[cur_nest] = warning_cc_id++;
        whole_file_cc_id = cc_id[cur_nest];
        cc_branch[cur_nest] = 0;
        have_else[cur_nest] = 0;
        if (fprintf(tfp, "#if 1\n") < 0)
            ERRExit;
        if (fprintf(tfp, "#warning %s_%d_%d\n", MAGIC, cc_id[cur_nest], 
                    cc_branch[cur_nest]) < 0) 
        {
            ERRExit;
        }

        /* parse the source file line by line */
        while (fgets(linebuf, sizeof(linebuf), sfp) != NULL) {
            logiline = NULL;
            lineno++;
            cc_type = NOTCC;
            /* check line length */
            if (strlen(linebuf) == sizeof(linebuf) - 1 
                && linebuf[sizeof(linebuf) - 2] != '\n') 
            {
                fprintf(stderr, "\033[31m  o \033[0mThe source line(%s: %d) is too long to parse.\n", 
                        srcfile, lineno);
                invalid_file = 1;
                goto invalid_file;
            }
            /* build a logical line */
            while (is_unterminate_line(linebuf)) {
                /* unterminated line */
                p = strrchr(linebuf, '\\');
                *p = '\0';
                logiline = build_logiline(logiline, linebuf);
                if (fgets(linebuf, sizeof(linebuf), sfp) == NULL) {
                    fprintf(stderr, "\033[31m  o \033[0mUnterminated line(%s: %d).\n", srcfile, lineno);
                    invalid_file = 1;
                    goto invalid_file;
                }
                lineno++;
                /* check line length */
                if (strlen(linebuf) == sizeof(linebuf) - 1
                    && linebuf[sizeof(linebuf) - 2] != '\n')
                {
                    fprintf(stderr, "\033[31m  o \033[0mThe source line(%s: %d) is too long to parse.\n", 
                            srcfile, lineno);
                    invalid_file = 1;
                    goto invalid_file;
                }
            }
            /* process source line */
            if (logiline != NULL) {
                /* add terminated line */
                logiline = build_logiline(logiline, linebuf);
                p = logiline->linebuf;
            } else {
                p = linebuf;
            }
            /* the begin of the comments in this line? */
            if (is_in_comment) 
                comment_begin_in_this_line = 0;
            else 
                comment_begin_in_this_line = 1;
            dont_output = 0;
            while (1) {
                /* now in comments? */
                if (is_in_comment) {
                    // search for "*/"
                    q = strstr(p, "*/");
                    if (q != NULL) {
                        /* found, remove the comments */
                        memmove(p, q + 2, strlen(q + 2) + 1);
                        is_in_comment = 0;
                        continue;
                    } else {
                        /* not found */
                        if (comment_begin_in_this_line) {
                            /* the begin of comments in this line */
                            /* bug? '\n' is missing */
                            comment_begin[0] = ' ';
                            comment_begin[1] = '\0';
                            /* search #xxx */
                            if (logiline != NULL) {
                                cc_type = get_line_type(logiline->linebuf);
                            } else {
                                cc_type = get_line_type(linebuf);
                            }
                            /* include main? */
                            if (cc_type == NOTCC_MAIN)
                                discovered_main(main_list, srcfile);
                        } else {
                            /* all line in comments, ignore it */
                            dont_output = 1;
                        }
                        break;
                    }
                } else {
                    /* search for "//" */
                    q = p;
                    while (1) {
                        r = strstr(q, "//");
                        if (r == NULL)
                            break;
                        if (is_in_quote(q, r)) {
                            /* in quote */
                            q = find_first_quote(r + 2);
                            if (q == NULL) {
                                /* invalid file, we'd better ignore it */
                                invalid_file = 1;
                                goto invalid_file;
                            } else {
                                q++;
                            }
                        } else {
                            break;
                        }
                    }
                    // search for "/*"
                    q = p;
                    while (1) {
                        s = strstr(q, "/*");
                        if (s == NULL)
                            break;
                        if (is_in_quote(q, s)) {
                            /* in quote */
                            q = find_first_quote(s + 2);
                            if (q == NULL) {
                                /* invalid file, we'd better ignore it */
                                invalid_file = 1;
                                goto invalid_file;
                            } else {
                                q++;
                            }
                        } else {
                            break;
                        }
                    }
                    /* compare */
                    if (r != NULL && s != NULL) {
                        if (r < s)
                            s = NULL;
                        else 
                            r = NULL;
                    }
                    if (r != NULL) {
                        /* "//" */
                        r[0] = ' ';
                        r[1] = '\n';
                        r[2] = '\0';
                        /* search #xxx */
                        if (logiline != NULL) {
                            cc_type = get_line_type(logiline->linebuf);
                        } else {
                            cc_type = get_line_type(linebuf);
                        }
                        /* include main? */
                        if (cc_type == NOTCC_MAIN)
                            discovered_main(main_list, srcfile);
                    } else if (s != NULL) {
                        // "/*"
                        s[0] = ' ';
                        s[1] = ' ';
                        p = &s[2];
                        comment_begin_in_this_line = 1;
                        comment_begin = &s[0];
                        is_in_comment = 1;
                        continue;
                    } else {
                        /* no comments, search #xxx */
                        if (logiline != NULL) {
                            cc_type = get_line_type(logiline->linebuf);
                        } else {
                            cc_type = get_line_type(linebuf);
                        }
                        /* include main? */
                        if (cc_type == NOTCC_MAIN)
                            discovered_main(main_list, srcfile);
                    }
                    break;
                }
            }
            /* process depends on line type */
            if (cc_type != ENDCC) {
                /* output source line */
                if (logiline != NULL) {
                    if (!dont_output) {
                        if (!fast_mode || (cc_type != NOTCC && cc_type != NOTCC_MAIN)) {
                            /* don't output normal line in fast mode */
                            fputs2(logiline->linebuf, tfp);
                        }
                    }
                    logiline_free(logiline);
                } else {
                    if (!dont_output) {
                        if (!fast_mode || (cc_type != NOTCC && cc_type != NOTCC_MAIN)) {
                            /* don't output normal line in fast mode */
                            fputs2(linebuf, tfp);
                        }
                    }
                }
            }
            /* process cc */
            if (cc_type == IFCC) {
                /* #if*** */
                cur_nest++;
                if (cur_nest >= CC_NEST_MAX) {
                    fprintf(stderr, "\033[31m  o \033[0mThe source file(%s) contains too many nesting CC.\n",
                            srcfile);
                    invalid_file = 1;
                    goto invalid_file;
                }
                cc_id[cur_nest] = warning_cc_id++;
                cc_branch[cur_nest] = 0;
                have_else[cur_nest] = 0;
                if (fprintf(tfp, "\n#warning %s_%d_%d\n", MAGIC, cc_id[cur_nest], 
                            cc_branch[cur_nest]) < 0) 
                {
                    ERRExit;
                }
            } else if (cc_type == ELIFCC) {
                /* #elif */
                cc_branch[cur_nest]++;
                if (fprintf(tfp, "\n#warning %s_%d_%d\n", MAGIC, cc_id[cur_nest], 
                            cc_branch[cur_nest]) < 0) 
                {
                    ERRExit;
                }
            } else if (cc_type == ELSECC) {
                /* #else */
                cc_branch[cur_nest]++;
                have_else[cur_nest] = 1;
                if (fprintf(tfp, "\n#warning %s_%d_%d\n", MAGIC, cc_id[cur_nest], 
                            cc_branch[cur_nest]) < 0) 
                {
                    ERRExit;
                }
            } else if (cc_type == ENDCC) {
                /* #endif */
                if (!have_else[cur_nest]) {
                    /* no #else in this CCs, add one */
                    cc_branch[cur_nest]++;
                    if (fprintf(tfp, "\n#else\n") < 0)
                        ERRExit;
                    if (fprintf(tfp, "\n#warning %s_%d_%d\n", MAGIC, cc_id[cur_nest], 
                                cc_branch[cur_nest]) < 0) 
                    {
                        ERRExit;
                    }
                }
                cur_nest--;
                if (cur_nest < 0) {
                    fprintf(stderr, "\033[31m  o \033[0mInvalid source line(%s: %d).\n", srcfile, lineno);
                    invalid_file = 1;
                    goto invalid_file;
                }
                /* output source line */
                if (logiline != NULL) {
                    if (!dont_output) {
                        if (!fast_mode || (cc_type != NOTCC && cc_type != NOTCC_MAIN)) {
                            /* don't output normal line in fast mode */
                            fputs2(logiline->linebuf, tfp);
                        }
                    }
                    logiline_free(logiline);
                } else {
                    if (!dont_output) {
                        if (!fast_mode || (cc_type != NOTCC && cc_type != NOTCC_MAIN)) {
                            /* don't output normal line in fast mode */
                            fputs2(linebuf, tfp);
                        }
                    }
                }
            }
        }

invalid_file:
        if (invalid_file) {
            /* prepend to rets */
            item = malloc(sizeof(struct list_t));
            if (item == NULL)
                OutOfMemory;
            item->data = strdup(srcfile);
            if (item->data == NULL)
                OutOfMemory;
            /* prepend */
            item->next = *invalid_file_list;
            *invalid_file_list = item;
            /* reset */
            fclose(tfp);
            cur_nest = 0;
            /* open a new temprary file */
            tfp = fopen(TMPFILE, "w");
            if (tfp == NULL) {
                fprintf(stderr, "\033[31m  o \033[0mFailed to open the temprary file(%s), errno = %d.\n", 
                        TMPFILE, errno);
                exit(1);
            }
            /* add a whole file CC(#if) */
            warning_cc_id = whole_file_cc_id;
            cc_id[cur_nest] = warning_cc_id++;
            cc_branch[cur_nest] = 0;
            have_else[cur_nest] = 0;
            if (fprintf(tfp, "#if 1\n") < 0)
                ERRExit;
            if (fprintf(tfp, "#warning %s_%d_%d\n", MAGIC, cc_id[cur_nest], 
                        cc_branch[cur_nest]) < 0) 
            {
                ERRExit;
            }
        }

        /* add a whole file CC(#endif) */
        /* the first '\n' is for source file which 
           dose not terminated by newline('\n') */
        if (fprintf(tfp, "\n#endif\n") < 0)
            ERRExit;

        /* close */
        fclose(sfp);
        fclose(tfp);
        /* replace the source file */
        if (rename(TMPFILE, srcfile) == -1)
            ERRExit;

        /* next */
        file_list = file_list->next;
    }
    return warning_cc_id;
}

static int
set_nonblocking(int fd)
{
    int flag;

    flag = fcntl(fd, F_GETFL);
    if (flag == -1)
        return -1;
    if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1) 
        return -1;
    return 0;
}

static int
is_lack_main(struct ringbuf_t *stderr_rbuf)
{
    char buf[4096], *p;
    int len;

    /* for the last sizeof(buf) in stderr_rbuf */
    len = ringbuf_read_last(stderr_rbuf, buf, sizeof(buf) - 1);
    buf[len] = '\0';
    /* parse line by line, from end to begin */
    while (1) {
        p = strrchr(buf, '\n');
        if (p == NULL) {
            /* the begin line */
            break;
        }
        if (strstr(p + 1, GCC_LACK_MAIN) != NULL) {
            return 1;
        } else {
            /* keep searching */
            *p = '\0';
        }
    }
    if (strstr(buf, GCC_LACK_MAIN) != NULL)
        return 1;
    return 0;
}

static struct list_t *
in_main_list(const char *obj_file, const char *cwd, int cwd_len, const char *leave_dir, 
             int leave_dir_len, struct list_t *main_list)
{
    char rel_path[PATH_MAX];
    int len;

    strcpy(rel_path, "./");
    if (*leave_dir != '\0') {
        if (strncmp(leave_dir, cwd, cwd_len) != 0)
            return NULL;
        if (leave_dir_len > cwd_len 
            && leave_dir_len - cwd_len < sizeof(rel_path) - strlen(rel_path)) 
        {
            strcat(rel_path, leave_dir + cwd_len + 1);
            strcat(rel_path, "/");
        } else if (leave_dir_len < cwd_len) {
            return NULL;
        }
    }
    /* append obj_file */
    len = strlen(obj_file);
    if (len < sizeof(rel_path) - strlen(rel_path)) {
        strcat(rel_path, obj_file);
    } else {
        return NULL;
    }
    /* .o to .c */
    rel_path[strlen(rel_path) - 1] = 'c';
    /* is it match? */
    while (main_list != NULL) {
        if (strcmp((char *)main_list->data, rel_path) == 0)
            return main_list;
        /* next */
        main_list = main_list->next;
    }
    return NULL;
}

static struct list_t *
guess_main_file(struct ringbuf_t *stdout_rbuf, struct list_t *main_list)
{
    char *p, *q, *r, cwd[PATH_MAX], buf[16384], leave_dir[PATH_MAX], *begin;
    int len, cwd_len, leave_dir_len, found_o = 0;
    struct list_t *rets;

    /* get current working dir */
    if (getcwd(cwd, sizeof(cwd) - 1) == NULL)
        ERRExit;
    cwd[sizeof(cwd) - 1] = '\0';
    cwd_len = strlen(cwd);

    /* for the last sizeof(buf) in stdout_rbuf */
    len = ringbuf_read_last(stdout_rbuf, buf, sizeof(buf) - 1);
    buf[len] = '\0';

    /* parse line by line, from end to begin */
    leave_dir[0] = '\0';
    leave_dir_len = 0;
    while (1) {
        p = strrchr(buf, '\n');
        if (p == NULL) {
            /* the begin line */
            break;
        }
        /* gcc "Leaving directory"? */
        q = strstr(p + 1, MAKE_LEAVE_DIR);
        if (q != NULL) {
            q += MAKE_LEAVE_DIR_LEN;
            r = strrchr(q, '\'');
            if (r != NULL && r - q < sizeof(leave_dir)) {
                memcpy(leave_dir, q, r - q);
                leave_dir[r - q] = '\0';
                leave_dir_len = r - q;
            }
            /* next line */
            *p = '\0';
            continue;
        }
        /* include *.o? */
        q = strstr(p + 1, ".o");
        if (q != NULL) {
            if (isspace(q[2]) || q[2] == '\0') {
                found_o = 1;
                begin = p + 1;
                break;
            }
        } else {
            /* keep searching */
            *p = '\0';
        }
    }
    /* found? */
    if (!found_o) {
        /* the begin line */
        q = strstr(buf, ".o");
        if (q != NULL) {
            if (isspace(q[2]) || q[2] == '\0') {
                found_o = 1;
                begin = buf;
            }
        }
        if (!found_o) {
            /* not found */
            return NULL;
        }
    }

    /* found, for each *.o in this line */
    p = begin;
    while (1) {
        p += strspn(p, " \t\r");
        q = strpbrk(p, " \t\r");
        if (q == NULL) {
            len = strlen(p);
            if (len >= 3 && p[len - 2] == '.' && p[len - 1] == 'o') {
                rets = in_main_list(p, cwd, cwd_len, leave_dir, leave_dir_len, main_list);
                if (rets != NULL)
                    return rets;
            }
            break;
        } else {
            if (q - p >= 3 && q[-2] == '.' && q[-1] == 'o') {
                *q = '\0';
                rets = in_main_list(p, cwd, cwd_len, leave_dir, leave_dir_len, main_list);
                if (rets != NULL)
                    return rets;
            }
        }
        /* next */
        p = q + 1;
    }
    
    return NULL;
}

static int
add_main_function(const char *file)
{
    FILE *fp;

    fp = fopen(file, "a");
    if (fp == NULL)
        return -1;
    if (fprintf(fp, "%s\n", MAIN_FUNC) < 0) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}

static void
rm_object_file(/* const */char *file)
{
    int len;

    len = strlen(file);
    file[len - 1] = 'o';
    unlink(file);
    file[len - 1] = 'c';
}

static void
dump_error_info(const char *file, struct ringbuf_t *stdout_rbuf, 
                struct ringbuf_t *stderr_rbuf)
{
    FILE *fp;
    char buf[4096];
    int len;

    fp = fopen(file, "w");
    if (fp == NULL)
        ERRExit;
    fputs2("stdout:\n\n", fp);
    len = ringbuf_read_last(stdout_rbuf, buf, sizeof(buf) - 1);
    if (len > 0) {
        buf[len] = '\0';
        fputs2(buf, fp);
    }
    fputs2("\nstderr:\n\n", fp);
    len = ringbuf_read_last(stderr_rbuf, buf, sizeof(buf) - 1);
    if (len > 0) {
        buf[len] = '\0';
        fputs2(buf, fp);
    }
    fclose(fp);
}

#define F_UNKNOWN       0
#define F_USED          (1 << 0)
#define F_RESERVECC     (1 << 1)
#define F_IFCC_OUTPUTED    (1 << 2)
struct cc_used_t {
    int flags;
    struct list_t *used_branch; 
};

static struct cc_used_t *
try_build_and_parse(int fast_mode, const char *comp_cmd, const char *comp_dir, int ncc, 
                    struct list_t **main_list)
{
    struct cc_used_t *rets;
    struct ringbuf_t *stdout_rbuf, *stderr_rbuf;
    int ret, i, stdout_pfd[2], stderr_pfd[2], null_fd, len, status, cc_id, cc_branch, 
        count = 0;
    pid_t pid;
    FILE *fp;
    char linebuf[LINE_MAX], *p, buf[4096];
    struct list_t *tmp_list, *tmp_list2;

    rets = malloc(ncc * sizeof(struct cc_used_t));
    if (rets == NULL)
        OutOfMemory;
    for (i = 0; i < ncc; i++) {
        rets[i].flags = F_UNKNOWN;
        rets[i].used_branch = NULL;
    }

    stdout_rbuf = ringbuf_new(RINGBUF_LEN);
    if (stdout_rbuf == NULL)
        OutOfMemory;
    stderr_rbuf = ringbuf_new(RINGBUF_LEN);
    if (stderr_rbuf == NULL)
        OutOfMemory;

    while (1) {
        /* clear ringbuf */
        ringbuf_clear(stdout_rbuf);
        ringbuf_clear(stderr_rbuf);
        /* create pipe for child's stdout */
        if (pipe(stdout_pfd) == -1)
            ERRExit;
        /* create pipe for child's stderr */
        if (pipe(stderr_pfd) == -1)
            ERRExit;
        /* create child */
        pid = fork();
        if (pid == -1) {
            /* error */
            ERRExit;
        } else if (pid == 0) {
            /* child */
            close(stdout_pfd[0]);
            close(stderr_pfd[0]);
            /* chdir */
            if (comp_dir != NULL) {
                if (chdir(comp_dir) == -1)
                    _ERRExit;
            }
            /* redirect stdin to /dev/null */
            null_fd = open("/dev/null", O_RDONLY);
            if (null_fd == -1)
                _ERRExit;
            dup2(null_fd, 0);
            close(null_fd);
            /* stdout */
            if (dup2(stdout_pfd[1], 1) == -1)
                _ERRExit;
            close(stdout_pfd[1]);
            /* stderr */
            if (dup2(stderr_pfd[1], 2) == -1)
                _ERRExit;
            close(stderr_pfd[1]);
            /* exec */
            execl("/bin/sh", "sh", "-c", comp_cmd, NULL);
            _exit(1);
        }
        /* parent */
        close(stdout_pfd[1]);
        close(stderr_pfd[1]);
        /* set stdout_pfd[0] in nonblocking mode */
        if (set_nonblocking(stdout_pfd[0]) == -1)
            ERRExit;
        /* read child's stderr */
        fp = fdopen(stderr_pfd[0], "r");
        if (fp == NULL)
            ERRExit;
        while (fgets(linebuf, sizeof(linebuf), fp) != NULL) {
            /* i'm alive */
            count++;
            if (count % 1200 == 0) {
                printf("\b\b\b   \b\b\b");
                fflush(stdout);
            } else if (count % 300 == 0) {
                printf(".");
                fflush(stdout);
            }
            /* line's length check */
            len = strlen(linebuf);
            if (len == sizeof(linebuf) - 1 && linebuf[sizeof(linebuf) - 2] != '\n')
                ERRExit;
            /* write err info to ringbuf */
            ringbuf_write(stderr_rbuf, linebuf, len);
            /* read stdout */
            while (1) {
                ret = read(stdout_pfd[0], buf, sizeof(buf));
                if (ret == -1) {
                    if (errno == EAGAIN)
                        break;
                    else
                        ERRExit;
                } else if (ret == 0) {
                    break;
                }
                /* write stdout info to ringbuf */
                ringbuf_write(stdout_rbuf, buf, ret);
            }
            /* parse error info */
            p = strstr(linebuf, MAGIC);
            if (p != NULL) {
                p += strlen(MAGIC) + 1;
                cc_id = atoi(p);
                if (cc_id >= ncc)
                    ERRExit;
                p = strchr(p, '_');
                if (p == NULL)
                    ERRExit;
                p++;
                cc_branch = atoi(p);
                /* check cc_id */
                if (rets[cc_id].flags == F_UNKNOWN) {
                    /* new */
                    rets[cc_id].flags = F_USED;
                    tmp_list = list_prepend_item_by_data(NULL, (void *)(long)cc_branch);
                    if (tmp_list == NULL)
                        OutOfMemory;
                    rets[cc_id].used_branch = tmp_list;
                } else {
                    /* exist */
                    if (list_search_item_by_data(rets[cc_id].used_branch, 
                                                 (void *)(long)cc_branch) == NULL) 
                    {
                        /* not found, at least 2 branch are valid */
                        rets[cc_id].flags |= F_RESERVECC;
                        /* append to used_branch */
                        tmp_list = list_prepend_item_by_data(rets[cc_id].used_branch, 
                                                             (void *)(long)cc_branch);
                        if (tmp_list == NULL)
                            OutOfMemory;
                        rets[cc_id].used_branch = tmp_list;
                    }
                }
            }
        }
        /* read stdout */
        while (1) {
            ret = read(stdout_pfd[0], buf, sizeof(buf));
            if (ret == -1) {
                if (errno == EAGAIN)
                    break;
                else
                    ERRExit;
            } else if (ret == 0) {
                break;
            }
            /* write stdout info to ringbuf */
            ringbuf_write(stdout_rbuf, buf, ret);
        }
        /* close(pfd[0]) included */
        close(stdout_pfd[0]);
        fclose(fp);
        /* wait child */
        if (wait(&status) == -1)
            ERRExit;
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            /* success */
            ringbuf_free(stdout_rbuf);
            ringbuf_free(stderr_rbuf);
            return rets;
        } else {
            if (!fast_mode) {
                /* error dump */
                dump_error_info(ERR_FILE, stdout_rbuf, stderr_rbuf);
                /* clear up */
                ringbuf_free(stdout_rbuf);
                ringbuf_free(stderr_rbuf);
                /* free rets */
                for (i = 0; i < ncc; i++) {
                    while (rets[i].used_branch != NULL) {
                        tmp_list = rets[i].used_branch;
                        rets[i].used_branch = rets[i].used_branch->next;
                        /* free */
                        free(tmp_list);
                    }
                }
                free(rets);
                return NULL;
            }
            /* fast_mode */
            /* something goes wrong, try to correct and recompile */
            if (!is_lack_main(stderr_rbuf)) {
                /* error dump */
                dump_error_info(ERR_FILE, stdout_rbuf, stderr_rbuf);
                /* clear up */
                ringbuf_free(stdout_rbuf);
                ringbuf_free(stderr_rbuf);
                /* free rets */
                for (i = 0; i < ncc; i++) {
                    while (rets[i].used_branch != NULL) {
                        tmp_list = rets[i].used_branch;
                        rets[i].used_branch = rets[i].used_branch->next;
                        /* free */
                        free(tmp_list);
                    }
                }
                free(rets);
                return NULL;
            } else {
                tmp_list = guess_main_file(stdout_rbuf, *main_list);
                if (tmp_list == NULL) {
                    /* error dump */
                    dump_error_info(ERR_FILE, stdout_rbuf, stderr_rbuf);
                    /* clear up */
                    ringbuf_free(stdout_rbuf);
                    ringbuf_free(stderr_rbuf);
                    /* free rets */
                    for (i = 0; i < ncc; i++) {
                        while (rets[i].used_branch != NULL) {
                            tmp_list = rets[i].used_branch;
                            rets[i].used_branch = rets[i].used_branch->next;
                            /* free */
                            free(tmp_list);
                        }
                    }
                    free(rets);
                    return NULL;
                }
                if (add_main_function((char *)tmp_list->data) == -1)
                    ERRExit;
                rm_object_file((char *)tmp_list->data);
                /* remove tmp_list from main_list */
                if (tmp_list == *main_list) {
                    *main_list = tmp_list->next;
                    free(tmp_list->data);
                    free(tmp_list);
                } else {
                    tmp_list2 = *main_list;
                    while (tmp_list2->next != tmp_list)
                        tmp_list2 = tmp_list2->next;
                    tmp_list2->next = tmp_list->next;
                    free(tmp_list->data);
                    free(tmp_list);
                }
            }
        }
    }
}

static void
restore_files(struct list_t *file_list, int rm_backup)
{
    int ret;
    char path[PATH_MAX];

    /* for each file */
    while (file_list != NULL) {
        /* restore */
        ret = snprintf(path, sizeof(path), "%s.%s", (char *)file_list->data, MAGIC);
        if (ret >= PATH_MAX)
            ERRExit;
        unlink((char *)file_list->data);
        ret = link(path, (char *)file_list->data);
        if (ret == -1)
            ERRExit;
        if (rm_backup)
            unlink(path);
        /* next */
        file_list = file_list->next;
    }
}

static void
fputs_phyline(struct logiline_t *logiline, FILE *stream, char *start, char *end)
{
    char c;
    int i, output;

    if (start != NULL && end != NULL) {
        /* unused, not implemented */
        ERRExit;
    } else if (start != NULL) {
        /* output from start */
        output = 0;
        for (i = 0; i < logiline->phyline_count; i++) {
            if (output == 1) {
                if (i < logiline->phyline_count - 1) {
                    c = *(logiline->linebuf + logiline->phyline_idx[i + 1]);
                    *(logiline->linebuf + logiline->phyline_idx[i + 1]) = '\0';
                    fputs2(logiline->linebuf + logiline->phyline_idx[i], stream);
                    fputs2("\\\n", stream);
                    *(logiline->linebuf + logiline->phyline_idx[i + 1]) = c;
                } else {
                    /* the last line */
                    fputs2(logiline->linebuf + logiline->phyline_idx[i], stream);
                }
            } else {
                if (i < logiline->phyline_count - 1) {
                    if (start < logiline->linebuf + logiline->phyline_idx[i + 1]) {
                        output = 1;
                        c = *(logiline->linebuf + logiline->phyline_idx[i + 1]);
                        *(logiline->linebuf + logiline->phyline_idx[i + 1]) = '\0';
                        fputs2(start, stream);
                        fputs2("\\\n", stream);
                        *(logiline->linebuf + logiline->phyline_idx[i + 1]) = c;
                    }
                } else {
                    /* the last line */
                    fputs2(start, stream);
                }
            }
        }
    } else if (end != NULL) {
        /* output before end */
        for (i = 0; i < logiline->phyline_count; i++) {
            if (i < logiline->phyline_count - 1) {
                if (end > logiline->linebuf + logiline->phyline_idx[i + 1]) {
                    c = *(logiline->linebuf + logiline->phyline_idx[i + 1]);
                    *(logiline->linebuf + logiline->phyline_idx[i + 1]) = '\0';
                    fputs2(logiline->linebuf + logiline->phyline_idx[i], stream);
                    fputs2("\\\n", stream);
                    *(logiline->linebuf + logiline->phyline_idx[i + 1]) = c;
                } else {
                    c = *end;
                    *end = '\0';
                    fputs2(logiline->linebuf + logiline->phyline_idx[i], stream);
                    fputs2("\n", stream);
                    *end = c;
                }
            } else {
                /* the last line */
                c = *end;
                *end = '\0';
                fputs2(logiline->linebuf + logiline->phyline_idx[i], stream);
                fputs2("\n", stream);
                *end = c;
            }
        }
    } else {
        /* output all */
        for (i = 0; i < logiline->phyline_count; i++) {
            if (i < logiline->phyline_count - 1) {
                c = *(logiline->linebuf + logiline->phyline_idx[i + 1]);
                *(logiline->linebuf + logiline->phyline_idx[i + 1]) = '\0';
                fputs2(logiline->linebuf + logiline->phyline_idx[i], stream);
                fputs2("\\\n", stream);
                *(logiline->linebuf + logiline->phyline_idx[i + 1]) = c;
            } else {
                /* the last line */
                fputs2(logiline->linebuf + logiline->phyline_idx[i], stream);
            }
        }
    }
}

static char *
get_left_comment_end(const char *line)
{
    return strstr(line, "*/");
}

static char *
get_right_comment_begin(const char *line)
{
    char *p = (char*)line, *q;

    // search the last "*/"
    while (1) {
        q = strstr(p, "*/");
        if (q != NULL)
            p = q + 2;
        else 
            break;
    }
    // search for "/*"
    while (1) {
        q = strstr(p, "/*");
        if (q == NULL)
            ERRExit;
        if (is_in_quote(p, q) == 1) {
            /* in quote */
            p = find_first_quote(q + 2);
            if (p == NULL) {
                fprintf(stderr, "\033[31m  o \033[0mUnexpected error: Invalid source line.\n");
                exit(1);
            } else {
                p++;
            }
        } else {
            // the first "/*"
            return q;
        }
    }
}

static void 
_stripcc(const char *src_file, struct list_t *invalid_file_list, 
         struct cc_used_t *comp_results, int *warning_cc_id)
{
    struct list_t *tmp_list;
    FILE *sfp, *tfp;
    struct logiline_t *logiline;
    char c, *p, *q, *r, *s, *pbak, *comment_begin = NULL, linebuf[LINE_MAX];
    int cc_type, comment_begin_in_this_line, all_line_is_comment, left_is_comment, 
        right_is_comment, defined[CC_NEST_MAX], cur_branch[CC_NEST_MAX], 
        cur_nest = 0, is_in_comment = 0;
    struct cc_used_t *cur_cc[CC_NEST_MAX];

    /* invalid src_file? */
    tmp_list = invalid_file_list;
    while (tmp_list != NULL) {
        if (strcmp((char *)tmp_list->data, src_file) == 0)
            break;
        /* next */
        tmp_list = tmp_list->next;
    }
    if (tmp_list != NULL) {
        /* invalid src_file */
        if (comp_results[*warning_cc_id].flags == F_UNKNOWN) {
            /* no-compiled files, let it be empty. */
            sfp = fopen(src_file, "w");
            if (sfp == NULL)
                ERRExit;
            fclose(sfp);
        } else {
            /* ignore, maybe stripcc's wrong */
            fprintf(stderr, "\033[31m  o \033[0mInvalid source file: %s, ignore it\n", src_file);
        }
        (*warning_cc_id)++;
        return;
    }

    sfp = fopen(src_file, "r");
    if (sfp == NULL)
        ERRExit;
    tfp = fopen(TMPFILE, "w");
    if (tfp == NULL)
        ERRExit;

    /* check whole file CC */
    cur_cc[cur_nest] = &comp_results[*warning_cc_id];
    (*warning_cc_id)++;
    if (cur_cc[cur_nest]->flags == F_UNKNOWN) {
        /* unused source file */
        defined[cur_nest] = 0;
    } else {
        /* used source file */
        defined[cur_nest] = 1;
    }
    cur_branch[cur_nest] = 0;

    /* parse the source file line by line */
    while (fgets(linebuf, sizeof(linebuf), sfp) != NULL) {
        logiline = NULL;
        cc_type = NOTCC;
        /* check line length */
        if (strlen(linebuf) == sizeof(linebuf) - 1 
            && linebuf[sizeof(linebuf) - 2] != '\n') 
        {
            ERRExit;
        }
        /* build a logical line */
        while (is_unterminate_line(linebuf)) {
            /* unterminated line */
            p = strrchr(linebuf, '\\');
            *p = '\0';
            logiline = build_logiline(logiline, linebuf);
            if (fgets(linebuf, sizeof(linebuf), sfp) == NULL)
                ERRExit;
            /* check line length */
            if (strlen(linebuf) == sizeof(linebuf) - 1
                && linebuf[sizeof(linebuf) - 2] != '\n')
            {
                ERRExit;
            }
        }
        /* process source line */
        if (logiline != NULL) {
            /* add terminated line */
            logiline = build_logiline(logiline, linebuf);
            p = strdup(logiline->linebuf);
        } else {
            p = strdup(linebuf);
        }
        if (p == NULL)
            OutOfMemory;
        pbak = p;
        /* the begin of the comments in this line? */
        all_line_is_comment = 0;
        right_is_comment = 0;
        if (is_in_comment) {
            comment_begin_in_this_line = 0;
            left_is_comment = 1;
        } else {
            comment_begin_in_this_line = 1;
            left_is_comment = 0;
        }
        while (1) {
            /* now in comments? */
            if (is_in_comment) {
                // search for "*/"
                q = strstr(p, "*/");
                if (q != NULL) {
                    /* found, remove the comments */
                    memmove(p, q + 2, strlen(q + 2) + 1);
                    is_in_comment = 0;
                    continue;
                } else {
                    /* not found */
                    if (comment_begin_in_this_line) {
                        /* the begin of comments in this line */
                        /* bug? '\n' is missing */
                        comment_begin[0] = ' ';
                        comment_begin[1] = '\0';
                        /* set right_is_comment flag */
                        right_is_comment = 1;
                        /* search #xxx */
                        cc_type = get_line_type(pbak);
                    } else {
                        /* all line in comments */
                        all_line_is_comment = 1;
                    }
                    break;
                }
            } else {
                /* search for "//" */
                q = p;
                while (1) {
                    r = strstr(q, "//");
                    if (r == NULL)
                        break;
                    if (is_in_quote(q, r)) {
                        /* in quote */
                        q = find_first_quote(r + 2);
                        if (q == NULL) {
                            ERRExit;
                        } else {
                            q++;
                        }
                    } else {
                        break;
                    }
                }
                // search for "/*"
                q = p;
                while (1) {
                    s = strstr(q, "/*");
                    if (s == NULL)
                        break;
                    if (is_in_quote(q, s)) {
                        /* in quote */
                        q = find_first_quote(s + 2);
                        if (q == NULL) {
                            ERRExit;
                        } else {
                            q++;
                        }
                    } else {
                        break;
                    }
                }
                /* compare */
                if (r != NULL && s != NULL) {
                    if (r < s)
                        s = NULL;
                    else 
                        r = NULL;
                }
                if (r != NULL) {
                    /* "//" */
                    r[0] = ' ';
                    r[1] = '\n';
                    r[2] = '\0';
                    /* search #xxx */
                    cc_type = get_line_type(pbak);
                } else if (s != NULL) {
                    // "/*"
                    s[0] = ' ';
                    s[1] = ' ';
                    p = &s[2];
                    comment_begin_in_this_line = 1;
                    comment_begin = &s[0];
                    is_in_comment = 1;
                    continue;
                } else {
                    /* no comments, search #xxx */
                    cc_type = get_line_type(pbak);
                }
                break;
            }
        }
        /* check cc */
        if (all_line_is_comment || cc_type == NOTCC || cc_type == NOTCC_MAIN 
            || cc_type == OTHERCC) 
        {
            /* all comments or normal line */
            if (defined[cur_nest]) {
                /* output */
                if (logiline != NULL) {
                    fputs_phyline(logiline, tfp, NULL, NULL);
                } else {
                    fputs2(linebuf, tfp);
                }
            }
        } else {
            /* have half-comments? */
            if (left_is_comment) {
                if (defined[cur_nest]) {
                    /* output */
                    if (logiline != NULL) {
                        fputs_phyline(logiline, tfp, NULL, 
                                      get_left_comment_end(logiline->linebuf) + 2);
                    } else {
                        q = get_left_comment_end(linebuf);
                        c = *(q + 2);
                        *(q + 2) = '\0';
                        fputs2(linebuf, tfp);
                        *(q + 2) = c;
                        fputs2("\n", tfp);
                    }
                }
            }
            /* CC type */
            if (cc_type == IFCC) {
                cur_nest++;
                if (cur_nest >= CC_NEST_MAX) 
                    ERRExit;
                cur_cc[cur_nest] = &comp_results[*warning_cc_id];
                (*warning_cc_id)++;
                cur_branch[cur_nest] = 0;

                if (cur_cc[cur_nest]->flags == F_UNKNOWN) {
                    /* undefined branch */
                    defined[cur_nest] = 0;
                } else {
                    /* used CC */
                    if (list_search_item_by_data(cur_cc[cur_nest]->used_branch, 
                                                 (void *)(long)cur_branch[cur_nest]) != NULL) 
                    {
                        /* defined branch */
                        defined[cur_nest] = 1;
                        if (cur_cc[cur_nest]->flags & F_RESERVECC) {
                            /* reserve CC */
                            cur_cc[cur_nest]->flags |= F_IFCC_OUTPUTED;
                            /* pbak is linebuf without any comments */
                            fputs2(pbak, tfp);
                        }
                    } else {
                        /* undefined branch */
                        defined[cur_nest] = 0;
                    }
                }
            } else if (cc_type == ELIFCC) {
                cur_branch[cur_nest]++;

                if (cur_cc[cur_nest]->flags != F_UNKNOWN) {
                    /* used CC */
                    if (list_search_item_by_data(cur_cc[cur_nest]->used_branch, 
                                                 (void *)(long)cur_branch[cur_nest]) != NULL) 
                    {
                        /* defined branch */
                        defined[cur_nest] = 1;
                        if (cur_cc[cur_nest]->flags & F_RESERVECC) {
                            /* reserve CC */
                            if (cur_cc[cur_nest]->flags & F_IFCC_OUTPUTED) {
                                /* #if has been outputed, output #elif */
                                fputs2(pbak, tfp);
                            } else {
                                /* make #if */
                                cur_cc[cur_nest]->flags |= F_IFCC_OUTPUTED;
                                p = strstr(pbak, "elif");
                                if (p == NULL)
                                    ERRExit;
                                memmove(p, p + 2, strlen(p + 2) + 1);
                                fputs2(pbak, tfp);
                            }
                        }
                    } else {
                        /* undefined branch */
                        defined[cur_nest] = 0;
                    }
                }
            } else if (cc_type == ELSECC) {
                cur_branch[cur_nest]++;

                if (cur_cc[cur_nest]->flags != F_UNKNOWN) {
                    /* used CC */
                    if (list_search_item_by_data(cur_cc[cur_nest]->used_branch, 
                                                 (void *)(long)cur_branch[cur_nest]) != NULL) 
                    {
                        /* defined branch */
                        defined[cur_nest] = 1;
                        if (cur_cc[cur_nest]->flags & F_RESERVECC) {
                            /* reserve CC */
                            fputs2(pbak, tfp);
                        }
                    } else {
                        /* undefined branch */
                        defined[cur_nest] = 0;
                    }
                }
            } else if (cc_type == ENDCC) {
                if (cur_cc[cur_nest]->flags != F_UNKNOWN) {
                    /* used CCs */
                    if (cur_cc[cur_nest]->flags & F_RESERVECC) {
                        /* reserve CC */
                        fputs2(pbak, tfp);
                    }
                }
                cur_nest--;
                if (cur_nest < 0) 
                    ERRExit;
            }
            /* have half-comments? */
            if (right_is_comment) {
                if (defined[cur_nest]) {
                    /* output */
                    if (logiline != NULL) {
                        fputs_phyline(logiline, tfp, 
                                      get_right_comment_begin(logiline->linebuf), NULL);
                    } else {
                        fputs2(get_right_comment_begin(linebuf), tfp);
                    }
                }
            }
        }
        /* clean */
        free(pbak);
        if (logiline != NULL)
            logiline_free(logiline);
    }

    /* close */
    fclose(sfp);
    fclose(tfp);
    /* replace the source file */
    if (rename(TMPFILE, src_file) == -1)
        ERRExit;
}

static void 
stripcc(struct list_t *file_list, struct list_t *invalid_file_list, 
        struct cc_used_t *comp_results)
{
    struct list_t *item;
    int warning_cc_id = 0;

    item = file_list;
    while (item != NULL) {
        _stripcc((char *)item->data, invalid_file_list, comp_results, &warning_cc_id);
        /* next src_file */
        item = item->next;
    }
}

static int
build(const char *comp_cmd, const char *comp_dir)
{
    pid_t pid;
    int ret, null_fd, status, count = 0;

    /* create child */
    pid = fork();
    if (pid == -1) {
        /* error */
        ERRExit;
    } else if (pid == 0) {
        /* child */
        /* chdir */
        if (comp_dir != NULL) {
            if (chdir(comp_dir) == -1)
                _ERRExit;
        }
        /* redirect stdin/stdout/stderr to /dev/null */
        null_fd = open("/dev/null", O_RDWR);
        if (null_fd == -1)
            _ERRExit;
        dup2(null_fd, 0);
        dup2(null_fd, 1);
        dup2(null_fd, 2);
        close(null_fd);
        /* exec */
        execl("/bin/sh", "sh", "-c", comp_cmd, NULL);
        _exit(1);
    }
    /* parent */
    /* wait child */
    while (1) {
        ret = waitpid(pid, &status, WNOHANG);
        if (ret == -1) {
            ERRExit;
        } else if (ret == 0) {
            /* i'm alive */
            count++;
            if (count % 4 == 0) {
                printf("\b\b\b   \b\b\b");
                fflush(stdout);
            } else {
                printf(".");
                fflush(stdout);
            }
            sleep(1);
        } else {
            break;
        }
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        /* success */
        return 0;
    } else {
        return -1;
    }
}

int 
main(int argc, char *argv[])
{
    int ret, ncc, fast_mode = 0, do_verify = 1;
    char *comp_cmd = "make", *comp_dir = NULL;
    struct list_t *file_list, *tmp_list, *invalid_file_list = NULL, *main_list = NULL;
    struct cc_used_t *comp_results;
    time_t t;
    struct stat st;

    /* parse args */
    while (1) {
        ret = getopt(argc, argv, "c:m:fnvh");
        if (ret == -1) {
            if (argv[optind] != NULL) {
                show_help();
                exit(1);
            }
            break;      /* break while */
        }
        switch (ret) {
        case 'c':      /* make ... */
            comp_cmd = optarg;
            break;
        case 'm':      /* make dir ... */
            comp_dir = optarg;
            break;
        case 'n':      /* dont verify ... */
            do_verify = 0;
            break;
        case 'f':      /* fast mode ... */
            fast_mode = 1;
            break;
        case 'v':      /* show stripcc version */
            show_ver();
            return 0;
        case 'h':      /* show help */
            show_help();
            return 0;
        default:
            show_help();
            exit(1);
        }
    }

    printf("\033[32mo \033[0mGetting a list of files which will be stripped...\n");
    file_list = list_need_strip_files();
    if (file_list == NULL) {
        printf("\033[31m  o \033[0mCouldn't get any file.\n");
        return 0;
    }

    printf("\033[32mo \033[0mBacking up files...\n");
    backup_files(file_list);

add_warning_cc:
    printf("\033[32mo \033[0mAdding \"#warning\"...\n");
    ncc = add_warning_cc_to_files(file_list, fast_mode, &invalid_file_list, &main_list);

    printf("\033[32mo \033[0mTry to build target, this operation may takes a few minutes");
    fflush(stdout);
    comp_results = try_build_and_parse(fast_mode, comp_cmd, comp_dir, ncc, &main_list);
    if (comp_results == NULL) {
        if (fast_mode) {
            printf("\n\033[31m  o \033[0mFailed to build target in fast mode, then we'll try it in normal mode.\n");
            fast_mode = 0;
            restore_files(file_list, 0);
            if (invalid_file_list) {
                list_free(invalid_file_list, item_free);
                invalid_file_list = NULL;
            }
            if (main_list) {
                list_free(main_list, item_free);
                main_list = NULL;
            }
            sleep(1);
            goto add_warning_cc;
        } else {
            printf("\n\033[31m  o Failed to build target.\033[0m\n");
            exit(1);
        }
    }

    printf("\n\033[32mo \033[0mRestore files...\n");
    restore_files(file_list, 1);

    printf("\033[32mo \033[0mStripping files...\n");
    stripcc(file_list, invalid_file_list, comp_results);

    if (do_verify) {
        printf("\033[32mo \033[0mStart to verify");
        fflush(stdout);
        t = time(NULL);
        sleep(1);
        if (build(comp_cmd, comp_dir) == -1) {
            printf("\n");
            printf("\033[31m  o Failed to verify.\033[0m\n");
            /* printf("\033[31m  o You can put follow files into the dont_strip_files section of %s and try again.\033[0m\n", CONF_FILE); */
            printf("\033[31m  o The following files were changed during compilation.\033[0m\n");
            /* check if some source file was changed */
            tmp_list = file_list;
            while (tmp_list != NULL) {
                /* next */
                if (lstat((char *)tmp_list->data, &st) == -1) {
                    printf("\033[31m    %s\033[0m\n", (char *)tmp_list->data);
                } else {
                    if (st.st_mtime > t)
                        printf("\033[31m    %s\033[0m\n", (char *)tmp_list->data);
                }
                tmp_list = tmp_list->next;
            }
            exit(1);
        }
        printf("\n");
    }

    printf("\033[32mo Done!\033[0m\n");
    return 0;
}
