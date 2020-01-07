from __init__ import *

if __name__ == '__main__':
    with MailServer() as server:
        server.serve_forever()