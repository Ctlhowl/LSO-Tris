import pygame
from config.settings import *

if __name__ == "__main__":
     # 1. Inizializza Pygame
    pygame.init()
    
    # 2. Crea la connessione di rete
    from server.server_socket import ServerSocket
    server = ServerSocket(SERVER, PORT, DISCONNECT_MESSAGE)

    try:
        server.connect()
        
        # 3. Avvia il gioco solo se la connessione ha successo
        from entity.game import Game
        game = Game(server)
        game.run()
        
    except ConnectionError as e:
        print(f"Errore di connessione: {e}")
    finally:
        server.close()
        pygame.quit()
