import collections

class ConnectionsCollection(collections.UserDict):
    def sockets(self):
        return list(self.data.keys())