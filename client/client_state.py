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

GREETING_pattern = re.compile("^220.*")
EHLO_pattern = re.compile("^250-.*")
EHLO_end_pattern = re.compile("^250 .*")
# AUTH_pattern = re.compile("^235 2\.7\.0 Authentication successful\..*")
MAIL_FROM_pattern = re.compile("^250 2\.1\.0 <.*> ok.*")
# N.B.: as stated in RFC-5321, in typical SMTP transaction scenario there is no distinguishment between server answers for mail_from and rcpt_to client-requests(:)
RCPT_TO_pattern = re.compile("^250 2\.1\.0 <.*> ok.*") #re.compile("^250 2\.1\.5 <.*> recipient ok.*")
DATA_pattern = re.compile("^354.*")
QUIT_pattern = re.compile("^250 2\.0\.0 Ok.*")

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