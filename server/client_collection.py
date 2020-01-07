import collections

class ClientsCollection(collections.UserDict):
    def sockets(self):
        return list(self.data.keys())