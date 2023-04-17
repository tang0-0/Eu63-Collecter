#ifndef __EUROMAP63_COLLECTER_H__
#define __EUROMAP63_COLLECTER_H__

#include "ty_list.h"

#define EUROMAP63_SHARE_FOLDER_PATH     "/home/tang/Inhand/EU63/"
#define EUROMAP63_IMM_NAME_MAX_LEN      48
#define EUROMAP63_PARAM_NAME_MAX_LEN    32
#define EUROMAP63_CONNECT_TIMEOUT_MS    5000
#define EUROMAP63_REQUEST_TIMEOUT_MS    30000
#define EUROMAP63_MIN_SESSION_NUM       0
#define EUROMAP63_MAX_SESSION_NUM       10
#define EUROMAP63_HAS_CYCLIC            1
#define EUROMAP63_ENCODE                1


typedef struct
{
    int connect_timeout;
    int request_timeout;
    int min_session;
    int max_session;
    int encode;
    int cyclic;
    char imm[EUROMAP63_IMM_NAME_MAX_LEN + 4];
} eu63_setup;

typedef struct
{
    int now_session;
    int now_presen;
    eu63_setup setup;
} eu63_collecter;

typedef struct
{
    char name[EUROMAP63_PARAM_NAME_MAX_LEN + 4];
    char *value;
    ty_list_t param_node;
} eu63_report_param;

typedef struct
{
    int req_count;
    int rsp_count;
    ty_list_t param_head;
} eu63_param_list;

typedef enum
{
    EU63_REQ_TYPE_NONE = 0,
    EU63_REQ_TYPE_REPORT = 1,
    EU63_REQ_TYPE_SET,
    EU63_REQ_TYPE_GETID
} eu63_req_type;

typedef enum
{
    EU63_ENCODE_ANSI = 0,
    EU63_ENCODE_UTF8 = 1
} eu63_encode;


int eu63_create_share_folder(void);
int eu63_delete_share_folder(void);

eu63_collecter *eu63_collecter_create(const char *imm);
void eu63_collecter_free(eu63_collecter *collecter);

int eu63_create_imm_folder(eu63_collecter *collecter);
int eu63_delete_imm_folder(eu63_collecter *collecter);
int eu63_clear_imm_folder(eu63_collecter *collecter);

eu63_report_param *eu63_create_param(void);
int eu63_free_param(eu63_report_param *param);
eu63_param_list *eu63_create_param_list(void);
int eu63_free_param_list(eu63_param_list *list);

int eu63_execute_connect_req(eu63_collecter *collecter);
int eu63_execute_report_req(eu63_collecter *collecter, eu63_param_list *report_list);
int eu63_execute_set_req(eu63_collecter *collecter, eu63_param_list *set_list);

#endif

