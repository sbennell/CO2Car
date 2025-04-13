from flask import Blueprint, render_template, redirect, url_for, flash, request, jsonify
from flask_login import login_required, current_user
from app.models.race import Event, Race, Racer, RaceResult
from app import db
from datetime import datetime

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
    return render_template('dashboard.html', events=events, upcoming_races=upcoming_races)

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