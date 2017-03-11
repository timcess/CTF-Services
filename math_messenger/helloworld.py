#!/usr/bin/env python
import tornado.httpserver
import tornado.ioloop
import tornado.options
import tornado.web

from random import seed,randrange
import os
#import db as my_db
#import scheme
from tornado.options import define, options
import base64

define("port", default=8888, help="run on the given port", type=int)

class BaseHandler(tornado.web.RequestHandler):
    def get_current_user(self):
        return self.get_secure_cookie("user")

class MainHandler(BaseHandler):
    def get(self):
        if self.get_current_user() == None:
            result = None
            self.render("index.html", result=None, user=None)
        else:
            result = self.get_current_user()
            ms = my_db.get_last_N_messages(10)
            self.render("index.html", result=result, ms=ms, user=result)
        
class RegisterHandler(BaseHandler):
    def get(self):
        self.render("register.html", result=None)

    def post(self):
        login = self.get_argument("user_login")
        password = self.get_argument("user_pass")
        if login == None or len(login) < 1:
            self.render("register.html", result="Fill login and password")
            return
        if password == None or len(password) < 1:
            self.render("register.html", result="Fill login and password")
            return
        self.clear_cookie("user")
        try:
            if not my_db.check_user_exists(login):
                key_pair = scheme.generate_keys()
                my_db.add_user(login, password, key_pair)
                self.set_secure_cookie("user", login)
                self.redirect("/")
            else:
                self.render("register.html", result="Such user already exists")
        except:
            self.render("register.html", result="Technical failure")

class LoginHandler(BaseHandler):
    def get(self):
        self.render("login.html", result=None)
    
    def post(self):
        login = self.get_argument("user_login")
        password = self.get_argument("user_pass")
        self.clear_cookie("user")
        try:
            if my_db.check_user_auth(login, password):
                self.set_secure_cookie("user", login)
                self.redirect("/")
            else:
                self.render("login.html", result="Wrong login or password")
        except:
            self.render("login.html", result="Technical failure")
            

class UserListHandler(BaseHandler):
    @tornado.web.authenticated
    def get(self):
        #TODO: get users from base
		all_users = my_db.get_all_users_and_keys()
		self.render("user_list.html", all_users=all_users, user=self.get_current_user())

class AllMessagesHandler(BaseHandler):
    @tornado.web.authenticated
    def get(self):
        #TODO: get messages from base
 #       all_msgs = ["FROM", "TO", "msg"*2]
		all_msgs = my_db.get_all_messages()
		self.render("messages_list.html", all_msgs=all_msgs, user=self.get_current_user())

class WhySoSecureHandler(BaseHandler):
    @tornado.web.authenticated
    def get(self):
        self.render("about_scheme.html", user=self.get_current_user())

class LogoutHandler(BaseHandler):
    def get(self):
        self.clear_cookie("user")
        self.redirect("/")

class PostMessageHandler(BaseHandler):
	@tornado.web.authenticated
	def get(self):
		name_r = self.get_argument("user_name")
		msg = self.get_argument("msg")
		if my_db.check_user_exists(name_r):
			crypt_msg = scheme.encrypt_msg(msg, self.current_user, name_r)
			my_db.add_message(base64.b64encode(crypt_msg), self.current_user, name_r)
		else:
			pass	
		self.redirect("/")

class CheckHandler(BaseHandler):
    def get(self):
        login = self.get_current_user()
        all_msg = my_db.get_all_user_messages(login)
        for item in all_msg:
    		dec_mes = scheme.decrypt_msg(base64.b64decode(item[1]), login, login)
	    	self.write(str(item[0])+"-"+dec_mes+"\n")
        

class CryptHandler(BaseHandler):
	@tornado.web.authenticated
	def get(self):
		mes_id = self.get_argument("m_id")
		sender, reciever, mes, date = my_db.get_message(mes_id)
		if not sender == self.current_user and not reciever == self.current_user:
			self.write("Merry Christmas, son of the bitch! And Happy New year!")
			return
		dec_mes = scheme.decrypt_msg(base64.b64decode(mes), sender, reciever)
		self.write(dec_mes)
		
def main():
    seed()
    tornado.options.parse_command_line()
    application = tornado.web.Application(
        [
        (r"/", MainHandler),
        (r"/register", RegisterHandler),
		(r"/post_message", PostMessageHandler),
        (r"/login", LoginHandler),
        (r"/userlist", UserListHandler),
        (r"/allmessages", AllMessagesHandler),
        (r"/whysecure", WhySoSecureHandler),
        (r"/logout", LogoutHandler),
		(r"/cryptor", CryptHandler),
		(r"/check", CheckHandler),
        ],
        static_path = os.path.curdir+"/static",
        template_path = os.path.curdir+"/template",
 #       cookie_secret = "".join(map(chr, [randrange(ord('a'), ord('z')) for i in xrange(50)])),
        cookie_secret = "asdfasdf",
        login_url="/login"
    )

    http_server = tornado.httpserver.HTTPServer(application)
    http_server.listen(options.port)
    tornado.ioloop.IOLoop.instance().start()


if __name__ == "__main__":
    main()
