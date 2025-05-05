import pygame
from config .settings import *
from config.utils import *


class TrisState:
    def __init__(self, display, game_state_manager, server):
        # Inizializzazione variabili istanza 
        self.display = display
        self.game_state_manager = game_state_manager
        self.server = server

        self.title = 'Tris'
        self.info_msg = ''
        self.error_msg = ''

        # Inizializzazione gioco
        self.game_id = -1
        self.board = [[1, 2, 3], [4, 5, 6], [7, 8, 9]]
        self.state = ''
        self.winner = None

        self.to_move = 'X'

        # Inizializzazione componenti ui
        self.components = {
            'button_menu': Button(self.display, "Torna al Menu", (WIDTH //2 - 150), (HEIGHT - 100), 300, 50, PRIMARY_COLOR, SECONDARY_COLOR, TEXT_COLOR, Text.get_font('large')),
            'label_title': Text(self.display, self.title, WIDTH // 2, 50),
            'label_info': Text(self.display, '',  WIDTH // 2, HEIGHT - 170, TEXT_SUCCESS_COLOR),
            'label_error': Text(self.display, '',  WIDTH // 2, HEIGHT - 170, TEXT_ERROR_COLOR),
        }

    def cleanup(self):
        self.info_msg = ''
        self.error_msg = ''
        self.board = [[1, 2, 3], [4, 5, 6], [7, 8, 9]]

    def run(self):
        self.display.fill(PRIMARY_COLOR)
        self.render()

    def handle_event(self, event):
        if event.type == pygame.MOUSEBUTTONDOWN:
            # Ritorno al menu
            if self.components['button_menu'].is_clicked(event):
                self.cleanup()
                self.game_state_manager.set_state('menu')
                return

            current_pos = pygame.mouse.get_pos()
            cell = self.get_cell(*current_pos)

            if cell is not None:
                row, col = cell
                if self.board[row][col] not in ('X', 'O'):
                    self.send_move(row, col)

        self.check_winner() 
    
    def render(self):
        self.components['button_menu'].draw()
        self.components['label_title'].draw('title', center=True)

        if self.error_msg:
            self.components['label_error'].set_text(self.error_msg)
            self.components['label_error'].draw(center=True)

        if self.info_msg:
            self.components['label_info'].set_text(self.info_msg)
            self.components['label_info'].draw(center=True)
        
        self.draw_board()

        for i in range(3):
            for j in range(3):
                center_x = 255 + (j * 150) + (150 // 2)
                center_y = 255 + (i * 150) + (150 // 2)

                if self.board[i][j] == 'X':
                    self.draw_X(center_x, center_y)
                elif self.board[i][j] == 'O':
                    self.draw_O(center_x, center_y)
    
    def send_move(self, x, y):
        try:    
            recv_msg = self.server.send_request_and_wait({
                "type": "request",
                "request": "game_move", 
                "data": {
                    "game_id": self.game_id,
                    "x": x,
                    "y": y,
                }
            }, "game_move")
            
            if recv_msg:
                if recv_msg.get('status') == "ok":
                    self.error_msg = ''
                    self.set_game_data(recv_msg.get('data'))

                elif recv_msg.get('status') == "error":
                    self.info_msg = ''
                    self.error_msg = recv_msg.get('description')
                
        except (ConnectionError) as e:
            print(f"Errore di connessione: {str(e)}")

    def get_cell(self, mouse_x, mouse_y):
        x_rel = mouse_x - 255
        y_rel = mouse_y - 255

        if 0 <= x_rel < 450 and 0 <= y_rel < 450:
            col = x_rel // 150
            row = y_rel // 150
            return int(row), int(col)
        return None

    def draw_X(self, x, y):
        offset = 40
        start_pos1 = (x - offset, y - offset)
        end_pos1 = (x + offset, y + offset)

        start_pos2 = (x + offset, y - offset)
        end_pos2 = (x - offset, y + offset)

        pygame.draw.line(self.display, X_COLOR, start_pos1, end_pos1, 5)
        pygame.draw.line(self.display, X_COLOR, start_pos2, end_pos2, 5)

    def draw_O(self, x, y):
        radius = 40
        pygame.draw.circle(self.display, O_COLOR, (x, y), radius, 5)

    def draw_board(self):
        width = 5

        for i in range(1, 3):
            x = 255 + i * 150
            pygame.draw.line(self.display, BOARD_COLOR, (x, 255), (x, 255 + 3 * 150), width)

        for i in range(1, 3):
            y = 255 + i * 150
            pygame.draw.line(self.display, BOARD_COLOR, (255, y), (255 + 3 * 150, y), width)

    def set_game_id(self, id):
        self.game_id = id

    def set_board(self, board):
        self.board = board

    def set_game_data(self, data):
        self.game_id = data.get('game_id')
        self.board = data.get('board')
        self.state = data.get('state')
        self.winner = data.get('winner')
        
    
    def check_winner(self):
        if self.state == "GAME_OVER" and self.winner != None:
            self.info_msg = f"Il vincitore è {self.winner}"
        elif self.state == "GAME_OVER":
            self.info_msg = "Pareggio"