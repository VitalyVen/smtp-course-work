import ssl
import socket
import smtplib


context = ssl._create_stdlib_context()
new_socket = socket.create_connection(("smtp.yandex.ru", 465), 5,
                                     None)
new_socket =context.wrap_socket(new_socket,
                                      server_hostname="smtp.yandex.ru")

new_socket.sendall( b'ehlo [127.0.1.1]\r\n')
print(new_socket.recv())
new_socket.sendall( b'AUTH PLAIN AE11c2ljRGV2ZWxvcEB5YW5kZXgucnUAMTJtdXNpYzkw=\r\n')
print(new_socket.recv())
new_socket.sendall( b'mail FROM:<MusicDevelop@yandex.ru> size=92\r\n')
print(new_socket.recv())
new_socket.sendall(b'rcpt TO:<MusicDevelop@yandex.ru>\r\n')
print(new_socket.recv())
new_socket.sendall( b'data\r\n')
print(new_socket.recv())
new_socket.sendall(b'Hello Jee.\r\n.\r\n')
print(new_socket.recv())
new_socket.sendall(b'quit\r\n')
print(new_socket.recv())


