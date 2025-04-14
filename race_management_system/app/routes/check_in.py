from flask import Blueprint, render_template, redirect, url_for, flash, request, jsonify
from flask_login import login_required, current_user
from app.models.race import Event, Racer
from app import db, socketio
from datetime import datetime, timedelta
import pytz

check_in_bp = Blueprint('check_in', __name__)

@check_in_bp.route('/check-in/dashboard')
@login_required
def check_in_dashboard():
    """Display check-in dashboard with all events and their check-in status"""
    events = Event.query.order_by(Event.date.desc()).all()
    
    # Get check-in status for each event
    event_statuses = {}
    for event in events:
        event_statuses[event.id] = event.get_check_in_status()
    
    return render_template('check_in/dashboard.html', 
                          events=events,
                          event_statuses=event_statuses)

@check_in_bp.route('/events/<int:event_id>/check-in-management')
@login_required
def check_in_management(event_id):
    """Manage check-in deadlines and notifications for an event"""
    event = Event.query.get_or_404(event_id)
    racers = Racer.query.all()
    checked_in_count = Racer.query.filter_by(checked_in=True).count()
    
    # Get check-in status
    check_in_status = event.get_check_in_status()
    
    # Calculate time remaining until deadline
    time_remaining = event.time_until_deadline()
    
    return render_template('check_in_management.html', 
                          event=event, 
                          racers=racers, 
                          checked_in_count=checked_in_count,
                          check_in_status=check_in_status,
                          time_remaining=time_remaining)

@check_in_bp.route('/events/<int:event_id>/set-deadline', methods=['POST'])
@login_required
def set_check_in_deadline(event_id):
    """Set the check-in deadline for an event"""
    event = Event.query.get_or_404(event_id)
    
    # Get deadline from form
    deadline_date = request.form.get('deadline_date')
    deadline_time = request.form.get('deadline_time')
    
    if not deadline_date or not deadline_time:
        flash('Please provide both date and time for the deadline', 'danger')
        return redirect(url_for('check_in.check_in_management', event_id=event_id))
    
    # Combine date and time
    deadline_str = f"{deadline_date} {deadline_time}"
    try:
        # Parse the datetime
        deadline = datetime.strptime(deadline_str, '%Y-%m-%d %H:%M')
        
        # Update the event
        event.set_check_in_deadline(deadline)
        event.set_check_in_notification_sent(False)  # Reset notification status
        
        flash('Check-in deadline set successfully', 'success')
    except ValueError:
        flash('Invalid date or time format', 'danger')
    
    return redirect(url_for('check_in.check_in_management', event_id=event_id))

@check_in_bp.route('/events/<int:event_id>/send-notifications', methods=['POST'])
@login_required
def send_check_in_notifications(event_id):
    """Send notifications to racers who haven't checked in yet"""
    event = Event.query.get_or_404(event_id)
    
    # Get racers who haven't checked in
    not_checked_in_racers = event.get_not_checked_in_racers()
    
    if not not_checked_in_racers:
        flash('All racers have already checked in', 'info')
        return redirect(url_for('check_in.check_in_management', event_id=event_id))
    
    # Send notifications via WebSocket
    for racer in not_checked_in_racers:
        deadline = event.get_check_in_deadline()
        notification_data = {
            'type': 'check_in_reminder',
            'event_id': event.id,
            'event_name': event.name,
            'racer_id': racer.id,
            'racer_name': racer.name,
            'message': f"Please check in for {event.name}",
            'deadline': deadline.strftime('%Y-%m-%d %H:%M') if deadline else None
        }
        
        # Emit notification to all connected clients
        socketio.emit('notification', notification_data)
    
    # Mark notifications as sent
    event.set_check_in_notification_sent(True)
    
    flash(f'Notifications sent to {len(not_checked_in_racers)} racers', 'success')
    return redirect(url_for('check_in.check_in_management', event_id=event_id))

@check_in_bp.route('/events/<int:event_id>/check-in-status')
@login_required
def check_in_status(event_id):
    """Get the current check-in status for an event"""
    event = Event.query.get_or_404(event_id)
    
    # Get check-in status
    status = event.get_check_in_status()
    
    # Add deadline information
    deadline = event.get_check_in_deadline()
    status['deadline'] = deadline.isoformat() if deadline else None
    status['deadline_passed'] = event.is_check_in_deadline_passed()
    status['time_remaining'] = event.time_until_deadline()
    
    return jsonify(status)

@check_in_bp.route('/racers/check-in/<int:racer_id>', methods=['POST'])
@login_required
def check_in_racer(racer_id):
    """Check in a racer"""
    racer = Racer.query.get_or_404(racer_id)
    racer.checked_in = True
    racer.check_in_time = datetime.utcnow()
    db.session.commit()
    
    # Emit check-in event via WebSocket
    socketio.emit('racer_checked_in', {
        'racer_id': racer.id,
        'racer_name': racer.name,
        'check_in_time': racer.check_in_time.isoformat() if racer.check_in_time else None
    })
    
    flash(f'Racer {racer.name} checked in successfully', 'success')
    return redirect(request.referrer or url_for('check_in.check_in_management', event_id=1))

@check_in_bp.route('/racers/check-in-bulk', methods=['POST'])
@login_required
def check_in_bulk():
    """Check in multiple racers at once"""
    racer_ids = request.form.getlist('racer_ids')
    
    if not racer_ids:
        flash('No racers selected', 'warning')
        return redirect(request.referrer)
    
    count = 0
    for racer_id in racer_ids:
        racer = Racer.query.get(int(racer_id))
        if racer and not racer.checked_in:
            racer.checked_in = True
            racer.check_in_time = datetime.utcnow()
            count += 1
    
    db.session.commit()
    
    flash(f'{count} racers checked in successfully', 'success')
    return redirect(request.referrer)

@check_in_bp.route('/racers/reset-check-in/<int:event_id>', methods=['POST'])
@login_required
def reset_check_in(event_id):
    """Reset check-in status for all racers in an event"""
    # Reset all racers' check-in status
    racers = Racer.query.all()
    for racer in racers:
        racer.checked_in = False
        racer.check_in_time = None
    
    # Reset event notification status
    event = Event.query.get_or_404(event_id)
    event.set_check_in_notification_sent(False)
    
    db.session.commit()
    
    flash('Check-in status reset for all racers', 'success')
    return redirect(url_for('check_in.check_in_management', event_id=event_id))
