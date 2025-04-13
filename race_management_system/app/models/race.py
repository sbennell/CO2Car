from app import db
from datetime import datetime
import json

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
    
    # Relationships
    lanes = db.relationship('Lane', backref='heat', lazy='dynamic', cascade='all, delete-orphan')
    
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
    
    # Relationships
    races = db.relationship('Race', backref='event', lazy='dynamic')
    rounds = db.relationship('Round', backref='event', lazy='dynamic', cascade='all, delete-orphan')
    
    def __repr__(self):
        return f'<Event {self.name} ({self.date})>' 