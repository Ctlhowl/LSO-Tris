import pygame

from config.settings import * 
from config.utils import *

class JoinRequestState:
    def __init__(self, display, game_state_manager, server, list_request):
        # Inizializzazione variabili istanza 
        self.display = display
        self.game_state_manager = game_state_manager
        self.server = server

        self.title = 'Richieste'
        self.info_msg = ''
        self.error_msg = ''

        self.list_request = list_request
        self.selected_game = None
        self.game_id = -1

        # Inizializzazione componenti ui
        self.components = {
            'button_menu': Button(self.display, "Torna al Menu", (WIDTH //2 - 150), (HEIGHT - 100), 300, 50, PRIMARY_COLOR, SECONDARY_COLOR, TEXT_COLOR, Text.get_font('large')),
            'label_title': Text(self.display, self.title, WIDTH // 2, 50),
            'label_info': Text(self.display, '',  WIDTH // 2, HEIGHT - 200, TEXT_SUCCESS_COLOR),
            'label_error': Text(self.display, '',  WIDTH // 2, HEIGHT - 200, TEXT_ERROR_COLOR),
            'button_accept': Button(self.display, "Accetta", (WIDTH //2 - 350), (HEIGHT - 150), 300, 50, TEXT_SUCCESS_COLOR, PRIMARY_COLOR, TEXT_COLOR, Text.get_font('large')),
            'button_reject': Button(self.display, "Rifiuta", (WIDTH //2 + 50), (HEIGHT - 150), 300, 50, TEXT_ERROR_COLOR, PRIMARY_COLOR, TEXT_COLOR, Text.get_font('large'))
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

            # Gestione click sul pulsante Accetta
            if self.selected_game and self.components['button_accept'].is_clicked(event):
                self.send_accept_joint_request()
                return
            
            # Gestione click sul pulsante Rifiuta
            if self.selected_game and self.components['button_reject'].is_clicked(event):
                self.send_reject_joint_request()
                return

             # Gestione selezione partita
            mouse_pos = pygame.mouse.get_pos()
            self.selected_game = None

            for i, game in enumerate(self.list_request):
                game_rect = pygame.Rect(50, 100 + i*60, WIDTH-100, 50)
                
                # Imposto la partita selezionata
                if game_rect.collidepoint(mouse_pos):
                    self.info_msg = ''
                    self.selected_game = game

    def render(self):
        if self.list_request:
            for i, game in enumerate(self.list_request):
                color = HIGHLIGHT_COLOR if game == self.selected_game else TEXT_COLOR
                bg_color = SELECTED_COLOR if game == self.selected_game else SECONDARY_COLOR

                game_rect = pygame.Rect(50, 100 + i*60, WIDTH-100, 50)
                pygame.draw.rect(self.display, bg_color, game_rect, border_radius=5)
                
                msg = f"Partita ID: {game['game_id']}, Giocatore 2: {game['player2']}"
                Text(self.display, msg, 60, 120 + i*60, color).draw()

        if self.selected_game:
            self.components['button_accept'].draw()
            self.components['button_reject'].draw()

        self.components['button_menu'].draw()
        self.components['label_title'].draw('title', center=True)

        if self.error_msg:
            self.components['label_error'].set_text(self.error_msg)
            self.components['label_error'].draw(center=True)

        if self.info_msg:
            self.components['label_info'].set_text(self.info_msg)
            self.components['label_info'].draw(center=True)


    def send_accept_joint_request(self):
        try:    
            recv_msg = self.server.send_request_and_wait({
                "type": "request",
                "request": "accept_join", 
                "data": {
                    "game_id": self.selected_game['game_id'],
                    "player2": self.selected_game['player2']
                }
            }, "accept_join")
            
            if recv_msg:
                if recv_msg.get('status') == "ok":
                    self.list_request[:] = [game  for game in self.list_request if game.get("game_id") != self.selected_game['game_id']]
                    self.cleanup()
                elif recv_msg.get('status') == "error":
                    self.info_msg = ''
                    self.error_msg = recv_msg.get('description')
                
        except (ConnectionError) as e:
            print(f"Errore di connessione: {str(e)}")
    
    def send_reject_joint_request(self):
        self.list_request.remove(self.selected_game)
        self.cleanup()
        

    def get_selected_game_id(self):
        return self.game_id