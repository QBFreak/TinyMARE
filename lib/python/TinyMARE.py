class object:
    def __init__(self, num):
        self.setNum(num)

    def __str__(self):
        return self.dbref

    def setNum(self, num):
        self.num = int(num)
        self.dbref = "#" + str(self.num)
