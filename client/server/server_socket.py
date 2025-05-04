import json, struct, socket, threading
from queue import Queue

class ServerSocket:
    def __init__(self, ip, port, disconnected_message):
        # Inizializzazione variabili istanza 
        self.header = 64
        self.format = 'utf-8'
        self.disconnected_message = disconnected_message

        self.port = port
        self.ip = ip
        self.add = (ip, port)
        self.server_sock = None


        # Inizializzazione thread e strutture dati per la ricezione dei messaggi 
        #self.thread = threading.Thread(target=self._thread_recv, daemon=True)
        #self.broadcast = Queue()

    def connect(self):
        self.server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_sock.connect(self.add)
        self.thread.start()

    def close(self):
        if self.server_sock:
            self.send_json_message({"request": self.disconnected_message})
            self.server_sock.close()
            self.server_sock = None

    def send_json_message(self, msg):
        try:
            json_str = json.dumps(msg, separators=(',', ':'))
            encoded = json_str.encode('utf-8')
            length = struct.pack('!I', len(encoded))  # 4-byte unsigned int in network byte order
            self.server_sock.sendall(length)
            self.server_sock.sendall(encoded)
            return True
        except Exception as e:
            print(f"Send error: {e}")
            return False

    def recv_json_message(self):
        try:
            # Riceve 4 byte della lunghezza
            raw_len = self.recv_all(4)
            if not raw_len:
                return None
            length = struct.unpack('!I', raw_len)[0]
            
            # Riceve il messaggio JSON
            raw_msg = self.recv_all(length)
            if not raw_msg:
                return None
            
            return json.loads(raw_msg.decode('utf-8'))
        except Exception as e:
            print(f"Receive error: {e}")
            return None
            
    def recv_all(self, n):
        # Riceve esattamente n byte dal socket
        data = bytearray()
        while len(data) < n:
            packet = self.server_sock.recv(n - len(data))
            if not packet:
                return None
            data.extend(packet)
        return data