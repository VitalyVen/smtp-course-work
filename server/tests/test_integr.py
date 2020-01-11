import re
import pytest
from smtplib import SMTP
import time
import socket
import datetime
try:
    from server.mail_server import MailServer
    from server.state import HELO_pattern, MAIL_FROM_pattern, DATA_start_pattern, DATA_end_pattern, RCPT_TO_pattern

except (ModuleNotFoundError, ImportError) as e:
    import sys
    import os
    sys.path.append(os.path.dirname(__file__))
    sys.path.append(os.path.dirname(os.path.dirname(__file__)))
    from mail_server import MailServer
    from state import HELO_pattern, MAIL_FROM_pattern, DATA_start_pattern, DATA_end_pattern, RCPT_TO_pattern


def test_send_simple_message_smtplib():
    with MailServer(port=11111) as server:
        server.serve(blocking=False)

        debuglevel = 0
        smtp = SMTP()
        smtp.set_debuglevel(debuglevel)
        smtp.connect('0.0.0.0', 11111)
        # smtp.login('USERNAME@DOMAIN', 'PASSWORD')

        from_addr = "<john@doe.net>"
        to_addr = "foo@bar.com"

        subj = "hello"
        date = datetime.datetime.now().strftime("%d/%m/%Y %H:%M")

        message_text = "Hello\nThis is a mail from your server\n\nBye\n"

        msg = "From: %s\nTo: %s\nSubject: %s\nDate: %s\n\n%s"  % (from_addr, to_addr, subj, date, message_text)

        smtp.sendmail(from_addr, to_addr, msg)
        time.sleep(1)
        smtp.quit()
        assert True

def test_send_simple_message_socket():
    with MailServer(port=25566) as server:
        server.serve(blocking=False)
        new_socket = socket.create_connection(('localhost', 25566), 5,    None)
        print(new_socket.recv(1000))
        new_socket.sendall(b'HELO [127.0.1.1]\r\n')
        print(new_socket.recv(1000))
        new_socket.sendall(b'MAIL FROM:<MusicDevelop@yandex.ru> size=92\r\n')
        print(new_socket.recv(1000))
        new_socket.sendall(b'RCPT TO:<cappyru@gmail.com>\r\n')
        print(new_socket.recv(1000))
        new_socket.sendall(b'DATA\r\n')
        print(new_socket.recv(1000))
        new_socket.sendall(
                b'FROM: v <vitalyven@mailhog.local>\r\nReply-To: vitalyven@mailhog.local\r\nTo: uyiu@yandex.ru\r\nDate: Sat, 07 Dec 2019 14:50:06 +0300\r\nSubject: \xd1\x82\xd0\xb5\xd0\xbc\xd0\xb034\r\n\r\nHello Jeeno\r\n.\r\n')
        print(new_socket.recv(1000))
        new_socket.sendall(b'QUIT\r\n')
        print(new_socket.recv(1000))
        assert True

def test_send_message_two_recepients():
    with MailServer(port=25567) as server:
        server.serve(blocking=False)

        debuglevel = 0
        smtp = SMTP()
        smtp.set_debuglevel(debuglevel)
        smtp.connect('0.0.0.0', 25567)
        # smtp.login('USERNAME@DOMAIN', 'PASSWORD')

        from_addr = "<john@doe.net>"
        to_addr_1 = "foo1@bar.com"
        to_addr_2 = "foo2@bar.com"

        subj = "hello"
        date = datetime.datetime.now().strftime("%d/%m/%Y %H:%M")

        message_text = "Hello\nThis is a mail from your server\n\nBye\n"

        msg = "From: %s\nTo: %s\nSubject: %s\nDate: %s\n\n%s"  % (from_addr, to_addr_1, subj, date, message_text)

        smtp.sendmail(from_addr, [to_addr_1, to_addr_2], msg)
        smtp.quit()
    assert True