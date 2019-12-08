import re
import pytest
from server.main import  HELO_pattern, MAIL_FROM_pattern, DATA_pattern, RCPT_TO_pattern
def test_hello_pattern():
    assert re.match(HELO_pattern, 'HELO jlkjl@coml.com \r\n').group(1)=='jlkjl@coml.com'
def test_email_pattern():
    assert re.match(MAIL_FROM_pattern, 'MAIL FROM: <jlkjl@coml.com>\r\n').group(1)=='jlkjl@coml.com'
def test_email_patternf():
    assert re.match(RCPT_TO_pattern, 'RCPT TO: <jlkjl@coml.com>\r\n').group(1)=='jlkjl@coml.com'
def test_data_patternf():
    assert re.match(DATA_pattern, 'DATA ljljlj.\r\n').group(1)=='ljljlj'