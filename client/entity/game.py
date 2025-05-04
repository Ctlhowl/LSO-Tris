import pygame

from config.settings import * 
from game_windows.login_window import LoginState
from game_windows.menu_window import MenuState
from game_windows.join_request_window import JoinRequestState
from game_windows.search_game import SearchGameState

class Game:
    def __init__(self, server):
        pygame.display.set_caption('Tris')

        self.screen = pygame.display.set_mode((WIDTH, HEIGHT))
        self.clock = pygame.time.Clock()

        # Inizializzazione variabili istanza
        self.server = server
        self.running = False

        # Inizializzazione stati gioco
        self.game_state_manager = GameStateManager('login')

        self.login = LoginState(self.screen, self.game_state_manager, self.server)
        self.menu = MenuState(self.screen, self.game_state_manager, self.server)
        self.join_request = JoinRequestState(self.screen, self.game_state_manager, self.server)
        self.search_game = SearchGameState(self.screen, self.game_state_manager, self.server)
        self.quit = QuitState(self.screen, self.game_state_manager)
    
        self.states = {
            'login': self.login, 
            'menu': self.menu, 
            'join_request': self.join_request,
            'search_game': self.search_game,
            'quit': self.quit
            }

    def run(self):
        self.running = True
        while self.running and not self.game_state_manager.quitting():
            # Gestione eventi
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.game_state_manager.set_state('quit')

                current_state = self.states[self.game_state_manager.get_state()].handle_event(event)
                
            # Aggiornamento e rendering
            current_state = self.states[self.game_state_manager.get_state()]

            if self.game_state_manager.get_state() == 'search_game':
                if hasattr(current_state, 'send_list_game_request'):
                    current_state.send_list_game_request()

            current_state.run()
            
            pygame.display.update()
            self.clock.tick(FPS)


class GameStateManager:
    def __init__(self, current_state):
        self.current_state = current_state
        self.should_quit = False
    
    def get_state(self):
        return self.current_state
    
    def set_state(self, state):
        if state == 'quit':
            self.should_quit = True
        else:
            self.current_state = state

    def quitting(self):
        return self.should_quit

class QuitState:
    def __init__(self, display, game_state_manager):
        self.display = display
        self.game_state_manager = game_state_manager
    
    def run(self):
        pass

    def handle_event(self, event):
        pass