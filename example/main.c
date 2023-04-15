#include <stdio.h>
#include <string.h>
#include "eu63_collecter.h"


static char *param_name[] = {"ABC001", "ABC002", "ABC003", "ABC004"};

int main()
{
    int ret = -1;
    eu63_collecter *collecter = NULL;
    eu63_param_list *list = NULL;

    ret = eu63_create_share_folder();
    if (ret)
    {
        printf("EU63 create share folder failed");
        return -1;
    }

    collecter = eu63_collecter_create("IMM01");
    if (!collecter)
    {
        printf("EU63 create collecter failed");
        return -2;
    }

    ret = eu63_create_imm_folder(collecter);
    if (ret)
    {
        printf("EU63 create imm folder failed");
        goto _error;
    }

    list = eu63_create_param_list();
    if (!list)
    {
        printf("EU63 create param list failed");
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
        ty_list_insert_after(&list->param_head, &param->param_node);
        printf("Append %s to param list", param->name);
    }

    if (ty_list_is_empty(list))
    {
        printf("EU63 param list is null");
        goto _error;
    }

    while (1)
    {
        eu63_execute_report_req(collecter, list);

        sleep(60);
    }

    return 0;

_error:
    if (collecter)
    {
        eu63_collecter_free(collecter);
    }
    if (list)
    {
        eu63_free_param_list(list);
    }

    return -9;
}