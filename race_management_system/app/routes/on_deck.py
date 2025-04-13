from flask import Blueprint, render_template
from flask_login import login_required
from app.models.race import Event, Heat, Lane
from sqlalchemy import and_
from collections import defaultdict

on_deck_bp = Blueprint('on_deck', __name__)

@on_deck_bp.route('/on-deck')
@login_required
def on_deck():
    """Display the 'On Deck' view showing current and upcoming heats"""
    # Get the current in-progress heat
    current_heat = Heat.query.filter_by(status='in_progress').first()
    current_heat_lanes = []
    
    if current_heat:
        current_heat_lanes = Lane.query.filter_by(heat_id=current_heat.id).order_by(Lane.lane_number).all()
    
    # Get upcoming heats (scheduled heats)
    upcoming_heats = Heat.query.filter_by(status='scheduled').order_by(Heat.id).limit(5).all()
    
    # Get the next heat (first upcoming heat)
    next_heat = upcoming_heats[0] if upcoming_heats else None
    
    # Get lanes for all upcoming heats
    heat_lanes = defaultdict(list)
    if upcoming_heats:
        heat_ids = [heat.id for heat in upcoming_heats]
        all_lanes = Lane.query.filter(Lane.heat_id.in_(heat_ids)).order_by(Lane.heat_id, Lane.lane_number).all()
        
        for lane in all_lanes:
            heat_lanes[lane.heat_id].append(lane)
    
    return render_template('on_deck.html', 
                          current_heat=current_heat,
                          current_heat_lanes=current_heat_lanes,
                          upcoming_heats=upcoming_heats,
                          next_heat=next_heat,
                          heat_lanes=heat_lanes)
