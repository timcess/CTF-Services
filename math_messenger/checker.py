#!/usr/bin/env python

import requests
import sys
import md5

EXITCODE_OK         = 0
EXITCODE_DOWN       = 1
EXITCODE_WRONGFLAG  = 5
EXITCODE_INVARG     = 3
EXITCODE_MUMBLE     = 4
EXITCODE_FUNCLACK   = 9

RECV_SIZE           = 8192

class Checker(object):
    url = None
    def __init__(self, host, port):
        try:
            self.url = "http://"+str(host)+":"+str(port)
            r = requests.get(self.url)
        except:
            exit(EXITCODE_DOWN)

    def get(self, flag_id, flag):
		r = requests.get(self.url+"/logout")
		if r.status_code != 200:
			return EXITCODE_DOWN
		m = md5.new()
		m.update(flag_id)
		login = m.hexdigest()[:7]
		m.update(login)
		password = m.hexdigest()[:7]
		payload={'user_login':login, 'user_pass':password}
		r = requests.post(self.url+"/login", data=payload, allow_redirects=False)
		if r.text.find("Technical") != -1:
			return(EXITCODE_MUMBLE)
		if r.text.find("Wrong") != -1:
			return(EXITCODE_INVARG)
		try:
			authcookies = dict(user=r.cookies["user"])
		except:
			print "Error while save cookies"
			return(EXITCODE_WRONGFLAG)
		r = requests.get(self.url+"/check", cookies=authcookies)
		i = r.text.find("-")
		j = r.text.find("\n")
		fl = r.text[i+1:j]
		if fl == flag:
			return EXITCODE_OK
		else:
			return EXITCODE_WRONGFLAG
				
    def put(self, flag_id, flag):
        r = requests.get(self.url+"/logout")
        if r.status_code != 200:
            return EXITCODE_DOWN
        m = md5.new()
        m.update(flag_id)
        login = m.hexdigest()[:7]
        m.update(login)
        password = m.hexdigest()[:7]
        print login
        print password
        payload={'user_login':login, 'user_pass':password}
        r = requests.post(self.url+"/register", data=payload, allow_redirects=False)
        if r.text.find("Technical") != -1:
            return(EXITCODE_MUMBLE)
        #two identical flag_id
        if r.text.find("Such user") != -1:
            return(EXITCODE_INVARG)
        authcookies = dict(user=r.cookies["user"])
        r = requests.get(self.url+"/post_message?user_name="+login+"&msg="+flag,  cookies=authcookies)
        if r.status_code != 200:
            return EXITCODE_DOWN
        return(self.get(flag_id, flag))

    def test(self):
        pass
        


if __name__ == '__main__':
    command = sys.argv[1].upper()
    if command not in ['STORE', 'RETRIEVE', 'TEST']:
        sys.exit(EXITCODE_INVARG)

    host = sys.argv[2]

    c = Checker(host, 8888)
    if command == 'RETRIEVE':
        exit(c.get(sys.argv[3], sys.argv[4]))
    if command == 'STORE':
        exit(c.put(sys.argv[3], sys.argv[4]))
    if command == 'TEST':
        exit(c.test())
