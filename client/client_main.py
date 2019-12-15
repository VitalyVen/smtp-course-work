import re
import select

READ_TIMEOUT = 300


from transitions import Machine
from transitions.extensions import GraphMachine as gMachine
import socket
import collections
import threading


RE_CRLF = r"\r\n"
#define RE_SPACE "\\s*"
#define RE_DOMAIN "(?<domain>.+)"
RE_EMAIL_OR_EMPTY = "<(?P<address>.+@.+)>|<>"
#define RE_EMAIL "<(?<address>.+@.+)>"
#define RE_DATA "[\\x00-\\x7F]+"
#define RE_DATA_AND_END_OR_DATA "((?<data>[\\x00-\\x7F]+)(?<end>\\r\\n\\.\\r\\n))|([\\x00-\\x7F]+\\r\\n)"

HELO_pattern = re.compile("^HELO (.*\.\w+)")
MAIL_FROM_pattern = re.compile(f"^MAIL FROM: {RE_EMAIL_OR_EMPTY}")
RCPT_TO_pattern = re.compile(f"^RCPT TO: {RE_EMAIL_OR_EMPTY}")
DATA_pattern = re.compile(f"^DATA (.*)\.{RE_CRLF}")
#define RE_CMND_NOOP "^NOOP" RE_CRLF
#define RE_CMND_HELO "^HELO" RE_SPACE RE_DOMAIN RE_CRLF
#define RE_CMND_EHLO "^EHLO" RE_SPACE RE_DOMAIN RE_CRLF
#define RE_CMND_MAIL "^MAIL FROM:" RE_SPACE RE_EMAIL_OR_EMPTY RE_CRLF
#define RE_CMND_RCPT "^RCPT TO:" RE_SPACE RE_EMAIL RE_CRLF
#define RE_CMND_VRFY "^VRFY:" RE_SPACE RE_DOMAIN RE_CRLF
#define RE_CMND_DATA "^DATA" RE_CRLF
#define RE_CMND_RSET "^RSET" RE_CRLF
#define RE_CMND_QUIT "^QUIT" RE_CRLF

class SMTP_FSM(object):

    states = ['init', 'greeting', 'hello', 'from', 'to', 'data']

    def __init__(self, name):
        self.name = name
        self.machine = gMachine(model=self, states=self.states, initial='init')

        self.machine.add_transition(trigger='GREETING_handler', source='init', dest='greeting')
        self.machine.add_transition(trigger='HELLO_handler', source='greeting', dest='hello')
        self.machine.add_transition(trigger='MAIL_FROM_handler', source='hello', dest='mail_from')
        self.machine.add_transition(trigger='RSPT_TO_handler', source='mail_from', dest='rcpt_to')
        self.machine.add_transition(trigger='DATA_handler', source='rcpt_to', dest='data')
    def GREETING_handler(self, socket):
        socket.send("220 SMTP BORIS KOSTYA 0.0.0.0.0.0.1 \n".encode())

    def HELO_handler(self, socket, address, domain):
        print("domain: {} connected".format(domain))
        socket.send("250 {} OK \n".format(domain).encode())

    def MAIL_FROM_handler(self, socket, email):
        print("f: {} mail from".format(email))
        socket.send("250 2.1.0 Ok \n".encode())

    def RSPT_TO_handler(self, socket, email):
        print("f: {} mail to".format(email))
        socket.send("250 2.1.5 Ok \n".encode())

    def DATA_handler(self, socket):

        socket.send("ok\n".encode())

class ClientSocket():
    def __init__(self, connection, address):
        self.connection = connection
        self.address = address
    def readline(self):
        #TODO: handle long message
        bufferSize = 4096
        # add timeout to the connection if no commands are recieved
        self.connection.settimeout(READ_TIMEOUT)
        buffer = self.connection.recv(bufferSize).decode()
        # print(buffer)
        # remove timeout if commands are recieved
        self.connection.settimeout(None)
        buffering = True
        while buffering:
            # prevent empty lines from being processed
            if buffer == "\r\n":
                self.connection.send("500 5.5.2 Error: bad syntax \n".encode())
            if "\r\n" in buffer:
                (line, buffer) = buffer.split("\n", 1)
                return line
            elif "\r\n.\r\n" in buffer:
                return buffer
            else:
                more = self.connection.recv(bufferSize).decode()
                if not more:
                    buffering = False
                else:
                    buffer += more

        return self.connection.recv(bufferSize).decode()

    def send(self, *args, **kwargs):
        return self.connection.send(*args, **kwargs)

class Client():
    def __init__(self, socket:ClientSocket):
        self.socket = socket
        self.machine = SMTP_FSM(socket.address)
        self.mail = ''

    def mail_to_file(self, filename='../maildir/test.txt'):
        with open(filename, 'w') as file:
            file.write(self.mail)

class MailServer():
    def __init__(self, host='localhost', port=2556, threads=5):
        self.host = host
        self.port = port
        self.threads_cnt = threads
    def __enter__(self):
        self.socket_init()
        return self

    def socket_init(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server_address = (self.host, self.port)
        self.sock.bind(server_address)
        self.sock.listen(0)

    def serve_forever(self):
        for i in range(self.threads_cnt):
            thread_sock = threading.Thread(target=thread_socket, args=(serv, clients, i,))
            thread_sock.daemon = True
            thread_sock.start()
        while True:
            pass
    def handle_client(self, cl:Client):
        try:
            line = cl.socket.readline()
        except socket.timeout:
            self.sock.close()
        else:
            #TODO:
            #check possible states from cl.machine.state
            #match exact state with re patterns
            #call handler for this state
            HELLO_matched = re.match(HELO_pattern, line)
            MAIL_FROM_matched = re.match(MAIL_FROM_pattern, line)
            RCPT_TO_matched = re.match(RCPT_TO_pattern, line)
            DATA_matched = re.match(DATA_pattern, line)

            if HELLO_matched:
                domain = HELLO_matched.group(1)
                cl.mail = domain
                cl.machine.HELO_handler(cl.socket, cl.socket.address, domain)
            elif MAIL_FROM_matched:
                mail_from = MAIL_FROM_matched.group(1)
                cl.mail += mail_from
                cl.machine.MAIL_FROM_handler(cl.socket, mail_from)
            elif RCPT_TO_matched:
                mail_to = RCPT_TO_matched.group(1)
                cl.mail += mail_to
                cl.machine.RSPT_TO_handler(cl.socket, mail_to)
            elif DATA_matched:
                mail_from = DATA_matched.group(1)
                cl.mail += mail_from
                cl.machine.DATA_handler(cl.socket)
                cl.mail_to_file()
            else:
                print('incorrect')

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.sock.close()

class ClientsCollection(collections.UserDict):
    def sockets(self):
        return list(self.data.keys())

def thread_socket(serv,clients,name):
    while True:
        client_sockets = clients.sockets()
        rfds, wfds, errfds = select.select([serv.sock] + client_sockets, [], [], 100)
        if len(rfds) != 0:
            for fds in rfds:
                if fds is serv.sock:
                    connection, client_address = fds.accept()
                    client = Client(socket=ClientSocket(connection, client_address))
                    client.machine.GREETING_handler(client.socket)
                    clients[connection] = client
                else:
                    print('Thread {}'.format(name))
                    serv.handle_client(clients[fds])

if __name__ == '__main__':
    clients = ClientsCollection()
    with MailServer(port=2556) as serv:
        serv.serve_forever()


