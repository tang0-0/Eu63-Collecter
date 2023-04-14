from ftplib import FTP
import os
import time


FTP_IP="10.5.23.52"
FTP_PORT = 28
FTP_USER = "adm"
FTP_PASSWORD = "123456"

IMM_FOLDER = "/IMM01"
SESSION_REQ_FILE_NAME = "SESS0000.REQ"
SESSION_RSP_FILE_NAME = "SESS0000.RSP"
REPORT_JOB_FILE_NAME = "0000R000.JOB"
REPORT_DAT_FILE_NAME = "0000R000.DAT"
SET_JOB_FILE_NAME = "0000S000.JOB"

PARAM_LIST=["ABC001","ABC002","ABC003","ABC004","ABC005","ABC006","ABC007","ABC008","ABC009","ABC010"]
PARAM_VALUE = 0


def write_session_rsp_file(job,result):
    if job == 0:
        return
    with open(os.path.abspath(SESSION_RSP_FILE_NAME), mode='wt+', encoding='utf-8') as file:
        if result:
            file.write("00000000 PROCESSED;\r")
            file.write("00000001 PROCESSED;")
        else:
            file.write("00000000 ERROR 05 00000001 \"This is a error\";\r")
            file.write("00000001 ERROR 05 00000007 \"This is a error\";")
        file.close()

def write_job_rsp_file(type,result):
    if type == 1:
        file_path = os.path.abspath("0000R000.RSP")
    elif type == 2:
        file_path = os.path.abspath("0000S000.RSP")
    else:
        return
    with open(file_path, mode='wt+', encoding='utf-8') as file:
        timestamp=time.time()
        local_time = time.localtime(timestamp)
        timestamp = time.strftime("%Y%m%d %H:%M:%S", local_time)
        if result:
            text1 = "COMMAND 1 PROCESSED \"OK\" "+timestamp+";\r"
            text2 = "COMMAND 1 PROCESSED \"OK\" "+timestamp+";"
            file.write(text1)
            file.write(text2)
        else:
            text1 = "00000000 ERROR 06 00000004 \"This is a error\" "+timestamp+";\r"
            text2 = "00000001 ERROR 06 00000006 \"This is a error\" "+timestamp+";"
            file.write(text1)
            file.write(text2)
        file.close()

def write_data_file(type):
    if type == 1:
        comma = ','
        global PARAM_VALUE
        print("Value:",PARAM_VALUE)
        with open(os.path.abspath(REPORT_DAT_FILE_NAME), mode='wt+', encoding='utf-8') as file:
            file.write(comma.join(PARAM_LIST))
            file.write("\r")
            value_list = [PARAM_VALUE for i in range(len(PARAM_LIST))]
            num_list_new = [str(x) for x in value_list]
            file.write(comma.join(num_list_new))
            file.write("\r")
            file.close()
            PARAM_VALUE = PARAM_VALUE+1

def ftp_connect():
    ftp = FTP()
    ftp.encoding = 'utf-8'
    ftp.set_debuglevel(0)
    try:
        ftp.connect(FTP_IP, FTP_PORT)
        ftp.login(FTP_USER, FTP_PASSWORD)
        print(ftp.getwelcome())
        ftp.cwd(IMM_FOLDER)
    except:
        print("FTP Connect failed")
        return None
    print("ftp connect success")
    return ftp

def check_job(ftp):
    filelist = ftp.nlst()
    # print(filelist)
    if SESSION_REQ_FILE_NAME in filelist:
        print("Has SESSION REQ File")
        if REPORT_JOB_FILE_NAME in filelist:
            print("Wao! Has REPORT JOB File")
            return 1
        if SET_JOB_FILE_NAME in filelist:
            print("Wao! Has SET JOB File")
            return 2
    return 0

def upload_file(ftp, job):
    if job == 0:
        return
    
    fp = open(os.path.abspath(SESSION_RSP_FILE_NAME), 'rb')
    res = ftp.storbinary('STOR ' + SESSION_RSP_FILE_NAME, fp, 1024)  # 上传文件
    if res.find('226') != -1:
        print('upload file complete', SESSION_RSP_FILE_NAME)
    fp.close()

    if 1 == job:
        fp = open(os.path.abspath('0000R000.RSP'), 'rb')
        res = ftp.storbinary('STOR ' + "0000R000.RSP", fp, 1024)  # 上传文件
        if res.find('226') != -1:
            print('upload file complete', "0000R000.RSP")
        fp.close()
    else:
        fp = open(os.path.abspath('0000S000.RSP'), 'rb')
        res = ftp.storbinary('STOR ' + "0000S000.RSP", fp, 1024)  # 上传文件
        if res.find('226') != -1:
            print('upload file complete', "0000S000.RSP")
        fp.close()

    if 1 == job:
        fp = open(os.path.abspath(REPORT_DAT_FILE_NAME), 'rb')
        res = ftp.storbinary('STOR ' + REPORT_DAT_FILE_NAME, fp, 1024)  # 上传文件
        if res.find('226') != -1:
            print('upload file complete', REPORT_DAT_FILE_NAME)
        fp.close()
    print("delte file complete",SESSION_REQ_FILE_NAME)
    ftp.delete(SESSION_REQ_FILE_NAME)

def ftp_quit(ftp):
    print("quit ftp")
    client.quit()


if __name__ == '__main__':
    with ftp_connect() as client:
        while True:
            job = check_job(client)
            write_session_rsp_file(job,False)
            write_job_rsp_file(job,False)
            write_data_file(job)
            upload_file(client, job)
            time.sleep(5)
