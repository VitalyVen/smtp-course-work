import os
SERVER_DOMAIN = 'superserver.local'
SERVER_PORT   = 2556
READ_TIMEOUT  = 40

DEFAULT_SUPR_DIR = os.path.join(os.path.dirname(__file__), f'../pst/local/')
DEFAULT_USER_DIR = os.path.join(os.path.dirname(__file__), f'../pst/maildir/')