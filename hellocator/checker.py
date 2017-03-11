#!/usr/bin/env python

import socket
import sys
import md5

EXITCODE_OK         = 0
EXITCODE_CONNECTION = 1
EXITCODE_WRONGFLAG  = 5
EXITCODE_INVARG     = 3
EXITCODE_FUNCLACK   = 9

RECV_SIZE           = 8192

ADMIN_LOGIN         = "admin"
TEST_LOGIN          = "test"
TEST_PASSWORD       = "passwtest"

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
        
    def get(self, flag_id, flag):
        self.send("LOGIN")
        self.send(ADMIN_LOGIN)
        self.send(ADMIN_PASSWORD, 1)
        self.send("DOWNLOAD")
        f = self.send(flag_id)
        if f.find(flag) == -1:
            return EXITCODE_WRONGFLAG
        else:
            return EXITCODE_OK
    def put(self, flag_id, flag):
        self.send("LOGIN")
        self.send(ADMIN_LOGIN)
        self.send(ADMIN_PASSWORD, 1)
        self.send("ALLOCATE")
        self.send(len(flag), 1)
        self.send("UPLOAD")
        self.send(flag)
        self.send(flag_id, 1)
        return self.get(flag_id, flag)
    def send(self, msg, wait_c=0):
        self.sock_desc.send(str(msg)+"\n")
        if wait_c:
            res = self.recv(1)
        else:
            res = self.recv()
        return res
    def test(self):
        self.send("REGISTER")
        self.send(TEST_LOGIN)
        res = self.send(TEST_PASSWORD)
        self.recv(1)
        if res.find("Success") == -1 and res.find("Login already exists") == -1:
            return EXITCODE_FUNCLACK 
        self.send("LOGIN")
        self.send(TEST_LOGIN)
        res = self.send(TEST_PASSWORD)
        self.recv(1)
        if res.find("Hello") == -1:
            return EXITCODE_FUNCLACK 
        self.send("ALLOCATE")
        res = self.send("1")
        self.recv(1)
        if res.find("Here") == -1:
            return EXITCODE_FUNCLACK 
        return EXITCODE_OK
        


if __name__ == '__main__':
    command = sys.argv[1].upper()
    if command not in ['STORE', 'RETRIEVE', 'TEST']:
        sys.exit(EXITCODE_INVARG)

    
    host = sys.argv[2]

    m = md5.new()
    m.update(sys.argv[2])
    ADMIN_PASSWORD = m.hexdigest()[:9][::-1]
    #ADMIN_PASSWORD = "uTe4aigh"
    print ADMIN_PASSWORD
    c = Checker(host, 31337)
    if command == 'RETRIEVE':
        exit(c.get(sys.argv[3], sys.argv[4]))
    if command == 'STORE':
        exit(c.put(sys.argv[3], sys.argv[4]))
    if command == 'TEST':
        exit(c.test())
