import re

HELO_STATE              = 'helo'
HELO_WRITE_STATE        = 'helo_write'

GREETING_STATE          = 'greeting'
GREETING_WRITE_STATE    = 'greeting_write'

MAIL_FROM_STATE         = 'mail_from'
MAIL_FROM_WRITE_STATE   = 'mail_from_write'

RCPT_TO_STATE           = 'rcpt_to'
RCPT_TO_WRITE_STATE     = 'rcpt_to_write'

DATA_STATE              = 'data'            # For reading start date message and all next, except last
                                            # Note we should't reply to on all additional data messages
DATA_WRITE_STATE        = 'data_write'
DATA_END_WRITE_STATE    = 'data_end_write'

QUIT_STATE              = 'quit'
QUIT_WRITE_STATE        = 'quit_write'
FINISH_STATE            = 'finish'

RE_CRLF                 = r"\r(\n)?"
RE_EMAIL_ADDRESS        = r'[\w\.-]+@[\w\.-]+'
RE_EMAIL_OR_EMPTY       = r" ?<(?P<address>.+@.+)>|<>"

HELO_pattern            = re.compile(f"^(HELO|EHLO) (.+){RE_CRLF}")     # TODO: for regular hosts and like localhost
MAIL_FROM_pattern       = re.compile(f"^MAIL FROM:{RE_EMAIL_OR_EMPTY}")
RCPT_TO_pattern         = re.compile(f"^RCPT TO:{RE_EMAIL_OR_EMPTY}")
DATA_start_pattern      = re.compile(f"^DATA( .*)*{RE_CRLF}")
DATA_end_pattern        = re.compile(f"([\s\S]*)\.{RE_CRLF}")
QUIT_pattern            = re.compile(f"^QUIT{RE_CRLF}")
RSET_pattern            = re.compile(f"^RSET{RE_CRLF}")

states = [
    GREETING_WRITE_STATE,

    HELO_STATE,
    HELO_WRITE_STATE,

    MAIL_FROM_STATE,
    MAIL_FROM_WRITE_STATE,

    RCPT_TO_STATE,
    RCPT_TO_WRITE_STATE,

    DATA_STATE,
    DATA_WRITE_STATE,
    DATA_END_WRITE_STATE,

    QUIT_STATE,
    QUIT_WRITE_STATE,
    FINISH_STATE
]