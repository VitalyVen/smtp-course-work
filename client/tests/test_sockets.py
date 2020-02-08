# TODO:08.02.2020: tests taken from https://github.com/Oreder/eMail/blob/master/server/tests/ "as is", PROBABLY (OR NOT, need to analize) need to refactor based on SMTP-client code from here(:)

import re
import socket
import pytest
from random import randrange

try:
    from server.mail_server import MailServer

except (ModuleNotFoundError, ImportError) as e:
    import sys
    import os

    sys.path.append(os.path.dirname(__file__))
    sys.path.append(os.path.dirname(os.path.dirname(__file__)))
    from mail_server import MailServer


def test_send_simple_message_socket():
    BYTE = 128
    PORT = randrange(16000, 40000)
    with MailServer(port=PORT) as server:
        server.serve(blocking=False)

        s = socket.create_connection(('localhost', PORT), 5, None)
        msg = s.recv(BYTE)
        assert msg[:3] == b'220'

        s.sendall(b'HELO [127.0.1.1]\r\n')
        msg = s.recv(BYTE)
        assert msg[:3] == b'250'

        s.sendall(b'MAIL FROM:<MusicDevelop@yandex.ru> size=92\r\n')
        msg = s.recv(BYTE)
        assert msg[:3] == b'250'

        s.sendall(b'RCPT TO:<cappyru@gmail.com>\r\n')
        msg = s.recv(BYTE)
        assert msg[:3] == b'250'

        s.sendall(b'DATA\r\n')
        msg = s.recv(BYTE)
        assert msg[:3] == b'354'

        s.sendall(
            b'FROM: v <vitalyven@mailhog.local>\r\nReply-To: vitalyven@mailhog.local\r\nTo: uyiu@yandex.ru\r\nDate: Sat, 07 Dec 2019 14:50:06 +0300\r\nSubject: \xd1\x82\xd0\xb5\xd0\xbc\xd0\xb034\r\n\r\nHello Jeeno\r\n.\r\n')
        msg = s.recv(BYTE)
        assert msg[:3] == b'250'

        s.sendall(b'QUIT\r\n')
        msg = s.recv(BYTE)
        assert msg[:3] == b'221'