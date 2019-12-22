import re
import ssl
import socket
import smtplib


GREETING_pattern = re.compile("^220.*")
HELO_pattern = re.compile("^250-.*")
AUTH_pattern = re.compile("^235 2\.7\.0 Authentication successful\..*")
MAIL_FROM_pattern = re.compile("^250 2\.1\.0 <.*> ok.*")
RCPT_TO_pattern = re.compile("^250 2\.1\.5 <.*> recipient ok.*")
DATA_pattern = re.compile("^354.*")
QUIT_pattern = re.compile("^250 2\.0\.0 Ok.*")


def handle_message(line):
        print(line)
        # TODO:
        # check possible states from cl.machine.state
        # match exact state with re patterns
        # call handler for this state
        GREETING_matched = re.search(GREETING_pattern, line)
        HELO_matched = re.search(HELO_pattern, line)
        AUTH_matched = re.search(AUTH_pattern, line)
        MAIL_FROM_matched = re.search(MAIL_FROM_pattern, line)
        RCPT_TO_matched = re.search(RCPT_TO_pattern, line)
        DATA_matched = re.search(DATA_pattern, line)
        QUIT_matched = re.search(QUIT_pattern, line)

        if GREETING_matched:
            pass

        elif HELO_matched:
            domain = HELO_matched

        elif AUTH_matched:
            pass

        elif MAIL_FROM_matched:
            mail_from = MAIL_FROM_matched

        elif RCPT_TO_matched:
            mail_to = RCPT_TO_matched

        elif DATA_matched:
            data = DATA_matched

        elif QUIT_matched:
            data = QUIT_matched


server = smtplib.SMTP_SSL("smtp.yandex.ru", 465)

server.login("123","123")
context = ssl._create_stdlib_context()
new_socket = socket.create_connection(("smtp.yandex.ru", 465), 5,
                                     None)
new_socket =context.wrap_socket(new_socket,
                                      server_hostname="smtp.yandex.ru")
handle_message(new_socket.recv().decode("utf-8"))




new_socket.sendall(b'ehlo [127.0.1.1]\r\n')
handle_message(new_socket.recv().decode("utf-8"))
new_socket.sendall(b'AUTH PLAIN AE11c2ljRGV2ZWxvcAAxMm11c2ljOTA=\r\n')
handle_message(new_socket.recv().decode("utf-8"))
new_socket.sendall(b'mail FROM:<MusicDevelop@yandex.ru> size=92\r\n')
handle_message(new_socket.recv().decode("utf-8"))
new_socket.sendall(b'rcpt TO:<cappyru@gmail.com>\r\n')
handle_message(new_socket.recv().decode("utf-8"))
new_socket.sendall(b'data\r\n')
handle_message(new_socket.recv().decode("utf-8"))
new_socket.sendall(b'From: v <vitalyven@mailhog.local>\r\nReply-To: vitalyven@mailhog.local\r\nTo: uyiu@yandex.ru\r\nDate: Sat, 07 Dec 2019 14:50:06 +0300\r\nSubject: \xd1\x82\xd0\xb5\xd0\xbc\xd0\xb034\r\n\r\nHello Jeeno\r\n.\r\n')
handle_message(new_socket.recv().decode("utf-8"))

new_socket.sendall(b'quit\r\n')
handle_message(new_socket.recv().decode("utf-8"))

# handle_message(self,)
# class Client():
#     def __init__(self, threads=5):
#
#     def __enter__(self):
#         self.socket_init()
#         return self
#
#     def socket_init(self):
#        pass
#
#     # def serve_forever(self):
#     #     for i in range(self.threads_cnt):
#     #         thread_sock = threading.Thread(target=thread_socket, args=(serv, i,))
#     #         thread_sock.daemon = True
#     #         thread_sock.start()
#     #     while True:
#     #         pass
#     def handle_message(self, socket):
#         try:
#             line = socket.readline()
#         except socket.timeout:
#             self.sock.close()
#         else:
#             print(line)
#             #TODO:
#             #check possible states from cl.machine.state
#             #match exact state with re patterns
#             #call handler for this state
#             HELO_matched = re.search(HELO_pattern, line)
#             MAIL_FROM_matched = re.search(MAIL_FROM_pattern, line)
#             RCPT_TO_matched = re.search(RCPT_TO_pattern, line)
#             DATA_start_matched = re.search(DATA_start_pattern, line)
#             DATA_end_matched = re.search(DATA_end_pattern, line)
#             QUIT_matched = re.search(QUIT_pattern, line)
#             RSET_matched = re.search(RSET_pattern, line)
#
#             if HELO_matched:
#                 domain = HELO_matched.group(1)
#
#             elif MAIL_FROM_matched:
#                 mail_from = MAIL_FROM_matched.group(1)
#
#             elif RCPT_TO_matched:
#                 mail_to = RCPT_TO_matched.group(1)
#
#             elif DATA_start_matched:
#                 data = DATA_start_matched.group(1)
#
#             elif DATA_end_matched:
#                 data = DATA_end_matched.group(1)
#
#
#
#     def __exit__(self, exc_type, exc_val, exc_tb):
#        pass