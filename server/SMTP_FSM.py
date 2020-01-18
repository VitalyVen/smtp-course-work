import socket
import logging

from common.logger_threads import CustomLogHandler
from state import *
try:
    from common.custom_logger_proc import QueueProcessLogger
except (ModuleNotFoundError, ImportError) as e:
    import sys
    import os

    from custom_logger_proc import QueueProcessLogger
from transitions import Machine
from transitions.extensions import GraphMachine as gMachine

class SMTP_FSM(object):
    def __init__(self, name, logdir):
        self.name = name
        logger = logging.getLogger()
        logger.addHandler(CustomLogHandler(logdir))
        logger.setLevel(logging.DEBUG)
        self.logger = logger
        self.machine = self.init_machine()
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

        # self.init_transition('GREETING'       , GREETING_STATE , GREETING_WRITE_STATE )
        self.init_transition('HELO'           , HELO_STATE     , HELO_WRITE_STATE     )
        self.init_transition('MAIL_FROM'      , MAIL_FROM_STATE, MAIL_FROM_WRITE_STATE)
        self.init_transition('RCPT_TO'        , RCPT_TO_STATE  , RCPT_TO_WRITE_STATE  )
        self.init_transition('DATA_start'     , DATA_START_STATE     , DATA_WRITE_STATE     )
        self.init_transition('DATA_additional', DATA_STATE     , DATA_STATE           )
        self.init_transition('DATA_end'       , DATA_STATE     , DATA_END_WRITE_STATE )
        self.init_transition('QUIT'           , '*'     , QUIT_WRITE_STATE     )

        self.init_transition('GREETING_write' , GREETING_WRITE_STATE , HELO_STATE     )
        self.init_transition('HELO_write'     , HELO_WRITE_STATE     , MAIL_FROM_STATE)
        self.init_transition('MAIL_FROM_write', MAIL_FROM_WRITE_STATE, RCPT_TO_STATE  )
        self.init_transition('RCPT_TO_write'  , RCPT_TO_WRITE_STATE  , DATA_START_STATE     )
        self.init_transition('ANOTHER_RECEPIENT', DATA_STATE  , RCPT_TO_WRITE_STATE)
        self.init_transition('DATA_start_write',DATA_WRITE_STATE     , DATA_STATE     )
        self.init_transition('DATA_end_write' , DATA_END_WRITE_STATE , QUIT_STATE     )
        self.init_transition('QUIT_write'     , QUIT_WRITE_STATE     , FINISH_STATE   )

        self.init_transition('RSET'      , source='*', destination=HELO_WRITE_STATE)
        self.init_transition('RSET_write', source='*', destination=HELO_STATE      )

    def init_machine(self):
        return gMachine(
            model=self,
            states=states,
            initial=GREETING_WRITE_STATE
        )

    def init_transition(self, trigger, source, destination):
        self.machine.add_transition(
            before=trigger + '_handler',
            trigger=trigger,
            source=source,
            dest=destination
        )

    def GREETING_handler(self, socket):
        self.logger.log(level=logging.DEBUG, msg="220 SMTP GREETING FROM 0.0.0.0.0.0.1\n")


    def GREETING_write_handler(self, socket):
        socket.send("220 SMTP GREETING FROM 0.0.0.0.0.0.1\n".encode())


    def HELO_handler(self, socket, address, domain):
        self.logger.log(level=logging.DEBUG, msg="domain: {} connected".format(domain))

    def HELO_write_handler(self, socket:socket.socket, address, domain):
        socket.send("250 {} OK \n".format(domain).encode())

    def MAIL_FROM_handler(self, socket, email):
        self.logger.log(level=logging.DEBUG, msg="f: {} mail from".format(email))

    def MAIL_FROM_write_handler(self, socket:socket.socket, email):
        socket.send("250 2.1.0 Ok \n".encode())

    def RCPT_TO_handler(self, socket, email):
        self.logger.log(level=logging.DEBUG, msg="f: {} mail to".format(email))

    def ANOTHER_RECEPIENT_handler(self, socket, email):
        self.logger.log(level=logging.DEBUG, msg="f: {} mail to".format(email))
        
    def RCPT_TO_write_handler(self, socket:socket.socket, email):
        socket.send("250 2.1.5 Ok \n".encode())

    def DATA_start_handler(self, socket):
        self.logger.log(level=logging.DEBUG, msg="Data started")
    
    def DATA_start_write_handler(self, socket:socket.socket):
        socket.send("354 Send message content; end with <CRLF>.<CRLF>\n".encode())
        self.machine.pool_metka = False

    def DATA_additional_handler(self, socket):
        self.logger.log(level=logging.DEBUG, msg="Additional data")


    def DATA_end_handler(self, socket):
        self.logger.log(level=logging.DEBUG, msg="Data end")
        self.machine.pool_metka = True

    def DATA_end_write_handler(self, socket:socket.socket, filename):
        socket.send(f"250 Ok: queued as {filename}\n".encode())

        self.machine.pool_metka = True

    def QUIT_handler(self, socket:socket.socket):
        self.logger.log(level=logging.DEBUG, msg='Disconnecting..\n')

    def QUIT_write_handler(self, socket:socket.socket):
        socket.send("221 Left conversation\n".encode())

    def RSET_handler(self, socket):
        self.logger.log(level=logging.DEBUG, msg=f"Rsetted \n")

    def RSET_write_handler(self, socket):
        socket.send(f"250 OK \n".encode())