import pygame, sys, uuid, threading
from entity.window import Window
from entity.board import Board
from entity.button import Button
from network.server_socket import ServerSocket

from config.settings import *


def tris_window():
    tris_window = Window("Tris", WIDTH, HEIGHT, PRIMARY_COLOR)
    tris_board = Board(tris_window.get_screen())
    clock = pygame.time.Clock()

    font = tris_window.get_font(32)
    buttons = [
        Button("Abbandona", 600, 50, 250, 50, PRIMARY_COLOR, SECONDARY_COLOR, TEXT_COLOR, font)
        ]
    
    run = True
    while run:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                run = False
            if event.type == pygame.MOUSEBUTTONDOWN:
                if buttons[0].is_clicked(event):
                    run = False
                else:
                    tris_board.add_XO()
                    
 
        tris_window.update()
        tris_board.update()
        for button in buttons:
            button.draw(tris_window.get_screen())


        pygame.display.update()
        clock.tick(60)

def menu_window(server):
    menu = Window("Menu", WIDTH // 2, HEIGHT // 2, PRIMARY_COLOR)
    clock = pygame.time.Clock()

    font = menu.get_font(32)
    buttons = [
        Button("Nuova Partita", 100, 150, 250, 50, PRIMARY_COLOR, SECONDARY_COLOR, TEXT_COLOR, font),
        Button("Cerca Partita", 100, 225, 250, 50, PRIMARY_COLOR, SECONDARY_COLOR, TEXT_COLOR, font),
        Button("Esci",          100, 300, 250, 50, PRIMARY_COLOR, SECONDARY_COLOR, TEXT_COLOR, font)
    ]

    run = True
    error_msg = ''
    return_status = 'quit'
    while run:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                run = False

            if event.type == pygame.MOUSEBUTTONDOWN:
                if buttons[0].is_clicked(event):

                    # Invio richiesta per la creazione di una nuova parita
                    message = {"method": "create_game"}
                    server.send_json_message(message)
                    
                    recv_msg = server.recv_json_message()                    
                    print(recv_msg)

                    # Controllo della risposta ricevuta
                    response = recv_msg['response']
                    if response == "game_created":
                        return_status = "new_game"
                        run = False
                        
                    elif response == "error":
                        error_msg = recv_msg['description']    

                elif buttons[1].is_clicked(event):
                    return_status = "search_game"
                    run = False
                
                elif buttons[2].is_clicked(event):
                    run = False


        if run:
            menu.update()
            for button in buttons:
                button.draw(menu.get_screen())

            menu.draw_text(error_msg, 20, TEXT_ERROR_COLOR, 100, 400)

            pygame.display.update()
            clock.tick(60)
    
    return return_status


def nickname_window(server):
    window = Window("Tris", WIDTH // 2, HEIGHT // 2, PRIMARY_COLOR)
    clock = pygame.time.Clock()

    font = window.get_font(32)
    
    button_login = Button("Accedi", 100, 300, 250, 50, PRIMARY_COLOR, SECONDARY_COLOR, TEXT_COLOR, font)
    input_nickname = pygame.Rect(100, 170, 250, 50)

        
    nickname = ''
    error_msg = ''

    run = True
    while run:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                return False

            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_BACKSPACE:
                    nickname = nickname[:-1]
                elif len(nickname) <= 15:
                    nickname += event.unicode

            if event.type == pygame.MOUSEBUTTONDOWN or event.type == pygame.K_INSERT:
                if button_login.is_clicked(event):
                    if nickname == '':
                        nickname = "guest_" + str(uuid.uuid1())

                    message = {"method": "login", "username": nickname}
                    server.send_json_message(message)

                    recv_msg = server.recv_json_message()
                    print(recv_msg)

                    response = recv_msg['response']

                    if response == "error":
                        error_msg = recv_msg['description']
                    elif response == "login_success":
                        run = False
                        
        if run:
            window.update()
            window.draw_text("Inserisci nickname:", 20, TEXT_COLOR, 100, 150)
            
            pygame.draw.rect(window.get_screen(), SECONDARY_COLOR, input_nickname, 2)
            text_surface = window.get_font(18).render(nickname, True, TEXT_COLOR)
            window.get_screen().blit(text_surface, (input_nickname.x + 5, input_nickname.y + 15))

            button_login.draw(window.get_screen())

            window.draw_text(error_msg, 20, TEXT_ERROR_COLOR ,100, 400)

            pygame.display.update()
            clock.tick(60)

    return True

def join_list_window(server):
    join_window = Window("Richieste in attesa", WIDTH, HEIGHT, PRIMARY_COLOR)
    clock = pygame.time.Clock()


    last_message = {"event": None}
    listener_thread = threading.Thread(target=server.listen_to_server, args=(last_message, ), daemon=True)
    listener_thread.start()
    
    error_msg = ''

    run = True
    while run:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                return False

            if event.type == pygame.MOUSEBUTTONDOWN or event.type == pygame.K_INSERT:
                #
                pass
                        
        if run:
            join_window.update()
            
            # Titolo
            join_window.draw_text("Richieste ricevute:", 16, TEXT_COLOR, 50, 30)

            # Elenco delle richieste
            #y_offset = 70
            #for i, req in enumerate(join_requests):
            #    username = req.get("username", "Sconosciuto")
            #    join_window.draw_text(f"- {username}", 20, TEXT_COLOR, 60, y_offset)
            #    y_offset += 30  # Spaziatura verticale


            pygame.display.update()
            clock.tick(60)


def list_game_window(server):
    list_game_window = Window("Lista partite", WIDTH, HEIGHT, PRIMARY_COLOR)
    clock = pygame.time.Clock()

    list_game_requests = []

    # Recupero lista partite
    message = {"method": "list_games"}
    server.send_json_message(message)
    recv_msg = server.recv_json_message()

    if recv_msg.get("response") == "list_all_games":
        list_game_requests = recv_msg['data']


    # Creo thread per ascoltare le successive partite    
    last_message = {"event": None}
    listener_thread = threading.Thread(target=server.listen_to_server, args=(last_message, ), daemon=True)
    listener_thread.start()
    
    error_msg = ''

    run = True
    while run:
        if last_message["event"]:
            list_game_requests.append(last_message["event"])
            last_message = {"event": None}
        
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                return False

            if event.type == pygame.MOUSEBUTTONDOWN or event.type == pygame.K_INSERT:
                #
                pass
                        
        if run:
            list_game_window.update()
            
            # Titolo
            list_game_window.draw_text("Richieste ricevute:", 16, TEXT_COLOR, 50, 30)

            # Elenco delle richieste
            y_offset = 300
            for i, game in enumerate(list_game_requests):
                msg = f"Partita ID: {game['id']}, Giocatore 1: {game['player1']}, Stato: {game['state']}"
                list_game_window.draw_text(msg, 22, TEXT_COLOR, 60, y_offset)
                y_offset += 30  # Vertical spacing

               

            pygame.display.update()
            clock.tick(60)


if __name__ == "__main__":
    pygame.init()
    server = ServerSocket(SERVER, PORT, DISCONNECT_MESSAGE)
    server.connect()
    print(server.recv_json_message())
    
    if nickname_window(server):
        run = True

        while run:
            choice = menu_window(server)

            if choice == "new_game":
                join_list_window(server)
            elif choice == "search_game":
                list_game_window(server)
                pass
            elif choice == "quit":
                run = False
    
    server.close()
    pygame.quit()
    sys.exit()
    
