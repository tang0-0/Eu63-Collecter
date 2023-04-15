/**
 * @File Name: euromap63_interface.c
 *
 * @brief :
 *
 * @copyright : Copyright (c) InHand Networks, Inc, 2001-2023
 *
 * @Author : tangzh (tangzh@inhand.com.cn)
 * @CreateTime : 2023-03-13
 *
 * @Change Logs:
 * Date       Author        Notes
 *
 */
#include "eu63_collecter.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>


eu63_collecter *eu63_collecter_create(const char *imm)
{
    eu63_collecter *collecter = NULL;

    collecter = calloc(1, sizeof(eu63_collecter));
    if (NULL != collecter)
    {
        collecter->now_session = 0;
        collecter->now_presen = 0;
        collecter->setup.connect_timeout = EUROMAP63_CONNECT_TIMEOUT_MS;
        collecter->setup.request_timeout = EUROMAP63_REQUEST_TIMEOUT_MS;
        collecter->setup.min_session = EUROMAP63_MIN_SESSION_NUM;
        collecter->setup.max_session = EUROMAP63_MAX_SESSION_NUM;
        collecter->setup.encode = EUROMAP63_ENCODE;
        collecter->setup.cyclic = EUROMAP63_HAS_CYCLIC;

        if (imm)
        {
            strncpy(collecter->setup.imm, imm, sizeof(collecter->setup.imm));
        }
    }
    else
    {
        printf("eu63_collecter_create failed");
    }

    return collecter;
}

void eu63_collecter_free(eu63_collecter *collecter)
{
    free(collecter);
}

int eu63_create_share_folder(void)
{
    if (0 != access(EUROMAP63_SHARE_FOLDER_PATH, F_OK))
    {
        printf("Create share folder!!!");
        if (0 != mkdir(EUROMAP63_SHARE_FOLDER_PATH, 0777))
        {
            printf("Mkdir share folder failed");
            return -2;
        }
    }

    return 0;
}

int eu63_delete_share_folder(void)
{
    if (0 == access(EUROMAP63_SHARE_FOLDER_PATH, F_OK))
    {
        system("rm -rf "EUROMAP63_SHARE_FOLDER_PATH);
    }

    return 0;
}

int eu63_create_imm_folder(eu63_collecter *collecter)
{
    if (strlen(collecter->setup.imm) == 0)
    {
        return -1;
    }

    char path[256] = {0};
    snprintf(path, sizeof(path), EUROMAP63_SHARE_FOLDER_PATH"%s", collecter->setup.imm);

    if (0 != access(path, F_OK))
    {
        printf("Create IMM folder!!!");
        if (0 != mkdir(path, 0777))
        {
            printf("Mkdir IMM folder failed");
            return -2;
        }
    }

    return 0;
}

int eu63_delete_imm_folder(eu63_collecter *collecter)
{
    char path[256] = {0};
    snprintf(path, sizeof(path), EUROMAP63_SHARE_FOLDER_PATH"%s", collecter->setup.imm);

    if (0 == access(path, F_OK))
    {
        snprintf(path, sizeof(path), "rm -rf "EUROMAP63_SHARE_FOLDER_PATH"%s", collecter->setup.imm);
        system(path);
    }

    return 0;
}

int eu63_clear_imm_folder(eu63_collecter *collecter)
{
    char path[256] = {0};
    snprintf(path, sizeof(path), EUROMAP63_SHARE_FOLDER_PATH"%s", collecter->setup.imm);
    unsigned int path_len = strlen(path);

    DIR *dir = opendir(path);
    if (NULL == dir)
    {
        return -1;
    }

    struct dirent *ptr = NULL;
    char file_name[64] = {0};
    while ((ptr = readdir(dir)) != NULL)
    {
        if (strcmp(".", ptr->d_name) != 0 && strcmp("..", ptr->d_name) != 0)
        {
            snprintf(file_name, sizeof(file_name), "/%s", ptr->d_name);
            strncpy(&path[path_len], file_name, sizeof(file_name));
            printf("EU63 clear imm:%s", path);
            remove(path);
        }
    }

    closedir(dir);

    return 0;
}

static int search_available_session(eu63_collecter *collecter)
{
    char path[256] = {0};
    snprintf(path, sizeof(path), EUROMAP63_SHARE_FOLDER_PATH"%s", collecter->setup.imm);
    int path_len = strlen(path);

    if (0 != access(path, F_OK))
        return -2;

    unsigned short nnnn = collecter->setup.min_session;
    char sess_name[16] = {0};
    int i = 0;
    for (i = collecter->setup.min_session; i < collecter->setup.max_session; i++)
    {
        snprintf(sess_name, sizeof(sess_name), "/SESS%04d.REQ", nnnn);
        strncpy(&path[path_len], sess_name, sizeof(sess_name));

        if (0 == access(path, F_OK))
        {
            nnnn++;
            continue;
        }

        snprintf(sess_name, sizeof(sess_name), "/SESS%04d.RSP", nnnn);
        strncpy(&path[path_len], sess_name, sizeof(sess_name));

        if (0 == access(path, F_OK))
        {
            nnnn++;
            continue;
        }

        break;
    }

    if (collecter->setup.max_session == i)
    {
        return -3;
    }

    return nnnn;
}

static int write_session_file(eu63_collecter *collecter, int is_connect, int execute_type)
{
    if (0 == is_connect && 0 == execute_type)
    {
        return -1;
    }

    collecter->now_session = search_available_session(collecter);
    if (collecter->now_session < 0) return -1;

    char sess_name[16] = {0};
    char path[256] = {0};
    snprintf(path, sizeof(path), EUROMAP63_SHARE_FOLDER_PATH"%s", collecter->setup.imm);
    snprintf(sess_name, sizeof(sess_name), "/SESS%04d.REQ", collecter->now_session);
    strncat(path, sess_name, sizeof(sess_name));

    FILE *fp = NULL;
    fp = fopen(path, "w");
    if (NULL == fp)
    {
        return -2;
    }

    unsigned int iden = 0;
    if (is_connect != 0)
    {
        fprintf(fp, "%08u CONNECT;\r\n", iden);
        iden++;
    }

    if (EU63_REQ_TYPE_REPORT == execute_type)
    {
        fprintf(fp, "%08u EXECUTE \"0000R%03d.JOB\";\r\n", iden, collecter->now_presen);
    }
    else if (EU63_REQ_TYPE_SET == execute_type)
    {
        fprintf(fp, "%08u EXECUTE \"0000S%03d.JOB\";\r\n", iden, collecter->now_presen);
    }

    fclose(fp);

    return 0;
}

static int prase_session_rsp_file(eu63_collecter *collecter)
{
    char path[256] = {0};
    char sess_name[16] = {0};

    if (collecter->now_session < 0) return -1;

    snprintf(path, sizeof(path), EUROMAP63_SHARE_FOLDER_PATH"%s", collecter->setup.imm);
    snprintf(sess_name, sizeof(sess_name), "/SESS%04d.RSP", collecter->now_session);
    strncat(path, sess_name, sizeof(sess_name));

    if (0 != access(path, F_OK))
        return -1;

    FILE *fp = NULL;
    fp = fopen(path, "r");
    if (NULL == fp)
    {
        return -2;
    }

    int error = 0, ch = 0;
    unsigned short i = 0;
    char line[320] = {0};
    while ((ch = fgetc(fp)))
    {
        if (';' == ch)
        {
            line[i] = '\0';
            i = 0;
            printf("SESS_RSP:%s;", line);
            if (NULL != strstr(line, "ERROR"))
            {
                error--;
            }
        }
        else if ('\n' == ch)
        {
            if (0 != i)
            {
                line[i] = '\0';
                i = 0;
                printf("SESS_RSP:%s", line);
                if (NULL != strstr(line, "ERROR"))
                {
                    error--;
                }
            }
        }
        else if (EOF == ch)
        {
            if (0 != i)
            {
                line[i] = '\0';
                i = 0;
                printf("SESS_RSP:%s", line);
                if (NULL != strstr(line, "ERROR"))
                {
                    error--;
                }
            }
            break;
        }
        else if ('\r' == ch)
        {
            continue;
        }
        else
        {
            line[i++] = ch;
        }
    }

    fclose(fp);

    if (strlen(line) == 0 || error < 0)
    {
        return -9;
    }

    return 0;
}

static int wait_file_delete(const char *path, int timeout)
{
    if (0 == timeout)
    {
        if (0 != access(EUROMAP63_SHARE_FOLDER_PATH, F_OK))
            return 0;
        else
            return -1;
    }

    int fd = inotify_init();
    if (fd < 0)
    {
        printf("inotify_init failed:%s", strerror(errno));
        return -2;
    }

    int wfd = -1;
    wfd = inotify_add_watch(fd, path, IN_DELETE_SELF);
    if (wfd < 0)
    {
        printf("inotify_add_watch failed:%s", strerror(errno));
        close(fd);
        return -2;
    }

    fd_set rfds;
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000; // 1000us = 1ms
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    int ret = 0;
    unsigned long event_mask = 0;
    unsigned char buff[64] = {0};

    ret = select(fd + 1, &rfds, NULL, NULL, &tv);
    if (ret > 0)
    {
        ret = read(fd, buff, sizeof(struct inotify_event));
        if (ret > 0)
        {
            event_mask = ((struct inotify_event *)buff)->mask;
            if (IN_DELETE_SELF == event_mask) ret = 0;
            else ret = -3;
        }
        else
        {
            printf("Read inotify error:%s", strerror(errno));
            ret = -3;
        }

    }
    else if (0 == ret)
    {
        printf("wait_file_delete select timeout!");
        ret = -3;
    }
    else
    {
        printf("wait_file_delete select error:%s", strerror(errno));
        ret = -3;
    }

    inotify_rm_watch(fd, wfd);
    close(fd);

    return ret;
}

static int wait_session_file_delete(eu63_collecter *collecter, int timeout)
{
    char path[256] = {0};
    char sess_name[16] = {0};

    if (collecter->now_session < 0) return -1;

    snprintf(path, sizeof(path), EUROMAP63_SHARE_FOLDER_PATH"%s", collecter->setup.imm);
    snprintf(sess_name, sizeof(sess_name), "/SESS%04d.REQ", collecter->now_session);
    strncat(path, sess_name, sizeof(sess_name));

    return wait_file_delete(path, timeout);
}

static int delete_session_file(eu63_collecter *collecter, int type)
{
    char path[256] = {0};
    char sess_name[16] = {0};

    if (collecter->now_session < 0) return -100;

    snprintf(path, sizeof(path), EUROMAP63_SHARE_FOLDER_PATH"%s", collecter->setup.imm);
    if (1 == type)
    {
        snprintf(sess_name, sizeof(sess_name), "/SESS%04d.REQ", collecter->now_session);
        strncat(path, sess_name, sizeof(sess_name));

        return remove(path);
    }
    else if (2 == type)
    {
        snprintf(sess_name, sizeof(sess_name), "/SESS%04d.RSP", collecter->now_session);
        strncat(path, sess_name, sizeof(sess_name));

        return remove(path);
    }
    else
    {
        return -99;
    }

    return 0;
}

static int prase_presen_rsp_file(eu63_collecter *collecter, int rsp_type)
{
    char path[256] = {0};
    char presen_name[16] = {0};

    snprintf(path, sizeof(path), EUROMAP63_SHARE_FOLDER_PATH"%s", collecter->setup.imm);
    if (EU63_REQ_TYPE_REPORT == rsp_type)
    {
        snprintf(presen_name, sizeof(presen_name), "/0000R%03d.RSP", collecter->now_presen);
    }
    else if (EU63_REQ_TYPE_SET == rsp_type)
    {
        snprintf(presen_name, sizeof(presen_name), "/0000S%03d.RSP", collecter->now_presen);
    }
    else
    {
        return -1;
    }
    strncat(path, presen_name, sizeof(presen_name));

    if (0 != access(path, F_OK))
        return -1;

    FILE *fp = NULL;
    fp = fopen(path, "r");
    if (NULL == fp)
    {
        return -2;
    }

    int error = 0, ch = 0;
    unsigned short i = 0;
    char line[320] = {0};
    while ((ch = fgetc(fp)))
    {
        if (';' == ch)
        {
            line[i] = '\0';
            i = 0;
            printf("PRESEN_RSP:%s;", line);
            if (NULL != strstr(line, "ERROR"))
            {
                error--;
            }
        }
        else if ('\n' == ch)
        {
            if (0 != i)
            {
                line[i] = '\0';
                i = 0;
                printf("PRESEN_RSP:%s", line);
                if (NULL != strstr(line, "ERROR"))
                {
                    error--;
                }
            }
        }
        else if (EOF == ch)
        {
            if (0 != i)
            {
                line[i] = '\0';
                i = 0;
                printf("PRESEN_RSP:%s", line);
                if (NULL != strstr(line, "ERROR"))
                {
                    error--;
                }
            }
            break;
        }
        else if ('\r' == ch)
        {
            continue;
        }
        else
        {
            line[i++] = ch;
        }
    }

    fclose(fp);

    if (strlen(line) == 0 || error < 0)
    {
        return -9;
    }

    return 0;
}

static int delete_presen_file(eu63_collecter *collecter, int delete_type, int cmd_type)
{
    char path[256] = {0};
    char presen_name[16] = {0};
    char ch_type = '\0';
    int ret = -1;

    if (EU63_REQ_TYPE_REPORT == cmd_type)
    {
        ch_type = 'R';
    }
    else if (EU63_REQ_TYPE_SET == cmd_type)
    {
        ch_type = 'S';
    }
    else
    {
        delete_type = -1;
    }

    if (1 == delete_type)
    {
        snprintf(path, sizeof(path), EUROMAP63_SHARE_FOLDER_PATH"%s", collecter->setup.imm);
        snprintf(presen_name, sizeof(presen_name), "/0000%c%03d.JOB", ch_type, collecter->now_presen);
        strncat(path, presen_name, sizeof(presen_name));

        ret = remove(path);
    }
    else if (2 == delete_type)
    {
        snprintf(path, sizeof(path), EUROMAP63_SHARE_FOLDER_PATH"%s", collecter->setup.imm);
        snprintf(presen_name, sizeof(presen_name), "/0000%c%03d.RSP", ch_type, collecter->now_presen);
        strncat(path, presen_name, sizeof(presen_name));

        ret = remove(path);
    }
    else if (3 == delete_type)
    {
        snprintf(path, sizeof(path), EUROMAP63_SHARE_FOLDER_PATH"%s", collecter->setup.imm);
        snprintf(presen_name, sizeof(presen_name), "/0000%c%03d.JOB", ch_type, collecter->now_presen);
        strncat(path, presen_name, sizeof(presen_name));
        ret = remove(path);

        snprintf(path, sizeof(path), EUROMAP63_SHARE_FOLDER_PATH"%s", collecter->setup.imm);
        snprintf(presen_name, sizeof(presen_name), "/0000%c%03d.RSP", ch_type, collecter->now_presen);
        strncat(path, presen_name, sizeof(presen_name));
        ret = ret + remove(path);
    }

    return ret;
}

static int delete_app_file(eu63_collecter *collecter, int cmd_type)
{
    char path[256] = {0};
    char presen_name[16] = {0};

    if (EU63_REQ_TYPE_REPORT == cmd_type)
    {
        snprintf(path, sizeof(path), EUROMAP63_SHARE_FOLDER_PATH"%s", collecter->setup.imm);
        snprintf(presen_name, sizeof(presen_name), "/0000R%03d.DAT", collecter->now_presen);
        strncat(path, presen_name, sizeof(presen_name));

        return remove(path);
    }
    else
    {
        return -99;
    }

    return 0;
}

eu63_report_param *eu63_create_param(void)
{
    eu63_report_param *report_param  = NULL;
    report_param = malloc(sizeof(eu63_report_param));
    if (NULL == report_param)
    {
        return NULL;
    }

    memset(report_param, 0, sizeof(eu63_report_param));

    return report_param;
}

int eu63_free_param(eu63_report_param *param)
{
    if (NULL == param) return -1;

    if (NULL != param->value)
    {
        free(param->value);
        param->value = NULL;
    }

    free(param);

    return 0;
}

eu63_param_list *eu63_create_param_list(void)
{
    eu63_param_list *report_list  = NULL;
    report_list = malloc(sizeof(eu63_param_list));
    if (NULL == report_list)
    {
        return NULL;
    }
    memset(report_list, 0, sizeof(eu63_param_list));

    return report_list;
}

int eu63_free_param_list(eu63_param_list *list)
{
    if (NULL == list) return -1;

    struct list4c_hlist_node *pos, *n;
    eu63_report_param *param;

    ty_list_for_each_entry_safe(param, pos, n, &(list->param_head), param_node)
    {
        ty_list_remove(&(param->param_node));
        eu63_free_param(param);
    }

    free(list);

    return 0;
}

static int search_available_presen(eu63_collecter *collecter, int cmd_type)
{
    char path[256] = {0};
    snprintf(path, sizeof(path), EUROMAP63_SHARE_FOLDER_PATH"%s", collecter->setup.imm);
    int path_len = strlen(path);

    if (0 != access(path, F_OK))
        return -1;

    char type = 'P';
    if (EU63_REQ_TYPE_REPORT == cmd_type)
    {
        type = 'P';
    }
    else if (EU63_REQ_TYPE_SET == cmd_type)
    {
        type = 'S';
    }
    else
    {
        return -2;
    }

    int i = 0, nnn = 0;
    char file_name[16] = {0};
    for (i = 0; i < 1000; i++)
    {
        snprintf(file_name, sizeof(file_name), "/0000%c%04d.JOB", type, nnn);
        strncpy(&path[path_len], file_name, sizeof(file_name));

        if (0 == access(path, F_OK))
        {
            nnn++;
            continue;
        }

        snprintf(file_name, sizeof(file_name), "/0000%c%04d.RSP", type, nnn);
        strncpy(&path[path_len], file_name, sizeof(file_name));

        if (0 == access(path, F_OK))
        {
            nnn++;
            continue;
        }

        snprintf(file_name, sizeof(file_name), "/0000%c%04d.DAT", type, nnn);
        strncpy(&path[path_len], file_name, sizeof(file_name));

        if (0 == access(path, F_OK))
        {
            nnn++;
            continue;
        }

        break;
    }

    if (1000 == i)
    {
        return -10;
    }

    return nnn;
}

static int write_presen_file_report(eu63_collecter *collecter, eu63_param_list *report_list)
{
    char path[256] = {0};
    char presen_name[16] = {0};

    if (0 == report_list->req_count) return -1;

    collecter->now_presen = search_available_presen(collecter, EU63_REQ_TYPE_REPORT);
    if (collecter->now_presen < 0) return -1;

    snprintf(path, sizeof(path), EUROMAP63_SHARE_FOLDER_PATH"%s", collecter->setup.imm);
    snprintf(presen_name, sizeof(presen_name), "/0000R%03d.JOB", collecter->now_presen);
    strncat(path, presen_name, sizeof(presen_name));

    FILE *fp = NULL;
    fp = fopen(path, "w");
    if (NULL == fp)
    {
        return -2;
    }

    fprintf(fp, "JOB 0000R%03d RESPONSE \"0000R%03d.JOB\";\r\n", collecter->now_presen, collecter->now_presen);
    fprintf(fp, "REPORT 0000R%03d REWRITE \"0000R%03d.DAT\"\r\n", collecter->now_presen, collecter->now_presen);

    fputs("START IMMEDIATE\r\n", fp);
    fputs("STOP NEVER\r\n", fp);
    if (collecter->setup.cyclic)
    {
        fputs("CYCLIC SHOT 1\r\n", fp);
        fputs("SAMPLES 1\r\n", fp);
        fputs("SESSIONS 1\r\n", fp);
    }

    fputs("PARAMETERS\r\n", fp);
    struct list4c_hlist_node *pos, *n;
    eu63_report_param *param;
    ty_list_for_each_entry_safe(param, pos, n, &(report_list->param_head), param_node)
    {
        if (n)
        {
            fprintf(fp, "%s,\r\n", param->name);
        }
        else
        {
            fprintf(fp, "%s;", param->name);
        }
    }

    fclose(fp);
    return 0;
}

eu63_report_param *find_report_param_from_list(eu63_param_list *list, unsigned int num)
{
    struct list4c_hlist_node *pos, *n;
    eu63_report_param *param = NULL;
    unsigned int i = 0;

    ty_list_for_each_entry_safe(param, pos, n, &(list->param_head), param_node)
    {
        if (i == num)
        {
            return param;
        }
        i++;
    }

    return NULL;
}

static int set_report_param(eu63_report_param *param, char *name, char *value, unsigned int value_len)
{
    if (NULL == param) return -1;

    if (name != NULL)
    {
        strncpy(param->name, name, sizeof(param->name));
    }

    if (value_len > 256) value_len = 256;

    if (value != NULL && value_len != 0)
    {
        char *value_buff = NULL;
        value_buff = calloc(1, value_len + 1);
        if (NULL != value_buff)
        {
            param->value = value_buff;
            strncpy(param->value, value, value_len + 1);
        }
        else
        {
            return -2;
        }
    }

    return 0;
}

static int prase_app_file_report(eu63_collecter *collecter, eu63_param_list *report_list)
{
    char path[256] = {0};
    char presen_name[16] = {0};

    snprintf(path, sizeof(path), EUROMAP63_SHARE_FOLDER_PATH"%s", collecter->setup.imm);
    snprintf(presen_name, sizeof(presen_name), "/0000R%03d.DAT", collecter->now_presen);
    strncat(path, presen_name, sizeof(presen_name));

    if (0 != access(path, F_OK))
        return -1;

    FILE *fp = NULL;
    fp = fopen(path, "r");
    if (NULL == fp)
    {
        return -2;
    }

    eu63_report_param *param = NULL;

    int ch = 0;
    char bracket_flag = 0;
    char comma_flag = 0;
    char name[32] = {0};
    unsigned short i = 0, name_count = 0;
    while ((ch = fgetc(fp)) != EOF)
    {
        if (' ' == ch)
        {
            continue;
        }
        else if ('[' == ch)
        {
            bracket_flag = 1;
            name[i++] = ch;
            comma_flag = 0;
        }
        else if (']' == ch)
        {
            bracket_flag = 2;
            name[i++] = ch;
            comma_flag = 0;
        }
        else if (',' == ch)
        {
            if (1 == bracket_flag)
            {
                name[i++] = ch;
                comma_flag = 0;
            }
            else
            {
                name[i] = '\0';
                i = 0;
                comma_flag = 1;

                param = find_report_param_from_list(report_list, name_count);
                if (param)
                {
                    set_report_param(param, name, NULL, 0);
                    name_count++;
                }
            }
        }
        else if ('\r' == ch)
        {
            if (0 == comma_flag)
            {
                name[i] = '\0';
                i = 0;

                param = find_report_param_from_list(report_list, name_count);
                if (param)
                {
                    set_report_param(param, name, NULL, 0);
                    name_count++;
                }
                break;
            }
        }
        else if ('\n' == ch)
        {
            continue;
        }
        else
        {
            name[i++] = ch;
            comma_flag = 0;
        }
    }

    char value[260] = {0};
    char string_flag = 0;
    unsigned short value_count = 0;
    while ((ch = fgetc(fp)))
    {
        if ('"' == ch && 0 == string_flag)
        {
            string_flag = 1;
            continue;
        }
        else if ('"' == ch && 1 == string_flag)
        {
            string_flag = 0;
            continue;
        }
        else if (' ' == ch)
        {
            if (1 == string_flag)
            {
                value[i++] = ch;
                comma_flag = 0;
            }
        }
        else if (',' == ch)
        {
            value[i] = '\0';
            comma_flag = 1;

            param = find_report_param_from_list(report_list, value_count);
            if (param)
            {
                set_report_param(param, NULL, value, i);
                value_count++;
                i = 0;
            }

        }
        else if ('\r' == ch)
        {
            if (0 == comma_flag)
            {
                param = find_report_param_from_list(report_list, value_count);
                if (param)
                {
                    set_report_param(param, NULL, value, i);
                    value_count++;
                    i = 0;
                }
                break;
            }
        }
        else if (EOF == ch)
        {
            if (0 == comma_flag)
            {
                param = find_report_param_from_list(report_list, value_count);
                if (param)
                {
                    set_report_param(param, NULL, value, i);
                    value_count++;
                    i = 0;
                }
                break;
            }
        }
        else if ('\n' == ch)
        {
            continue;
        }
        else
        {
            value[i++] = ch;
            comma_flag = 0;
        }

    }

    fclose(fp);

    report_list->rsp_count = value_count;
    if (value_count != name_count)
    {
        report_list->rsp_count = (value_count <= name_count) ? value_count : name_count;
        printf("value_count != name_count:%d|%d", value_count, name_count);
    }

    if (report_list->rsp_count != report_list->req_count)
    {
        printf("rsp_count != req_count:%d|%d", report_list->rsp_count, report_list->req_count);
    }

    return 0;
}

static int write_presen_file_set(eu63_collecter *collecter, eu63_param_list *set_list)
{
    char path[256] = {0};
    char presen_name[16] = {0};

    if (set_list->req_count == 0) return -1;

    collecter->now_presen = search_available_presen(collecter, EU63_REQ_TYPE_REPORT);
    if (collecter->now_presen < 0) return -1;

    snprintf(path, sizeof(path), EUROMAP63_SHARE_FOLDER_PATH"%s", collecter->setup.imm);
    snprintf(presen_name, sizeof(presen_name), "/0000S%03d.JOB", collecter->now_presen);
    strncat(path, presen_name, sizeof(presen_name));

    FILE *fp = NULL;
    fp = fopen(path, "w");
    if (NULL == fp)
    {
        return -2;
    }

    fprintf(fp, "JOB 0000S%03d RESPONSE \"0000S%03d.JOB\";\r\n", collecter->now_presen, collecter->now_presen);

    struct list4c_hlist_node *pos, *n;
    eu63_report_param *param;
    ty_list_for_each_entry_safe(param, pos, n, &(set_list->param_head), param_node)
    {
        printf("EU63 SET addr:%s", param->name);
        if (param->value)
        {
            fprintf(fp, "SET %s %s;\r\n", param->name, param->value);
            printf("EU63 SET value:%s", param->value);
        }
    }

    fclose(fp);
    return 0;
}


int eu63_execute_connect_req(eu63_collecter *collecter)
{
    int ret = -1;

    /* write req file*/
    ret = write_session_file(collecter, 1, 0);
    if (ret < 0)
    {
        printf("write_session_file failed:%d", ret);
        return -1;
    }

    /* listen file delete event */
    ret = wait_session_file_delete(collecter, collecter->setup.connect_timeout);
    if (ret < 0)
    {
        printf("wait_session_file_delete failed:%d", ret);
        delete_session_file(collecter, 1);
        return -2;
    }

    /* handle req_rsp file */
    ret = prase_session_rsp_file(collecter);
    if (ret < 0)
    {
        printf("prase_session_rsp_file failed:%d", ret);
        ret = -3;
    }

    /* delete files */
    delete_session_file(collecter, 2);

    return ret;
}

int eu63_execute_report_req(eu63_collecter *collecter, eu63_param_list *report_list)
{
    int ret = -1;

    /* write req file*/
    printf("EU63 write report session file");
    ret = write_session_file(collecter, 1, EU63_REQ_TYPE_REPORT);
    if (ret < 0)
    {
        ilog_error("EU63 write report session file failed:%d", ret);
        return -1;
    }

    /* write job file */
    printf("EU63 write report job file");
    ret = write_presen_file_report(collecter, report_list);
    if (ret < 0)
    {
        ilog_error("EU63 write report job file failed:%d", ret);
        delete_session_file(collecter, 1);
        return -1;
    }

    /* listen file delete event */
    printf("EU63 wait delete report session file");
    ret = wait_session_file_delete(collecter, collecter->setup.request_timeout);
    if (ret < 0)
    {
        ilog_error("EU63 wait delete report session file failed:%d", ret);
        delete_session_file(collecter, 1);
        delete_presen_file(collecter, 1, EU63_REQ_TYPE_REPORT);
        return -1;
    }

    /* prase session rsp */
    printf("EU63 prase report session rsp file");
    ret = prase_session_rsp_file(collecter);
    if (ret < 0)
    {
        ilog_error("EU63 prase report session rsp fil failed:%d", ret);
        delete_session_file(collecter, 2);
        delete_presen_file(collecter, 1, EU63_REQ_TYPE_REPORT);
        return -1;
    }

    /* prase presen rsp */
    printf("EU63 prase report job rsp file");
    ret = prase_presen_rsp_file(collecter, EU63_REQ_TYPE_REPORT);
    if (ret < 0)
    {
        ilog_error("EU63 prase report job rsp fil failed:%d", ret);
        delete_session_file(collecter, 2);
        delete_presen_file(collecter, 3, EU63_REQ_TYPE_REPORT);
        return -1;
    }

    /* prase app dat */
    printf("EU63 prase report dat file");
    ret = prase_app_file_report(collecter, report_list);
    if (ret < 0)
    {
        ilog_error("EU63 prase report dat file failed:%d", ret);
    }

    /* delete files */
    printf("EU63 delete report files");
    delete_app_file(collecter, EU63_REQ_TYPE_REPORT);
    delete_session_file(collecter, 2);
    delete_presen_file(collecter, 3, EU63_REQ_TYPE_REPORT);

    return ret;
}

int eu63_execute_set_req(eu63_collecter *collecter, eu63_param_list *set_list)
{
    int ret = -1;

    /* write req file*/
    printf("EU63 write set session file");
    ret = write_session_file(collecter, 1, EU63_REQ_TYPE_SET);
    if (ret < 0)
    {
        ilog_error("EU63 write set session file failed:%d", ret);
        return -1;
    }

    /* write job file */
    printf("EU63 write set job file");
    ret = write_presen_file_set(collecter, set_list);
    if (ret < 0)
    {
        ilog_error("EU63 write set job file failed:%d", ret);
        delete_session_file(collecter, 1);
        return -1;
    }

    /* listen file delete event */
    printf("EU63 wait delete set session file");
    ret = wait_session_file_delete(collecter, collecter->setup.request_timeout);
    if (ret < 0)
    {
        ilog_error("EU63 wait delete set session file failed:%d", ret);
        delete_session_file(collecter, 1);
        return -1;
    }

    /* prase session rsp */
    printf("EU63 prase set session rsp file");
    ret = prase_session_rsp_file(collecter);
    if (ret < 0)
    {
        ilog_error("EU63 prase set session rsp file failed:%d", ret);
        delete_session_file(collecter, 2);
        delete_presen_file(collecter, 1, EU63_REQ_TYPE_SET);
        return -1;
    }

    /* prase presen rsp */
    printf("EU63 prase set job rsp file");
    ret = prase_presen_rsp_file(collecter, EU63_REQ_TYPE_SET);
    if (ret < 0)
    {
        ilog_error("EU63 prase set job rsp file failed:%d", ret);
    }

    /* delete files */
    printf("EU63 delete set files");
    delete_session_file(collecter, 2);
    delete_presen_file(collecter, 3, EU63_REQ_TYPE_SET);

    return ret;
}

