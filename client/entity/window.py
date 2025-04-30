import pygame
from config.settings import * 

class Window:
    def __init__(self, title, width, height, bg_color):
        pygame.display.set_caption(title)
        self.screen = pygame.display.set_mode((width, height))
        
        self.title = title 
        self.bg_color = bg_color
        self.screen.fill(bg_color)

    def get_font(self, size):
        font = pygame.font.Font(FONT_PATH, size)
        return font
    
    def get_screen(self):
        return self.screen

    def update(self):
        self.screen.fill(self.bg_color)
        self.draw_text(self.title, 50, TEXT_COLOR, 50, 50)

    def draw_text(self, text, size, color, x, y):    
        text_obj = self.get_font(size).render(text, True, color)
        text_rect = text_obj.get_rect()
        text_rect.topleft = (x,y)

        self.screen.blit(text_obj, text_rect)