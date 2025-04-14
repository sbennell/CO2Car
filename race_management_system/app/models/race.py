from app import db
from datetime import datetime
import json
from sqlalchemy import func

class Racer(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(100), nullable=False)
    car_number = db.Column(db.String(20), unique=True, nullable=False)
    group = db.Column(db.String(50))
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    checked_in = db.Column(db.Boolean, default=False)
    check_in_time = db.Column(db.DateTime)
    
    # Relationships
    race_results = db.relationship('RaceResult', backref='racer', lazy='dynamic')
    lane_assignments = db.relationship('Lane', backref='racer', lazy='dynamic')
    
    def __repr__(self):
        return f'<Racer {self.name} ({self.car_number})>'
    
    def get_standing_for_event(self, event_id):
        """Get the current standing for this racer in the specified event"""
        return Standing.query.filter_by(event_id=event_id, racer_id=self.id).first()
    
    def calculate_points(self, event_id):
        """Calculate total points for this racer in the specified event"""
        from sqlalchemy import func
        # Get all race results for this racer in the event
        results = RaceResult.query.join(Race).filter(
            RaceResult.racer_id == self.id,
            Race.event_id == event_id,
            RaceResult.disqualified == False
        ).all()
        
        total_points = sum(result.points for result in results if result.points is not None)
        return total_points
    
    def calculate_best_time(self, event_id):
        """Calculate the best time for this racer in the specified event"""
        # Get the best (minimum) time for this racer in the event
        best_time_result = RaceResult.query.join(Race).filter(
            RaceResult.racer_id == self.id,
            Race.event_id == event_id,
            RaceResult.disqualified == False,
            RaceResult.time != None
        ).order_by(RaceResult.time.asc()).first()
        
        return best_time_result.time if best_time_result else None

class Round(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    event_id = db.Column(db.Integer, db.ForeignKey('event.id'))
    number = db.Column(db.Integer)
    name = db.Column(db.String(100))  # e.g., "Qualifying", "Semi-Finals", "Finals"
    status = db.Column(db.String(20), default='pending')  # pending, in_progress, completed
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    
    # Relationships
    heats = db.relationship('Heat', backref='round', lazy='dynamic', cascade='all, delete-orphan')
    
    def __repr__(self):
        return f'<Round {self.number} - {self.name}>'    

class Heat(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    round_id = db.Column(db.Integer, db.ForeignKey('round.id'))
    number = db.Column(db.Integer)
    status = db.Column(db.String(20), default='scheduled')  # scheduled, in_progress, completed, cancelled
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    countdown_duration = db.Column(db.Integer, default=300)  # Default 5 minutes (300 seconds)
    countdown_start_time = db.Column(db.DateTime, nullable=True)  # When the countdown was started
    countdown_paused_at = db.Column(db.Integer, nullable=True)  # Seconds remaining when paused
    
    # Relationships
    lanes = db.relationship('Lane', backref='heat', lazy='dynamic', cascade='all, delete-orphan')
    
    def get_countdown_status(self):
        """Get the current countdown status"""
        if self.countdown_paused_at is not None:
            # Countdown is paused
            return {
                'status': 'paused',
                'remaining': self.countdown_paused_at
            }
        
        if self.countdown_start_time is None:
            # Countdown hasn't started yet
            return {
                'status': 'ready',
                'remaining': self.countdown_duration
            }
        
        # Countdown is running
        elapsed = (datetime.utcnow() - self.countdown_start_time).total_seconds()
        remaining = max(0, self.countdown_duration - int(elapsed))
        
        if remaining <= 0:
            return {
                'status': 'completed',
                'remaining': 0
            }
        
        return {
            'status': 'running',
            'remaining': remaining
        }
    
    def start_countdown(self):
        """Start or resume the countdown"""
        if self.countdown_paused_at is not None:
            # Resume from paused state
            elapsed = self.countdown_duration - self.countdown_paused_at
            self.countdown_start_time = datetime.utcnow() - timedelta(seconds=elapsed)
            self.countdown_paused_at = None
        else:
            # Start fresh countdown
            self.countdown_start_time = datetime.utcnow()
            self.countdown_paused_at = None
    
    def pause_countdown(self):
        """Pause the countdown"""
        if self.countdown_start_time and self.countdown_paused_at is None:
            # Only pause if countdown is running
            countdown_status = self.get_countdown_status()
            self.countdown_paused_at = countdown_status['remaining']
    
    def reset_countdown(self, duration=None):
        """Reset the countdown"""
        if duration is not None:
            self.countdown_duration = duration
        self.countdown_start_time = None
        self.countdown_paused_at = None
    
    def __repr__(self):
        return f'<Heat {self.number} in Round {self.round_id}>'    

class Lane(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    heat_id = db.Column(db.Integer, db.ForeignKey('heat.id'))
    racer_id = db.Column(db.Integer, db.ForeignKey('racer.id'))
    lane_number = db.Column(db.Integer)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    
    # Relationships
    result = db.relationship('RaceResult', backref='lane', uselist=False, cascade='all, delete-orphan')
    
    def __repr__(self):
        return f'<Lane {self.lane_number} - Racer {self.racer_id}>'    

class Race(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    event_id = db.Column(db.Integer, db.ForeignKey('event.id'))
    round_number = db.Column(db.Integer)
    heat_number = db.Column(db.Integer)
    start_time = db.Column(db.DateTime)
    end_time = db.Column(db.DateTime)
    status = db.Column(db.String(20), default='scheduled')  # scheduled, in_progress, completed, cancelled
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    
    # Relationships
    results = db.relationship('RaceResult', backref='race', lazy='dynamic')
    
    def __repr__(self):
        return f'<Race {self.id} - Round {self.round_number} Heat {self.heat_number}>'

class RaceResult(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    race_id = db.Column(db.Integer, db.ForeignKey('race.id'))
    racer_id = db.Column(db.Integer, db.ForeignKey('racer.id'))
    lane_id = db.Column(db.Integer, db.ForeignKey('lane.id'))
    lane_number = db.Column(db.Integer)
    time = db.Column(db.Float)  # time in seconds
    position = db.Column(db.Integer)
    points = db.Column(db.Integer, default=0)
    disqualified = db.Column(db.Boolean, default=False)
    notes = db.Column(db.Text)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    
    def __repr__(self):
        return f'<RaceResult Race={self.race_id} Racer={self.racer_id} Time={self.time}>'

class Standing(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    event_id = db.Column(db.Integer, db.ForeignKey('event.id'))
    racer_id = db.Column(db.Integer, db.ForeignKey('racer.id'))
    total_points = db.Column(db.Integer, default=0)
    best_time = db.Column(db.Float)  # Best time in seconds
    average_time = db.Column(db.Float)  # Average time in seconds
    race_count = db.Column(db.Integer, default=0)  # Number of races completed
    wins = db.Column(db.Integer, default=0)  # Number of first place finishes
    rank = db.Column(db.Integer)  # Current rank in the event
    last_updated = db.Column(db.DateTime, default=datetime.utcnow)
    
    # Relationships
    racer = db.relationship('Racer', backref='standings')
    
    def __repr__(self):
        return f'<Standing Event={self.event_id} Racer={self.racer_id} Points={self.total_points} Rank={self.rank}>'

class Event(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(100), nullable=False)
    date = db.Column(db.Date, nullable=False)
    location = db.Column(db.String(100))
    description = db.Column(db.Text)
    status = db.Column(db.String(20), default='upcoming')  # upcoming, active, completed
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    updated_at = db.Column(db.DateTime)
    creator_id = db.Column(db.Integer, db.ForeignKey('user.id'))
    is_archived = db.Column(db.Boolean, default=False)
    archived_at = db.Column(db.DateTime)
    lane_count = db.Column(db.Integer, default=2)  # Number of lanes on the track
    points_system = db.Column(db.String(50), default='standard')  # standard, custom
    
    # Relationships
    races = db.relationship('Race', backref='event', lazy='dynamic')
    rounds = db.relationship('Round', backref='event', lazy='dynamic', cascade='all, delete-orphan')
    standings = db.relationship('Standing', backref='event', lazy='dynamic', cascade='all, delete-orphan')
    
    def __repr__(self):
        return f'<Event {self.name} ({self.date})>'
    
    def get_check_in_status(self):
        """Get the check-in status for the event"""
        racers = self.get_racers()
        total_racers = len(racers)
        checked_in = sum(1 for racer in racers if racer.checked_in)
        not_checked_in = total_racers - checked_in
        
        return {
            'total': total_racers,
            'checked_in': checked_in,
            'not_checked_in': not_checked_in,
            'percentage': round((checked_in / total_racers) * 100) if total_racers > 0 else 0
        }
    
    def get_check_in_deadline(self):
        """Get check-in deadline for this event"""
        from app.utils.config_manager import config_manager
        return config_manager.get_check_in_deadline(self.id)
    
    def set_check_in_deadline(self, deadline):
        """Set check-in deadline for this event"""
        from app.utils.config_manager import config_manager
        config_manager.set_check_in_deadline(self.id, deadline)
    
    def is_check_in_notification_sent(self):
        """Check if notification has been sent for this event"""
        from app.utils.config_manager import config_manager
        return config_manager.get_notification_status(self.id)
    
    def set_check_in_notification_sent(self, status=True):
        """Set notification status for this event"""
        from app.utils.config_manager import config_manager
        config_manager.set_notification_status(self.id, status)
        
    def is_check_in_deadline_passed(self):
        """Check if the check-in deadline has passed"""
        deadline = self.get_check_in_deadline()
        if not deadline:
            return False
        
        return datetime.utcnow() > deadline
    
    def get_racers(self):
        """Get all racers participating in this event"""
        # Get unique racers from lanes in this event's rounds and heats
        racers = set()
        for round in self.rounds:
            for heat in round.heats:
                for lane in heat.lanes:
                    if lane.racer:
                        racers.add(lane.racer)
        return list(racers)
        
    def get_not_checked_in_racers(self):
        """Get a list of racers who haven't checked in yet"""
        return [racer for racer in self.get_racers() if not racer.checked_in]
    
    def time_until_deadline(self):
        """Get the time remaining until the check-in deadline"""
        deadline = self.get_check_in_deadline()
        if not deadline:
            return None
        
        now = datetime.utcnow()
        if now > deadline:
            return 0
        
        # Return seconds until deadline
        return int((deadline - now).total_seconds())