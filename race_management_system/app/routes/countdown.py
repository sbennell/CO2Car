from flask import Blueprint, jsonify, request
from flask_login import login_required
from app.models.race import Heat
from app.utils.countdown_manager import countdown_manager
from app import socketio, db

countdown_bp = Blueprint('countdown', __name__)

@countdown_bp.route('/api/heats/<int:heat_id>/countdown/start', methods=['POST'])
@login_required
def start_countdown(heat_id):
    """Start or resume the countdown for a heat"""
    heat = Heat.query.get_or_404(heat_id)
    
    # Get duration from request if provided
    duration = request.json.get('duration') if request.is_json else None
    
    # Start the countdown
    if duration:
        countdown_manager.reset_timer(heat_id, duration)
    status = countdown_manager.start_timer(heat_id)
    
    # Emit countdown update to all clients
    socketio.emit('countdown_update', status)
    
    return jsonify(status)

@countdown_bp.route('/api/heats/<int:heat_id>/countdown/pause', methods=['POST'])
@login_required
def pause_countdown(heat_id):
    """Pause the countdown for a heat"""
    heat = Heat.query.get_or_404(heat_id)
    
    # Pause the countdown
    status = countdown_manager.pause_timer(heat_id)
    
    # Emit countdown update to all clients
    socketio.emit('countdown_update', status)
    
    return jsonify(status)

@countdown_bp.route('/api/heats/<int:heat_id>/countdown/reset', methods=['POST'])
@login_required
def reset_countdown(heat_id):
    """Reset the countdown for a heat"""
    heat = Heat.query.get_or_404(heat_id)
    
    # Get duration from request if provided
    duration = request.json.get('duration') if request.is_json else None
    
    # Reset the countdown
    status = countdown_manager.reset_timer(heat_id, duration)
    
    # Emit countdown update to all clients
    socketio.emit('countdown_update', status)
    
    return jsonify(status)

@countdown_bp.route('/api/heats/<int:heat_id>/countdown/status', methods=['GET'])
@login_required
def get_countdown_status(heat_id):
    """Get the current countdown status for a heat"""
    heat = Heat.query.get_or_404(heat_id)
    
    # Get the countdown status
    status = countdown_manager.get_timer_status(heat_id)
    
    return jsonify(status)

@socketio.on('connect')
def handle_connect():
    """Handle WebSocket connection"""
    pass

@socketio.on('join_heat')
def handle_join_heat(data):
    """Join a heat room to receive countdown updates"""
    heat_id = data.get('heat_id')
    if heat_id:
        # Get the current countdown status
        status = countdown_manager.get_timer_status(heat_id)
        
        # Emit the current status to the client
        socketio.emit('countdown_update', status, room=request.sid)

@socketio.on('leave_heat')
def handle_leave_heat(data):
    """Leave a heat room"""
    heat_id = data.get('heat_id')
    if heat_id:
        pass
