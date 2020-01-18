import collections

class ClientsCollection(collections.UserDict):
    def socket(self,fd):
        try:
            socket=[item for item in list(self.data) if item.fileno() == fd][0]
        except:
            pass
        return socket

    def __exit__(self, exc_type, exc_val, exc_tb):
        pass