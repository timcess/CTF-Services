import random
import string
import socket
import sys
import pexpect
import pexpect.fdpexpect

# To create random strings
POSSIBILITIES = string.ascii_uppercase + string.digits + string.ascii_lowercase

def register(c, login, password, high_level):
    c.expect(">")
    c.sendline("2")
    c.sendline(login)
    c.sendline(password)
    c.expect("Hmm, by the way, can you understand what is written there?")
    c.expect("0x[0-9a-z]*")
    hl = True
    if high_level:
        cipher_text = int(c.match.group(0), 16)
        plain_text = pow(cipher_text, D, N)
        c.sendline(str(plain_text))
        c.expect("Come in, my lord")
        hl = (c.match != None)
    else:
        c.sendline("1")
    c.expect("You have been successfully registered")
    return (c.match != None) & hl

def login_user(c, login, password):
    c.expect(">")
    c.sendline("1")
    c.expect("Enter login:")
    c.sendline(login)
    c.expect("Enter password:")
    c.sendline(password)
    c.expect("Logged as "+login)
    return (c.match != None)

def write_poem(c, poem):
    c.expect(">")
    c.sendline("3")
    c.expect(">")
    p = poem.split('\n')
    if p[-1] == '':
        p = p[:-1]
    for item in p:
        c.sendline(item)
    for i in xrange(25-len(p)):
        c.sendline("1")
    c.expect("Your poem is successfully posted")
    return (c.match != None)

def set_flag(ip, port, flag):
    login = ''.join(random.choice(POSSIBILITIES) for x in range(20))
    password = ''.join(random.choice(POSSIBILITIES) for x in range(20))
    content = flag

    if ip:
        conn = socket.create_connection((ip,port))
        c = pexpect.fdpexpect.fdspawn(conn.fileno())
    else:
        c = pexpect.spawn("../service/ro/poetry")
        c.logfile = sys.stdout

    register(c, login, password, False)
    login_user(c, login, password)
    write_poem(c, content)

    c.expect(">")
    c.sendline("7")

    c.close()
    if ip: conn.close()

    return {
            'FLAG_ID': login, # Unique id for each flag
            'TOKEN': password,  # benign (get_flag) will know this, exploits will not
            }


if __name__ == "__main__":
    print set_flag(None, None, "FLG_just_testing")
