import pygame

class Button:
    def __init__(self, text, x, y, width, height, base_color, hover_color, text_color, font):
        self.text = text
        self.rect = pygame.Rect(x, y, width, height)
        
        self.base_color = base_color
        self.hover_color = hover_color
        
        self.text_color = text_color
        self.font = font

    def draw(self, screen):
        mouse_pos = pygame.mouse.get_pos()

        is_hovered = self.rect.collidepoint(mouse_pos)
        text_color = self.hover_color if is_hovered else self.text_color

        pygame.draw.rect(screen, self.base_color, self.rect, border_radius=8)

        text_surf = self.font.render(self.text, True, text_color)
        text_rect = text_surf.get_rect(center=self.rect.center)
        screen.blit(text_surf, text_rect)

    def is_clicked(self, event):
        return event.type == pygame.MOUSEBUTTONDOWN and self.rect.collidepoint(pygame.mouse.get_pos())
