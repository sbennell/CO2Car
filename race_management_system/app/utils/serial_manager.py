import serial
import serial.tools.list_ports
import threading
import time
import json
import logging
from datetime import datetime
from flask_socketio import SocketIO

# Set up logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class SerialManager:
    """
    Manages serial communication with the ESP32 race timer hardware.
    Handles sending commands and receiving race timing data.
    """
    def __init__(self, socketio=None, baudrate=115200, timeout=1):
        self.serial_port = None
        self.port_name = None
        self.baudrate = baudrate
        self.timeout = timeout
        self.connected = False
        self.running = False
        self.read_thread = None
        self.socketio = socketio
        self.last_status = {}
        self.available_ports = []
        
    def get_available_ports(self):
        """Get a list of available serial ports"""
        self.available_ports = [
            {
                "port": port.device,
                "description": port.description,
                "hwid": port.hwid
            }
            for port in serial.tools.list_ports.comports()
        ]
        return self.available_ports
    
    def connect(self, port=None):
        """Connect to the ESP32 via serial port"""
        if self.connected:
            return True
        
        try:
            # If no port specified, try to find an ESP32
            if port is None:
                ports = self.get_available_ports()
                for p in ports:
                    if "CP210" in p["description"] or "CH340" in p["description"] or "ESP32" in p["description"]:
                        port = p["port"]
                        break
            
            if port is None:
                logger.error("No ESP32 device found")
                return False
            
            self.serial_port = serial.Serial(port, self.baudrate, timeout=self.timeout)
            self.port_name = port
            self.connected = True
            logger.info(f"Connected to ESP32 on port {port}")
            
            # Start the read thread
            self.running = True
            self.read_thread = threading.Thread(target=self._read_serial_data)
            self.read_thread.daemon = True
            self.read_thread.start()
            
            # Send initial status request
            self.send_command("status")
            
            return True
        
        except Exception as e:
            logger.error(f"Error connecting to serial port: {e}")
            self.connected = False
            return False
    
    def disconnect(self):
        """Disconnect from the serial port"""
        if not self.connected:
            return True
        
        try:
            self.running = False
            if self.read_thread:
                self.read_thread.join(timeout=2.0)
            
            if self.serial_port:
                self.serial_port.close()
            
            self.connected = False
            logger.info("Disconnected from ESP32")
            return True
        
        except Exception as e:
            logger.error(f"Error disconnecting from serial port: {e}")
            return False
    
    def send_command(self, command, params=None):
        """Send a command to the ESP32"""
        if not self.connected:
            logger.error("Cannot send command: Not connected to ESP32")
            return False
        
        try:
            cmd_obj = {"cmd": command}
            if params:
                cmd_obj.update(params)
            
            cmd_str = json.dumps(cmd_obj) + "\n"
            self.serial_port.write(cmd_str.encode())
            logger.info(f"Sent command: {cmd_str.strip()}")
            return True
        
        except Exception as e:
            logger.error(f"Error sending command: {e}")
            return False
    
    def _read_serial_data(self):
        """Background thread to read data from the serial port"""
        logger.info("Serial read thread started")
        
        while self.running:
            try:
                if self.serial_port and self.serial_port.is_open:
                    if self.serial_port.in_waiting > 0:
                        line = self.serial_port.readline().decode('utf-8').strip()
                        if line:
                            self._process_serial_data(line)
                time.sleep(0.01)  # Small delay to prevent CPU hogging
            
            except Exception as e:
                logger.error(f"Error reading from serial port: {e}")
                time.sleep(1)  # Longer delay on error
    
    def _process_serial_data(self, data):
        """Process data received from the ESP32"""
        logger.info(f"Received data: {data}")
        
        try:
            # Parse JSON data
            json_data = json.loads(data)
            
            # Handle different message types
            if "type" in json_data:
                msg_type = json_data["type"]
                
                if msg_type == "status":
                    # ESP32 status update
                    self.last_status = json_data
                    if self.socketio:
                        self.socketio.emit('esp32_status', json_data)
                
                elif msg_type == "race_start":
                    # Race has started
                    if self.socketio:
                        self.socketio.emit('race_start', json_data)
                
                elif msg_type == "race_finish":
                    # Race has finished with timing data
                    if self.socketio:
                        self.socketio.emit('race_finish', json_data)
                        
                        # Also update the race results in the database
                        self._update_race_results(json_data)
                
                elif msg_type == "sensor_reading":
                    # Sensor data update
                    if self.socketio:
                        self.socketio.emit('sensor_reading', json_data)
                
                elif msg_type == "error":
                    # Error message from ESP32
                    logger.error(f"ESP32 error: {json_data.get('message', 'Unknown error')}")
                    if self.socketio:
                        self.socketio.emit('esp32_error', json_data)
            
        except json.JSONDecodeError:
            logger.warning(f"Received non-JSON data: {data}")
        except Exception as e:
            logger.error(f"Error processing serial data: {e}")
    
    def _update_race_results(self, race_data):
        """Update race results in the database"""
        # This will be implemented to update the race results in the database
        # based on the timing data received from the ESP32
        pass
    
    def start_race(self, heat_id):
        """Send command to start a race for the given heat"""
        return self.send_command("start_race", {"heat_id": heat_id})
    
    def reset_timer(self):
        """Reset the race timer"""
        return self.send_command("reset_timer")
    
    def get_status(self):
        """Get the current status of the ESP32"""
        self.send_command("status")
        return self.last_status
    
    def calibrate_sensors(self):
        """Calibrate the sensors on the ESP32"""
        return self.send_command("calibrate")

# Create a singleton instance
serial_manager = None

def init_serial_manager(socketio_instance):
    """Initialize the serial manager with the SocketIO instance"""
    global serial_manager
    serial_manager = SerialManager(socketio=socketio_instance)
    return serial_manager

def get_serial_manager():
    """Get the serial manager instance"""
    global serial_manager
    if serial_manager is None:
        raise RuntimeError("Serial manager not initialized. Call init_serial_manager first.")
    return serial_manager
