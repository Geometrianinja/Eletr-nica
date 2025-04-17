import serial
import struct
import pygame
import receiver
import asyncio
import threading
import time
import os
import win32api

class Circle:
    def __init__(self, x, y, radius, color):
        self.x = x
        self.y = y
        self.radius = radius
        self.color = color

    def set_position(self, x, y):
        self.x = x
        self.y = y

    def draw(self, screen):
        pygame.draw.circle(screen, self.color, (self.x, self.y), self.radius)

    def intersects(self, other_circle):
        distance = ((self.x - other_circle.x) ** 2 + (self.y - other_circle.y) ** 2) ** 0.5
        return distance < (self.radius + other_circle.radius)
    

class Calibrattion:
    def __init__(self):
        self.calibrated = False
        self.ax = None
        self.bx = None
        self.ay = None
        self.by = None

    def set_calibration_data(self, gyro1, point1, gyro2, point2):
        assert gyro1[0] != gyro2[0], "Gyro X values must be different for calibration."
        assert gyro1[1] != gyro2[1], "Gyro Y values must be different for calibration."
        assert point1[0] != point2[0], "Point X values must be different for calibration."
        assert point1[1] != point2[1], "Point Y values must be different for calibration."

        self.ax = (point2[0] - point1[0]) / (gyro2[0] - gyro1[0])
        self.bx = point1[0] - self.ax * gyro1[0]
        self.ay = (point2[1] - point1[1]) / (gyro2[1] - gyro1[1])
        self.by = point1[1] - self.ay * gyro1[1]
        self.calibrated = True

    def get_point(self, gyro):
        assert self.calibrated, "Calibration data not set."
        x = int(self.ax * gyro[0] + self.bx)
        y = int(self.ay * gyro[1] + self.by)
        return (x, y)

controller = receiver.Controller()

def game_loop():
    s_width = 1920
    s_height = 1080
    pointer = Circle(0, 0, 10, (255, 255, 255))
    target1 = Circle(100, 100, 40, (255, 255, 255))
    target2 = Circle(s_width-100, s_height-100, 40, (255, 255, 255))
    calibration = Calibrattion()
    calib_pairs = []
    vibrating = False

    ypr = (0, 0, 0)
    pygame.init()
    # Code to start on display 2 if available
    try:
        monitors = win32api.EnumDisplayMonitors()
        if len(monitors) > 1:
            monitor_info = win32api.GetMonitorInfo(monitors[1][0])
            x, y, _, _ = monitor_info["Monitor"]
            os.environ['SDL_VIDEO_WINDOW_POS'] = f"{x},{y}"
        else:
            os.environ['SDL_VIDEO_CENTERED'] = "1"
    except ImportError:
        os.environ['SDL_VIDEO_CENTERED'] = "1"
    
    screen = pygame.display.set_mode((s_width, s_height))
    pygame.display.set_caption("Cursor")
    clock = pygame.time.Clock()
    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    running = False
                if event.key == pygame.K_SPACE:
                    calibration.calibrated = False
                    calib_pairs = []

        
        while True:
            event = controller.get_event()

            if isinstance(event, receiver.GyroEvent):
                ypr = (event.yaw, event.pitch, event.roll)

            if isinstance(event, receiver.ButtonEvent):
                if event.button == receiver.ButtonID.BUTTON_BACK:
                    running = False
                elif event.button == receiver.ButtonID.BUTTON_SELECT and event.event_type == receiver.ButtonEventType.PRESSED:
                    pointer.color = (100, 100, 255)
                    if not calibration.calibrated:
                        if len(calib_pairs) == 0:
                            calib_pairs.append((ypr[0], ypr[2]))
                            calib_pairs.append((target1.x, target1.y))

                        else:
                            calib_pairs.append((ypr[0], ypr[2]))
                            calib_pairs.append((target2.x, target2.y))
                            print(calib_pairs)
                            calibration.set_calibration_data(calib_pairs[0], calib_pairs[1], calib_pairs[2], calib_pairs[3])

                elif event.button == receiver.ButtonID.BUTTON_SELECT and event.event_type == receiver.ButtonEventType.RELEASED:
                    pointer.color = (255, 255, 255)

            if controller.get_queue_size() < 3:
                break

        if not calibration.calibrated:
            screen.fill((0, 0, 0))
            if len(calib_pairs) == 0:
                target1.draw(screen)
            else:
                target2.draw(screen)

        else:
            screen.fill((0, 0, 0))
            pointer.set_position(*calibration.get_point((ypr[0], ypr[2])))
            pointer.draw(screen)


        pygame.display.flip()
        clock.tick(60)
        #print("FPS:", clock.get_fps())
    pygame.quit()


def main():
    while(not controller.connect()):
        print("Trying to connect...")
        time.sleep(0.5)
        pass
    ble_thread = threading.Thread(target=controller.run)
    ble_thread.start()
    game_loop()
    controller.stop()

if __name__ == "__main__":
    main()


