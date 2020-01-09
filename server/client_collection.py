import collections

class ClientsCollection(collections.UserDict):
    def socket(self,fd):
        return [item for item in list(self.data) if item.fileno() == fd][0]