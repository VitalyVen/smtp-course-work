import re
import pytest
try:
    from server.state import HELO_pattern, MAIL_FROM_pattern, DATA_start_pattern, DATA_end_pattern, RCPT_TO_pattern
except (ModuleNotFoundError, ImportError) as e:
    import sys
    import os
    sys.path.append(os.path.dirname(__file__))
    sys.path.append(os.path.dirname(os.path.dirname(__file__)))
    from state import  HELO_pattern, MAIL_FROM_pattern, DATA_start_pattern, DATA_end_pattern, RCPT_TO_pattern

def test_hello_pattern():
    assert re.match(HELO_pattern, 'HELO coml.com\r\n').group(1)=='HELO'
    assert re.match(HELO_pattern, 'HELO coml.com\r\n').group(2)=='coml.com'
def test_email_pattern():
    assert re.match(MAIL_FROM_pattern, 'MAIL FROM: <jlkjl@coml.com>\r\n').group(1)=='jlkjl@coml.com'
def test_email_patternf():
    assert re.match(RCPT_TO_pattern, 'RCPT TO: <jlkjl@coml.com>\r\n').group(1)=='jlkjl@coml.com'
def test_data_end_patternf():
    assert not re.match(DATA_end_pattern, '.\r\n').group(1)
def test_data_end_patternf_should_match():
    assert re.match(DATA_end_pattern, 'somestring.\r\n').group(1)=='somestring'
# def test_data_patternf():
#     assert re.match(DATA_pattern, 'DATA ljljlj.\r\n').group(1)=='ljljlj'