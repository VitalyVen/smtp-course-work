import re
from time import sleep

import pytest
from smtplib import SMTP
import datetime

import main

try:
    from server.mail_server import MailServer
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
        proc=main.mail_start()
        sleep(2)
        debuglevel = 0
        smtp = SMTP()
        smtp.set_debuglevel(debuglevel)
        smtp.connect('localhost', 2556)
        # smtp.login('USERNAME@DOMAIN', 'PASSWORD')

        from_addr = "<john@doe.net>"
        to_addr = "foo@bar.com"

        subj = "hello"
        date = datetime.datetime.now().strftime("%d/%m/%Y %H:%M")

        message_text = "Hello\nThis is a mail from your server\n\nBye\n"

        msg = "From: %s\nTo: %s\nSubject: %s\nDate: %s\n\n%s"  % (from_addr, to_addr, subj, date, message_text)

        smtp.sendmail(from_addr, to_addr, msg)
        smtp.quit()
        proc.kill()
