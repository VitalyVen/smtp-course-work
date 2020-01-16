import re

GREETING_STATE          = 'greeting'

EHLO_WRITE_STATE        = 'ehlo_write'
EHLO_STATE              = 'ehlo'

MAIL_FROM_WRITE_STATE   = 'mail_from_write'
MAIL_FROM_STATE         = 'mail_from'

RCPT_TO_WRITE_STATE     = 'rcpt_to_write'
RCPT_TO_STATE           = 'rcpt_to'

DATA_STATE              = 'data'            # For reading start date message and all next, except last
                                            # Note we should't reply to on all additional data messages
DATA_WRITE_STATE        = 'data_write'
DATA_END_WRITE_STATE    = 'data_end_write'

QUIT_WRITE_STATE        = 'quit_write'
QUIT_STATE              = 'quit'
FINISH_STATE            = 'finish'

RE_CRLF                 = r"\r(\n)?"
RE_EMAIL_ADDRESS        = r'[\w\.-]+@[\w\.-]+'
RE_EMAIL_OR_EMPTY       = r" ?<(?P<address>.+@.+)>|<>"

HELO_pattern = re.compile("^(HELO|EHLO) (.*\.\w+|localhost)")


states = [
    GREETING_STATE,

    EHLO_STATE,
    EHLO_WRITE_STATE,

    MAIL_FROM_WRITE_STATE,
    MAIL_FROM_STATE,

    RCPT_TO_WRITE_STATE,
    RCPT_TO_STATE,

    DATA_STATE,
    DATA_WRITE_STATE,
    DATA_END_WRITE_STATE,

    QUIT_WRITE_STATE,
    QUIT_STATE,
    FINISH_STATE
]