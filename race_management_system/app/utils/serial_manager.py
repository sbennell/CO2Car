import json
import serial
import serial.tools.list_ports
import threading
import time
import logging
from datetime import datetime
from flask_socketio import SocketIO
from flask import current_app
from app.models.race import Race, RaceResult, Heat
from app import db
from flask import _app_ctx_stack
import importlib
from app import flask_app

# Set up logging
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

class SerialManager:
    """
    Manages serial communication with the ESP32 hardware
    Provides methods for sending commands and processing responses
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
            # Only try to parse as JSON if the data starts with '{'
            if data.startswith('{'):
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
                    
                    elif msg_type == "race_update":
                        if self.socketio:
                            self.logger.info(f"Emitting race update: {json_data}")
                            self.socketio.emit('race_update', json_data)
                    
                    elif msg_type == "race_result":
                        if self.socketio:
                            self.logger.info(f"Emitting race result as race_completed: {json_data}")
                            self.socketio.emit('race_completed', json_data)
                            
                            # Update race results in the database
                            try:
                                self.logger.info(f"Updating race results in database: {json_data}")
                                # Add a race/heat ID to the data if not present
                                if 'heat_id' not in json_data and 'race_id' not in json_data:
                                    # Log the race result with a warning about missing IDs
                                    self.logger.warning(f"Race result received without heat_id or race_id: {json_data}")
                                    # Try to find the most recent in-progress race
                                    from app.models.race import Race
                                    active_race = Race.query.filter_by(status='in_progress').order_by(Race.id.desc()).first()
                                    if active_race:
                                        json_data['race_id'] = active_race.id
                                        self.logger.info(f"Using most recent in-progress race ID: {active_race.id}")
                                
                                self._update_race_results(json_data)
                            except Exception as e:
                                self.logger.error(f"Error updating race results: {e}")
                    
                    elif msg_type == "race_completed":
                        if self.socketio:
                            self.logger.info(f"Emitting race completed: {json_data}")
                            self.socketio.emit('race_completed', json_data)
                            
                            # Update race results in the database 
                            try:
                                self.logger.info(f"Updating race results in database: {json_data}")
                                # Add a race/heat ID to the data if not present
                                if 'heat_id' not in json_data and 'race_id' not in json_data:
                                    # Log the race result with a warning about missing IDs
                                    self.logger.warning(f"Race result received without heat_id or race_id: {json_data}")
                                    # Try to find the most recent in-progress race
                                    from app.models.race import Race
                                    active_race = Race.query.filter_by(status='in_progress').order_by(Race.id.desc()).first()
                                    if active_race:
                                        json_data['race_id'] = active_race.id
                                        self.logger.info(f"Using most recent in-progress race ID: {active_race.id}")
                                
                                self._update_race_results(json_data)
                            except Exception as e:
                                self.logger.error(f"Error updating race results: {e}")
                    
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
            else:
                # It's not JSON, check if it's a boot message or other debug output we can handle
                if any(boot_msg in data for boot_msg in ['rst:', 'boot:', 'mode:', 'load:', 'entry', 'configsip']):
                    self.logger.debug(f"ESP32 boot message: {data}")
                elif data.strip():  # Only log if not empty
                    self.logger.debug(f"Non-JSON message from ESP32: {data}")
        
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
        from app import flask_app
        from app.models.race import Race, RaceResult
        from app import db
        
        # Enhanced debugging
        self.logger.info(f"🔍 ATTEMPTING TO UPDATE RACE RESULTS WITH DATA: {race_data}")
        
        try:
            # Use the global flask_app reference
            with flask_app.app_context():
                return self._do_update_race_results(race_data)
        except Exception as e:
            self.logger.error(f"❌ Error setting up application context: {e}")
            import traceback
            self.logger.error(traceback.format_exc())
            return False
    
    def _do_update_race_results(self, race_data):
        """Internal method to update race results within an application context"""
        from app.models.race import Race, RaceResult, Heat
        from app import db
        
        race = None
        
        # Check if we have a race ID or heat ID
        if 'race_id' in race_data:
            race_id = race_data.get('race_id')
            self.logger.info(f"🔍 Looking up race by race_id: {race_id}")
            race = Race.query.get(race_id)
            if race:
                self.logger.info(f"🔍 Found race with ID {race.id}, status: {race.status}")
            else:
                self.logger.error(f"⚠️ No race found with ID {race_id}")
        elif 'heat_id' in race_data:
            heat_id = race_data.get('heat_id')
            self.logger.info(f"🔍 Looking up race by heat_id: {heat_id}")
            # Need to find the Heat model first, then find the race by the heat and round numbers
            heat = Heat.query.get(heat_id)
            if heat:
                self.logger.info(f"🔍 Found heat with number {heat.number} in round {heat.round.number}")
                race = Race.query.filter_by(
                    event_id=heat.round.event_id,
                    round_number=heat.round.number,
                    heat_number=heat.number
                ).first()
                if race:
                    self.logger.info(f"🔍 Found race with ID {race.id} for heat {heat_id}, status: {race.status}")
                else:
                    self.logger.error(f"⚠️ No race found for heat {heat_id} (round {heat.round.number}, heat {heat.number})")
            else:
                self.logger.error(f"⚠️ No heat found with ID {heat_id}")
        
        if not race:
            # Try one more approach - find most recent in-progress race
            self.logger.warning(f"⚠️ Falling back to most recent in-progress race")
            race = Race.query.filter_by(status='in_progress').order_by(Race.id.desc()).first()
            if race:
                self.logger.info(f"🔍 Found most recent in-progress race: {race.id}")
            else:
                self.logger.error(f"❌ Cannot update race results: No race found by any method")
                return False
            
        self.logger.info(f"Updating results for race ID {race.id}")
        
        # Get car times, converting from milliseconds to seconds if needed
        car1_time = race_data.get('car1_time', 0)
        car2_time = race_data.get('car2_time', 0)
        
        # Check if values are large, indicating milliseconds
        if car1_time > 1000:
            car1_time /= 1000.0
        if car2_time > 1000:
            car2_time /= 1000.0
        
        # Determine winner for positions
        winner = race_data.get('winner', '')
        
        # Create/update lane 1 result
        lane1_result = RaceResult.query.filter_by(race_id=race.id, lane_number=1).first()
        if not lane1_result:
            lane1_result = RaceResult(
                race_id=race.id,
                lane_number=1,
                time=car1_time,
                position=1 if winner == 'car1' else (0 if winner == 'tie' else 2)
            )
            db.session.add(lane1_result)
        else:
            lane1_result.time = car1_time
            lane1_result.position = 1 if winner == 'car1' else (0 if winner == 'tie' else 2)
                
        # Create/update lane 2 result
        lane2_result = RaceResult.query.filter_by(race_id=race.id, lane_number=2).first()
        if not lane2_result:
            lane2_result = RaceResult(
                race_id=race.id,
                lane_number=2,
                time=car2_time,
                position=1 if winner == 'car2' else (0 if winner == 'tie' else 2)
            )
            db.session.add(lane2_result)
        else:
            lane2_result.time = car2_time
            lane2_result.position = 1 if winner == 'car2' else (0 if winner == 'tie' else 2)
        
        # Update race status to completed
        race.status = 'completed'
        race.end_time = datetime.utcnow()
            
        # Save changes to database
        try:
            db.session.commit()
            self.logger.info(f"Race results updated: Lane 1={car1_time}s, Lane 2={car2_time}s, Winner={winner}")
            return True
        except Exception as e:
            self.logger.error(f"Error updating race results: {e}")
            db.session.rollback()
            return False
    
    def start_race(self, race_id):
        """Send command to start a race"""
        try:
            from app import flask_app
            
            # Use the global flask_app reference
            with flask_app.app_context():
                race = Race.query.get(race_id)
                if race:
                    self.logger.info(f"Starting race id {race.id}")
                    # Update race status in the database
                    race.status = 'in_progress'
                    race.start_time = datetime.utcnow()
                    db.session.commit()
                
                return self.send_command("start_race", {"race_id": race_id})
        except Exception as e:
            self.logger.error(f"Error starting race: {e}")
            return self.send_command("start_race", {"race_id": race_id})
    
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
    
    def is_connected(self):
        """Check if the serial manager is connected to hardware"""
        return self.connected and self.serial_port and self.serial_port.is_open
    
    def get_port_name(self):
        """Get the name of the connected port"""
        return self.port_name if self.connected else None
    
    def get_latest_response(self):
        """Get the most recent response from the ESP32 for debugging purposes"""
        if self.last_status:
            return json.dumps(self.last_status, indent=2)
        return None
    
    def calibrate_sensors(self):
        """Calibrate the sensors on the ESP32"""
        self.logger.info("Calibrating sensors...")
        
        # Force request a status update first to ensure communication is working
        self.send_command("status")
        time.sleep(0.1)  # Small delay
        
        # Send the calibration command
        result = self.send_command("calibrate")
        self.logger.info(f"Calibrate command sent, result: {result}")
        
        # Log the raw command being sent for debugging
        cmd_obj = {"cmd": "calibrate"}
        cmd_str = json.dumps(cmd_obj) + "\n"
        self.logger.info(f"Raw calibrate command: {cmd_str.strip()}")
        
        return result

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
