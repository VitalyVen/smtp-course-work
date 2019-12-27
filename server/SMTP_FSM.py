import socket
from state import *
from transitions import Machine
from transitions.extensions import GraphMachine as gMachine

class SMTP_FSM(object):
    def __init__(self, name):
        self.name = name
        self.machine = self.init_machine()

        # self.init_transition('GREETING'       , GREETING_STATE , GREETING_WRITE_STATE )
        self.init_transition('HELO'           , HELO_STATE     , HELO_WRITE_STATE     )
        self.init_transition('MAIL_FROM'      , MAIL_FROM_STATE, MAIL_FROM_WRITE_STATE)
        self.init_transition('RCPT_TO'        , RCPT_TO_STATE  , RCPT_TO_WRITE_STATE  )
        self.init_transition('DATA_start'     , DATA_STATE     , DATA_WRITE_STATE     )
        self.init_transition('DATA_additional', DATA_STATE     , DATA_STATE           )
        self.init_transition('DATA_end'       , DATA_STATE     , DATA_END_WRITE_STATE )
        self.init_transition('QUIT'           , QUIT_STATE     , QUIT_WRITE_STATE     )

        self.init_transition('GREETING_write' , GREETING_WRITE_STATE , HELO_STATE     )
        self.init_transition('HELO_write'     , HELO_WRITE_STATE     , MAIL_FROM_STATE)
        self.init_transition('MAIL_FROM_write', MAIL_FROM_WRITE_STATE, RCPT_TO_STATE  )
        self.init_transition('RCPT_TO_write'  , RCPT_TO_WRITE_STATE  , DATA_STATE     )
        self.init_transition('DATA_start_write',DATA_WRITE_STATE     , DATA_STATE     )
        self.init_transition('DATA_end_write' , DATA_END_WRITE_STATE , QUIT_STATE     )
        self.init_transition('QUIT_write'     , QUIT_WRITE_STATE     , FINISH_STATE   )

        self.init_transition('RSET'      , source='*', destination=HELO_WRITE_STATE)
        self.init_transition('RSET_write', source='*', destination=HELO_STATE      )

        self.init_transition('ANOTHER_recepient', DATA_END_WRITE_STATE, MAIL_FROM_STATE)
    
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
        print("220 SMTP GREETING FROM 0.0.0.0.0.0.1\n".encode())

    def GREETING_write_handler(self, socket):
        socket.send("220 SMTP GREETING FROM 0.0.0.0.0.0.1\n".encode())

    def HELO_handler(self, socket, address, domain):
        print("domain: {} connected".format(domain))

    def HELO_write_handler(self, socket, address, domain):
        socket.send("250 {} OK \n".format(domain).encode())

    def MAIL_FROM_handler(self, socket, email):
        print("f: {} mail from".format(email))

    def MAIL_FROM_write_handler(self, socket, email):
        socket.send("250 2.1.0 Ok \n".encode())

    def RCPT_TO_handler(self, socket, email):
        print("f: {} mail to".format(email))
        
    def RCPT_TO_write_handler(self, socket, email):
        socket.send("250 2.1.5 Ok \n".encode())

    def DATA_start_handler(self, socket):
        print("354 Send message content; end with <CRLF>.<CRLF>\n".encode())
    
    def DATA_start_write_handler(self, socket):
        socket.send("354 Send message content; end with <CRLF>.<CRLF>\n".encode())

    def DATA_additional_handler(self, socket):
        pass

    def DATA_end_handler(self, socket):
        filename = 'cHQR7GZI1DOjOeB0g3pvMptrYeEkVzE2PQkK-y1TyFg=@'
        pass
    
    def DATA_end_write_handler(self, socket):
        filename = 'cHQR7GZI1DOjOeB0g3pvMptrYeEkVzE2PQkK-y1TyFg=@'
        socket.send(f"250 Ok: queued as {filename}\n".encode())

    def QUIT_handler(self, socket:socket.socket):
        print('Disconnecting..\n')

    def QUIT_write_handler(self, socket:socket.socket):
        socket.send("221 Left conversation\n".encode())
        socket.close()

    def RSET_handler(self, socket):
        print(f"250 OK \n".encode())

    def RSET_write_handler(self, socket):
        socket.send(f"250 OK \n".encode())