from http.server import BaseHTTPRequestHandler, HTTPServer
import urllib.parse as urlparse
import os
import pyautogui
import pygetwindow as gw
import time

class RequestHandler(BaseHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.current_window = None  # Инициализируем атрибут для текущего окна

    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)
        post_data = urlparse.parse_qs(post_data.decode('utf-8'))

        if 'command' in post_data:
            command = post_data['command'][0]
            print(f"Получена команда: {command}")  # Лог для проверки
            self.execute_command(command)
            self.send_response(200)
            self.end_headers()
            self.wfile.write(b"Command executed")
        else:
            self.send_response(400)
            self.end_headers()
            self.wfile.write(b"Bad Request")

    def execute_command(self, command):
        if command == "calc":
            os.system("calc")
        elif command == "notepad":
            os.system("notepad")
        elif command == "press_alt_tab":
            pyautogui.hotkey('alt', 'tab')
        elif command == "mute_mic":
            self.focus_discord()
            time.sleep(0.5)
            self.mute_mic()
            self.focus_previous_window()
        else:
            print(f"Неизвестная команда: {command}")

    def mute_mic(self):
        # Эмулируем нажатие клавиш Ctrl + Shift + M
        pyautogui.hotkey('ctrl', 'shift', 'm')
        print("Mic toggled!")

    def focus_discord(self):
        # Получаем все окна
        windows = gw.getWindowsWithTitle("Discord")  # Название окна Discord
        if windows:
            # Если окно найдено, фокусируемся на нем
            self.current_window = gw.getActiveWindow()  # Сохраняем текущее активное окно
            discord_window = windows[0]
            discord_window.activate()  # Активируем окно
            time.sleep(1)  # Ждем, чтобы окно точно стало активным
            print("Discord window activated")
        else:
            print("Discord window not found!")

    def focus_previous_window(self):
        # Возвращаем фокус на предыдущее окно
        if self.current_window:
            self.current_window.activate()
            print("Returned to the previous window")
        else:
            print("No previous window found!")

def run(server_class=HTTPServer, handler_class=RequestHandler, port=8000):
    server_address = ('', port)
    httpd = server_class(server_address, handler_class)
    print(f'Server running on port {port}...')
    httpd.serve_forever()

if __name__ == '__main__':
    run()
