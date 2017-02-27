import random
import string
import socket
import sys
import pexpect
import pexpect.fdpexpect
import re

# To create random strings
POSSIBILITIES = string.ascii_uppercase + string.digits + string.ascii_lowercase
MORE_THAN_FLAG_LEN = 100
FLAG_REGEXP = "FLG\w*"

def login_user(c, login, password):
    c.expect(">")
    c.sendline("1")
    c.expect("Enter login:")
    c.sendline(login)
    c.expect("Enter password:")
    c.sendline(password)
    c.expect("Logged as "+login)
    return (c.match != None)

def read_own_poem(c, poem):
    c.expect(">")
    c.sendline("6")
    c.expect("Your poem")

    content = c.read(MORE_THAN_FLAG_LEN)
    f = re.search(poem, content)
    if f != None:
        content = f.group(0)
    else:
        content = ''
    return content

def get_flag(ip, port, flag_id, token):
    login = flag_id
    password = token

    if ip:
        conn = socket.create_connection((ip,port))
        c = pexpect.fdpexpect.fdspawn(conn.fileno())
    else:
        c = pexpect.spawn("../service/ro/poetry")
        c.logfile = sys.stdout

    login_user(c, login, password)
    content = read_own_poem(c, FLAG_REGEXP)

    c.expect(">")
    c.sendline("7")

    c.close()
    if ip: conn.close()

    return { 'FLAG': content }


if __name__ == "__main__":
    print get_flag(None, None, sys.argv[1], sys.argv[2])
