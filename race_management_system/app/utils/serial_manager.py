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
        
        # Initialize logger
        self.logger = logging.getLogger(__name__)
        self.logger.setLevel(logging.INFO)

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
                self.logger.error("No ESP32 device found")
                if self.socketio:
                    self.socketio.emit('esp32_error', {
                        'type': 'error',
                        'message': 'No ESP32 device found'
                    })
                return False
            
            self.serial_port = serial.Serial(port, self.baudrate, timeout=self.timeout)
            self.port_name = port
            self.connected = True
            self.logger.info(f"Connected to ESP32 on port {port}")
            
            # Start the read thread
            self.running = True
            self.read_thread = threading.Thread(target=self._read_serial_data)
            self.read_thread.daemon = True
            self.read_thread.start()
            
            # Send initial status request
            self.send_command("status")
            
            # Emit connection success
            if self.socketio:
                self.socketio.emit('hardware_status', {
                    'connected': True,
                    'port': self.port_name,
                    'status': self.last_status
                })
            
            return True
        
        except Exception as e:
            self.logger.error(f"Error connecting to serial port: {e}")
            if self.socketio:
                self.socketio.emit('esp32_error', {
                    'type': 'error',
                    'message': f'Error connecting to serial port: {e}'
                })
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
        self.logger.info(f"Received data: {data}")
        
        try:
            # Parse JSON data
            json_data = json.loads(data)
            self.logger.info(f"Parsed JSON: {json_data}")
            
            # Force-add the port name to all messages for the frontend
            json_data['port'] = self.port_name
            
            # Always maintain last status for hardware status requests
            if "type" in json_data and json_data["type"] == "status":
                self.last_status = json_data
                
                # Create hardware status message
                hardware_status = {
                    'connected': True,
                    'port': self.port_name,
                    'status': json_data
                }
                
                # Emit hardware status update
                if self.socketio:
                    self.logger.info(f"Emitting hardware_status: {hardware_status}")
                    self.socketio.emit('hardware_status', hardware_status)
            
            # Handle specific message types
            if "type" in json_data:
                msg_type = json_data["type"]
                
                # 1. Sensor readings
                if msg_type == "sensor_reading":
                    if self.socketio:
                        self.logger.info(f"Emitting sensor reading: {json_data}")
                        self.socketio.emit('sensor_reading', json_data)
                
                # 2. Race events
                elif msg_type == "race_start":
                    if self.socketio:
                        self.logger.info(f"Emitting race start: {json_data}")
                        self.socketio.emit('race_start', json_data)
                
                elif msg_type == "race_finish":
                    if self.socketio:
                        self.logger.info(f"Emitting race finish: {json_data}")
                        self.socketio.emit('race_finish', json_data)
                
                # 3. Error messages
                elif msg_type == "error":
                    if self.socketio:
                        self.logger.error(f"ESP32 error: {json_data.get('message', 'Unknown error')}")
                        self.socketio.emit('esp32_error', json_data)
            
            # Fallback for legacy messages
            elif all(key in json_data for key in ['sensor1', 'sensor2']):
                if self.socketio:
                    self.logger.info(f"Emitting legacy sensor reading: {json_data}")
                    self.socketio.emit('sensor_reading', json_data)
        
        except json.JSONDecodeError as e:
            self.logger.error(f"Error parsing JSON data: {e}")
            if self.socketio:
                self.socketio.emit('esp32_error', {
                    'type': 'error',
                    'message': f'Invalid JSON data received: {e}'
                })
        
        except Exception as e:
            self.logger.error(f"Error processing serial data: {e}")
            if self.socketio:
                self.socketio.emit('esp32_error', {
                    'type': 'error',
                    'message': f'Error processing data: {e}'
                })
    
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
        if not self.connected:
            return {'connected': False}
            
        # Request a fresh status
        self.send_command("status")
        
        # Format the response correctly for the dashboard
        if self.last_status:
            return {'connected': True, 'port': self.port_name, 'status': self.last_status}
        else:
            return {'connected': True, 'port': self.port_name}
    
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
