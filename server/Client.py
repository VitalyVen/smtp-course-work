import socket
import os
import uuid
from SMTP_FSM import *
from ClientSocket import *
from default import SERVER_DOMAIN, DEFAULT_SUPR_DIR, DEFAULT_USER_DIR
from state import RE_EMAIL_ADDRESS


class Client(object):
    def __init__(self, socket:ClientSocket):
        self.socket = socket
        self.machine = SMTP_FSM(socket.address)
        self.mail = ''

    def mail_to_file(self, fileName=None):
        if fileName is None:
            fileName = f'{uuid.uuid4()}=@'

        target = ''
        for item in self.mail.splitlines():
            if 'From:' in item:
                target = re.search(RE_EMAIL_ADDRESS, item).group(0)
                break

        tmp = target.split('@')
        user, domain = tmp[0], tmp[1]
        
        directory = DEFAULT_USER_DIR
        if domain == SERVER_DOMAIN:
            directory = DEFAULT_SUPR_DIR + user + '/maildir/'

        # TODO: new, cur, tmp
        if not os.path.exists(directory):
            os.makedirs(directory)

        with open(directory + fileName, 'w') as fs:
            fs.write(self.mail)