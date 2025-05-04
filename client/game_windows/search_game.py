import pygame, uuid

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

        # Inizializzazione componenti ui
        self.components = {
            'button_menu': Button(self.display, "Torna al Menu", (WIDTH //2 - 150), (HEIGHT - 100), 300, 50, PRIMARY_COLOR, SECONDARY_COLOR, TEXT_COLOR, Text.get_font('large')),
            'label_title': Text(self.display, self.title, WIDTH // 2, 50),
            'label_info': Text(self.display, '',  WIDTH // 2, HEIGHT - 200, TEXT_SUCCESS_COLOR),
            'label_error': Text(self.display, '',  WIDTH // 2, HEIGHT - 200, TEXT_ERROR_COLOR)
        }

    def cleanup(self):
        self.info_msg = ''
        self.error_msg = ''
        self.list_games = []
        self.selected_game = None

    def run(self):
        self.display.fill(PRIMARY_COLOR)
        self.render()

    def handle_event(self, event):
         if event.type == pygame.MOUSEBUTTONDOWN:

            # Gestione selezione partita
            mouse_pos = pygame.mouse.get_pos()
            for i, game in enumerate(self.list_games):
                game_rect = pygame.Rect(50, 100 + i*60, WIDTH-100, 50)
                
                # Invio richiesta al server per il join di una determinata partita
                if game_rect.collidepoint(mouse_pos):
                    self.selected_game = game        
                    self.send_join_request()
    
            # Ritorno al menu
            if self.components['button_menu'].is_clicked(event):
                self.cleanup()
                self.game_state_manager.set_state('menu')

    def render(self):
        for i, game in enumerate(self.list_games):
            color = HIGHLIGHT_COLOR if game == self.selected_game else TEXT_COLOR
            bg_color = SELECTED_COLOR if game == self.selected_game else SECONDARY_COLOR

            game_rect = pygame.Rect(50, 100 + i*60, WIDTH-100, 50)
            pygame.draw.rect(self.display, bg_color, game_rect, border_radius=5)
            
            msg = f"Partita ID: {game['id']}, Giocatore 1: {game['player1']}, Stato: {game['state']}"
            Text(self.display, msg, 60, 120 + i*60, color).draw()

        if self.selected_game:
            join_rect = pygame.Rect(WIDTH//2 - 60, HEIGHT - 100, 120, 40)
            pygame.draw.rect(self.display, (50, 200, 50), join_rect, border_radius=5)
            Text(self.display ,"Join", WIDTH//2, HEIGHT-80, (255, 255, 255)).draw()


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
                self.server.send_json_message({
                    "method": "list_games"
                })

                print("Invio richiesta per lista partite")
                recv_msg = self.server.recv_json_message()
                print(recv_msg)

                if recv_msg.get('response') == "list_all_games":
                    self.info_msg = recv_msg.get('description')
                    self.list_games = recv_msg.get('data')            
                    self.error_msg = ''

                elif recv_msg.get('response') == "error":
                    self.info_msg = ''
                    self.error_msg = recv_msg.get('description')
        except (ConnectionError) as e:
            print(f"Errore di connessione: {str(e)}")

    def send_join_request(self):
        try:    
            self.server.send_json_message({
                "method": "join_request", 
                "game_id": self.selected_game['id']
            })
            
            recv_msg = self.server.recv_json_message()
            
            if recv_msg.get('response') == "join_requested":
                self.info_msg = recv_msg.get('description')
                self.error_msg = ''

            elif recv_msg.get('response') == "error":
                self.info_msg = ''
                self.error_msg = recv_msg.get('description')
        except (ConnectionError) as e:
            print(f"Errore di connessione: {str(e)}")