import re
import socket
from time import sleep

import pytest
from smtplib import SMTP
import datetime

import main
from server_config import SERVER_PORT, LOG_FILES_DIR, THREADS_CNT

try:
    from server.mail_server import MailServer, mail_start, stop_server
    from server.state import HELO_pattern, MAIL_FROM_pattern, DATA_start_pattern, DATA_end_pattern, RCPT_TO_pattern

except (ModuleNotFoundError, ImportError) as e:
    import sys
    import os
    from common.custom_logger_proc import QueueProcessLogger
    sys.path.append(os.path.dirname(__file__))
    sys.path.append(os.path.dirname(os.path.dirname(__file__)))
    from mail_server import MailServer,mailServerStart
    from state import HELO_pattern, MAIL_FROM_pattern, DATA_start_pattern, DATA_end_pattern, RCPT_TO_pattern


def test_send_simple_message():
        server=mail_start()
        sleep(2)
        new_socket = socket.create_connection(('localhost', 2558), 5,
                                             None)
        new_socket.sendall(b'HELO [127.0.1.1]\r\n')

        new_socket.sendall(b'MAIL FROM:<MusicDevelop@yandex.ru> size=92\r\n')

        new_socket.sendall(b'RCPT TO:<cappyru@gmail.com>\r\n')

        new_socket.sendall(b'DATA\r\n')

        new_socket.sendall(
            b'FROM: v <vitalyven@mailhog.local>\r\nReply-To: vitalyven@mailhog.local\r\nTo: uyiu@yandex.ru\r\nDate: Sat, 07 Dec 2019 14:50:06 +0300\r\nSubject: \xd1\x82\xd0\xb5\xd0\xbc\xd0\xb034\r\n\r\nHello Jeeno\r\n.\r\n')

        new_socket.sendall(b'QUIT\r\n')

        sleep(2)
        stop_server(server)
