from pyart import Tree

class ARTWrapper:
    def __init__(self):
        self.tree = Tree()
        self.origin_method = False

    def insert(self, key, value, origin_method=False):
        self.tree.replace(key.encode(), value.encode(), origin_method)
        # self.tree.update({key.encode(): value.encode()})

    def query(self, key, origin):
        # try:
        self.origin_method = origin
        self.tree.get(key.encode(), self.origin_method)
        # except KeyError:
            # return None

    def delete(self, key):
        self.tree.pop(key.encode())

    def update(self, key, value):
        # update() can modify multiple data，replace() can only update one pair
        self.tree.update({key.encode(): value.encode()})
        # self.tree.replace(key.encode(), value.encode())

    def get_latency_energy(self):
        self.tree.get_latency_energy()

    def get_space(self):
        self.tree.get_space()

    def iterTree(self, callback):
        self.tree.each(callback)