import socket

class ServerSocket:
    def __init__(self, server, port, disconnected_message):
        self.header = 64
        self.format = 'utf-8'
        self.disconnected_message = disconnected_message

        self.port = port
        self.server = server
        self.add = (server, port)

    def connect(self):
        self.client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client.connect(self.add)

    def close(self):
        self.send_msg(self.disconnected_message)
        self.client.close()

    def send_msg(self, msg):
        message = msg.encode(self.format)
        msg_length = len(message)

        send_length = str(msg_length).encode(self.format)
        send_length += b' ' * (self.header - len(send_length))

        self.client.send(send_length)
        self.client.send(message)

   
