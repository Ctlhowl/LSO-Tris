import pygame, uuid, json

from config.settings import * 
from config.utils import *

class SearchGameState:
    def __init__(self, display, game_state_manager, server):
        # Inizializzazione variabili istanza 
        self.display = display
        self.game_state_manager = game_state_manager
        self.server = server

        self.title = 'Ricerca Partite'
        self.info_msg = ''
        self.error_msg = ''

        self.list_games = []
        self.selected_game = None

        # Imposta listener per i messaggi in broadcast
        self.server.add_broadcast_listener(self.broadcast_handler)

        # Inizializzazione componenti ui
        self.components = {
            'button_menu': Button(self.display, "Torna al Menu", (WIDTH //2 - 150), (HEIGHT - 100), 300, 50, PRIMARY_COLOR, SECONDARY_COLOR, TEXT_COLOR, Text.get_font('large')),
            'label_title': Text(self.display, self.title, WIDTH // 2, 50),
            'label_info': Text(self.display, '',  WIDTH // 2, HEIGHT - 200, TEXT_SUCCESS_COLOR),
            'label_error': Text(self.display, '',  WIDTH // 2, HEIGHT - 200, TEXT_ERROR_COLOR),
            'button_join': Button(self.display, "Invia Richiesta", (WIDTH //2 - 150), (HEIGHT - 150), 300, 50, TEXT_SUCCESS_COLOR, PRIMARY_COLOR, TEXT_COLOR, Text.get_font('large'))
        }

    def cleanup(self):
        self.info_msg = ''
        self.error_msg = ''
        self.selected_game = None

    def run(self):
        self.display.fill(PRIMARY_COLOR)
        self.render()

    def handle_event(self, event):
         if event.type == pygame.QUIT:
            self.cleanup()
            self.game_state_manager.set_state('quit')
            return
         
         if event.type == pygame.MOUSEBUTTONDOWN:
            # Ritorno al menu
            if self.components['button_menu'].is_clicked(event):
                self.cleanup()
                self.game_state_manager.set_state('menu')
                return

            # Gestione click sul pulsante Join
            if self.selected_game and self.components['button_join'].is_clicked(event):
                self.send_join_request()
                self.selected_game = None
                return
                    
            # Gestione selezione partita
            mouse_pos = pygame.mouse.get_pos()
            self.selected_game = None

            for i, game in enumerate(self.list_games):
                game_rect = pygame.Rect(50, 100 + i*60, WIDTH-100, 50)
                
                # Imposto la partita selezionata
                if game_rect.collidepoint(mouse_pos):
                    self.info_msg = ''
                    self.selected_game = game   
            

    def render(self):
        if self.list_games:
            for i, game in enumerate(self.list_games):
                color = HIGHLIGHT_COLOR if game == self.selected_game else TEXT_COLOR
                bg_color = SELECTED_COLOR if game == self.selected_game else SECONDARY_COLOR

                game_rect = pygame.Rect(50, 100 + i*60, WIDTH-100, 50)
                pygame.draw.rect(self.display, bg_color, game_rect, border_radius=5)
                
                msg = f"Partita ID: {game['game_id']}, Giocatore 1: {game['player1']}, Stato: {game['state']}"
                Text(self.display, msg, 60, 120 + i*60, color).draw()

        if self.selected_game:
            self.components['button_join'].draw()

        self.components['button_menu'].draw()
        self.components['label_title'].draw('title', center=True)

        if self.error_msg:
            self.components['label_error'].set_text(self.error_msg)
            self.components['label_error'].draw(center=True)

        if self.info_msg:
            self.components['label_info'].set_text(self.info_msg)
            self.components['label_info'].draw(center=True)


    
    def send_list_game_request(self):
        try:
            if self.list_games == []:
                recv_msg = self.server.send_request_and_wait({
                    "type": "request",
                    "request": "list_games",
                }, "list_games")

                if recv_msg.get('status') == "ok":
                    data = recv_msg.get('data')
                                        
                    if isinstance(data, dict):  # Se è un singolo dizionario
                        self.list_games.append(data)
                    elif isinstance(data, list):  # Se è una lista di dizionari
                        for game in data:
                            if isinstance(game, dict):
                                self.list_games.append(game)

                elif recv_msg.get('status') == "error":
                    self.info_msg = ''
                    self.error_msg = recv_msg.get('description')
        except (ConnectionError) as e:
            print(f"Errore di connessione: {str(e)}")

    def send_join_request(self):
        try:    
            recv_msg = self.server.send_request_and_wait({
                "type": "request",
                "request": "join_request", 
                "data": {
                    "game_id": self.selected_game['game_id']
                }
            }, "join_request")
            
            if recv_msg:
                if recv_msg.get('status') == "ok":
                    self.info_msg = recv_msg.get('description')
                    self.error_msg = ''

                elif recv_msg.get('status') == "error":
                    self.info_msg = ''
                    self.error_msg = recv_msg.get('description')
        except (ConnectionError) as e:
            print(f"Errore di connessione: {str(e)}")
    
    def broadcast_handler(self, msg):
        print("Ricevuto broadcast:", msg)
        if msg.get('event') == "new_game_available":
            self.list_games.append(msg.get("data"))

        if msg.get('event') == "game_not_available" or msg.get('event') == "game_ended":
            game_id_to_remove = msg.get("data").get("game_id") 
            self.list_games = list(filter(lambda game: game.get("game_id") != game_id_to_remove, self.list_games))
            self.list_games.append(msg.get("data"))

        if msg.get('event') == "game_removed":
            game_id_to_remove = msg.get("data").get("game_id") 
            self.list_games = list(filter(lambda game: game.get("game_id") != game_id_to_remove, self.list_games))