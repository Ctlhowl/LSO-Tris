import pygame, uuid

from config.settings import * 
from config.utils import *

class LoginState:
    def __init__(self, display, game_state_manager, server):

        # Inizializzazione variabili istanza 
        self.display = display
        self.game_state_manager = game_state_manager
        self.server = server

        self.title = 'Login'
        self.current_player = ''
        self.error_msg = ''

        # Inizializzazione componenti ui
        self.components = {
            'input_username': pygame.Rect((WIDTH //2 - 125), (HEIGHT // 2 - 250), 250, 50),
            'button_login': Button(self.display, "Accedi", (WIDTH //2 - 125) , (HEIGHT  // 2 - 100), 250, 50, PRIMARY_COLOR, SECONDARY_COLOR, TEXT_COLOR, Text.get_font('large')),
            'label_username': Text(self.display, "Inserisci username: ", (WIDTH // 2 - 125), 150),
            'label_title': Text(self.display, self.title, WIDTH // 2, 50),
            'label_error': Text(self.display, '',  WIDTH // 2, HEIGHT - 200, TEXT_ERROR_COLOR)
        }

    def cleanup(self):
        self.current_player = ''
        self.error_msg = ''

    def run(self):
        self.display.fill(PRIMARY_COLOR)
        self.render()

    def handle_event(self, event):
        # Gestisce inserimento username
        if event.type == pygame.KEYDOWN:
            if event.key == pygame.K_BACKSPACE:
                self.current_player = self.current_player[:-1]
            elif len(self.current_player) <= 15 and event.unicode.isalnum():
                self.current_player += event.unicode

        # Gestione click bottone login
        if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            if self.components['button_login'].is_clicked(event):
                self.send_auth_request()
                return

    def render(self):        
        pygame.draw.rect(self.display,  SECONDARY_COLOR, self.components['input_username'], 2)
        text_surface = Text.get_font('medium').render(self.current_player, True, TEXT_COLOR)

        self.display.blit(text_surface, (self.components['input_username'].x + 5, self.components['input_username'].y + 15))
        
        self.components['label_username'].draw()
        self.components['label_title'].draw('title', center=True)
        self.components['button_login'].draw()

        if self.error_msg:
            self.components['label_error'].set_text(self.error_msg)
            self.components['label_error'].draw(center=True)

    def send_auth_request(self):
        try:
            if self.current_player == '':
                self.current_player = "guest_" + str(uuid.uuid4().hex[:8])

            recv_msg = self.server.send_request_and_wait({
                "type": "request",
                "request": "login",
                "data": {
                    "username": self.current_player
                }
            }, "login")

            if recv_msg is None:
                self.error_msg = "Nessuna risposta dal server"
                return
        
            if recv_msg:
                if recv_msg.get('status') == "ok":
                    self.cleanup()
                    self.game_state_manager.set_state('menu')
                elif recv_msg.get('status') == "error":
                    self.error_msg = recv_msg.get('description')
                
        except ConnectionError as e:
            self.error_msg = f"Errore di connessione: {str(e)}"
        except Exception as e:
            self.error_msg = f"Errore sconosciuto: {str(e)}"