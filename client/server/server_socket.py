import json, struct, socket, threading
from queue import Queue
from typing import Dict

class ServerSocket:
    def __init__(self, ip, port, disconnected_message):
        # Inizializzazione variabili istanza 
        self.header = 64
        self.format = 'utf-8'
        self.disconnected_message = disconnected_message
        self.running = False

        self.port = port
        self.ip = ip
        self.add = (ip, port)
        self.server_sock = None


        # Strutture dati
        self.broadcast_queue = Queue()
        self.broadcast_listeners = []

        self.request_queue = Queue()
        self.request_listeners = []

        self.pending_responses: Dict[str, Queue] = {}
       
        # Thread di ricezione
        self.thread = threading.Thread(target=self._thread_recv, daemon=True)

    def connect(self):
        self.server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_sock.connect(self.add)
        self.running = True
        self.thread.start()

    def close(self):
        if self.server_sock:
            self.running = False  # Imposta il flag a False per fermare il thread
            try:
                self.send_json_message({"request": self.disconnected_message})
            except:
                pass  # Ignora errori di invio durante la chiusura

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

    def _thread_recv(self):
        while self.running:
            try:
                msg = self.recv_json_message()
                if not msg:
                    if not self.running:
                        break
                    continue

                type = msg.get("type")
                
                print(f"Messaggio ricevuto: {msg}")

                if type == "response":
                    res = msg.get("response")
                    
                    if res in self.pending_responses:
                        self.pending_responses[res].put(msg)
                    else:
                        q = Queue()
                        q.put(msg)
                        self.pending_responses[res] = q

                elif type == "request":
                    self.request_queue.put(msg)
                    for callback in self.request_listeners:
                        callback(msg)
                
                elif type == "broadcast":
                    self.broadcast_queue.put(msg)
                    for callback in self.broadcast_listeners:
                        callback(msg)
                else:
                    print(f"Messaggio sconosciuto ricevuto: {msg}")

            except Exception as e:
                if self.running:
                    print(f"Errore nel thread di ricezione: {e}")
                break


    def send_request_and_wait(self, request_json: dict, request: str, timeout=5):
        if request in self.pending_responses:
            print(f"Errore: una risposta per '{request}' è già in attesa.")
            return None

        q = Queue()
        self.pending_responses[request] = q
        
        if not self.send_json_message(request_json):
            del self.pending_responses[request]
            return None

        try:
            response = q.get(timeout=timeout)
            return response
        except Exception as e:
            print(f"Timeout o errore durante l'attesa della risposta: {e}")
            return None
        finally:
            self.pending_responses.pop(request, None)
    
    def add_broadcast_listener(self, callback):
        self.broadcast_listeners.append(callback)

    def add_request_listener(self, callback):
        self.request_listeners.append(callback)
