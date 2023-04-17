#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../Src/eu63_collecter.h"


static char *param_name[] = {"ABC001", "ABC002", "ABC003", "ABC004"};
static void print_param_value(eu63_param_list *list)
{
    ty_list_t *pos, *n;
    eu63_report_param *param;

    ty_list_for_each_entry_safe(param, pos, n, &(list->param_head), param_node)
    {
        printf("%s value:%s\n", param->name, param->value);
    }
}

int main(int argc, char *argv[])
{
    int ret = -1;
    eu63_collecter *collecter = NULL;
    eu63_param_list *list = NULL;

    ret = eu63_create_share_folder();
    if (ret)
    {
        printf("EU63 create share folder failed\n");
        return -1;
    }

    collecter = eu63_collecter_create("IMM01");
    if (!collecter)
    {
        printf("EU63 create collecter failed\n");
        return -2;
    }

    ret = eu63_create_imm_folder(collecter);
    if (ret)
    {
        printf("EU63 create imm folder failed\n");
        ret = -3;
        goto _error;
    }

    list = eu63_create_param_list();
    if (!list)
    {
        printf("EU63 create param list failed\n");
        ret = -4;
        goto _error;
    }

    eu63_report_param *param = NULL;
    for (int i = 0; i < 4; i++)
    {
        param = eu63_create_param();
        if (!param)
        {
            continue;
        }

        strncpy(param->name, param_name[i], sizeof(param->name));
        ty_list_append_head(&list->param_head, &param->param_node);
        list->req_count++;
        printf("Append %s to param list\n", param->name);
    }

    if (0 == list->req_count)
    {
        printf("EU63 param list is null\n");
        ret = -5;
        goto _error;
    }

    while (1)
    {
        printf("REQ report file\n");
        ret = eu63_execute_report_req(collecter, list);
        if (!ret)
        {
            print_param_value(list);
        }
        else
        {
            printf("EU63 REQ report failed\n");
        }

        sleep(60);
    }

_error:
    if (collecter)
    {
        eu63_collecter_free(collecter);
    }
    if (list)
    {
        eu63_free_param_list(list);
    }

    return ret;
}