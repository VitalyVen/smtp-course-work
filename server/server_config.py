import os
SERVER_DOMAIN = 'superserver.local'
SERVER_PORT   = 2556
READ_TIMEOUT  = 40
PROCESSES_CNT  = None
THREADS_CNT  = 5

DEFAULT_SUPR_DIR = os.path.join(os.path.dirname(__file__), f'../pst/local/')
DEFAULT_USER_DIR = os.path.join(os.path.dirname(__file__), f'../pst/maildir/')
LOG_FILES_DIR = os.path.join(os.path.dirname(__file__), f'../logs/')