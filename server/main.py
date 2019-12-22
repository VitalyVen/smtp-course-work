import re
import select

DATA_STATE = 'data'
DATA_WRITE_STATE = 'data_write'
DATA_END_WRITE_STATE = 'data_end_write'
QUIT_STATE = 'quit'
QUIT_WRITE_STATE = 'quit_write'
MAIL_FROM_STATE = 'mail_from'
RCPT_TO_STATE = 'rcpt_to'
RCPT_TO_WRITE_STATE = 'rcpt_to_write'
MAIL_FROM_WRITE_STATE = 'mail_from_write'
HELO_WRITE_STATE = 'helo_write'
HELO_STATE = 'helo'
GREETING_WRITE_STATE = 'greeting_write'
FINISH_STATE = 'finish'


READ_TIMEOUT = 40


from transitions import Machine
from transitions.extensions import GraphMachine as gMachine
import socket
import collections
import threading
import uuid

RE_CRLF = r"\r(\n)?"
#define RE_SPACE "\\s*"
#define RE_DOMAIN "(?<domain>.+)"
RE_EMAIL_OR_EMPTY = " ?<(?P<address>.+@.+)>|<>"
#define RE_EMAIL "<(?<address>.+@.+)>"
#define RE_DATA "[\\x00-\\x7F]+"
#define RE_DATA_AND_END_OR_DATA "((?<data>[\\x00-\\x7F]+)(?<end>\\r\\n\\.\\r\\n))|([\\x00-\\x7F]+\\r\\n)"

HELO_pattern = re.compile(f"^(HELO|EHLO) (.+){RE_CRLF}")#TODO: for regular hosts and like localhost
MAIL_FROM_pattern = re.compile(f"^MAIL FROM:{RE_EMAIL_OR_EMPTY}")
RCPT_TO_pattern = re.compile(f"^RCPT TO:{RE_EMAIL_OR_EMPTY}")
DATA_start_pattern = re.compile(f"^DATA( .*)*{RE_CRLF}")
DATA_end_pattern = re.compile(f"([\s\S]*)\.{RE_CRLF}")
QUIT_pattern = re.compile(f"^QUIT{RE_CRLF}")
RSET_pattern = re.compile(f"^RSET{RE_CRLF}")
#define RE_CMND_NOOP "^NOOP" RE_CRLF
#define RE_CMND_HELO "^HELO" RE_SPACE RE_DOMAIN RE_CRLF
#define RE_CMND_EHLO "^EHLO" RE_SPACE RE_DOMAIN RE_CRLF
#define RE_CMND_MAIL "^MAIL FROM:" RE_SPACE RE_EMAIL_OR_EMPTY RE_CRLF
#define RE_CMND_RCPT "^RCPT TO:" RE_SPACE RE_EMAIL RE_CRLF
#define RE_CMND_VRFY "^VRFY:" RE_SPACE RE_DOMAIN RE_CRLF
#define RE_CMND_DATA "^DATA" RE_CRLF
#define RE_CMND_QUIT "^QUIT" RE_CRLF

class SMTP_FSM(object):

    states = [
        GREETING_WRITE_STATE,

        HELO_STATE,
        HELO_WRITE_STATE,

        MAIL_FROM_STATE,
        MAIL_FROM_WRITE_STATE,

        RCPT_TO_STATE,
        RCPT_TO_WRITE_STATE,

        DATA_STATE,#for reading start date message and all next, except last Note we should't reply to on all additional data messages
        DATA_WRITE_STATE,
        DATA_END_WRITE_STATE,

        QUIT_STATE,
        QUIT_WRITE_STATE,
        FINISH_STATE
        ]

    def __init__(self, name):
        self.name = name
        self.machine = gMachine(model=self, states=self.states, initial=GREETING_WRITE_STATE)

        # self.machine.add_transition(before='GREETING_handler', trigger='GREETING', source='greeting', dest='greeting_write')
        self.machine.add_transition(before='HELO_handler', trigger='HELO', source=HELO_STATE, dest=HELO_WRITE_STATE)
        self.machine.add_transition(before='MAIL_FROM_handler', trigger='MAIL_FROM', source=MAIL_FROM_STATE, dest=MAIL_FROM_WRITE_STATE)
        self.machine.add_transition(before='RCPT_TO_handler', trigger='RCPT_TO', source=RCPT_TO_STATE, dest=RCPT_TO_WRITE_STATE)
        self.machine.add_transition(before='DATA_start_handler', trigger='DATA_start', source=DATA_STATE, dest=DATA_WRITE_STATE)
        self.machine.add_transition(before='DATA_additional_handler', trigger='DATA_additional', source=DATA_STATE, dest=DATA_STATE)
        self.machine.add_transition(before='DATA_end_handler', trigger='DATA_end', source=DATA_STATE, dest=DATA_END_WRITE_STATE)
        self.machine.add_transition(before='QUIT_handler', trigger='QUIT', source=QUIT_STATE, dest=QUIT_WRITE_STATE)

        self.machine.add_transition(before='GREETING_handler_write', trigger='GREETING_write', source=GREETING_WRITE_STATE, dest=HELO_STATE)
        self.machine.add_transition(before='HELO_handler_write', trigger='HELO_write', source=HELO_WRITE_STATE, dest=MAIL_FROM_STATE)
        self.machine.add_transition(before='MAIL_FROM_handler_write', trigger='MAIL_FROM_write', source=MAIL_FROM_WRITE_STATE, dest=RCPT_TO_STATE)
        self.machine.add_transition(before='RCPT_TO_handler_write', trigger='RCPT_TO_write', source=RCPT_TO_WRITE_STATE, dest=DATA_STATE)
        self.machine.add_transition(before='DATA_start_handler_write', trigger='DATA_start_write', source=DATA_WRITE_STATE, dest=DATA_STATE)
        self.machine.add_transition(before='DATA_end_handler_write', trigger='DATA_end_write', source=DATA_END_WRITE_STATE, dest=QUIT_STATE)
        self.machine.add_transition(before='QUIT_handler_write', trigger='QUIT_write', source=QUIT_WRITE_STATE, dest='finish')

        self.machine.add_transition(before='RSET_handler', trigger='RSET', source='*', dest='helo_write')
        self.machine.add_transition(before='RSET_handler_write', trigger='RSET_write', source='*', dest='helo')

        self.machine.add_transition(before='ANOTHER_RECEPIENT_handler', trigger='ANOTHER_recepient', source=DATA_END_WRITE_STATE, dest=MAIL_FROM_STATE)

    def GREETING_handler(self, socket):
        print("220 SMTP BORIS KOSTYA VITALY PAVEL GREET YOU 0.0.0.0.0.0.1 \n".encode())
    def GREETING_handler_write(self, socket):
        socket.send("220 SMTP BORIS KOSTYA VITALY PAVEL GREET YOU 0.0.0.0.0.0.1 \n".encode())

    def HELO_handler(self, socket, address, domain):
        print("domain: {} connected".format(domain))

    def HELO_handler_write(self, socket, address, domain):
        socket.send("250 {} OK \n".format(domain).encode())

    def MAIL_FROM_handler(self, socket, email):
        print("f: {} mail from".format(email))

    def MAIL_FROM_handler_write(self, socket, email):
        socket.send("250 2.1.0 Ok \n".encode())

    def RCPT_TO_handler(self, socket, email):
        print("f: {} mail to".format(email))
    def RCPT_TO_handler_write(self, socket, email):
        socket.send("250 2.1.5 Ok \n".encode())

    def DATA_start_handler(self, socket):

        print("354 Send message content; end with <CRLF>.<CRLF>\n".encode())
    def DATA_start_handler_write(self, socket):
        socket.send("354 Send message content; end with <CRLF>.<CRLF>\n".encode())

    def DATA_additional_handler(self, socket):
        pass
    def DATA_end_handler(self, socket):
        filename='cHQR7GZI1DOjOeB0g3pvMptrYeEkVzE2PQkK-y1TyFg=@'
    def DATA_end_handler_write(self, socket):
        filename='cHQR7GZI1DOjOeB0g3pvMptrYeEkVzE2PQkK-y1TyFg=@'
        socket.send(f"250 Ok: queued as {filename}\n".encode())
    def QUIT_handler(self, socket:socket.socket):
        print('someone_want_to quit')
    def QUIT_handler_write(self, socket:socket.socket):
        socket.send("221 Bye\n".encode())
        socket.close()

    def RSET_handler(self, socket):
        print(f"250 OK \n".encode())
    def RSET_handler_write(self, socket):
        socket.send(f"250 OK \n".encode())

class ClientSocket():
    def __init__(self, connection, address):
        self.connection = connection
        self.address = address
    def readline(self):
        #TODO: handle long message
        bufferSize = 4096
        # add timeout to the connection if no commands are recieved
        self.connection.settimeout(READ_TIMEOUT)
        # buffer = self.connection.recv(bufferSize).decode()
        # # print(buffer)
        # # remove timeout if commands are recieved
        # self.connection.settimeout(None)
        # buffering = True
        # while buffering:
        #     # prevent empty lines from being processed
        #     if buffer == "\r\n":
        #         self.connection.send("500 5.5.2 Error: bad syntax \n".encode())
        #     if "\r\n" in buffer:
        #         (line, buffer) = buffer.split("\n", 1)
        #         return line
        #     elif "\r\n.\r\n" in buffer:
        #         return buffer
        #     else:
        #         more = self.connection.recv(bufferSize).decode()
        #         if not more:
        #             buffering = False
        #         else:
        #             buffer += more

        return self.connection.recv(bufferSize).decode()

    def send(self, *args, **kwargs):
        return self.connection.send(*args, **kwargs)
    def close(self, *args, **kwargs):
        return self.connection.close(*args, **kwargs)

class Client():
    def __init__(self, socket:ClientSocket):
        self.socket = socket
        self.machine = SMTP_FSM(socket.address)
        self.mail = None
        self.domain = None
        self.mail_to = None
        self.mail_from = None

    def mail_to_file(self, filename=None):
        if filename is None:
            filename = f'../maildir/{uuid.uuid4()}=@'
        with open(filename, 'w') as file:
            file.write(self.mail)

class MailServer():
    def __init__(self, host='localhost', port=2556, threads=5):
        self.host = host
        self.port = port
        self.clients = ClientsCollection()
        self.threads_cnt = threads
    def __enter__(self):
        self.socket_init()
        return self

    def socket_init(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.settimeout(READ_TIMEOUT)
        server_address = (self.host, self.port)
        self.sock.bind(server_address)
        self.sock.listen(0)

    def serve_forever(self):
        for i in range(self.threads_cnt):
            thread_sock = threading.Thread(target=thread_socket, args=(serv, i,))
            thread_sock.daemon = True
            thread_sock.start()
        while True:
            pass
    def handle_client_read(self, cl:Client):
        try:
            line = cl.socket.readline()
        except socket.timeout:
            self.sock.close()
            return
        print(line)
        #check possible states from cl.machine.state
        #match exact state with re patterns
        #call handler for this state

        current_state = cl.machine.state

        if current_state==HELO_STATE:
            HELO_matched = re.search(HELO_pattern, line)
            if HELO_matched:
                command = HELO_matched.group(1)
                domain = HELO_matched.group(2) or "unknown"
                cl.mail = f"{command}:<{domain}>\r\n"
                cl.domain = domain
                cl.machine.HELO(cl.socket, cl.socket.address, domain)
                return
        elif current_state==MAIL_FROM_STATE:
            MAIL_FROM_matched = re.search(MAIL_FROM_pattern, line)
            if MAIL_FROM_matched:
                cl.mail_from = MAIL_FROM_matched.group(1)
                cl.mail += f"FROM:<{cl.mail_from}>\r\n"
                cl.machine.MAIL_FROM(cl.socket, cl.mail_from)
                return
        elif current_state==RCPT_TO_STATE:
            RCPT_TO_matched = re.search(RCPT_TO_pattern, line)
            if RCPT_TO_matched:
                mail_to = RCPT_TO_matched.group(1)
                cl.mail += f"TO:<{mail_to}>\r\n\r\n"
                cl.mail_to = mail_to
                cl.machine.RCPT_TO(cl.socket, mail_to)
                return
        elif current_state==DATA_STATE:#TODO as fsm?
            DATA_start_matched = re.search(DATA_start_pattern, line)
            DATA_end_matched = re.search(DATA_end_pattern, line)
            if DATA_start_matched:#TODO case when data additional match data_start
                data = DATA_start_matched.group(1)
                if data:
                    cl.mail += data
                cl.machine.DATA_start(cl.socket)
            elif DATA_end_matched:#TODO and already started
                data = DATA_end_matched.group(1)
                if data:
                    cl.mail += data
                cl.machine.DATA_end(cl.socket)
                cl.mail_to_file()
            else:#TODO only if started
                cl.mail += line
                cl.machine.DATA_additional(cl.socket)
            return
        elif current_state==QUIT_STATE:
            QUIT_matched = re.search(QUIT_pattern, line)
            if QUIT_matched:
                cl.machine.QUIT(cl.socket)
        #Transition possible from any states
        RSET_matched = re.search(RSET_pattern, line)
        if RSET_matched:
            cl.machine.RSET(cl.socket)
        else:
            pass
        # cl.socket.send(f'500 Unrecognised command {line}\n'.encode())
        # print('500 Unrecognised command')

    def handle_client_write(self, cl:Client):
        current_state = cl.machine.state
        if current_state=='greeting_write':
            cl.machine.GREETING_write(cl.socket)
        elif current_state=='helo_write':
            cl.machine.HELO_write(cl.socket, cl.socket.address, cl.domain)
        elif current_state==MAIL_FROM_WRITE_STATE:
            cl.machine.MAIL_FROM_write(cl.socket, cl.mail_from)
        elif current_state=='rcpt_to_write':
            cl.machine.RCPT_TO_write(cl.socket, cl.mail_to)
        elif current_state=='data_write':
            cl.machine.DATA_start_write(cl.socket)
        elif current_state=='data_end_write':
            cl.machine.DATA_end_write(cl.socket)
        elif current_state=='quit_write':
            cl.machine.QUIT_write(cl.socket)
            self.clients.pop(cl.socket.connection)
        else:
            pass
            # print(current_state)
            # cl.socket.send(f'500 Unrecognised command TODO\n'.encode())
            # print('500 Unrecognised command')
    def __exit__(self, exc_type, exc_val, exc_tb):
        self.sock.close()

class ClientsCollection(collections.UserDict):
    def sockets(self):
        return list(self.data.keys())

def thread_socket(serv, name):
    while True:
        client_sockets = serv.clients.sockets()
        rfds, wfds, errfds = select.select([serv.sock] + client_sockets, client_sockets, [], 100)
        if len(rfds) != 0:
            for fds in rfds:
                if fds is serv.sock:
                    connection, client_address = fds.accept()
                    client = Client(socket=ClientSocket(connection, client_address))
                    client.machine.GREETING_write(client.socket)
                    serv.clients[connection] = client
                else:
                    # print('Thread {}'.format(name))
                    serv.handle_client_read(serv.clients[fds])
        if len(wfds) != 0:
            for fds in rfds:
                if fds is serv.sock:
                    # connection, client_address = fds.accept()
                    # client = Client(socket=ClientSocket(connection, client_address))
                    # client.machine.GREETING(client.socket)
                    # serv.clients[connection] = client
                    pass
                else:
                    # print('Thread {}'.format(name))
                    serv.handle_client_write(serv.clients[fds])

if __name__ == '__main__':
    with MailServer(port=2556) as serv:
        serv.serve_forever()


