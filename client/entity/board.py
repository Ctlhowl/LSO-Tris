import pygame
from config .settings import *

class Board:

    def __init__(self, screen):
        self.screen = screen
        self.board = [[1, 2, 3], [4, 5, 6], [7, 8, 9]]

        self.board_offset_x = 225
        self.board_offset_y = 255
        self.cell_size = 150
        
        self.board_color = BOARD_COLOR
        self.x_color = X_COLOR
        self.o_color = O_COLOR

    def add_XO(self, to_move):
        current_pos = pygame.mouse.get_pos()
        cell = self.get_cell(*current_pos)

        if cell is not None:
            row, col = cell
            if self.board[row][col] not in ('X', 'O'):
                self.board[row][col] = to_move
        
        self.update()

    def get_cell(self, mouse_x, mouse_y):
        x_rel = mouse_x - self.board_offset_x
        y_rel = mouse_y - self.board_offset_y

        if 0 <= x_rel < 450 and 0 <= y_rel < 450:
            col = x_rel // self.cell_size
            row = y_rel // self.cell_size
            return int(row), int(col)
        return None

    def update(self):
        self.draw_board()

        for i in range(3):
            for j in range(3):
                center_x = self.board_offset_x + (j * self.cell_size) + (self.cell_size // 2)
                center_y = self.board_offset_y + (i * self.cell_size) + (self.cell_size // 2)

                if self.board[i][j] == 'X':
                    self.draw_X(center_x, center_y)
                elif self.board[i][j] == 'O':
                    self.draw_O(center_x, center_y)


    def draw_X(self, x, y):
        offset = 40
        start_pos1 = (x - offset, y - offset)
        end_pos1 = (x + offset, y + offset)

        start_pos2 = (x + offset, y - offset)
        end_pos2 = (x - offset, y + offset)

        pygame.draw.line(self.screen, self.x_color, start_pos1, end_pos1, 5)
        pygame.draw.line(self.screen, self.x_color, start_pos2, end_pos2, 5)

    def draw_O(self, x, y):
        radius = 40
        pygame.draw.circle(self.screen, self.o_color, (x, y), radius, 5)

    def draw_board(self):
        width = 5

        for i in range(1, 3):
            x = self.board_offset_x + i * self.cell_size
            pygame.draw.line(self.screen, self.board_color, (x, self.board_offset_y), (x, self.board_offset_y + 3 * self.cell_size), width)

        for i in range(1, 3):
            y = self.board_offset_y + i * self.cell_size
            pygame.draw.line(self.screen, self.board_color, (self.board_offset_x, y), (self.board_offset_x + 3 * self.cell_size, y), width)
