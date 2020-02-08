# TODO:08.02.2020: tests taken from https://github.com/Oreder/eMail/blob/master/server/tests/ "as is", need to refactor based on SMTP-client code from here
# (refactor, using mail.from_file())(:)

import time
import datetime
from random import randrange
try:
    from common.mail import Mail

except (ModuleNotFoundError, ImportError) as e:
    import sys
    import os
    sys.path.append(os.path.dirname(__file__))
    sys.path.append(os.path.dirname(os.path.dirname(__file__)))
    from common.mail import Mail

def test_maildir_duplicate():
    """
    Simple emails to test
    """
    _mail = Mail(to=["a@b.c", "a@b.c", "a@b.c", "A@b.c"])
    assert _mail.to_file()[0] == 432


def test_maildir_oversize():
    emails = ["a{}@b.c".format(i) for i in range(0, 200)]
    _mail = Mail(to=emails)
    assert _mail.to_file()[0] == 452

def test_maildir_normal():
    emails = ["a@b.c", "A@b.c", "Aa@Bb.Cc"]
    _mail = Mail(to=emails)

    assert _mail.to_file()[0] == 250