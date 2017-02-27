import tornado.httpserver
import tornado.websocket
import tornado.ioloop
import tornado.web

from tornado.options import define, options

import md5
import png
import json
import binascii
import pyqrcode
import base64

import database

define("port", default=8889, help="run on the given port", type=int)


class BaseHandler(tornado.web.RequestHandler):
    def get_current_user(self):
        return self.get_cookie('patient_token')

    def logout(self):
        self.clear_cookie('patient_token')


class MainHandler(BaseHandler):
    def get(self):
        self.render("templates/index.html")


class AuthHandler(BaseHandler):
    def get(self, url):
        if url == 'login':
            self.render("templates/login.html", result=None)
        if url == 'logout':
            self.logout()
            self.redirect('/')

    def post(self, url):
        if url == 'login':
            if self.check_agruments():
                self.redirect("/")
            else:
                self.render("templates/login.html", result="Wrong key")

    def check_agruments(self):
        secret_key = str(self.request.arguments['secret_key'][0])
        if database.get_data(secret_key) != None:
            self.set_cookie('patient_token', secret_key)
            return True
        else:
            return False


class RegisterHandler(BaseHandler):
    def get(self):
        self.render("templates/register.html")

    def post(self):
        content_type = self.request.headers['Content-type']
        if content_type.find('image') == -1:
            self.write("Not an image")
        else:
            secret = self.secret_from_png(self.request.body)
            if database.get_data(secret) != None:
                self.write("You are alredy registered")
            else:
                if database.add_data(secret, {}):
                    self.write(secret)
                else:
                    self.write("Something went wrong..")

    def secret_from_png(self, f):
        pp = png.Reader(bytes=f)
        all_crcs = ''
        for i in pp.chunks():
            crc = binascii.crc32(i[0] + i[1])
            if crc < 0:
                crc += pow(2, 32)
            all_crcs += hex(crc)[2:]
        return md5.new(all_crcs).hexdigest()


class Data(BaseHandler):
    def generate_qrxray(self, patient_information):
        image = pyqrcode.create(json.dumps(patient_information))
        return image

    @tornado.web.authenticated
    def get(self, url):
        if url == "set_data":
            self.render("templates/set_data.html", result=None)
        elif url == "get_data":
            token = self.get_current_user()
            self.render("templates/get_data.html", patient_info=database.get_data(token), xray=None)
        elif url == "get_qrxray":
            token = self.get_current_user()
            xray_file = 'static/xrays/'+str(token)
            
            try:
                xrf = open(xray_file, 'r')
                img = xrf.read()
                xrf.close()
            except IOError as e:
                patient_info = database.get_data(token)
                qrxray = self.generate_qrxray(patient_info)
                qrxray.png(xray_file, scale=6)
                xrf = open(xray_file, 'r')
                img = xrf.read()
            
            patient_info = database.get_data(token)
            xrayb64 = base64.b64encode(img)    
            self.render("templates/get_data.html", patient_info=patient_info, xray=xrayb64)

    @tornado.web.authenticated
    def post(self, url):
        if url == 'set_data':
            token = self.get_current_user()
            patient_info = self.request.arguments
            database.add_data(token, patient_info)
            self.render("templates/set_data.html", result=True)


def main():
    application = tornado.web.Application([
        (r"/", MainHandler),
        (r"/auth/(.*)", AuthHandler),
        (r"/register", RegisterHandler),
        (r"/data/(.*)", Data),
        (r'/static/(.*)', tornado.web.StaticFileHandler, {'path': 'static/'})],
        login_url=r"/auth/login"
    )
    http_server = tornado.httpserver.HTTPServer(application)
    http_server.listen(options.port)
    tornado.ioloop.IOLoop.current().start()


if __name__ == "__main__":
    main()
