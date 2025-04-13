from flask import Blueprint, request, jsonify
from flask_login import login_required
from app.models.race import Race, RaceResult, Racer, Event
from app import db, socketio
import json
from datetime import datetime

api_bp = Blueprint('api', __name__)

@api_bp.route('/device/race_result', methods=['POST'])
def receive_race_result():
    """Endpoint for ESP32 to send race results"""
    if not request.is_json:
        return jsonify({"error": "Request must be JSON"}), 400
    
    data = request.get_json()
    
    # Validate required fields
    required_fields = ['race_id', 'car1_time', 'car2_time', 'winner']
    for field in required_fields:
        if field not in data:
            return jsonify({"error": f"Missing required field: {field}"}), 400
    
    # Get the race
    race = Race.query.get(data['race_id'])
    if not race:
        return jsonify({"error": "Race not found"}), 404
    
    # Update race status
    race.status = 'completed'
    race.end_time = datetime.utcnow()
    
    # Create race results
    lane1_result = RaceResult(
        race_id=race.id,
        lane=1,
        time=data['car1_time'] / 1000.0,  # Convert from ms to seconds
        position=1 if data['winner'] == 'car1' else (0 if data['winner'] == 'tie' else 2)
    )
    
    lane2_result = RaceResult(
        race_id=race.id,
        lane=2,
        time=data['car2_time'] / 1000.0,  # Convert from ms to seconds
        position=1 if data['winner'] == 'car2' else (0 if data['winner'] == 'tie' else 2)
    )
    
    db.session.add(lane1_result)
    db.session.add(lane2_result)
    db.session.commit()
    
    # Emit real-time update via SocketIO
    socketio.emit('race_completed', {
        'race_id': race.id,
        'car1_time': data['car1_time'],
        'car2_time': data['car2_time'],
        'winner': data['winner']
    })
    
    return jsonify({"success": True, "message": "Race result saved successfully"}), 201

@api_bp.route('/races', methods=['GET'])
@login_required
def get_races():
    """Get all races or filter by event_id"""
    event_id = request.args.get('event_id', type=int)
    
    query = Race.query
    if event_id:
        query = query.filter_by(event_id=event_id)
    
    races = query.order_by(Race.round_number, Race.heat_number).all()
    
    result = []
    for race in races:
        race_data = {
            'id': race.id,
            'event_id': race.event_id,
            'round_number': race.round_number,
            'heat_number': race.heat_number,
            'status': race.status,
            'results': []
        }
        
        # Add race results if available
        for result in race.results:
            racer = Racer.query.get(result.racer_id) if result.racer_id else None
            race_data['results'].append({
                'lane': result.lane,
                'time': result.time,
                'position': result.position,
                'racer_name': racer.name if racer else 'Unknown',
                'car_number': racer.car_number if racer else 'N/A'
            })
        
        result.append(race_data)
    
    return jsonify(result)

@api_bp.route('/device/status', methods=['POST'])
def update_device_status():
    """Endpoint for ESP32 to send its status"""
    if not request.is_json:
        return jsonify({"error": "Request must be JSON"}), 400
    
    data = request.get_json()
    
    # Broadcast the device status to all connected clients
    socketio.emit('device_status', data)
    
    return jsonify({"success": True}), 200 