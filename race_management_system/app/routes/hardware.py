from flask import Blueprint, render_template, request, jsonify, flash, redirect, url_for, session, current_app
from flask_login import login_required, current_user
from app.utils.serial_manager import get_serial_manager, SerialManager
from app import socketio
import json
import logging
import time

# Set up logging
logger = logging.getLogger(__name__)

# Create blueprint
hardware_bp = Blueprint('hardware', __name__)

@hardware_bp.route('/hardware')
@login_required
def hardware_dashboard():
    """Hardware management dashboard"""
    serial_manager = get_serial_manager()
    available_ports = serial_manager.get_available_ports()
    connected = serial_manager.connected
    port_name = serial_manager.port_name if connected else None
    
    return render_template(
        'hardware/dashboard.html',
        available_ports=available_ports,
        connected=connected,
        port_name=port_name
    )

@hardware_bp.route('/hardware/connect', methods=['POST'])
@login_required
def connect_hardware():
    """Connect to the ESP32 hardware"""
    port = request.form.get('port')
    
    serial_manager = get_serial_manager()
    try:
        serial_manager.connect(port)
        flash('Successfully connected to hardware on port ' + port, 'success')
    except Exception as e:
        flash('Failed to connect to hardware: ' + str(e), 'danger')
    
    return redirect(url_for('hardware.hardware_dashboard'))

@hardware_bp.route('/hardware/disconnect', methods=['POST'])
@login_required
def disconnect_hardware():
    """Disconnect from the ESP32 hardware"""
    serial_manager = get_serial_manager()
    success = serial_manager.disconnect()
    
    if success:
        flash('Successfully disconnected from ESP32 hardware', 'success')
    else:
        flash('Failed to disconnect from ESP32 hardware', 'danger')
    
    return redirect(url_for('hardware.hardware_dashboard'))

@hardware_bp.route('/api/hardware/status')
@login_required
def hardware_status():
    """Get the current status of the ESP32 hardware"""
    serial_manager = get_serial_manager()
    
    if not serial_manager.connected:
        return jsonify({
            'connected': False,
            'message': 'Not connected to ESP32 hardware'
        })
    
    # Request a status update
    serial_manager.send_command('status')
    
    # Return the last known status
    status = serial_manager.last_status
    status['connected'] = True
    
    return jsonify(status)

@hardware_bp.route('/api/hardware/start_race', methods=['POST'])
@login_required
def start_race():
    """Send command to start a race"""
    data = request.json
    heat_id = data.get('heat_id')
    
    if not heat_id:
        return jsonify({
            'success': False,
            'message': 'Heat ID is required'
        }), 400
    
    serial_manager = get_serial_manager()
    
    if not serial_manager.connected:
        return jsonify({
            'success': False,
            'message': 'Not connected to ESP32 hardware'
        }), 400
    
    success = serial_manager.start_race(heat_id)
    
    return jsonify({
        'success': success,
        'message': 'Race start command sent' if success else 'Failed to send race start command'
    })

@hardware_bp.route('/api/hardware/reset_timer', methods=['POST'])
@login_required
def reset_timer():
    """Send command to reset the race timer"""
    serial_manager = get_serial_manager()
    
    if not serial_manager.connected:
        return jsonify({
            'success': False,
            'message': 'Not connected to ESP32 hardware'
        }), 400
    
    success = serial_manager.reset_timer()
    
    return jsonify({
        'success': success,
        'message': 'Timer reset command sent' if success else 'Failed to send timer reset command'
    })

@hardware_bp.route('/hardware/calibrate', methods=['POST'])
@login_required
def calibrate_sensors_frontend():
    """Send command to calibrate the sensors (frontend route)"""
    logger.info("[DEBUG] Received calibration request from frontend")
    logger.info(f"[DEBUG-TRACE] Request info: method={request.method}, url={request.url}, headers={dict(request.headers)}")
    
    serial_manager = get_serial_manager()
    
    logger.info(f"[DEBUG] Serial manager state: connected={serial_manager.connected}, port={serial_manager.port_name if serial_manager.connected else 'None'}")
    
    if not serial_manager.connected:
        logger.error("[DEBUG] Cannot calibrate: Not connected to ESP32 hardware")
        return jsonify({
            'success': False,
            'message': 'Not connected to ESP32 hardware'
        }), 400
    
    logger.info(f"[DEBUG] Sending calibration command to ESP32 on port {serial_manager.port_name}")
    
    # Try multiple command variations to ensure one works
    success = False
    error_message = ""
    
    try:
        # First try the direct 'calibrate' command
        logger.info("[DEBUG] About to write 'calibrate\\n' to serial port")
        serial_manager.serial_port.write(b'calibrate\n')
        logger.info("[DEBUG] Successfully sent direct 'calibrate' command")
        success = True
    except Exception as e:
        error_message = str(e)
        logger.error(f"[DEBUG] Error sending 'calibrate' command: {error_message}")
        
        # Try fallback using JSON command
        try:
            logger.info("[DEBUG] Trying fallback: JSON command")
            serial_manager.send_command("calibrate")
            logger.info("[DEBUG] Successfully sent fallback calibrate command via JSON")
            success = True
        except Exception as e2:
            logger.error(f"[DEBUG] Fallback also failed: {str(e2)}")
            error_message += f" | Fallback error: {str(e2)}"
    
    logger.info(f"[DEBUG] Calibration command result: {success}")
    
    # Wait a moment to allow ESP32 to process
    time.sleep(0.1)
    
    # Request a status update
    try:
        serial_manager.send_command('status')
        logger.info("[DEBUG] Sent status request after calibration")
    except Exception as e:
        logger.error(f"[DEBUG] Error requesting status: {str(e)}")
    
    return jsonify({
        'success': success,
        'message': 'Calibration command sent' if success else f'Failed to send calibration command: {error_message}',
        'debug_info': {
            'port': serial_manager.port_name,
            'connected': serial_manager.connected,
            'timestamp': int(time.time())
        }
    })

@hardware_bp.route('/api/hardware/ports')
@login_required
def get_available_ports():
    """Get a list of available serial ports"""
    serial_manager = get_serial_manager()
    ports = serial_manager.get_available_ports()
    
    return jsonify({
        'ports': ports
    })

@hardware_bp.route('/hardware/reset_timer', methods=['POST'])
@login_required
def reset_timer_frontend():
    """Send command to reset the race timer (frontend route)"""
    logger.info("Received reset timer request from frontend")
    
    serial_manager = get_serial_manager()
    
    if not serial_manager.connected:
        logger.error("Cannot reset timer: Not connected to ESP32 hardware")
        return jsonify({
            'success': False,
            'message': 'Not connected to ESP32 hardware'
        }), 400
    
    logger.info(f"Sending reset timer command to ESP32 on port {serial_manager.port_name}")
    
    # Send the direct 'resetTimer' command
    try:
        serial_manager.serial_port.write(b'resetTimer\n')
        logger.info("Sent direct 'resetTimer' command")
        success = True
    except Exception as e:
        logger.error(f"Error sending direct resetTimer command: {str(e)}")
        success = False
    
    logger.info(f"Reset timer command result: {success}")
    return jsonify({
        'success': success,
        'message': 'Reset timer command sent' if success else 'Failed to send reset timer command'
    })

@hardware_bp.route('/hardware/test_race', methods=['POST'])
@login_required
def test_race_frontend():
    """Send command to start a test race (frontend route)"""
    logger.info("[DEBUG] Received test race request from frontend")
    logger.info(f"[DEBUG-TRACE] Request info: method={request.method}, url={request.url}, headers={dict(request.headers)}")
    
    serial_manager = get_serial_manager()
    
    logger.info(f"[DEBUG] Serial manager state: connected={serial_manager.connected}, port={serial_manager.port_name if serial_manager.connected else 'None'}")
    
    if not serial_manager.connected:
        logger.error("[DEBUG] Cannot start test race: Not connected to ESP32 hardware")
        return jsonify({
            'success': False,
            'message': 'Not connected to ESP32 hardware'
        }), 400
    
    logger.info(f"[DEBUG] Sending test race command to ESP32 on port {serial_manager.port_name}")
    
    # Try multiple command variations to ensure one works
    success = False
    error_message = ""
    
    try:
        # First try the direct 'testrace' command
        logger.info("[DEBUG] About to write 'testrace\\n' to serial port")
        serial_manager.serial_port.write(b'testrace\n')
        logger.info("[DEBUG] Successfully sent direct 'testrace' command")
        success = True
    except Exception as e:
        error_message = str(e)
        logger.error(f"[DEBUG] Error sending 'testrace' command: {error_message}")
        
        # Try fallback using direct socket write
        try:
            logger.info("[DEBUG] Trying fallback: Writing raw bytes directly to serial port")
            serial_manager.serial_port.write(b'testrace\n')
            logger.info("[DEBUG] Successfully sent fallback 'testrace' command")
            success = True
        except Exception as e2:
            logger.error(f"[DEBUG] Fallback also failed: {str(e2)}")
            error_message += f" | Fallback error: {str(e2)}"
    
    logger.info(f"[DEBUG] Test race command result: {success}")
    
    # Wait a moment to allow ESP32 to process
    time.sleep(0.1)
    
    # Request a status update
    try:
        serial_manager.send_command('status')
        logger.info("[DEBUG] Sent status request after test race command")
    except Exception as e:
        logger.error(f"[DEBUG] Error requesting status: {str(e)}")
    
    return jsonify({
        'success': success,
        'message': 'Test race command sent' if success else f'Failed to send test race command: {error_message}',
        'debug_info': {
            'port': serial_manager.port_name,
            'connected': serial_manager.connected,
            'timestamp': int(time.time())
        }
    })

@hardware_bp.route('/hardware/car_loaded', methods=['POST'])
@login_required
def car_loaded_frontend():
    """Send command to set cars as loaded (frontend route)"""
    logger.info("[DEBUG] Received car loaded request from frontend")
    logger.info(f"[DEBUG-TRACE] Request info: method={request.method}, url={request.url}, headers={dict(request.headers)}")
    
    serial_manager = get_serial_manager()
    
    logger.info(f"[DEBUG] Serial manager state: connected={serial_manager.connected}, port={serial_manager.port_name if serial_manager.connected else 'None'}")
    
    if not serial_manager.connected:
        logger.error("[DEBUG] Cannot set cars loaded: Not connected to ESP32 hardware")
        return jsonify({
            'success': False,
            'message': 'Not connected to ESP32 hardware'
        }), 400
    
    logger.info(f"[DEBUG] Sending car loaded command to ESP32 on port {serial_manager.port_name}")
    
    # Try multiple command variations to ensure one works
    success = False
    error_message = ""
    
    try:
        # First try the direct 'carLoaded' command
        logger.info("[DEBUG] About to write 'carLoaded\\n' to serial port")
        serial_manager.serial_port.write(b'carLoaded\n')
        logger.info("[DEBUG] Successfully sent direct 'carLoaded' command")
        success = True
    except Exception as e:
        error_message = str(e)
        logger.error(f"[DEBUG] Error sending 'carLoaded' command: {error_message}")
        
        # Try fallback using JSON command
        try:
            logger.info("[DEBUG] Trying fallback: JSON command")
            serial_manager.send_command("car_loaded")
            logger.info("[DEBUG] Successfully sent fallback car_loaded command via JSON")
            success = True
        except Exception as e2:
            logger.error(f"[DEBUG] Fallback also failed: {str(e2)}")
            error_message += f" | Fallback error: {str(e2)}"
    
    logger.info(f"[DEBUG] Car loaded command result: {success}")
    
    # Wait a moment to allow ESP32 to process
    time.sleep(0.1)
    
    # Request a status update
    try:
        serial_manager.send_command('status')
        logger.info("[DEBUG] Sent status request after setting cars loaded")
    except Exception as e:
        logger.error(f"[DEBUG] Error requesting status: {str(e)}")
    
    return jsonify({
        'success': success,
        'message': 'Car loaded command sent' if success else f'Failed to send car loaded command: {error_message}',
        'debug_info': {
            'port': serial_manager.port_name,
            'connected': serial_manager.connected,
            'timestamp': int(time.time())
        }
    })

@hardware_bp.route('/hardware/force_reset', methods=['POST'])
@login_required
def force_reset_frontend():
    """Send command to force reset the system (frontend route)"""
    logger.info("Received force reset request from frontend")
    
    serial_manager = get_serial_manager()
    
    if not serial_manager.connected:
        logger.error("Cannot force reset: Not connected to ESP32 hardware")
        return jsonify({
            'success': False,
            'message': 'Not connected to ESP32 hardware'
        }), 400
    
    logger.info(f"Sending force reset command to ESP32 on port {serial_manager.port_name}")
    
    # Send the direct 'forceReset' command
    try:
        serial_manager.serial_port.write(b'forceReset\n')
        logger.info("Sent direct 'forceReset' command")
        success = True
    except Exception as e:
        logger.error(f"Error sending direct forceReset command: {str(e)}")
        success = False
    
    logger.info(f"Force reset command result: {success}")
    return jsonify({
        'success': success,
        'message': 'Force reset command sent' if success else 'Failed to send force reset command'
    })

# WebSocket event handlers
@socketio.on('connect_hardware')
def handle_connect_hardware(data):
    """Handle WebSocket request to connect to hardware"""
    port = data.get('port')
    
    serial_manager = get_serial_manager()
    success = serial_manager.connect(port)
    
    socketio.emit('hardware_connection', {
        'success': success,
        'connected': serial_manager.connected,
        'port': serial_manager.port_name if success else None,
        'message': 'Connected to ESP32' if success else 'Failed to connect to ESP32'
    })

@socketio.on('disconnect_hardware')
def handle_disconnect_hardware():
    """Handle WebSocket request to disconnect from hardware"""
    serial_manager = get_serial_manager()
    success = serial_manager.disconnect()
    
    socketio.emit('hardware_connection', {
        'success': success,
        'connected': serial_manager.connected,
        'message': 'Disconnected from ESP32' if success else 'Failed to disconnect from ESP32'
    })

@socketio.on('send_direct_command')
def handle_direct_command(data):
    """Handle WebSocket request to send a direct command to the ESP32"""
    logger.info(f"Received direct command request: {data}")
    
    if not data or 'command' not in data:
        logger.error("Invalid direct command request: missing 'command' field")
        socketio.emit('esp32_error', {
            'type': 'error',
            'message': "Invalid direct command: missing 'command' field"
        })
        return
    
    command = data['command']
    logger.info(f"Sending direct command: {command}")
    
    serial_manager = get_serial_manager()
    
    if not serial_manager.connected:
        logger.error("Cannot send direct command: Not connected to ESP32 hardware")
        socketio.emit('esp32_error', {
            'type': 'error',
            'message': 'Not connected to ESP32 hardware'
        })
        return
    
    # Send the raw command (without JSON wrapping)
    try:
        serial_manager.serial_port.write((command + "\n").encode())
        logger.info(f"Sent direct command: {command}")
        
        # Emit acknowledgement
        socketio.emit('command_sent', {
            'success': True,
            'command': command,
            'message': f"Direct command '{command}' sent to ESP32"
        })
    except Exception as e:
        logger.error(f"Error sending direct command: {e}")
        socketio.emit('esp32_error', {
            'type': 'error',
            'message': f"Error sending direct command: {e}"
        })

@socketio.on('request_hardware_status')
def handle_request_hardware_status():
    """Handle WebSocket request for hardware status"""
    logger.info("Received WebSocket request for hardware status")
    serial_manager = get_serial_manager()
    
    # First log the current state
    logger.info(f"Serial connection status: connected={serial_manager.connected}, port={serial_manager.port_name if serial_manager.connected else None}")
    if serial_manager.connected and serial_manager.last_status:
        logger.info(f"Last received status: {serial_manager.last_status}")
    
    # Request fresh status from ESP32
    if serial_manager.connected:
        logger.info("Requesting fresh status from ESP32")
        serial_manager.send_command('status')
    
    # Wait a brief moment for ESP32 to respond (100ms)
    time.sleep(0.1)
    
    # Create status message with proper structure
    status_message = {
        'connected': serial_manager.connected,
        'port': serial_manager.port_name if serial_manager.connected else None,
        'status': serial_manager.last_status if serial_manager.connected else {}
    }
    
    # Log what we're sending
    logger.info(f"Emitting hardware_status: {status_message}")
    
    # Send the status to the client
    socketio.emit('hardware_status', status_message)
    
    # Also emit dummy status data to ensure the dashboard is updated
    if serial_manager.connected and not serial_manager.last_status:
        dummy_status = {
            "timestamp": int(time.time() * 1000),
            "race_started": False,
            "cars_loaded": False,
            "car1_finished": False,
            "car2_finished": False,
            "car1_time": 0,
            "car2_time": 0,
            "sensor_calibrated": False,
            "current_heat_id": "None",
            "type": "status"
        }
        logger.info(f"Emitting backup status data: {dummy_status}")
        socketio.emit('esp32_status', dummy_status)

@hardware_bp.route('/api/hardware/direct_command', methods=['POST'])
@login_required
def direct_command_api():
    """Send a direct command to the ESP32 via API"""
    data = request.json
    
    if not data or 'command' not in data:
        logger.error("[DEBUG] Invalid direct command request: missing 'command' field")
        return jsonify({
            'success': False,
            'message': "Invalid direct command: missing 'command' field"
        }), 400
    
    command = data['command']
    logger.info(f"[DEBUG] Received API request to send direct command: {command}")
    
    serial_manager = get_serial_manager()
    
    logger.info(f"[DEBUG] Serial manager state: connected={serial_manager.connected}, port={serial_manager.port_name if serial_manager.connected else 'None'}")
    
    if not serial_manager.connected:
        logger.error("[DEBUG] Cannot send direct command: Not connected to ESP32 hardware")
        return jsonify({
            'success': False,
            'message': 'Not connected to ESP32 hardware'
        }), 400
    
    logger.info(f"[DEBUG] Sending direct command to ESP32 on port {serial_manager.port_name}")
    
    # Try to send the raw command
    success = False
    error_message = ""
    response_data = {}
    
    try:
        # Write the direct command followed by newline
        command_bytes = command.encode() + b'\n'
        logger.info(f"[DEBUG] Sending bytes: {command_bytes}")
        serial_manager.serial_port.write(command_bytes)
        
        # Wait briefly for a response
        time.sleep(0.2)
        
        # Read any available response
        if serial_manager.serial_port.in_waiting > 0:
            response = serial_manager.serial_port.read(serial_manager.serial_port.in_waiting).decode('utf-8')
            logger.info(f"[DEBUG] Direct response: {response}")
            try:
                # Try to parse any JSON in the response
                response_data = json.loads(response)
            except:
                # If not JSON, just include as text
                response_data = {"raw_response": response}
        
        logger.info("[DEBUG] Direct command sent successfully")
        success = True
    except Exception as e:
        error_message = str(e)
        logger.error(f"[DEBUG] Error sending direct command: {error_message}")
        logger.exception("[DEBUG] Detailed exception:")
    
    return jsonify({
        'success': success,
        'message': 'Command sent successfully' if success else f'Failed to send command: {error_message}',
        'command': command,
        'response': response_data,
        'timestamp': int(time.time())
    })

@hardware_bp.route('/hardware/debug', methods=['GET'])
@login_required
def debug_hardware():
    """Debug page for testing direct commands to ESP32"""
    serial_manager = get_serial_manager()
    
    # Get response from session if it exists
    command = session.pop('last_command', '')
    response = session.pop('last_response', '')
    
    return render_template(
        'hardware/debug.html',
        connected=serial_manager.connected,
        port_name=serial_manager.port_name if serial_manager.connected else None,
        command=command,
        response=response
    )

@hardware_bp.route('/hardware/send_raw_command', methods=['POST'])
@login_required
def send_raw_command():
    """Send a raw command to the ESP32 and get the response"""
    command = request.form.get('command')
    
    if not command:
        flash('Command cannot be empty', 'danger')
        return redirect(url_for('hardware.debug_hardware'))
    
    serial_manager = get_serial_manager()
    
    if not serial_manager.connected:
        flash('Not connected to ESP32', 'danger')
        return redirect(url_for('hardware.debug_hardware'))
    
    try:
        # Send the command with newline terminator
        serial_manager.serial_port.write((command + '\n').encode())
        logger.info(f"Sent raw command: {command}")
        
        # Wait for a response
        time.sleep(0.5)
        
        # Read response
        response = ""
        if serial_manager.serial_port.in_waiting > 0:
            response = serial_manager.serial_port.read(serial_manager.serial_port.in_waiting).decode('utf-8')
            logger.info(f"Raw response: {response}")
        
        flash(f'Command sent: {command}', 'success')
        
        return render_template(
            'hardware/debug.html',
            connected=serial_manager.connected,
            port_name=serial_manager.port_name,
            command=command,
            response=response
        )
    
    except Exception as e:
        logger.error(f"Error sending raw command: {str(e)}")
        flash(f'Error: {str(e)}', 'danger')
        return redirect(url_for('hardware.debug_hardware'))

@hardware_bp.route('/api/hardware/send_command', methods=['POST'])
@login_required
def send_command():
    """Send a command to the ESP32 hardware"""
    data = request.json
    command = data.get('command')
    params = {}
    
    # Get additional parameters if provided
    if 'race_id' in data:
        params['race_id'] = data['race_id']
    if 'heat_id' in data:
        params['heat_id'] = data['heat_id']
    
    logger.info(f"Received command request: {command} with params {params}")
    
    serial_manager = get_serial_manager()
    
    if not serial_manager.connected:
        return jsonify({
            'success': False,
            'message': 'Not connected to ESP32 hardware'
        }), 400
    
    # Handle special commands
    if command == 'fire_relay':
        logger.info("Sending fire_relay command to ESP32")
        success = serial_manager.send_command("fire_relay", params)
    else:
        # Default to sending the command as-is
        success = serial_manager.send_command(command, params)
    
    return jsonify({
        'success': success,
        'message': f'Command {command} sent successfully' if success else f'Failed to send command {command}'
    })
