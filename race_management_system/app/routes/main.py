from flask import Blueprint, render_template, redirect, url_for, flash, request
from flask_login import login_required, current_user
from app.models.race import Event, Race, Racer, RaceResult
from app import db

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
    all_events = Event.query.order_by(Event.date.desc()).all()
    return render_template('events.html', events=all_events)

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

@main_bp.route('/races/<int:race_id>')
@login_required
def race_detail(race_id):
    """Detail view for a specific race"""
    race = Race.query.get_or_404(race_id)
    results = RaceResult.query.filter_by(race_id=race_id).order_by(RaceResult.position).all()
    return render_template('race_detail.html', race=race, results=results) 