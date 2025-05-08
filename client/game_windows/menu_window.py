import pygame, uuid

from config.settings import * 
from config.utils import *

class MenuState:
    def __init__(self, display, game_state_manager, server):
        # Inizializzazione variabili istanza 
        self.display = display
        self.game_state_manager = game_state_manager
        self.server = server

        self.title = 'Menu'
        self.info_msg = ''
        self.error_msg = ''

        # Inizializzazione componenti ui
        self.components = {
            'button_new_game': Button(self.display, "Nuova Partita", (WIDTH //2 - 150), (HEIGHT // 2 - 300), 300, 50, PRIMARY_COLOR, SECONDARY_COLOR, TEXT_COLOR, Text.get_font('large')),
            'button_join_request': Button(self.display, "Richieste di Join", (WIDTH //2 - 150), (HEIGHT // 2 - 225), 300, 50, PRIMARY_COLOR, SECONDARY_COLOR, TEXT_COLOR, Text.get_font('large')),
            'button_search_game': Button(self.display, "Cerca Partita", (WIDTH //2 - 150), (HEIGHT // 2 - 150), 300, 50, PRIMARY_COLOR, SECONDARY_COLOR, TEXT_COLOR, Text.get_font('large')),
            'button_quit': Button(self.display, "Esci", (WIDTH //2 - 150), (HEIGHT // 2 - 75), 300, 50, PRIMARY_COLOR, SECONDARY_COLOR, TEXT_COLOR, Text.get_font('large')),
            'label_title': Text(self.display, self.title, WIDTH // 2, 50),
            'label_info': Text(self.display, '',  WIDTH // 2, HEIGHT - 200, TEXT_SUCCESS_COLOR),
            'label_error': Text(self.display, '',  WIDTH // 2, HEIGHT - 200, TEXT_ERROR_COLOR)
        }

    def cleanup(self):
        self.info_msg = ''
        self.error_msg = ''

    def run(self):
        self.display.fill(PRIMARY_COLOR)
        self.render()

    def handle_event(self, event):
         if event.type == pygame.QUIT:
            self.cleanup()
            self.game_state_manager.set_state('quit')
            return
         
         if event.type == pygame.MOUSEBUTTONDOWN:
            # Gestione click voci menu
            if self.components['button_new_game'].is_clicked(event):
                self.send_create_game_request()
            elif self.components['button_join_request'].is_clicked(event):
                self.cleanup()
                self.game_state_manager.set_state('join_request')
            elif self.components['button_search_game'].is_clicked(event):
                self.cleanup()
                self.game_state_manager.set_state('search_game')
            elif self.components['button_quit'].is_clicked(event):
                self.cleanup()
                self.game_state_manager.set_state('quit')

    def render(self):
        self.components['button_new_game'].draw()
        self.components['button_join_request'].draw()
        self.components['button_search_game'].draw()
        self.components['button_quit'].draw()

        self.components['label_title'].draw('title', center=True)
        
        if self.error_msg:
            self.components['label_error'].set_text(self.error_msg)
            self.components['label_error'].draw(center=True)

        if self.info_msg:
            self.components['label_info'].set_text(self.info_msg)
            self.components['label_info'].draw(center=True)

    def send_create_game_request(self):
        try:    
            recv_msg = self.server.send_request_and_wait({
                "type": "request",
                "request": "create_game",
            }, "create_game")

            if recv_msg is None:
                self.error_msg = "Nessuna risposta dal server"
                return
                
            print(recv_msg)
            if recv_msg.get('status') == "ok":
                self.info_msg = recv_msg.get('description')
                self.error_msg = ''
            elif recv_msg.get('status') == "error":
                self.info_msg = ''
                self.error_msg = recv_msg.get('description')
                
        except ConnectionError as e:
            self.error_msg = f"Errore di connessione: {str(e)}"
        except Exception as e:
            self.error_msg = f"Errore sconosciuto: {str(e)}"