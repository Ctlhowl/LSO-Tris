import pygame

from config.settings import * 
from game_windows.login_window import LoginState
from game_windows.menu_window import MenuState
from game_windows.join_request_window import JoinRequestState
from game_windows.search_game import SearchGameState
from game_windows.tris_window import TrisState

class Game:
    def __init__(self, server):
        pygame.display.set_caption('Tris')

        self.screen = pygame.display.set_mode((WIDTH, HEIGHT))
        self.clock = pygame.time.Clock()

        # Inizializzazione variabili istanza
        self.server = server
        self.running = False

        # Imposta listener per richieste
        self.server.add_request_listener(self.request_handler)
        self.list_join_request = []

        # Inizializzazione stati gioco
        self.game_state_manager = GameStateManager('login')

        self.login = LoginState(self.screen, self.game_state_manager, self.server)
        self.menu = MenuState(self.screen, self.game_state_manager, self.server)
        self.join_request = JoinRequestState(self.screen, self.game_state_manager, self.server, self.list_join_request)
        self.search_game = SearchGameState(self.screen, self.game_state_manager, self.server)
        self.tris = TrisState(self.screen, self.game_state_manager, server)
        self.quit = QuitState(self.screen, self.game_state_manager)

        self.states = {
            'login': self.login, 
            'menu': self.menu, 
            'join_request': self.join_request,
            'search_game': self.search_game,
            'tris': self.tris,
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

            #if self.game_state_manager.get_state() == 'tris' and self.game_state_manager.get_prev_state() == 'join_request':
            #    game_id = self.states['join_request'].get_selected_game_id()
            #    current_state.set_game_id(game_id)

            current_state.run()
            
            pygame.display.update()
            self.clock.tick(FPS)
    
    def request_handler(self, msg):
        print("Ricevuta richiesta:", msg)
        if msg.get('request') == "join_request":
            self.list_join_request.append(msg.get("data"))
        if msg.get('request') == "game_update":
            self.states['tris'].set_game_data(msg.get('data'))
        if msg.get('request') == "game_started":
            self.game_state_manager.set_state("tris")
            self.states['tris'].set_game_data(msg.get('data'))
            


class GameStateManager:
    def __init__(self, current_state):
        self.prev_state = None
        self.current_state = current_state
        self.should_quit = False
    
    def get_state(self):
        return self.current_state
    
    def get_prev_state(self):
        return self.prev_state
    
    def set_state(self, state):
        if state == 'quit':
            self.should_quit = True
        else:
            self.prev_state = self.current_state
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