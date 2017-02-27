import random
import string
import socket
import sys
import pexpect
import pexpect.fdpexpect

# To create random strings
POSSIBILITIES = string.ascii_uppercase + string.digits + string.ascii_lowercase

# N is fixed RSA modulus. N and E are hardcoded into service
N = 93894598162875798175827979368825444470413609355497216638482102588337222100905983384833387646774634453957300089247103674921587066608960089800886344717774902375183504092230928149527263412343033142463971233777442075805670730606942549614969981720080148225089889230491227885597487768298864492978828904912463416033
D = 26196846474635457369806591738391645210209696004779996127907063884946611930895004443164601570430050063179718817338042490439617613148982029113465770582857191761955011121912820724474549710323046309815595968447616612665451043259663567664809850687434345003591938126387921069696573634127600189664583337893385825973

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
    c.sendline(login)
    c.sendline(password)
    c.expect("Logged as "+login)
    return (c.match != None)

def level_up(c, login, higher_level):
    c.expect(">")
    c.sendline("4")
    c.sendline(login)
    hl = True;
    if higher_level:
        c.expect("Level upped!")
        hl = (c.match != None)
    else:
        c.expect("Your level is too low")
    return (c.match != None) & hl

def write_poem(c, poem):
    c.expect(">")
    c.sendline("3")
    c.expect(">")
    c.sendline(poem)
    for i in xrange(24):
        c.sendline("1")
    c.expect("Your poem is successfully posted")
    return (c.match != None)

def read_poem(c, login, poem, high_level):
    c.expect(">")
    c.sendline("5")
    c.sendline(login)
    hl = True
    if high_level:
        c.expect("------------------------")
        c.expect("\w*\n")
        c.expect(poem)
        hl = (c.match != None)
    else:
        c.expect("Your level is too low")
    return (c.match != None) & hl



def benign(ip, port):
    login = ''.join(random.choice(POSSIBILITIES) for x in range(20))
    password = ''.join(random.choice(POSSIBILITIES) for x in range(20))
    poem = ''.join(random.choice(POSSIBILITIES) for x in range(35))

    while True:
        login2 = ''.join(random.choice(POSSIBILITIES) for x in range(20))
        password2 = ''.join(random.choice(POSSIBILITIES) for x in range(20))
        if (login2 != login) and (password2 != password):
            break

    while True:
        login3 = ''.join(random.choice(POSSIBILITIES) for x in range(20))
        password3 = ''.join(random.choice(POSSIBILITIES) for x in range(20))
        if (login3 != login2) and (login3 != login):
            if (password3 != password2) and (password3 != password):
                break;


    if ip:
        conn = socket.create_connection((ip,port))
        c = pexpect.fdpexpect.fdspawn(conn.fileno())
    else:
        c = pexpect.spawn("../service/ro/poetry")
        c.logfile = sys.stdout

    ret = False
    # Register the first user
    ret = register(c, login, password, True)
    assert ret == True
    # Register the second user
    register(c, login2, password2, False)
    assert ret == True
    # Register the third user and write a poem
    register(c, login3, password3, False)
    assert ret == True
    login_user(c, login3, password3)
    assert ret == True
    write_poem(c, poem)
    assert ret == True
    # Login the second user and try to read the third user poem
    login_user(c, login2, password2)
    assert ret == True
    read_poem(c, login3, poem, False)
    assert ret == True
    # Login the first user and level up the second user
    login_user(c, login, password)
    assert ret == True
    level_up(c, login2, True)
    assert ret == True
    # Login the second user and try to read the third user poem again
    login_user(c, login2, password2)
    assert ret == True
    read_poem(c, login3, poem, True)
    assert ret == True


    c.close()
    if ip: conn.close()


if __name__ == "__main__":
    benign(None, None)
