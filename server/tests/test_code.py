import re
import pytest
from smtplib import SMTP
import time
import datetime
from random import randrange
from email import encoders
from email.mime.text import MIMEText
from email.mime.base import MIMEBase
from email.mime.multipart import MIMEMultipart
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

def test_connection_to_server():
    PORT = randrange(16000, 40000)
    with MailServer(port=PORT) as server:
        server.serve(blocking=False)
        
        smtp = SMTP()
        code, _ = smtp.connect('0.0.0.0', PORT)
        smtp.quit()
        assert code in range(200, 300)  # successful

def test_send_mail_return():
    PORT = randrange(16000, 40000)
    with MailServer(port=PORT) as server:
        server.serve(blocking=False)
        
        smtp = SMTP()
        smtp.set_debuglevel(0)
        smtp.connect('0.0.0.0', PORT)
        from_addr = "<binh@local.local>"
        to_addr = "test@test.com"

        subj = "Test2"
        date = datetime.datetime.now().strftime("%d/%m/%Y %H:%M")
        message_text = "Test2"

        msg = "From:%s\nTo:%s\nSubject:%s\nDate:%s\n\n%s"  % (from_addr, to_addr, subj, date, message_text)

        errs = smtp.sendmail(from_addr, to_addr, msg)
        smtp.quit()
        
        assert len(errs) == 0