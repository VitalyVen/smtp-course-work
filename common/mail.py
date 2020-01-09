import os
import re
import uuid
from dataclasses import dataclass

from server_config import DEFAULT_USER_DIR, SERVER_DOMAIN, DEFAULT_SUPR_DIR
from state import RE_EMAIL_ADDRESS
from server.state import HELO_pattern

@dataclass
class Mail():
    to:list
    body:str = ''
    from_:str = ''
    domain:str = ''
    helo_command:str = ''
    file_path: str = ''

    @property
    def mail(self):
        mail = f"{self.helo_command}:<{self.domain}>\r\n"
        mail+= f"FROM:<{self.from_}>\r\n"
        for i in self.to:
            mail+= f"TO:<{i}>\r\n\r\n"
        mail+= self.body
        return mail

    def to_file(self, file_path=None):
        if file_path is None:
            self.file_path = f'{uuid.uuid4()}=@'
            file_path = self.file_path
        target = re.search(RE_EMAIL_ADDRESS, self.to).group(0)
        tmp = target.split('@')
        user, domain = tmp[0], tmp[1]

        directory = DEFAULT_USER_DIR
        if domain == SERVER_DOMAIN:
            directory = DEFAULT_SUPR_DIR + user + '/maildir/'

        # TODO: new, cur, tmp
        if not os.path.exists(directory):
            os.makedirs(directory)

        with open(directory + file_path, 'w') as fs:
            fs.write(self.mail)

    @classmethod
    def from_file(cls, filepath):
        with open(filepath, 'r') as f:
            helo_string = f.readline()

            HELO_matched = re.search(HELO_pattern, helo_string)
            if HELO_matched:
                command = helo_string
                domain = HELO_matched.group(2) or "unknown"
            else:
                command = helo_string
                domain = 'unknown'

            from_ = f.readline()
            to = f.readline()
            f.readline()
            body = f.readlines()

        return cls(helo_command=command, from_=from_, to=to, body=body, domain=domain)