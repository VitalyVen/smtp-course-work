from mail_server import MailServer

if __name__ == '__main__':
    with MailServer() as server:
        server.serve()