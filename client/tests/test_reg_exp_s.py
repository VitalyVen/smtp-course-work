import re
import pytest
from SMTP_CLIENT_FSM import *
from client_state import HELO_pattern_CLIENT

# HELO_pattern_CLIENT = re.compile(f"^(HELO|EHLO):<(.+)>{RE_CRLF}", re.IGNORECASE)

# GREETING_pattern = re.compile("^(220) (.+\.\w+)\s+(.+)$")
# # HELO_pattern_CLIENT = re.compile(f"^(HELO|EHLO) (.+){RE_CRLF}", re.IGNORECASE)
# EHLO_pattern = re.compile("^250-.*")
# EHLO_end_pattern = re.compile("^250 .*")
# # AUTH_pattern = re.compile("^235 2\.7\.0 Authentication successful\..*")
# MAIL_FROM_pattern = re.compile("^250.*") #  re.compile("^250 2\.1\.0 <.*> ok.*")
# # N.B.: as stated in RFC-5321, in typical SMTP transaction scenario there is no distinguishment between server answers for mail_from and rcpt_to client-requests(:)
# RCPT_TO_pattern = re.compile("^250.*")  #  re.compile("^250 2\.1\.0 <.*> ok.*") #re.compile("^250 2\.1\.5 <.*> recipient ok.*")
# RCPT_TO_WRONG_pattern = re.compile("^550.*") #re.compile("^250 2\.1\.5 <.*> recipient ok.*")
# DATA_pattern = re.compile("^354.*")
# DATA_END_pattern = re.compile("^250.*")
# QUIT_pattern = re.compile("^221.*")  # re.compile("^250 2\.0\.0 Ok.*")

def test_hello_pattern_CLIENT():
    assert re.match(HELO_pattern_CLIENT, 'HELO:<comnew.com>\r\n').group(1)=='HELO'
    assert re.match(HELO_pattern_CLIENT, 'EHLO:<comnew.com>\r\n').group(2)=='comnew.com'

def test_greeting_pattern():
    # GREETING_pattern = re.compile("^(220) (\S+\.\S+)(.*)$")
    assert re.match(GREETING_pattern, '220 mxs.mail.ru ESMTP ready\r\n').group(1)=='220'
    assert re.search(GREETING_pattern, '220 mxfront9q.mail.yandex.net (Want to use Yandex.Mail for your domain? Visit http://pdd.yandex.ru)\r\n').group(2)=='mxfront9q.mail.yandex.net'

def test_ehlo_pattern():
    # EHLO_NOT_matched = re.search(EHLO_pattern, '230-foo.com greets bar.com\r\n')
    # EHLO_matched = re.search(EHLO_pattern, '250-foo.com greets bar.com\r\n')
    assert re.search(EHLO_pattern, '250-foo.com greets bar.com\r\n')

def test_ehlo_end_pattern():
    assert re.search(EHLO_end_pattern, '250 HELP\r\n')

def test_ehlo_ON_LARGE_STRING_pattern():
    assert re.search(EHLO_pattern, '250-8BITMIME\r\n250-PIPELINING\r\n250-SIZE 42991616\r\n250-STARTTLS\r\n250-DSN\r\n250 ENHANCEDSTATUSCODES\r\n')

def test_ehlo_end_ON_LARGE_STRING_pattern():
    EHLO_end_pattern = re.compile(".*(250 ).*")
    assert re.search(EHLO_end_pattern, '250-8BITMIME\r\n250-PIPELINING\r\n250-SIZE 42991616\r\n250-STARTTLS\r\n250-DSN\r\n250 ENHANCEDSTATUSCODES\r\n')

def test_mail_from_pattern():
    assert re.search(MAIL_FROM_pattern, '250 OK\r\n')

def test_rcpnt_to_pattern():
    assert re.search(RCPT_TO_pattern, '250 OK\r\n')

def test_rcpnt_to_wrong_pattern():
    assert re.search(RCPT_TO_WRONG_pattern, '550 No such user here\r\n')

def test_data_pattern():
    assert re.search(DATA_pattern, '354 Start mail input; end with <CRLF>.<CRLF>\r\n')

def test_data_end_pattern():
    assert re.search(DATA_END_pattern, '250 OK\r\n')

def test_quit_pattern():
    assert re.search(QUIT_pattern, '221 foo.com Service closing transmission channel\r\n')



