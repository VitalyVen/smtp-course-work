import re
import select
import ssl

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

HELO_pattern = re.compile("^(HELO|EHLO) (.*\.\w+|localhost)")
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

class SMTP_FSM_CLIENT(object):

    states = ['init', 'greeting', 'hello', 'mail_from', 'rcpt_to', 'data', 'data_end', 'finish']

    def __init__(self, name):
        self.name = name
        self.machine = gMachine(model=self, states=self.states, initial='init')

        self.machine.add_transition(before='GREETING_handler', trigger='GREETING', source='init', dest='greeting')
        self.machine.add_transition(before='HELO_handler', trigger='HELO', source='greeting', dest='hello')
        self.machine.add_transition(before='RSET_handler', trigger='RSET', source='*', dest='hello')
        self.machine.add_transition(before='MAIL_FROM_handler', trigger='MAIL_FROM', source='hello', dest='mail_from')
        self.machine.add_transition(before='RCPT_TO_handler', trigger='RCPT_TO', source='mail_from', dest='rcpt_to')
        self.machine.add_transition(before='DATA_start_handler', trigger='DATA_start', source='rcpt_to', dest='data')
        self.machine.add_transition(before='DATA_additional_handler', trigger='DATA_additional', source='data', dest='data')
        self.machine.add_transition(before='DATA_end_handler', trigger='DATA_end', source='data', dest='data_end')
        self.machine.add_transition(before='QUIT_handler', trigger='QUIT', source='data_end', dest='finish')

    def GREETING_handler(self, socket):
        socket.send("220 SMTP BORIS KOSTYA VITALY PAVEL GREET YOU 0.0.0.0.0.0.1 \n".encode())

    def HELO_handler(self, socket, address, domain):
        print("domain: {} connected".format(domain))
        socket.send("250 {} OK \n".format(domain).encode())

    def MAIL_FROM_handler(self, socket, email):
        print("f: {} mail from".format(email))
        socket.send("250 2.1.0 Ok \n".encode())

    def RCPT_TO_handler(self, socket, email):
        print("f: {} mail to".format(email))
        socket.send("250 2.1.5 Ok \n".encode())

    def DATA_start_handler(self, socket):

        socket.send("354 Send message content; end with <CRLF>.<CRLF>\n".encode())
    def DATA_additional_handler(self, socket):
        pass
    def DATA_end_handler(self, socket):
        filename='cHQR7GZI1DOjOeB0g3pvMptrYeEkVzE2PQkK-y1TyFg=@'
        socket.send(f"250 Ok: queued as {filename}\n".encode())
    def QUIT_handler(self, socket:socket.socket):
        socket.send("221 Bye\n".encode())
    def RSET_handler(self, socket):
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
        self.machine = SMTP_FSM_CLIENT(socket.address)
        self.mail = ''

    def mail_to_file(self, filename=None):
        if filename is None:
            filename = f'../maildir/{uuid.uuid4()}=@'
        with open(filename, 'w') as file:
            file.write(self.mail)

class MailServer():
    def __init__(self, host='localhost', port=2556, threads=5):
        self.clients = ClientsCollection()

    def __enter__(self):
        self.socket_init()
        return self

    def socket_init(self):
       pass

    # def serve_forever(self):
    #     for i in range(self.threads_cnt):
    #         thread_sock = threading.Thread(target=thread_socket, args=(serv, i,))
    #         thread_sock.daemon = True
    #         thread_sock.start()
    #     while True:
    #         pass
    def handle_client(self, cl:Client):
        try:

            line = cl.socket.readline()
        except socket.timeout:
            self.sock.close()
        else:
            print(line)
            #TODO:
            #check possible states from cl.machine.state
            #match exact state with re patterns
            #call handler for this state
            HELO_matched = re.search(HELO_pattern, line)
            MAIL_FROM_matched = re.search(MAIL_FROM_pattern, line)
            RCPT_TO_matched = re.search(RCPT_TO_pattern, line)
            DATA_start_matched = re.search(DATA_start_pattern, line)
            DATA_end_matched = re.search(DATA_end_pattern, line)
            QUIT_matched = re.search(QUIT_pattern, line)
            RSET_matched = re.search(RSET_pattern, line)

            if HELO_matched:
                domain = HELO_matched.group(1)
                cl.mail = domain
                cl.machine.HELO(cl.socket, cl.socket.address, domain)
            elif MAIL_FROM_matched:
                mail_from = MAIL_FROM_matched.group(1)
                cl.mail += mail_from + '\r\n'
                cl.machine.MAIL_FROM(cl.socket, mail_from)
            elif RCPT_TO_matched:
                mail_to = RCPT_TO_matched.group(1)
                cl.mail += mail_to + '\r\n'
                cl.machine.RCPT_TO(cl.socket, mail_to)
            elif DATA_start_matched:
                data = DATA_start_matched.group(1)
                if data:
                    cl.mail += data
                cl.machine.DATA_start(cl.socket)
            elif DATA_end_matched:
                data = DATA_end_matched.group(1)
                if data:
                    cl.mail += data
                cl.machine.DATA_end(cl.socket)
                cl.mail_to_file()
            elif QUIT_matched:
                cl.machine.QUIT(cl.socket)
                cl.socket.close()
                self.clients.pop(cl.socket.connection)
            elif RSET_matched:
                cl.machine.RSET(cl.socket)
            else:
                if cl.machine.state=='data':
                    cl.mail+=line
                    cl.machine.DATA_additional(cl.socket)
                else:
                    cl.socket.send(f'500 Unrecognised command {line}\n'.encode())
                    print('500 Unrecognised command')

    def __exit__(self, exc_type, exc_val, exc_tb):
        pass

class ClientsCollection(collections.UserDict):
    def sockets(self):
        return list(self.data.keys())

def thread_socket(serv, name):
    while True:
        client_sockets = serv.clients.sockets()
        rfds, wfds, errfds = select.select([serv.sock] + client_sockets, [], [], 100)
        if len(rfds) != 0:
            for fds in rfds:
                if fds is serv.sock:
                    connection, client_address = fds.accept()
                    client = Client(socket=ClientSocket(connection, client_address))
                    client.machine.GREETING(client.socket)
                    serv.clients[connection] = client
                else:
                    # print('Thread {}'.format(name))
                    serv.handle_client(serv.clients[fds])

if __name__ == '__main__':
    with MailServer() as client:
        soc= []
        context = ssl._create_stdlib_context()
        client_sockets = socket.create_connection(("smtp.yandex.ru", 465), 5,
                                                  None)
        client_sockets = context.wrap_socket(client_sockets,
                                         server_hostname="smtp.yandex.ru")
        client_sockets2 = socket.create_connection(("smtp.yandex.ru", 465), 5,
                                                  None)
        client_sockets2 = context.wrap_socket(client_sockets2,
                                             server_hostname="smtp.yandex.ru")
        soc.append(client_sockets)
        soc.append(client_sockets2)
        while True:
            rfds, wfds, errfds = select.select(soc,[], [], 100)
            if len(rfds) != 0:
                for fds in rfds:
                        # serv.handle_client(serv.clients[fds])
                    print(fds.recv())
                    print(fds.send(b'ehlo [127.0.1.1]\r\n'))

