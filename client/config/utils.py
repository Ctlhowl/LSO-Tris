import pygame
from config.settings import * 

class Button:
    def __init__(self, screen, text, x, y, width, height, base_color, hover_color, text_color, font):
        self.text = text
        self.screen = screen

        self.rect = pygame.Rect(x, y, width, height)
        
        self.base_color = base_color
        self.hover_color = hover_color
        
        self.text_color = text_color
        self.font = font

    def draw(self):
        mouse_pos = pygame.mouse.get_pos()

        is_hovered = self.rect.collidepoint(mouse_pos)
        text_color = self.hover_color if is_hovered else self.text_color

        pygame.draw.rect(self.screen, self.base_color, self.rect, border_radius=8)

        text_surf = self.font.render(self.text, True, text_color)
        text_rect = text_surf.get_rect(center=self.rect.center)
        self.screen.blit(text_surf, text_rect)

    def is_clicked(self, event):
        return event.type == pygame.MOUSEBUTTONDOWN and self.rect.collidepoint(pygame.mouse.get_pos())


class Text:
    def __init__(self, screen, text, x, y, color=TEXT_COLOR):
        self.screen = screen
        
        self.text = text
        
        self.color = color

        self.x = x
        self.y = y

        self.font = {
            'small': pygame.font.Font(FONT_PATH, 22),
            'medium': pygame.font.Font(FONT_PATH, 26),
            'large': pygame.font.Font(FONT_PATH, 36),
            'title': pygame.font.Font(FONT_PATH, 54)
            }

    
    def draw(self, font_size='medium', center=False):
        font = self.font.get(font_size, self.font['medium'])
        
        text_surface = font.render(self.text, True, self.color)
        text_rect = text_surface.get_rect()
        
        if center:
            text_rect.center = (self.x, self.y)
        else:
            text_rect.topleft = (self.x, self.y)
            
        self.screen.blit(text_surface, text_rect)

    def set_text(self, text):
        self.text = text

    @staticmethod
    def get_font(font_size):
        fonts = {
        'small': pygame.font.Font(FONT_PATH, 18),
        'medium': pygame.font.Font(FONT_PATH, 22),
        'large': pygame.font.Font(FONT_PATH, 32),
        'title': pygame.font.Font(FONT_PATH, 50)
        }

        return fonts.get(font_size, fonts['medium'])
