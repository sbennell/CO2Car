from flask import Blueprint, render_template, request, jsonify, flash, redirect, url_for
from flask_login import login_required, current_user
from app.utils.serial_manager import get_serial_manager
from app import socketio
import json
import logging

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
    success = serial_manager.connect(port)
    
    if success:
        flash('Successfully connected to ESP32 hardware', 'success')
    else:
        flash('Failed to connect to ESP32 hardware', 'danger')
    
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

@hardware_bp.route('/api/hardware/calibrate', methods=['POST'])
@login_required
def calibrate_sensors():
    """Send command to calibrate the sensors"""
    serial_manager = get_serial_manager()
    
    if not serial_manager.connected:
        return jsonify({
            'success': False,
            'message': 'Not connected to ESP32 hardware'
        }), 400
    
    success = serial_manager.calibrate_sensors()
    
    return jsonify({
        'success': success,
        'message': 'Calibration command sent' if success else 'Failed to send calibration command'
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
    import time
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
