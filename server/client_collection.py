import collections

class ClientsCollection(collections.UserDict):
    def sockets(self):
        return list(self.data.keys())
    def __exit__(self, exc_type, exc_val, exc_tb):
        pass