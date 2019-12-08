import re
import select

READ_TIMEOUT = 300


from transitions import Machine
import socket
import collections

HELO_pattern = re.compile("^HELO (.*\.\w+)")

class SMTP_FSM(object):

    states = ['init', 'greeting', 'hello', 'from', 'to', 'data']

    def __init__(self, name):
        self.name = name
        self.machine = Machine(model=self, states=self.states, initial='init')
        self.machine.add_transition(trigger='HELLO_handler', source='greeting', dest='hello')
        self.machine.add_transition(trigger='GREETING_handler', source='init', dest='greeting')

    def HELO_handler(self, socket, address, domain):
        print("domain: {} connected".format(domain))
        socket.send("250 {} OK \n".format(domain).encode())

    def GREETING_handler(self, socket):
        socket.send("220 SMTP BORIS KOSTYA 0.0.0.0.0.0.1 \n".encode())

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

class MailServer():
    def __init__(self, port):
        self.port = port
    def __enter__(self):
        self.socket_init()
        return self
    def socket_init(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server_address = ('localhost', self.port)
        self.sock.bind(server_address)
        self.sock.listen(0)

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
            matched = re.match(HELO_pattern, line)
            if matched:
                domain = matched.group(1)
                cl.mail = domain
                cl.machine.HELO_handler(cl.socket, cl.socket.address, domain)
    def __exit__(self, exc_type, exc_val, exc_tb):
        self.sock.close()

class ClientsCollection(collections.UserDict):
    def sockets(self):
        return list(self.data.keys())

if __name__ == '__main__':
    clients = ClientsCollection()
    with MailServer(port=2556) as serv:
        while True:
            client_sockets = clients.sockets()
            rfds, wfds, errfds = select.select([serv.sock]+client_sockets, [], [], 5)
            if len(rfds) != 0:
                for fds in rfds:
                    if fds is serv.sock:
                        connection, client_address = fds.accept()
                        client = Client(socket=ClientSocket(connection, client_address))
                        client.machine.GREETING_handler(client.socket)
                        clients[connection] = client
                    else:
                        serv.handle_client(clients[fds])

