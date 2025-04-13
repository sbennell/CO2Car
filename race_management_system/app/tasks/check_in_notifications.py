import os
import sys
import logging
from datetime import datetime, timedelta
from app import create_app, db, socketio
from app.models.race import Event, Racer
from app.utils.config_manager import config_manager

# Set up logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(sys.stdout)
    ]
)
logger = logging.getLogger('check_in_notifications')

def send_check_in_notifications():
    """Send notifications to racers who haven't checked in for upcoming events"""
    app = create_app()
    with app.app_context():
        # Get all upcoming events
        upcoming_events = Event.query.filter_by(status='upcoming').all()
        
        for event in upcoming_events:
            # Skip if notifications have already been sent
            if event.is_check_in_notification_sent():
                logger.info(f"Notifications already sent for event {event.name} (ID: {event.id})")
                continue
            
            # Get check-in deadline
            deadline = event.get_check_in_deadline()
            if not deadline:
                logger.info(f"No check-in deadline set for event {event.name} (ID: {event.id})")
                continue
            
            # Check if it's time to send notifications (24 hours before deadline)
            now = datetime.utcnow()
            notification_time = deadline - timedelta(hours=24)
            
            if now >= notification_time and not event.is_check_in_deadline_passed():
                # Get racers who haven't checked in
                not_checked_in_racers = event.get_not_checked_in_racers()
                
                if not not_checked_in_racers:
                    logger.info(f"All racers already checked in for event {event.name} (ID: {event.id})")
                    continue
                
                logger.info(f"Sending notifications to {len(not_checked_in_racers)} racers for event {event.name} (ID: {event.id})")
                
                # Send notifications via WebSocket
                for racer in not_checked_in_racers:
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
                    with app.test_request_context('/'):
                        socketio.emit('notification', notification_data)
                
                # Mark notifications as sent
                event.set_check_in_notification_sent(True)
                logger.info(f"Notifications sent successfully for event {event.name} (ID: {event.id})")

if __name__ == '__main__':
    send_check_in_notifications()
