import ssl
import socket
import smtplib


context = ssl._create_stdlib_context()
new_socket = socket.create_connection(("smtp.yandex.ru", 465), 5,
                                     None)
new_socket =context.wrap_socket(new_socket,
                                      server_hostname="smtp.yandex.ru")

# new_socket.sendall(b'ehlo [127.0.1.1]\r\n')
# print(new_socket.recv())
# #new_socket.sendall(b'AUTH PLAIN AE11c2ljRGV2ZWxvcEB5YW5kZXgucnUAMTJtdXNpYzkw=\r\n')
# new_socket.sendall(b'AUTH PLAIN AE11c2ljRGV2ZWxvcAAxMm11c2ljOTA=\r\n')
# print(new_socket.recv())
# new_socket.sendall(b'mail FROM:<MusicDevelop@yandex.ru>\r\n')
# print(new_socket.recv())
# new_socket.sendall(b'rcpt TO:<cappyru@Gmail.com>\r\n')
# print(new_socket.recv())
# new_socket.sendall(b'data\r\n')
# print(new_socket.recv())
# new_socket.sendall(b'From: v <vitalyven@mailhog.local>\r\nReply-To: vitalyven@mailhog.local\r\nTo: uyiu@yandex.ru\r\nDate: Sat, 07 Dec 2019 14:50:06 +0300\r\nSubject: \xd1\x82\xd0\xb5\xd0\xbc\xd0\xb034\r\n\r\nHello Jeeno\r\n.\r\n')
# print(new_socket.recv())
# new_socket.sendall(b'quit\r\n')
# print(new_socket.recv())

new_socket.sendall(b'ehlo [127.0.1.1]\r\n')
print(new_socket.recv())
new_socket.sendall(b'AUTH PLAIN AE11c2ljRGV2ZWxvcAAxMm11c2ljOTA=\r\n')
print(new_socket.recv())
new_socket.sendall(b'mail FROM:<MusicDevelop@yandex.ru> size=92\r\n')
print(new_socket.recv())
new_socket.sendall(b'rcpt TO:<cappyru@gmail.com>\r\n')
print(new_socket.recv())
new_socket.sendall(b'data\r\n')
print(new_socket.recv())
new_socket.sendall(b'From: v <vitalyven@mailhog.local>\r\nReply-To: vitalyven@mailhog.local\r\nTo: uyiu@yandex.ru\r\nDate: Sat, 07 Dec 2019 14:50:06 +0300\r\nSubject: \xd1\x82\xd0\xb5\xd0\xbc\xd0\xb034\r\n\r\nHello Jeeno\r\n.\r\n')
print(new_socket.recv())
new_socket.sendall(b'quit\r\n')
print(new_socket.recv())

#
# b'220 iva1-db9fc35c0844.qloud-c.yandex.net ESMTP (Want to use Yandex.Mail for your domain? Visit http://pdd.yandex.ru)\r\n'
# b'250-iva1-db9fc35c0844.qloud-c.yandex.net\r\n250-8BITMIME\r\n250-PIPELINING\r\n250-SIZE 42991616\r\n250-AUTH LOGIN PLAIN XOAUTH2\r\n250-DSN\r\n250 ENHANCEDSTATUSCODES\r\n'
# b'235 2.7.0 Authentication successful.\r\n'
# b'250 2.1.0 <MusicDevelop@yandex.ru> ok\r\n'
# b'250 2.1.5 <MusicDevelop@yandex.ru> recipient ok\r\n'
# b'354 Enter mail, end with "." on a line by itself\r\n'
# b'250 2.0.0 Ok: queued on iva1-db9fc35c0844.qloud-c.yandex.net as 1576799901-ptmcIpTstA-wLWGReg9\r\n'
#
# Process finished with exit code 0
