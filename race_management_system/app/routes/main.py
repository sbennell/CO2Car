from flask import Blueprint, render_template, redirect, url_for, flash, request, jsonify
from flask_login import login_required, current_user
from app.models.race import Event, Race, Racer, RaceResult, Round, Heat, Lane, Standing
from app import db
from datetime import datetime, timedelta
import random
import math
from sqlalchemy import func

main_bp = Blueprint('main', __name__)

@main_bp.route('/')
def index():
    """Landing page"""
    return render_template('index.html')

@main_bp.route('/dashboard')
@login_required
def dashboard():
    """Main dashboard view"""
    events = Event.query.order_by(Event.date.desc()).limit(5).all()
    upcoming_races = Race.query.filter_by(status='scheduled').order_by(Race.start_time).limit(10).all()
    
    return render_template('dashboard.html', 
                          events=events, 
                          upcoming_races=upcoming_races)

@main_bp.route('/events')
@login_required
def events():
    """List all events"""
    all_events = Event.query.filter_by(is_archived=False).order_by(Event.date.desc()).all()
    return render_template('events.html', events=all_events, now=datetime.utcnow().date())

@main_bp.route('/events/create', methods=['POST'])
@login_required
def create_event():
    """Create a new event"""
    name = request.form.get('name')
    date = datetime.strptime(request.form.get('date'), '%Y-%m-%d').date()
    location = request.form.get('location')
    description = request.form.get('description')
    
    event = Event(
        name=name,
        date=date,
        location=location,
        description=description,
        creator_id=current_user.id
    )
    db.session.add(event)
    db.session.commit()
    
    flash('Event created successfully')
    return redirect(url_for('main.events'))

@main_bp.route('/events/<int:event_id>/edit', methods=['GET', 'POST'])
@login_required
def edit_event(event_id):
    """Edit an existing event"""
    event = Event.query.get_or_404(event_id)
    
    # Check if user has permission to edit
    if current_user.role != 'admin' and event.creator_id != current_user.id:
        flash('You do not have permission to edit this event.', 'error')
        return redirect(url_for('main.events'))
    
    if request.method == 'POST':
        event.name = request.form.get('name')
        event.date = datetime.strptime(request.form.get('date'), '%Y-%m-%d')
        event.location = request.form.get('location')
        event.description = request.form.get('description')
        event.updated_at = datetime.utcnow()
        
        db.session.commit()
        flash('Event updated successfully')
        return redirect(url_for('main.event_detail', event_id=event.id))
    
    return render_template('event_edit.html', event=event)

@main_bp.route('/events/<int:event_id>/delete', methods=['POST'])
@login_required
def delete_event(event_id):
    """Archive an event"""
    event = Event.query.get_or_404(event_id)
    
    # Check if user has permission to delete
    if current_user.role != 'admin' and event.creator_id != current_user.id:
        flash('You do not have permission to delete this event.', 'error')
        return redirect(url_for('main.events'))
    
    # Archive instead of hard delete
    event.is_archived = True
    event.archived_at = datetime.utcnow()
    db.session.commit()
    
    flash('Event archived successfully')
    return redirect(url_for('main.events'))

@main_bp.route('/events/<int:event_id>')
@login_required
def event_detail(event_id):
    """Detail view for a specific event"""
    event = Event.query.get_or_404(event_id)
    races = Race.query.filter_by(event_id=event_id).order_by(Race.round_number, Race.heat_number).all()
    return render_template('event_detail.html', event=event, races=races)

@main_bp.route('/racers')
@login_required
def racers():
    """List all racers"""
    all_racers = Racer.query.order_by(Racer.name).all()
    return render_template('racers.html', racers=all_racers)

@main_bp.route('/racers/create', methods=['POST'])
@login_required
def create_racer():
    """Create a new racer"""
    name = request.form.get('name')
    car_number = request.form.get('car_number')
    group = request.form.get('class_name')
    
    racer = Racer(name=name, car_number=car_number, group=group)
    db.session.add(racer)
    db.session.commit()
    
    flash('Racer added successfully')
    return redirect(url_for('main.racers'))

@main_bp.route('/racers/<int:racer_id>/edit', methods=['POST'])
@login_required
def edit_racer(racer_id):
    """Edit an existing racer"""
    racer = Racer.query.get_or_404(racer_id)
    
    racer.name = request.form.get('name')
    racer.car_number = request.form.get('car_number')
    racer.group = request.form.get('class_name')
    
    db.session.commit()
    flash('Racer updated successfully')
    return redirect(url_for('main.racers'))

@main_bp.route('/racers/<int:racer_id>/delete', methods=['POST'])
@login_required
def delete_racer(racer_id):
    """Delete a racer"""
    racer = Racer.query.get_or_404(racer_id)
    db.session.delete(racer)
    db.session.commit()
    
    flash('Racer deleted successfully')
    return redirect(url_for('main.racers'))

@main_bp.route('/races/<int:race_id>')
@login_required
def race_detail(race_id):
    """Detail view for a specific race"""
    race = Race.query.get_or_404(race_id)
    results = RaceResult.query.filter_by(race_id=race_id).order_by(RaceResult.position).all()
    return render_template('race_detail.html', race=race, results=results)

@main_bp.route('/races/<int:race_id>/start', methods=['POST'])
@login_required
def start_race(race_id):
    """Start a race"""
    race = Race.query.get_or_404(race_id)
    
    # Only start if race is in scheduled state
    if race.status == 'scheduled':
        # Update race status to prevent duplicate starts
        race.status = 'in_progress'
        race.start_time = datetime.utcnow()
        db.session.commit()
        
        try:
            # Import here to avoid circular imports
            from app.utils.serial_manager import get_serial_manager
            serial_manager = get_serial_manager()
            
            # Check if hardware is connected before attempting to start race
            if serial_manager and serial_manager.is_connected():
                # Send command to hardware to start the race - with skip_confirm=True to avoid second confirmation
                serial_manager.send_command({
                    'cmd': 'startRace',
                    'race_id': str(race_id),
                    'skip_confirm': True
                })
                flash('Race started successfully', 'success')
            else:
                flash('Race started (hardware not connected)', 'warning')
        except Exception as e:
            flash(f'Race started (hardware error: {str(e)})', 'warning')
            
        return redirect(url_for('main.race_detail', race_id=race_id))
    else:
        flash('Race cannot be started (already in progress or completed)', 'error')
        return redirect(url_for('main.race_detail', race_id=race_id))

@main_bp.route('/races/<int:race_id>/end', methods=['POST'])
@login_required
def end_race(race_id):
    """End a race"""
    race = Race.query.get_or_404(race_id)
    if race.status == 'in_progress':
        race.status = 'completed'
        db.session.commit()
        return jsonify({'status': 'success'})
    return jsonify({'status': 'error', 'message': 'Race cannot be ended'}), 400

@main_bp.route('/events/<int:event_id>/races/create', methods=['POST'])
@login_required
def create_race(event_id):
    """Create a new race for an event"""
    event = Event.query.get_or_404(event_id)
    round_number = request.form.get('round_number', type=int)
    heat_number = request.form.get('heat_number', type=int)
    
    race = Race(
        event_id=event_id,
        round_number=round_number,
        heat_number=heat_number,
        status='scheduled'
    )
    
    db.session.add(race)
    db.session.commit()
    
    flash('Race created successfully')
    return redirect(url_for('main.event_detail', event_id=event_id))

@main_bp.route('/events/<int:event_id>/schedule', methods=['GET', 'POST'])
@login_required
def schedule_races(event_id):
    """Schedule races for an event"""
    event = Event.query.get_or_404(event_id)
    
    if request.method == 'POST':
        # Get form data
        round_count = request.form.get('round_count', type=int)
        lane_count = request.form.get('lane_count', type=int)
        
        # Update event lane count
        event.lane_count = lane_count
        db.session.commit()
        
        # Get checked-in racers
        racers = Racer.query.filter_by(checked_in=True).all()
        if not racers:
            # If no racers are checked in, use all racers
            racers = Racer.query.all()
        
        if not racers:
            flash('No racers available to schedule races', 'error')
            return redirect(url_for('main.event_detail', event_id=event_id))
        
        # Generate rounds and heats
        generate_race_schedule(event, racers, round_count, lane_count)
        
        flash('Race schedule generated successfully')
        return redirect(url_for('main.event_detail', event_id=event_id))
    
    # Get all racers for the form
    racers = Racer.query.all()
    checked_in_count = Racer.query.filter_by(checked_in=True).count()
    
    return render_template('schedule_races.html', 
                           event=event, 
                           racers=racers, 
                           checked_in_count=checked_in_count)

@main_bp.route('/events/<int:event_id>/standings')
@login_required
def event_standings(event_id):
    """View standings for an event"""
    event = Event.query.get_or_404(event_id)
    standings = Standing.query.filter_by(event_id=event_id).order_by(Standing.rank).all()
    
    return render_template('event_standings.html', event=event, standings=standings)

@main_bp.route('/racers/<int:racer_id>')
@login_required
def racer_detail(racer_id):
    """Detail view for a specific racer"""
    racer = Racer.query.get_or_404(racer_id)
    results = RaceResult.query.filter_by(racer_id=racer_id).order_by(RaceResult.created_at.desc()).all()
    
    # Group results by event
    events = {}
    for result in results:
        event_id = result.race.event_id
        if event_id not in events:
            events[event_id] = {
                'event': result.race.event,
                'results': []
            }
        events[event_id]['results'].append(result)
    
    return render_template('racer_detail.html', racer=racer, events=events)

@main_bp.route('/racers/<int:racer_id>/check-in', methods=['POST'])
@login_required
def check_in_racer(racer_id):
    """Check in a racer"""
    racer = Racer.query.get_or_404(racer_id)
    racer.checked_in = True
    racer.check_in_time = datetime.utcnow()
    db.session.commit()
    
    return jsonify({'status': 'success'})

@main_bp.route('/racers/<int:racer_id>/check-out', methods=['POST'])
@login_required
def check_out_racer(racer_id):
    """Check out a racer"""
    racer = Racer.query.get_or_404(racer_id)
    racer.checked_in = False
    racer.check_in_time = None
    db.session.commit()
    
    return jsonify({'status': 'success'})

def generate_race_schedule(event, racers, round_count, lane_count):
    """Generate a race schedule for an event
    
    This function creates rounds and heats for an event based on the number of racers
    and the number of lanes available. It uses a round-robin algorithm to ensure
    each racer competes against different opponents in each round.
    
    Args:
        event: The Event object
        racers: List of Racer objects
        round_count: Number of rounds to generate
        lane_count: Number of lanes available
    """
    # Delete any existing rounds for this event
    existing_rounds = Round.query.filter_by(event_id=event.id).all()
    for round_obj in existing_rounds:
        db.session.delete(round_obj)
    db.session.commit()
    
    # Create rounds
    rounds = []
    for round_num in range(1, round_count + 1):
        round_name = "Qualifying" if round_num == 1 else f"Round {round_num}"
        if round_num == round_count:
            round_name = "Finals"
        
        new_round = Round(
            event_id=event.id,
            number=round_num,
            name=round_name,
            status='pending'
        )
        db.session.add(new_round)
        db.session.flush()  # Get the ID without committing
        rounds.append(new_round)
    
    # Calculate number of heats needed
    racer_count = len(racers)
    heats_per_round = math.ceil(racer_count / lane_count)
    
    # Shuffle racers for initial assignment
    shuffled_racers = racers.copy()
    random.shuffle(shuffled_racers)
    
    # Create heats and lane assignments for each round
    for round_idx, round_obj in enumerate(rounds):
        # For each round, we'll create a different arrangement of racers
        if round_idx > 0:
            # For subsequent rounds, rotate racers to create new matchups
            # This is a simple round-robin algorithm
            first = shuffled_racers[0]
            last = shuffled_racers[-1]
            shuffled_racers = [first] + [shuffled_racers[i] for i in range(racer_count-1, 0, -1)]
            shuffled_racers[-1] = last
        
        # Create heats for this round
        for heat_num in range(1, heats_per_round + 1):
            new_heat = Heat(
                round_id=round_obj.id,
                number=heat_num,
                status='scheduled'
            )
            db.session.add(new_heat)
            db.session.flush()  # Get the ID without committing
            
            # Assign racers to lanes in this heat
            start_idx = (heat_num - 1) * lane_count
            end_idx = min(start_idx + lane_count, racer_count)
            
            for lane_idx, racer_idx in enumerate(range(start_idx, end_idx)):
                if racer_idx < len(shuffled_racers):
                    lane = Lane(
                        heat_id=new_heat.id,
                        racer_id=shuffled_racers[racer_idx].id,
                        lane_number=lane_idx + 1
                    )
                    db.session.add(lane)
            
            # Create a Race object for this heat
            # Calculate start time based on heat number and round number
            # Each heat takes 5 minutes, with 15-minute gaps between rounds
            base_time = datetime.utcnow().replace(minute=0, second=0, microsecond=0)  # Start at the beginning of the hour
            heat_duration = 5  # minutes
            round_gap = 15  # minutes
            
            start_time = base_time + timedelta(
                minutes=(round_obj.number - 1) * (heats_per_round * heat_duration + round_gap) + 
                         (heat_num - 1) * heat_duration
            )
            
            # Create the Race object linked to this event, round, and heat
            new_race = Race(
                event_id=event.id,
                round_number=round_obj.number,
                heat_number=heat_num,
                start_time=start_time,
                status='scheduled'
            )
            db.session.add(new_race)
            db.session.flush()  # Get the ID without committing
            
            # Create RaceResult objects for each racer in this heat
            for lane_idx, racer_idx in enumerate(range(start_idx, end_idx)):
                if racer_idx < len(shuffled_racers):
                    race_result = RaceResult(
                        race_id=new_race.id,
                        racer_id=shuffled_racers[racer_idx].id,
                        lane_number=lane_idx + 1
                    )
                    db.session.add(race_result)
    
    # Commit all changes
    db.session.commit()