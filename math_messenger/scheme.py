
from sage.all import *
from Crypto.Cipher import  AES
import db
import re

KEY_LEN = 16
IV = "0123456789abcdef"
q = next_prime(1 << 128)
m = 10
F = GF(q)

def generate_invertible_matrix(F, m): 
	a = [[0] * m for i in xrange(m)]
	M = matrix(F, a)	
	while True:
		for i in xrange(m):
			for j in xrange(i,m):
				r = F.random_element()
				M[i,j] = M[j,i] = r
		if M.is_invertible():
			return M
	
	
M = generate_invertible_matrix(F,m)

class crypt_message_exception(Exception):
	def __init__(self, mes):
		self.message = mes
	def __str__(self):
		return repr(self.message)

def generate_keys():
	pk = vector([F.random_element() for i in xrange(m)])
	sk = M * pk
	return {'sk': sk, 'pk' : pk}

def create_shared_key(s_k, r_k):
	sh_k = s_k['sk'] * r_k['pk']
	sh_k = int(sh_k)
	s = ""
	while sh_k != 0:
		s += chr(sh_k & 0xff)
		sh_k = sh_k >> 8
	if len(s) != KEY_LEN:
		s += "\x00" * (KEY_LEN - len(s))
	return s

def pad_message(mes):
	len_block = KEY_LEN - len(mes) % KEY_LEN
	pad = '\x00' * (len_block - 1) + chr(len_block)
	return mes + pad

def unpad_message(mes):
	len_block = ord(mes[-1])
	if len_block > KEY_LEN:
		raise crypt_message_exception("Bad padding, go fuck yourself!")
	if any(map(lambda x: ord(x) != 0, mes[-len_block:-1])):
		raise crypt_message_exception("Bad padding, go fuck yourself!")
	return mes[:-len_block]

def get_key(name):
	key = db.get_key_by_name(name)
	sk = tuple(F(v) for v in re.findall('[0-9]+', key['sk']))
	pk = tuple(F(v) for v in re.findall('[0-9]+', key['pk']))
	return {'sk':vector(F,sk), 'pk':vector(F,pk)}

def encrypt_msg(message, sender, reciever):
	s_k = get_key(sender)
	r_k = get_key(reciever)
	sh_k = create_shared_key(s_k, r_k)
	ciph = AES.new(str(sh_k), AES.MODE_CBC, IV)
	padded_mes = pad_message(message)
	cr_mes = ciph.encrypt(padded_mes)
	return cr_mes

def decrypt_msg(message, sender, reciever):
	s_k = get_key(sender)
	r_k = get_key(reciever)
	sh_k = create_shared_key(s_k, r_k)
	ciph = AES.new(str(sh_k), AES.MODE_CBC, IV)
	p_m = ciph.decrypt(message)
	return unpad_message(p_m)
