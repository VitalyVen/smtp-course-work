import os
import re
import uuid
from dataclasses import dataclass

from server_config import DEFAULT_USER_DIR, SERVER_DOMAIN, DEFAULT_SUPR_DIR, MAX_RECIPIENTS
from state import RE_EMAIL_ADDRESS
from server.state import domain_pattern

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
            mail+= f"TO:<{i}>\r\n"
        mail+= "\r\n"
        mail+= self.body
        return mail

    def to_file(self, file_path=None):
        if file_path is None:
            self.file_path = f'{uuid.uuid4()}=@'
            file_path = self.file_path

        targets = self.to

        # Check duplicate adresses
        if len(targets) != len(set(targets)):
            return (432, "Recipient’s incoming mail queue has been stopped")
        # Check max recipients
        elif len(targets) > MAX_RECIPIENTS:
            return (452, 'Too many emails sent or too many recipients')
        else:
            # Select the first one, which is non-local to save to maildir
            nonLocalFound = False
            for target in targets:
                tmp = target.split('@')
                user, domain = tmp[0], tmp[1]
                directory = DEFAULT_USER_DIR
                if domain == SERVER_DOMAIN:
                    directory = DEFAULT_SUPR_DIR + user + '/maildir/'

                    if not os.path.exists(directory):
                        os.makedirs(directory)
                    with open(directory + file_path, 'w') as fs:
                        fs.write(self.mail)
                else:
                    if not nonLocalFound:
                        with open(directory + file_path, 'w') as fs:
                            fs.write(self.mail)
                        nonLocalFound = True
            return (250, 'Requested mail action okay completed')

    @classmethod
    def from_file(cls, filepath):
        with open(filepath, 'r') as f:
            helo_string = f.readline()
            from_ = f.readline()
            to = f.readline()           # TODO: multiple recipients (client)

            domain_matched = re.search(domain_pattern, to)
            if domain_matched:
                domain = domain_matched.group(1) or "unknown"
            else:
                domain = 'unknown'

            f.readline()
            body = f.readlines()

        return cls(helo_command=helo_string, from_=from_, to=to, body=body, domain=domain)