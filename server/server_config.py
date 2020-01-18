import os
SERVER_DOMAIN = 'kostya.local'
SERVER_PORT   = 2556
READ_TIMEOUT  = 10
WRITE_TIMEOUT=10
DNS_RESERV=False


DEFAULT_SUPR_DIR = os.path.join(os.path.dirname(__file__), f'../pst/local/')
DEFAULT_USER_DIR = os.path.join(os.path.dirname(__file__), f'../pst/maildir/')
LOG_FILES_DIR = os.path.join(os.path.dirname(__file__), f'../logs/')