import os

WIDTH, HEIGHT = 900, 900
FONT_PATH = os.path.join(os.path.dirname(__file__), "../assets", "VT323-Regular.ttf")

PRIMARY_COLOR = (217, 220, 214)
SECONDARY_COLOR = (58, 124, 165)

X_COLOR = (250, 163, 7)
O_COLOR = (44, 125, 160)
BOARD_COLOR = (26, 26, 29)

TEXT_COLOR = (26, 26, 29)
TEXT_ERROR_COLOR = (255, 0, 0) 
TEXT_SUCCESS_COLOR = (25, 135, 84)

DISCONNECT_MESSAGE = "!DISCONNECT"
PORT = 8080
SERVER = "127.0.1.1"