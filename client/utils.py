import socket
import threading
import select
import os
import dns.resolver
from client import Client
from common.mail import Mail

from connections_collection import ConnectionsCollection

class ClientHelper(object):
    def __init__(self, host='localhost', port=25, threads=5):
        self.host = host
        self.port = port
        self.threads_cnt = threads
        self.connections = ConnectionsCollection()

    def __enter__(self):
        self.socket_init(self.host, self.port)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        #self.sock.close()
        pass

    def socket_init(self, host, port=25):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.settimeout(60)
        server_address = (str(host), port)
        sock.connect(server_address)

        #b = self.sock.sendall(f'ehlo {host}\r\n'.encode()) #?

        return socket


    # def serve(self, blocking=True):
    #     for i in range(self.threads_cnt):
    #         thread_sock = threading.Thread(target=thread_socket, args=(self,))
    #         thread_sock.daemon = True
    #         thread_sock.name = 'Working Thread {}'.format(i)
    #         thread_sock.start()
    #     while blocking:
    #         pass
    def get_mx(self, domain):
        try:
            answers = dns.resolver.query(domain, 'MX')
            mx_list = []
            for rdata in answers:
                mx_list.append(rdata.exchange)
        except Exception:
            mx_list = []
            mx_list.append('-1')
            return mx_list

        return mx_list

    # returns a list of files which are not in process yet
    def maildir_handler(self):
        files_in_process = set()
        folder = os.walk('../pst/maildir/')
        cur_files = set()
        for address, dirs, files in folder:
            for file in files:
                # if file not in files_in_process:
                    files_in_process.add(address+file)
                    # m = Mail(to=[])
                    # mail = m.from_file(address+file)
                    # mx = self.get_mx(mail.domain)[0]
                    # if mx == '-1':
                    #     print(mail.domain + ' error')
                    #     continue
                    # socket = self.socket_init(mx)
                    # # self.connections[socket] = Client(socket,'.', m)

        return files_in_process

def thread_socket(cl:ClientHelper):
    while True:
        connection_sockets = cl.connections.sockets()
        rfds, wfds, errfds = select.select([cl.sock], cl.sock, [], 5)

        #new connect if thread does not exist for this domain

        # for fds in rfds:
        #     if fds is cl.sock:
        #         connection, client_address = fds.accept()
        #         client = Client(socket=ClientSocket(connection, client_address))
        #         cl.clients[connection] = client
        #     else:
        #         cl.handle_client_read(cl.clients[fds])
        # for fds in wfds:
        #     cl.handle_client_write(cl.clients[fds])

if __name__ == '__main__':
    c = ClientHelper()
    #files_in_process = {'fd994392-4e03-4b60-9d99-447481940148=@',}
    files_in_process = set()
    files_in_process = c.maildir_handler(files_in_process)
    # print(files_in_process)
    # thread_socket(c)

