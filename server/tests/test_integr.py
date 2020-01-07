import re
import pytest
from server.state import  HELO_pattern, MAIL_FROM_pattern, DATA_start_pattern, DATA_end_pattern, RCPT_TO_pattern
from smtplib import SMTP
import datetime
from server.mail_server import MailServer
def test_send_simple_message():
    with MailServer() as server:
        server.serve(blocking=False)

        debuglevel = 0
        smtp = SMTP()
        smtp.set_debuglevel(debuglevel)
        smtp.connect('0.0.0.0', 2556)
        # smtp.login('USERNAME@DOMAIN', 'PASSWORD')

        from_addr = "<john@doe.net>"
        to_addr = "foo@bar.com"

        subj = "hello"
        date = datetime.datetime.now().strftime("%d/%m/%Y %H:%M")

        message_text = "Hello\nThis is a mail from your server\n\nBye\n"

        msg = "From: %s\nTo: %s\nSubject: %s\nDate: %s\n\n%s"  % (from_addr, to_addr, subj, date, message_text)

        smtp.sendmail(from_addr, to_addr, msg)
        smtp.quit()