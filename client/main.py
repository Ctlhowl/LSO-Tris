import pygame, sys, uuid
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

    menu_window()

def menu_window():
    menu = Window("Menu", WIDTH // 2, HEIGHT // 2, PRIMARY_COLOR)
    clock = pygame.time.Clock()

    font = menu.get_font(32)
    buttons = [
        Button("Nuova Partita", 100, 150, 250, 50, PRIMARY_COLOR, SECONDARY_COLOR, TEXT_COLOR, font),
        Button("Cerca Partita", 100, 225, 250, 50, PRIMARY_COLOR, SECONDARY_COLOR, TEXT_COLOR, font),
        Button("Esci",          100, 300, 250, 50, PRIMARY_COLOR, SECONDARY_COLOR, TEXT_COLOR, font)
    ]

    run = True
    while run:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                run = False

            if event.type == pygame.MOUSEBUTTONDOWN:
                if buttons[0].is_clicked(event):
                    tris_window()
                elif buttons[1].is_clicked(event):
                    print("Funzione non implementata")
                elif buttons[2].is_clicked(event):
                    run = False
                    

        menu.update()
        for button in buttons:
            button.draw(menu.get_screen())

        pygame.display.update()
        clock.tick(60)


def nickname_window():
    window = Window("Tris", WIDTH // 2, HEIGHT // 2, PRIMARY_COLOR)
    clock = pygame.time.Clock()

    font = window.get_font(32)
    
    button_login = Button("Accedi", 100, 300, 250, 50, PRIMARY_COLOR, SECONDARY_COLOR, TEXT_COLOR, font)
    input_nickname = pygame.Rect(100, 170, 250, 50)

        
    nickname = ''
    run = True
    while run:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                return None

            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_BACKSPACE:
                    nickname = nickname[:-1]
                elif len(nickname) <= 15:
                    nickname += event.unicode

            if event.type == pygame.MOUSEBUTTONDOWN or event.type == pygame.K_INSERT:
                if button_login.is_clicked(event):
                    run = False
                
        
        window.update()

        window.draw_text("Inserisci nickname:", 20, TEXT_COLOR, 100, 150)
        pygame.draw.rect(window.get_screen(), SECONDARY_COLOR, input_nickname, 2)
        text_surface = window.get_font(18).render(nickname, True, TEXT_COLOR)
        window.get_screen().blit(text_surface, (input_nickname.x + 5, input_nickname.y + 15))


        button_login.draw(window.get_screen())

        pygame.display.update()
        clock.tick(60)

    if nickname == '':
        nickname = "guest_" + uuid.uuid1()
    
    return nickname


if __name__ == "__main__":
    pygame.init()

    nickname = nickname_window()
    if  nickname != None:
        server = ServerSocket(SERVER, PORT, DISCONNECT_MESSAGE)
        server.connect()
        server.send_msg(nickname)

        menu_window()
        
        server.close()
    

    pygame.quit()
    sys.exit()
    
