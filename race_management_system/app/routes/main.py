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
    
    # Get the current in-progress heat
    current_heat = Heat.query.filter_by(status='in_progress').first()
    
    # Get upcoming heats (scheduled heats)
    next_heat = Heat.query.filter_by(status='scheduled').order_by(Heat.id).first()
    next_heat_lanes = []
    
    if next_heat:
        next_heat_lanes = Lane.query.filter_by(heat_id=next_heat.id).order_by(Lane.lane_number).all()
    
    return render_template('dashboard.html', 
                          events=events, 
                          upcoming_races=upcoming_races,
                          current_heat=current_heat,
                          next_heat=next_heat,
                          next_heat_lanes=next_heat_lanes)

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
    if race.status == 'scheduled':
        race.status = 'in_progress'
        race.start_time = datetime.utcnow()
        db.session.commit()
        return jsonify({'status': 'success'})
    return jsonify({'status': 'error', 'message': 'Race cannot be started'}), 400

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

@main_bp.route('/events/<int:event_id>/rounds')
@login_required
def event_rounds(event_id):
    """View all rounds for an event"""
    event = Event.query.get_or_404(event_id)
    rounds = Round.query.filter_by(event_id=event_id).order_by(Round.number).all()
    
    return render_template('event_rounds.html', event=event, rounds=rounds)

@main_bp.route('/rounds/<int:round_id>')
@login_required
def round_detail(round_id):
    """View details of a round including all heats"""
    round = Round.query.get_or_404(round_id)
    heats = Heat.query.filter_by(round_id=round_id).order_by(Heat.number).all()
    
    return render_template('round_detail.html', round=round, heats=heats)

@main_bp.route('/heats/<int:heat_id>')
@login_required
def heat_detail(heat_id):
    """View details of a heat including lane assignments"""
    heat = Heat.query.get_or_404(heat_id)
    lanes = Lane.query.filter_by(heat_id=heat_id).order_by(Lane.lane_number).all()
    
    return render_template('heat_detail.html', heat=heat, lanes=lanes)

@main_bp.route('/api/heats/<int:heat_id>/start', methods=['POST'])
@login_required
def start_heat(heat_id):
    """Start a heat"""
    heat = Heat.query.get_or_404(heat_id)
    
    if heat.status == 'scheduled':
        heat.status = 'in_progress'
        db.session.commit()
        return jsonify({'status': 'success', 'message': 'Heat started successfully'})
    
    return jsonify({'status': 'error', 'message': 'Heat cannot be started'}), 400

@main_bp.route('/api/heats/<int:heat_id>/complete', methods=['POST'])
@login_required
def complete_heat(heat_id):
    """Complete a heat"""
    heat = Heat.query.get_or_404(heat_id)
    
    if heat.status == 'in_progress':
        heat.status = 'completed'
        db.session.commit()
        return jsonify({'status': 'success', 'message': 'Heat completed successfully'})
    
    return jsonify({'status': 'error', 'message': 'Heat cannot be completed'}), 400

@main_bp.route('/racers/check-in/<int:racer_id>', methods=['POST'])
@login_required
def check_in_racer(racer_id):
    """Check in a racer for an event"""
    racer = Racer.query.get_or_404(racer_id)
    racer.checked_in = True
    racer.check_in_time = datetime.utcnow()
    db.session.commit()
    
    flash(f'Racer {racer.name} checked in successfully')
    return redirect(request.referrer or url_for('main.racers'))

@main_bp.route('/events/<int:event_id>/standings')
@login_required
def event_standings(event_id):
    """View standings for an event"""
    event = Event.query.get_or_404(event_id)
    
    # Update standings before displaying
    update_event_standings(event_id)
    
    # Get all standings for this event, ordered by rank
    standings = Standing.query.filter_by(event_id=event_id).order_by(Standing.rank).all()
    
    return render_template('event_standings.html', event=event, standings=standings)

@main_bp.route('/events/<int:event_id>/standings/update', methods=['POST'])
@login_required
def update_standings(event_id):
    """Update standings for an event"""
    event = Event.query.get_or_404(event_id)
    
    # Update standings
    update_event_standings(event_id)
    
    flash('Standings updated successfully')
    return redirect(url_for('main.event_standings', event_id=event_id))

@main_bp.route('/heats/<int:heat_id>/record-results', methods=['POST'])
@login_required
def record_heat_results(heat_id):
    """Record results for a heat"""
    heat = Heat.query.get_or_404(heat_id)
    
    # Get all lanes for this heat
    lanes = Lane.query.filter_by(heat_id=heat_id).all()
    
    # Process each lane result
    for lane in lanes:
        time_str = request.form.get(f'time_{lane.id}')
        if time_str and time_str.strip():
            try:
                time = float(time_str)
                
                # Check if a result already exists
                if lane.result:
                    # Update existing result
                    lane.result.time = time
                else:
                    # Create new result
                    result = RaceResult(
                        lane_id=lane.id,
                        racer_id=lane.racer_id,
                        lane_number=lane.lane_number,
                        time=time
                    )
                    db.session.add(result)
            except ValueError:
                flash(f'Invalid time format for lane {lane.lane_number}', 'error')
    
    # Update heat status
    heat.status = 'completed'
    db.session.commit()
    
    # Calculate positions and points
    calculate_heat_positions(heat_id)
    
    # Update event standings
    update_event_standings(heat.round.event_id)
    
    flash('Heat results recorded successfully')
    return redirect(url_for('main.heat_detail', heat_id=heat_id))

def calculate_heat_positions(heat_id):
    """Calculate positions and points for a heat based on race times"""
    heat = Heat.query.get_or_404(heat_id)
    
    # Get all lanes with results for this heat, ordered by time
    lanes_with_results = Lane.query.filter_by(heat_id=heat_id).join(RaceResult, Lane.id == RaceResult.lane_id)\
        .filter(RaceResult.time != None).order_by(RaceResult.time).all()
    
    # Points allocation based on position (1st = 10, 2nd = 8, 3rd = 6, 4th = 5, 5th = 4, 6th = 3, 7th = 2, 8th = 1)
    points_map = {1: 10, 2: 8, 3: 6, 4: 5, 5: 4, 6: 3, 7: 2, 8: 1}
    
    # Assign positions and points
    for position, lane in enumerate(lanes_with_results, 1):
        lane.result.position = position
        lane.result.points = points_map.get(position, 0)  # Default to 0 points if position > 8
    
    db.session.commit()

def update_event_standings(event_id):
    """Update standings for all racers in an event"""
    event = Event.query.get_or_404(event_id)
    
    # Get all racers who have participated in this event
    racers_with_results = db.session.query(Racer).join(RaceResult, Racer.id == RaceResult.racer_id)\
        .join(Lane, Lane.id == RaceResult.lane_id)\
        .join(Heat, Heat.id == Lane.heat_id)\
        .join(Round, Round.id == Heat.round_id)\
        .filter(Round.event_id == event_id).distinct().all()
    
    # If no racers have results yet, get all racers assigned to lanes in this event
    if not racers_with_results:
        racers_with_results = db.session.query(Racer).join(Lane, Racer.id == Lane.racer_id)\
            .join(Heat, Heat.id == Lane.heat_id)\
            .join(Round, Round.id == Heat.round_id)\
            .filter(Round.event_id == event_id).distinct().all()
    
    # Update or create standings for each racer
    for racer in racers_with_results:
        # Check if standing already exists
        standing = Standing.query.filter_by(event_id=event_id, racer_id=racer.id).first()
        if not standing:
            standing = Standing(event_id=event_id, racer_id=racer.id)
            db.session.add(standing)
        
        # Calculate total points
        total_points = racer.calculate_points(event_id)
        standing.total_points = total_points
        
        # Calculate best time
        best_time = racer.calculate_best_time(event_id)
        standing.best_time = best_time
        
        # Calculate race count and wins
        race_results = RaceResult.query.join(Lane).join(Heat).join(Round)\
            .filter(RaceResult.racer_id == racer.id, Round.event_id == event_id).all()
        
        standing.race_count = len(race_results)
        standing.wins = sum(1 for result in race_results if result.position == 1)
        
        # Calculate average time
        valid_times = [result.time for result in race_results if result.time is not None]
        if valid_times:
            standing.average_time = sum(valid_times) / len(valid_times)
        
        # Update timestamp
        standing.last_updated = datetime.utcnow()
    
    # Commit changes to get all standings updated
    db.session.commit()
    
    # Now calculate ranks based on points
    standings = Standing.query.filter_by(event_id=event_id).order_by(Standing.total_points.desc()).all()
    
    # Assign ranks (handle ties by giving same rank)
    current_rank = 1
    previous_points = None
    
    for i, standing in enumerate(standings):
        if previous_points is not None and standing.total_points != previous_points:
            current_rank = i + 1
        
        standing.rank = current_rank
        previous_points = standing.total_points
    
    # Commit final ranks
    db.session.commit()

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