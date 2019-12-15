import ssl
import socket

def sendEMail(text):
    context = ssl._create_stdlib_context()
    new_socket = socket.create_connection(("smtp.yandex.ru", 465), 5,
                                         None)
    new_socket =context.wrap_socket(new_socket,
                                          server_hostname="smtp.yandex.ru")
    new_socket.sendall( b'ehlo [192.168.43.94]\r\n')
    new_socket.sendall( b'AUTH PLAIN AE11c2ljRGV2ZWxvcAAxMm11c2ljOTA=\r\n')
    new_socket.sendall( b'mail FROM:<MusicDevelop@yandex.ru> size=92\r\n')
    new_socket.sendall(b'rcpt TO:<cappyru@gmail.com>\r\n')
    new_socket.sendall( b'data\r\n')
    new_socket.sendall(b'From: v <vitalyven@mailhog.local>\r\nReply-To: vitalyven@mailhog.local\r\nTo: uyiu@yandex.ru\r\nDate: Sat, 07 Dec 2019 14:50:06 +0300\r\nSubject: \xd1\x82\xd0\xb5\xd0\xbc\xd0\xb034\r\n\r\nHello Jeeno\r\n.\r\n')
    new_socket.sendall(b'quit\r\n')


sendEMail('Hello Jeeno')
