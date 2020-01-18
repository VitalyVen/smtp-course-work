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

GREETING_pattern = re.compile("^220.*")
EHLO_pattern = re.compile("^250-.*")
EHLO_end_pattern = re.compile("^250 .*")
# AUTH_pattern = re.compile("^235 2\.7\.0 Authentication successful\..*")
MAIL_FROM_pattern = re.compile("^250 2\.1\.0 <.*> ok.*")
# N.B.: as stated in RFC-5321, in typical SMTP transaction scenario there is no distinguishment between server answers for mail_from and rcpt_to client-requests(:)
RCPT_TO_pattern = re.compile("^250 2\.1\.0 <.*> ok.*") #re.compile("^250 2\.1\.5 <.*> recipient ok.*")
DATA_pattern = re.compile("^354.*")
QUIT_pattern = re.compile("^250 2\.0\.0 Ok.*")

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
        # S: 250-foo.com greets bar.com //ehlo
        # S: 250-8BITMIME //ehlo
        # S: 250-SIZE //ehlo
        # S: 250-DSN //ehlo
        # EHLO_pattern = re.compile("^250-.*")
        self.init_transition('EHLO_again'           , EHLO_STATE, EHLO_STATE)
        # S: 250 HELP //ehlo
        # EHLO_end_pattern = re.compile("^250 .*")
        self.init_transition('MAIL_FROM', EHLO_STATE, MAIL_FROM_WRITE_STATE)
        # C: MAIL FROM:<Smith@bar.com>  //mail_from_write
        self.init_transition('MAIL_FROM_write'      , MAIL_FROM_WRITE_STATE, MAIL_FROM_STATE)
        # S: 250 OK  //mail_from
        # MAIL_FROM_pattern = re.compile("^250 2\.1\.0 <.*> ok.*")
        self.init_transition('RCPT_TO'      , MAIL_FROM_STATE, RCPT_TO_WRITE_STATE)
        # C: RCPT TO:<Jones@foo.com> //rcpt_to_write
        self.init_transition('RCPT_TO_write'      , RCPT_TO_WRITE_STATE, RCPT_TO_STATE)
        # S: 250 OK  //rcpt_to
        # RCPT_TO_pattern = re.compile("^250 2\.1\.5 <.*> recipient ok.*")
        self.init_transition('RCPT_TO_additional'      , RCPT_TO_STATE, RCPT_TO_WRITE_STATE) #goto rcpt_to_write if pending recepient only (check outside rcpt_to_handler)
        # C: DATA  //data_write
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

    def EHLO_handler(self):
        self.logger.log(level=logging.DEBUG, msg="220 SMTP GREETING FROM 0.0.0.0.0.0.1\n")

    def EHLO_write_handler(self, socket, domain):
        socket.send(f'EHLO {domain}\r\n'.encode())
        self.logger.log(level=logging.DEBUG, msg=f"Socket sent EHLO message. (domain: {domain})\n")

    # def GREETING_write_handler(self, socket):
    #     socket.send("220 SMTP GREETING FROM 0.0.0.0.0.0.1\n".encode())

    def EHLO_again_handler(self):
        self.logger.log(level=logging.DEBUG, msg="Socket received: (EHLO) 250 ENHANCEDSTATUSCODES.\n")

    # def HELO_handler(self, socket, address, domain):
    #     self.logger.log(level=logging.DEBUG, msg="domain: {} connected".format(domain))

    def MAIL_FROM_handler(self):
        self.logger.log(level=logging.DEBUG, msg="Socket received answer for MAIL FROM message.\n")

    def MAIL_FROM_write_handler(self, socket, from_):
        socket.send(f('MAIL ' + from_ + '\r\n').encode())
        self.logger.log(level=logging.DEBUG, msg=f"Socket sent MAIL FROM message. ({from_})\n")

    #TODO: 15.01.2020: wrap to cycle in main handler for many recipients
    def RCPT_TO_write_handler(self, socket, to):
        socket.sendall(f'RCPT TO:<{to}>\r\n'.encode())
        self.logger.log(level=logging.DEBUG, msg=f"Socket sent RCPT TO message. (to: {to})\n")

    def RCPT_TO_handler(self):
        self.logger.log(level=logging.DEBUG, msg=f"Socket received answer for RCPT TO message.\n")

    def DATA_start_write_handler(self, socket):
        socket.sendall('DATA\r\n'.encode())
        self.logger.log(level=logging.DEBUG, msg=f"Socket sent DATA message.\n")

    def DATA_start_handler(self, socket):
        self.logger.log(level=logging.DEBUG, msg=f"Socket received answer for DATA message.\n")

    # TODO: 15.01.2020: wrap to cycle in main handler to send all strings
    def DATA_additional_handler(self, socket, string):
        socket.sendall(f'{string}\r\n'.encode())
        self.logger.log(level=logging.DEBUG, msg=f"Socket sent DATA string.\n")

    def DATA_end_write_handler(self):
        socket.sendall('.\r\n'.encode())

    def DATA_end_handler(self, socket):
        self.logger.log(level=logging.DEBUG, msg=f"Socket recieved answer for DATA end (250 OK)\n")

    def QUIT_write_handler(self, socket:socket.socket):
        socket.sendall('QUIT\r\n'.encode())
        self.logger.log(level=logging.DEBUG, msg=f"Socket sent QUIT message.\n")

    def QUIT_handler(self, socket:socket.socket):
        self.logger.log(level=logging.DEBUG, msg='Socket recieved answer for QUIT message. (221 Service closing transmission channel) Closing socket.\n')
        socket.close()


