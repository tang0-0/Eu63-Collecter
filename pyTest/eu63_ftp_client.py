from ftplib import FTP
from glob import glob
from multiprocessing.sharedctypes import Value
import os
from pickle import TRUE
import time
import sys
from tkinter.messagebox import NO


FTP_IP="10.5.23.54"
FTP_PORT = 21
FTP_USER = "user"
FTP_PASSWORD = "password"

IMM_FOLDER = "/IMM01"
PARAM_LIST=["ABC001","ABC002","ABC003","ABC004","ABC005","ABC006","ABC007","ABC008","ABC009","ABC010"]
PARAM_VALUE = 0

def write_report_dat_file():
    comma = ','
    global PARAM_VALUE
    with open(os.path.abspath('0000R000.DAT'), mode='wt', encoding='utf-8') as file:
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
    for file in filelist:
        if file == "0000R000.JOB":
            print("Have REPORT JOB File")
            return 1
        elif file == "0000S000.JOB":
            print("Have SET JOB File")
            return 2
    return 0


def upload_file(ftp, job):
    fp = open(os.path.abspath('SESS0000.RSP'), 'rb')
    res = ftp.storbinary('STOR ' + "SESS0000.RSP", fp, 1024)  # 上传文件
    if res.find('226') != -1:
        print('upload file complete', "SESS0000.RSP")
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
        fp = open(os.path.abspath('0000R000.DAT'), 'rb')
        res = ftp.storbinary('STOR ' + "0000R000.DAT", fp, 1024)  # 上传文件
        if res.find('226') != -1:
            print('upload file complete', "0000R000.DAT")
        fp.close()
    print("delte SESS0000.REQ")
    ftp.delete("SESS0000.REQ")

def ftp_quit(ftp):
    print("quit ftp")
    client.quit()

if __name__ == '__main__':
    client = ftp_connect()
    if client:
        while TRUE:
            try:
                check = check_job(client)
                if check != 0:
                    print("PARAM_VALUE:",PARAM_VALUE)
                    write_report_dat_file()
                    upload_file(client,check)
            except:
                ftp_quit(client)
                sys.exit()
            time.sleep(5)
    




    
