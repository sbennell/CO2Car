from app import db
from datetime import datetime
import json

class Racer(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(100), nullable=False)
    car_number = db.Column(db.String(20), unique=True, nullable=False)
    group = db.Column(db.String(50))
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    
    # Relationships
    race_results = db.relationship('RaceResult', backref='racer', lazy='dynamic')
    
    def __repr__(self):
        return f'<Racer {self.name} ({self.car_number})>'

class Race(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    event_id = db.Column(db.Integer, db.ForeignKey('event.id'))
    round_number = db.Column(db.Integer)
    heat_number = db.Column(db.Integer)
    start_time = db.Column(db.DateTime)
    end_time = db.Column(db.DateTime)
    status = db.Column(db.String(20), default='scheduled')  # scheduled, completed, cancelled
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    
    # Relationships
    results = db.relationship('RaceResult', backref='race', lazy='dynamic')
    
    def __repr__(self):
        return f'<Race {self.id} - Round {self.round_number} Heat {self.heat_number}>'

class RaceResult(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    race_id = db.Column(db.Integer, db.ForeignKey('race.id'))
    racer_id = db.Column(db.Integer, db.ForeignKey('racer.id'))
    lane = db.Column(db.Integer)
    time = db.Column(db.Float)  # time in seconds
    position = db.Column(db.Integer)
    points = db.Column(db.Integer, default=0)
    disqualified = db.Column(db.Boolean, default=False)
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
    
    # Relationships
    races = db.relationship('Race', backref='event', lazy='dynamic')
    
    def __repr__(self):
        return f'<Event {self.name} ({self.date})>' 