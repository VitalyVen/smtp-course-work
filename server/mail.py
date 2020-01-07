import os
import re
import uuid
from dataclasses import dataclass

from server_config import DEFAULT_USER_DIR, SERVER_DOMAIN, DEFAULT_SUPR_DIR
from state import RE_EMAIL_ADDRESS

@dataclass
class Mail():
    body:str = ''
    to:str = ''
    from_:str = ''
    domain:str = ''
    helo_command:str = ''

    @property
    def mail(self):
        mail = f"{self.helo_command}:<{self.domain}>\r\n"
        mail+= f"FROM:<{self.from_}>\r\n"
        mail+= f"TO:<{self.to}>\r\n\r\n"
        mail+= self.body
        return mail

    def to_file(self, file_path=None):
        if file_path is None:
            file_path = f'{uuid.uuid4()}=@'
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