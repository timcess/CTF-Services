#!/usr/bin/env python

import socket
import sys
import md5
import random
import string

SERVICE_PORT        = 31337

LOGIN_LEN           = 15
PASSW_LEN           = 8
CONTENT_LEN         = 50
FNAME_LEN           = 10

EXITCODE_OK         = 101
EXITCODE_WRONGFLAG  = 102
EXITCODE_FUNCLACK   = 103
EXITCODE_DOWN       = 104
EXITCODE_INVARG     = 110

RECV_SIZE           = 8192

# You probably should set it to 0 during the game
DEBUG               = 1



def gen_rand_printable(count):
    alphabet = string.uppercase + string.lowercase + string.digits
    lst = [random.choice(alphabet) for _ in xrange(count)]
    return ''.join(lst)

def gen_random_creds():
    login = gen_rand_printable(LOGIN_LEN)
    passw = gen_rand_printable(PASSW_LEN)

    return login, passw

def gen_creds_by_flag_id(flag_id):
    m = md5.new()
    m.update("1ai!E0S_" + flag_id)
    digest = m.hexdigest()
    login = digest[:LOGIN_LEN][::-1]
    passw = digest[-PASSW_LEN:][::-1]

    return login, passw

def debug(msg):
    if DEBUG: print(msg)
    
    

class Checker(object):
    sock_desc = None
    def __init__(self, host, port):
        self.sock_desc = socket.socket()
        self.sock_desc.settimeout(3)
        try:
            self.sock_desc.connect((host, port))
            self.recv(1)
        except:
            exit(EXITCODE_DOWN)

    def __del__(self):
        self.sock_desc.close()

    def recv(self, c=0):
        res = self.sock_desc.recv(RECV_SIZE)
        if c == 1:
            while res.find("->") == -1:
                res = self.sock_desc.recv(RECV_SIZE)
        return res

    def send(self, msg, wait_c=0):
        self.sock_desc.send(str(msg)+"\n")
        if wait_c:
            res = self.recv(1)
        else:
            res = self.recv()
        return res

    def login_to(self, login, passw):
        res = self.send("LOGIN")
        res += self.send(login)
        res += self.send(passw)
        res += self.recv(1)
        if res.find("Hello") == -1:
            debug(res)
            return EXITCODE_FUNCLACK

        debug("Logged ok" + " ({}:{})".format(login, passw))
        return 0

    def register(self, login, passw):
        self.send("REGISTER")
        res = self.send(login)
        res += self.send(passw)
        res += self.recv(1)
        if res.find("Success registration") == -1:
            debug(res)
            return EXITCODE_FUNCLACK

        debug("Registered ok" + " ({}:{})".format(login, passw))
        return 0

    def allocate(self, count):
        self.send("ALLOCATE")
        res = self.send(count)
        res += self.recv(1)
        if res.find("Here, take free memory!") == -1:
            debug(res)
            return EXITCODE_FUNCLACK 

        debug("Allocated ok")
        return 0

    def upload(self, fname, content):
        self.send("UPLOAD")
        res = self.send(content)
        if res.find("Which file do you want this to write in?") == -1:
            debug(res)
            self.recv(1)
            return EXITCODE_FUNCLACK 

        res += self.send(fname)
        res += self.recv(1)
        if res.find("Uploading went well, you may now continue.") == -1:
            debug(res)
            return EXITCODE_FUNCLACK 

        debug("Uploaded ok")
        return 0

    def download(self, fname):
        res = self.send("DOWNLOAD")
        if res.find("What file do you want download?") == -1:
            debug(res)
            self.recv(1)
            return EXITCODE_FUNCLACK, ""

        res = self.send(fname)
        res += self.recv(1)
        debug("Downloaded ok")
        return 0, res

    def list_users(self):
        res = self.send("LIST USERS")
        res += self.recv(1)
        ul = map(lambda x: x.strip(), res.split('\n'))
        res_ul = []
        for item in ul:
            if item != '' and item != '->':
                res_ul.append(item)

        debug("User list: ")
        for item in res_ul:
            debug("\t"+item)

        return 0, res_ul

    def list_files(self):
        res = self.send("LIST FILES")
        res += self.recv(1)
        if res.find("Log in firstly") != -1:
            res_fl = []
        else:
            fl = map(lambda x: x.strip(), res.split('\n'))
            res_fl = []
            for item in fl:
                if item != '' and item != '->':
                    res_fl.append(item)

        debug("Files list: ")
        for item in res_fl:
            debug("\t"+item)

        return 0, res_fl
        
        
    def get(self, flag_id, flag):
        login, passw = gen_creds_by_flag_id(flag_id)

        ret = self.login_to(login, passw)
        if ret: return ret

        ret, file_content = self.download(flag_id)
        if ret: return ret
        if file_content.find(flag) == -1:
            return EXITCODE_WRONGFLAG

        return EXITCODE_OK

    # Service generates equals login:pass for two equals flag ids
    # So, be sure that you always run with different flag ids
    # Otherwise service will get wrong state
    def put(self, flag_id, flag):
        login, passw = gen_creds_by_flag_id(flag_id)

        ret = self.register(login, passw)
        if ret: return ret

        ret = self.login_to(login, passw)
        if ret: return ret

        ret = self.allocate(len(flag))
        if ret: return ret

        ret = self.upload(flag_id, flag)
        if ret: return ret
        
        # PUT always try to GET to ensure that flag on the service
        # If checksystem does it for you just comment this string
        # and uncomment next one.
        return self.get(flag_id, flag)
        # return EXITCODE_OK


    def test(self):
        login, passw = gen_random_creds()

        ret = self.register(login, passw)
        if ret: return ret

        ret = self.login_to(login, passw)
        if ret: return ret

        ret, ul = self.list_users()
        if ret: return ret
        try:
            ul.index(login)
        except:
            return EXITCODE_FUNCLACK

        
        # Check base functionality
        ret = self.allocate(str(CONTENT_LEN))
        content = gen_rand_printable(CONTENT_LEN)
        fname = gen_rand_printable(FNAME_LEN)
        if ret: return ret

        ret = self.upload(fname, content)
        if ret: return ret

        ret, fl = self.list_files()
        if ret: return ret
        try:
            fl.index(fname)
        except:
            return EXITCODE_FUNCLACK

        ret, file_content  = self.download(fname)
        if ret: return ret
        if file_content.find(content) == -1:
            return EXITCODE_FUNCLACK
        

        return EXITCODE_OK
        


if __name__ == '__main__':

    if (len(sys.argv) < 3):
        print("python {} [PUT|GET|CHECK] <host> <cmd args>".format(sys.argv[0]))
        sys.exit(EXITCODE_INVARG)
        
    command = sys.argv[1].upper()
    if command not in ['PUT', 'GET', 'CHECK']:
        sys.exit(EXITCODE_INVARG)

    
    host = sys.argv[2]
    c = Checker(host, SERVICE_PORT)

    if command == 'GET':
        if (len(sys.argv) < 5):
            print("python {} GET <host> <flag id> <expected flag>")
            sys.exit(EXITCODE_INVARG)
        else:
            exit(c.get(sys.argv[3], sys.argv[4]))
    if command == 'PUT':
        if (len(sys.argv) < 5):
            print("python {} PUT <host> <flag id> <flag>")
            sys.exit(EXITCODE_INVARG)
        else:
            exit(c.put(sys.argv[3], sys.argv[4]))
    if command == 'CHECK':
        exit(c.test())
