import socket
import logging
from client_state import *
from common.custom_logger_proc import QueueProcessLogger
# from transitions import Machine
from transitions.extensions import GraphMachine as gMachine


# new_socket.sendall(b'ehlo [127.0.1.1]\r\n')
# handle_message(new_socket.recv().decode("utf-8"))
# # new_socket.sendall(b'AUTH PLAIN AE11c2ljRGV2ZWxvcAAxMm11c2ljOTA=\r\n')
# # handle_message(new_socket.recv().decode("utf-8"))
# new_socket.sendall(b'mail FROM:<MusicDevelop@yandex.ru> size=92\r\n')
# handle_message(new_socket.recv().decode("utf-8"))
# new_socket.sendall(b'rcpt TO:<cappyru@gmail.com>\r\n')
# handle_message(new_socket.recv().decode("utf-8"))
# new_socket.sendall(b'data\r\n')
# handle_message(new_socket.recv().decode("utf-8"))
# new_socket.sendall(b'From: v <vitalyven@mailhog.local>\r\nReply-To: vitalyven@mailhog.local\r\nTo: uyiu@yandex.ru\r\nDate: Sat, 07 Dec 2019 14:50:06 +0300\r\nSubject: \xd1\x82\xd0\xb5\xd0\xbc\xd0\xb034\r\n\r\nHello Jeeno\r\n.\r\n')
# handle_message(new_socket.recv().decode("utf-8"))
#
# new_socket.sendall(b'quit\r\n')
# handle_message(new_socket.recv().decode("utf-8"))

      # N.B.: client-server typical SMTP-dialog from RFC-5321, appendix D.1. (:)
      #
      # S: 220 foo.com Simple Mail Transfer Service Ready //greetings
      # C: EHLO bar.com  //ehlo_write
      # S: 250-foo.com greets bar.com //ehlo
      # S: 250-8BITMIME //ehlo
      # S: 250-SIZE //ehlo
      # S: 250-DSN //ehlo
      # S: 250 HELP //ehlo
      # C: MAIL FROM:<Smith@bar.com>  //mail_from_write
      # S: 250 OK  //mail_from
      # C: RCPT TO:<Jones@foo.com> //rcpt_to_write
      # S: 250 OK  //rcpt_to
      # C: RCPT TO:<Green@foo.com> //rcpt_to_write
      # S: 550 No such user here //rcpt_to
      # C: RCPT TO:<Brown@foo.com> //rcpt_to_write
      # S: 250 OK  //rcpt_to
      # C: DATA  //data_write
      # S: 354 Start mail input; end with <CRLF>.<CRLF>  //data
      # C: Blah blah blah...  //data_write
      # C: ...etc. etc. etc.//data_write
      # C: . //data_end_write
      # S: 250 OK //quit
      # C: QUIT //quit_write
      # S: 221 foo.com Service closing transmission channel //finish

        # GREETING_pattern = re.compile("^220.*")
        # EHLO_pattern = re.compile("^250-.*")
        # EHLO_end_pattern = re.compile("^250 .*")
        # # AUTH_pattern = re.compile("^235 2\.7\.0 Authentication successful\..*")
        # MAIL_FROM_pattern = re.compile("^250 2\.1\.0 <.*> ok.*")
        # RCPT_TO_pattern = re.compile("^250 2\.1\.5 <.*> recipient ok.*")
        # DATA_pattern = re.compile("^354.*")
        # QUIT_pattern = re.compile("^250 2\.0\.0 Ok.*")

class SmtpClientFsm(object):
    def __init__(self, name, logdir):
        self.name = name
        self.logger = QueueProcessLogger(filename=f'{logdir}/fsm.log')
        self.machine = self.init_machine()

        # S: 220 foo.com Simple Mail Transfer Service Ready //greetings
        # GREETING_pattern = re.compile("^220.*")
        self.init_transition('EHLO'       , GREETING_STATE, EHLO_WRITE_STATE)
        # C: EHLO bar.com  //ehlo_write
        self.init_transition('EHLO_write'           , EHLO_WRITE_STATE, EHLO_STATE)
        # EHLO_pattern = re.compile("^250-.*")
        # S: 250-foo.com greets bar.com //ehlo
        # S: 250-8BITMIME //ehlo
        # S: 250-SIZE //ehlo
        # S: 250-DSN //ehlo
        self.init_transition('EHLO_again'           , EHLO_STATE, EHLO_STATE)
        # EHLO_end_pattern = re.compile("^250 .*")
        # S: 250 HELP //ehlo
        self.init_transition('MAIL_FROM', EHLO_STATE, MAIL_FROM_WRITE_STATE)
        # C: MAIL FROM:<Smith@bar.com>  //mail_from_write
        self.init_transition('MAIL_FROM_write'      , MAIL_FROM_WRITE_STATE, MAIL_FROM_STATE)
        # MAIL_FROM_pattern = re.compile("^250 2\.1\.0 <.*> ok.*")
        # S: 250 OK  //mail_from
        self.init_transition('RCPT_TO'      , MAIL_FROM_STATE, RCPT_TO_WRITE_STATE)
        # C: RCPT TO:<Jones@foo.com> //rcpt_to_write
        self.init_transition('RCPT_TO_write'      , RCPT_TO_WRITE_STATE, RCPT_TO_STATE)
        # RCPT_TO_pattern = re.compile("^250 2\.1\.5 <.*> recipient ok.*")
        # S: 250 OK  //rcpt_to
        self.init_transition('RCPT_TO_additional'      , RCPT_TO_STATE, RCPT_TO_WRITE_STATE) #goto rcpt_to_write if pending recepient only (check outside rcpt_to_handler)
        self.init_transition('DATA_start'      , RCPT_TO_STATE, DATA_WRITE_STATE)
        self.init_transition('DATA_start_write'      , DATA_WRITE_STATE, DATA_STATE)
        self.init_transition('DATA_write'      , DATA_STATE, DATA_WRITE_STATE)
        self.init_transition('DATA_additional_write'      , DATA_WRITE_STATE, DATA_WRITE_STATE)
        self.init_transition('DATA_end'      , DATA_WRITE_STATE, DATA_END_WRITE_STATE)
        self.init_transition('QUIT_write'      , DATA_END_WRITE_STATE, QUIT_STATE)
        self.init_transition('QUIT'      , QUIT_STATE, QUIT_WRITE_STATE)
        self.init_transition('FINISH'      , QUIT_WRITE_STATE, FINISH_STATE)

        # self.init_transition('RSET'      , source='*', destination=HELO_WRITE_STATE)
        # self.init_transition('RSET_write', source='*', destination=HELO_STATE      )
        # self.init_transition('ANOTHER_recepient', DATA_END_WRITE_STATE, MAIL_FROM_STATE)
    
    def init_machine(self):
        return gMachine(
            model=self,
            states=states,
            initial=GREETING_STATE
        )

    def init_transition(self, trigger, source, destination):
        self.machine.add_transition(
            before=trigger + '_handler',
            trigger=trigger,
            source=source,
            dest=destination
        )

    def EHLO_handler(self, socket):
        self.logger.log(level=logging.DEBUG, msg="220 SMTP GREETING FROM 0.0.0.0.0.0.1\n")

    def GREETING_write_handler(self, socket):
        socket.send("220 SMTP GREETING FROM 0.0.0.0.0.0.1\n".encode())

    def HELO_handler(self, socket, address, domain):
        self.logger.log(level=logging.DEBUG, msg="domain: {} connected".format(domain))

    def HELO_write_handler(self, socket, address, domain):
        socket.send("250 {} OK \n".format(domain).encode())

    def MAIL_FROM_handler(self, socket, email):
        self.logger.log(level=logging.DEBUG, msg="f: {} mail from".format(email))

    def MAIL_FROM_write_handler(self, socket, email):
        socket.send("250 2.1.0 Ok \n".encode())

    def RCPT_TO_handler(self, socket, email):
        self.logger.log(level=logging.DEBUG, msg="f: {} mail to".format(email))
        
    def RCPT_TO_write_handler(self, socket, email):
        socket.send("250 2.1.5 Ok \n".encode())

    def DATA_start_handler(self, socket):
        self.logger.log(level=logging.DEBUG, msg="Data started")
    
    def DATA_start_write_handler(self, socket):
        socket.send("354 Send message content; end with <CRLF>.<CRLF>\n".encode())

    def DATA_additional_handler(self, socket):
        self.logger.log(level=logging.DEBUG, msg="Additional data")

    def DATA_end_handler(self, socket):
        self.logger.log(level=logging.DEBUG, msg="Data end")
    
    def DATA_end_write_handler(self, socket, filename):
        socket.send(f"250 Ok: queued as {filename}\n".encode())

    def QUIT_handler(self, socket:socket.socket):
        self.logger.log(level=logging.DEBUG, msg='Disconnecting..\n')

    def QUIT_write_handler(self, socket:socket.socket):
        socket.send("221 Left conversation\n".encode())
        socket.close()

    def RSET_handler(self, socket):
        self.logger.log(level=logging.DEBUG, msg=f"Rsetted \n")

    def RSET_write_handler(self, socket):
        socket.send(f"250 OK \n".encode())


