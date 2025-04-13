import serial
import json
import threading
import time
import logging
from app import socketio

logger = logging.getLogger(__name__)

class ESP32Communication:
    def __init__(self, port=None, baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.serial = None
        self.connected = False
        self.thread = None
        self.running = False
    
    def connect(self, port=None):
        """Connect to the ESP32 device"""
        if port:
            self.port = port
        
        if not self.port:
            logger.error("No port specified for ESP32 connection")
            return False
        
        try:
            self.serial = serial.Serial(self.port, self.baudrate, timeout=1)
            self.connected = True
            logger.info(f"Connected to ESP32 on {self.port}")
            
            # Start reading thread
            self.running = True
            self.thread = threading.Thread(target=self._read_serial)
            self.thread.daemon = True
            self.thread.start()
            
            return True
        except Exception as e:
            logger.error(f"Failed to connect to ESP32: {str(e)}")
            self.connected = False
            return False
    
    def disconnect(self):
        """Disconnect from the ESP32 device"""
        self.running = False
        if self.thread:
            self.thread.join(timeout=1.0)
        
        if self.serial:
            self.serial.close()
            self.serial = None
        
        self.connected = False
        logger.info("Disconnected from ESP32")
    
    def send_command(self, command, data=None):
        """Send a command to the ESP32"""
        if not self.connected or not self.serial:
            logger.error("Cannot send command - not connected to ESP32")
            return False
        
        message = {
            "cmd": command
        }
        
        if data:
            message["data"] = data
        
        try:
            self.serial.write((json.dumps(message) + "\n").encode('utf-8'))
            logger.debug(f"Sent command to ESP32: {command}")
            return True
        except Exception as e:
            logger.error(f"Failed to send command to ESP32: {str(e)}")
            return False
    
    def _read_serial(self):
        """Read data from the serial port in a separate thread"""
        while self.running and self.serial:
            try:
                if self.serial.in_waiting > 0:
                    line = self.serial.readline().decode('utf-8').strip()
                    if line:
                        self._process_data(line)
            except Exception as e:
                logger.error(f"Error reading from serial port: {str(e)}")
                time.sleep(1)
    
    def _process_data(self, data):
        """Process data received from the ESP32"""
        try:
            json_data = json.loads(data)
            
            # Handle different message types
            if "status" in json_data:
                socketio.emit('device_status', json_data)
            elif "race_result" in json_data:
                socketio.emit('race_completed', json_data["race_result"])
            elif "error" in json_data:
                logger.error(f"Error from ESP32: {json_data['error']}")
                socketio.emit('device_error', {"message": json_data['error']})
            else:
                logger.debug(f"Received data from ESP32: {data}")
                socketio.emit('device_message', json_data)
        except json.JSONDecodeError:
            logger.warning(f"Received non-JSON data from ESP32: {data}")
        except Exception as e:
            logger.error(f"Error processing data from ESP32: {str(e)}")


# Create a singleton instance
esp32_comm = ESP32Communication()

def get_esp32_comm():
    """Get the ESP32 communication singleton"""
    return esp32_comm 