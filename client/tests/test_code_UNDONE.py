# TODO:08.02.2020: tests taken from https://github.com/Oreder/eMail/blob/master/server/tests/test_code.py "as is", need to refactor based on SMTP-client code from here(:)

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

        msg = "From:%s\nTo:%s\nSubject:%s\nDate:%s\n\n%s" % (from_addr, to_addr, subj, date, message_text)

        errs = smtp.sendmail(from_addr, to_addr, msg)
        smtp.quit()

        assert len(errs) == 0


def test_email_to_multiple_recipients():
    PORT = randrange(16000, 40000)
    with MailServer(port=PORT) as server:
        server.serve(blocking=False)

        smtp = SMTP()
        smtp.set_debuglevel(0)
        smtp.connect('0.0.0.0', PORT)
        # smtp.login('USERNAME@DOMAIN', 'PASSWORD')

        from_addr = "<john@doe.net>"
        to_addrs = ["foo1@bar.com", "foo2@bar.com"]

        subj = "Test3"
        date = datetime.datetime.now().strftime("%d/%m/%Y %H:%M")
        message_text = "Some text ..."

        msg = "From: %s\nSubject: %s\nDate: %s\n\n%s" % (from_addr, subj, date, message_text)

        errs = smtp.sendmail(from_addr, to_addrs, msg)
        smtp.quit()

        """
        In this case, the message was accepted for delivery to three
        of the four addresses, and one was rejected, with the error code
        550.  If all addresses are accepted, then the method will return an
        empty dictionary.
        """
        assert len(errs) == 0


def test_send_big_mail_return():
    PORT = randrange(16000, 40000)
    with MailServer(port=PORT) as server:
        server.serve(blocking=False)

        smtp = SMTP()
        smtp.set_debuglevel(0)
        smtp.connect('0.0.0.0', PORT)
        from_email = "someone@gmail.com"
        to_emails = ["mike@someAddress.org", "nedry@jp.net"]

        subject = "Test email with attachment from Python"
        body_text = "This email contains an attachment!"
        file_to_attach = "tests/dat/test.jpg"

        header = 'Content-Disposition', 'attachment; filename="%s"' % file_to_attach

        # create the message
        msg = MIMEMultipart()
        msg["From"] = from_email
        msg["Subject"] = subject
        msg["Date"] = datetime.datetime.now().strftime("%d/%m/%Y %H:%M")

        if body_text:
            msg.attach(MIMEText(body_text))

        msg["To"] = ', '.join(to_emails[0])
        msg["cc"] = ', '.join(to_emails[1])

        attachment = MIMEBase('application', "octet-stream")

        data = open(file_to_attach, "rb").read()

        if len(data) > 0:
            attachment.set_payload(data)
            encoders.encode_base64(attachment)
            attachment.add_header(*header)
            msg.attach(attachment)

        errs = smtp.sendmail(from_email, to_emails, msg.as_string())
        smtp.quit()

        assert len(errs) == 0