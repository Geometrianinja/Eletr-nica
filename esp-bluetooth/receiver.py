from dataclasses import dataclass
from enum import Enum
import asyncio
from bleak import BleakClient, BleakScanner
import struct
import pygame
import threading
import time
import serial.tools.list_ports


@dataclass
class GyroEvent:
    yaw: float
    pitch: float
    roll: float

class ButtonEventType(Enum):
    PRESSED = 1
    RELEASED = 2

class ButtonID(Enum):
    BUTTON_BACK = 1
    BUTTON_SELECT = 2

@dataclass
class ButtonEvent:
    button: ButtonID
    event_type: ButtonEventType


class Controller():
    def __init__(self):
        self._queue = []
        self._esp = None
        self._bt = None
        self.last_time_ping = pygame.time.get_ticks()
        self._lock = threading.Lock()
        self._running = True

    def connect(self):
        ports = list(serial.tools.list_ports.comports())

        for port in ports:
            print(port.device, port.description)
            if "Bluetooth" in port.description:
                try:
                    bt = serial.Serial(port.device, 115200, timeout=1, write_timeout=1)
                    bt.write('?'.encode())
                    time.sleep(0.3)  # Wait for the device to respond
                    if bt.in_waiting:
                        data = bt.read(bt.in_waiting)
                        if b"ESP32" in data:
                            self._bt = bt
                            self._running = True
                            return True
                            
                except serial.SerialException:
                    print(f"Failed to connect to {port.device}.")
                    continue
        return False

    def get_events(self):
        with self._lock:
            events = self._queue.copy()
            self._queue.clear()
        return events
    
    def get_event(self):
        with self._lock:
            if self._queue:
                return self._queue.pop(0)
        return None
    
    def get_queue_size(self):
        with self._lock:
            return len(self._queue)
    
    def is_empty(self):
        return len(self._queue) == 0
    
    def vibrate_off(self):
        if self._bt:
            self._bt.write(b'\x00')
            self._bt.flush()

    def vibrate_on(self):
        if self._bt:
            self._bt.write(b'\x01')
            self._bt.flush()

    def stop(self):
        with self._lock:
            self._running = False
            if self._bt:
                self._bt.close()
                self._bt = None
    
    def run(self):
        if not self._bt:
            print("Bluetooth device not found.")
            return
        
        while True:
            with self._lock:
                if not self._running:
                    break

            new_time_ping = pygame.time.get_ticks()
            if new_time_ping - self.last_time_ping > 1000:
                self._bt.write(b'\x02')
                self.last_time_ping = new_time_ping

            if self._bt.in_waiting:
                data = self._bt.read(1).decode('utf-8')
                
                if data == 'B':
                    event = ButtonEvent(ButtonID.BUTTON_BACK, ButtonEventType.PRESSED)
                elif data == 'b':
                    event = ButtonEvent(ButtonID.BUTTON_BACK, ButtonEventType.RELEASED)
                elif data == 'S':
                    event = ButtonEvent(ButtonID.BUTTON_SELECT, ButtonEventType.PRESSED)
                elif data == 's':
                    event = ButtonEvent(ButtonID.BUTTON_SELECT, ButtonEventType.RELEASED)
                elif data == 'G':
                    data = self._bt.read(12)
                    event = GyroEvent(*struct.unpack('fff', data))
                else:
                    print(f"Unknown data: {data}")
                    event = None
                        
                if event:
                    with self._lock:
                        self._queue.append(event)
        
        
    
        
    
