from flask import Blueprint, send_file, request, jsonify, render_template, flash, redirect, url_for
from flask_login import login_required, current_user
from app.models.race import Event, Race, Racer, RaceResult, Round, Heat, Lane, Standing
from app import db
import os
import tempfile
import xlsxwriter
from datetime import datetime
import io

export_bp = Blueprint('export', __name__)

@export_bp.route('/export/dashboard')
@login_required
def export_dashboard():
    """Export dashboard view"""
    events = Event.query.filter_by(is_archived=False).order_by(Event.date.desc()).all()
    races = Race.query.order_by(Race.created_at.desc()).limit(10).all()
    
    return render_template('export/dashboard.html', 
                          events=events,
                          races=races)

@export_bp.route('/export/event/<int:event_id>')
@login_required
def export_event_menu(event_id):
    """Show export options for a specific event"""
    event = Event.query.get_or_404(event_id)
    return render_template('export_menu.html', event=event)

@export_bp.route('/export/event/<int:event_id>/results')
@login_required
def export_event_results(event_id):
    """Export all results for an event as Excel file"""
    event = Event.query.get_or_404(event_id)
    
    # Create a BytesIO object to store the Excel file
    output = io.BytesIO()
    
    # Create a workbook and add a worksheet
    workbook = xlsxwriter.Workbook(output)
    worksheet = workbook.add_worksheet("Event Results")
    
    # Add formats
    header_format = workbook.add_format({
        'bold': True,
        'bg_color': '#4B5563',
        'font_color': 'white',
        'border': 1
    })
    
    round_header_format = workbook.add_format({
        'bold': True,
        'bg_color': '#6B7280',
        'font_color': 'white',
        'border': 1
    })
    
    cell_format = workbook.add_format({
        'border': 1
    })
    
    # Set column widths
    worksheet.set_column('A:A', 10)  # Round
    worksheet.set_column('B:B', 10)  # Heat
    worksheet.set_column('C:C', 10)  # Lane
    worksheet.set_column('D:D', 25)  # Racer
    worksheet.set_column('E:E', 10)  # Car #
    worksheet.set_column('F:F', 10)  # Time
    worksheet.set_column('G:G', 10)  # Position
    worksheet.set_column('H:H', 10)  # Points
    worksheet.set_column('I:I', 15)  # Status
    
    # Write headers
    worksheet.write('A1', 'Round', header_format)
    worksheet.write('B1', 'Heat', header_format)
    worksheet.write('C1', 'Lane', header_format)
    worksheet.write('D1', 'Racer', header_format)
    worksheet.write('E1', 'Car #', header_format)
    worksheet.write('F1', 'Time', header_format)
    worksheet.write('G1', 'Position', header_format)
    worksheet.write('H1', 'Points', header_format)
    worksheet.write('I1', 'Status', header_format)
    
    # Get all races for this event, ordered by round number and heat number
    races = Race.query.filter_by(event_id=event_id).order_by(Race.round_number, Race.heat_number).all()
    
    row = 1
    for race in races:
        # Get results for this race
        results = RaceResult.query.filter_by(race_id=race.id).order_by(RaceResult.lane_number).all()
        
        for result in results:
            worksheet.write(row, 0, race.round_number, cell_format)
            worksheet.write(row, 1, race.heat_number, cell_format)
            worksheet.write(row, 2, result.lane_number, cell_format)
            worksheet.write(row, 3, result.racer.name, cell_format)
            worksheet.write(row, 4, result.racer.car_number, cell_format)
            worksheet.write(row, 5, f"{result.time:.3f}" if result.time else "N/A", cell_format)
            worksheet.write(row, 6, result.position if result.position else "N/A", cell_format)
            worksheet.write(row, 7, result.points if result.points else 0, cell_format)
            worksheet.write(row, 8, "Disqualified" if result.disqualified else 
                          ("Completed" if race.status == 'completed' else race.status.capitalize()), 
                          cell_format)
            
            row += 1
    
    # Add a standings worksheet
    standings_worksheet = workbook.add_worksheet("Standings")
    
    # Set column widths for standings
    standings_worksheet.set_column('A:A', 10)  # Rank
    standings_worksheet.set_column('B:B', 25)  # Racer
    standings_worksheet.set_column('C:C', 10)  # Car #
    standings_worksheet.set_column('D:D', 10)  # Points
    standings_worksheet.set_column('E:E', 10)  # Best Time
    standings_worksheet.set_column('F:F', 10)  # Avg Time
    standings_worksheet.set_column('G:G', 10)  # Races
    standings_worksheet.set_column('H:H', 10)  # Wins
    
    # Write standings headers
    standings_worksheet.write('A1', 'Rank', header_format)
    standings_worksheet.write('B1', 'Racer', header_format)
    standings_worksheet.write('C1', 'Car #', header_format)
    standings_worksheet.write('D1', 'Points', header_format)
    standings_worksheet.write('E1', 'Best Time', header_format)
    standings_worksheet.write('F1', 'Avg Time', header_format)
    standings_worksheet.write('G1', 'Races', header_format)
    standings_worksheet.write('H1', 'Wins', header_format)
    
    # Get standings for this event
    standings = Standing.query.filter_by(event_id=event_id).order_by(Standing.rank).all()
    
    # Write standings data
    for i, standing in enumerate(standings):
        row = i + 1
        standings_worksheet.write(row, 0, standing.rank, cell_format)
        standings_worksheet.write(row, 1, standing.racer.name, cell_format)
        standings_worksheet.write(row, 2, standing.racer.car_number, cell_format)
        standings_worksheet.write(row, 3, standing.total_points, cell_format)
        standings_worksheet.write(row, 4, f"{standing.best_time:.3f}" if standing.best_time else "N/A", cell_format)
        standings_worksheet.write(row, 5, f"{standing.average_time:.3f}" if standing.average_time else "N/A", cell_format)
        standings_worksheet.write(row, 6, standing.race_count, cell_format)
        standings_worksheet.write(row, 7, standing.wins, cell_format)
    
    # Close the workbook
    workbook.close()
    
    # Seek to the beginning of the BytesIO object
    output.seek(0)
    
    # Create a filename with the event name and date
    filename = f"{event.name.replace(' ', '_')}_Results_{datetime.now().strftime('%Y%m%d')}.xlsx"
    
    # Return the Excel file as an attachment
    return send_file(
        output,
        as_attachment=True,
        download_name=filename,
        mimetype='application/vnd.openxmlformats-officedocument.spreadsheetml.sheet'
    )

@export_bp.route('/export/event/<int:event_id>/standings')
@login_required
def export_event_standings(event_id):
    """Export standings for an event as Excel file"""
    event = Event.query.get_or_404(event_id)
    
    # Create a BytesIO object to store the Excel file
    output = io.BytesIO()
    
    # Create a workbook and add a worksheet
    workbook = xlsxwriter.Workbook(output)
    worksheet = workbook.add_worksheet()
    
    # Add formats
    header_format = workbook.add_format({
        'bold': True,
        'bg_color': '#4B5563',
        'font_color': 'white',
        'border': 1
    })
    
    cell_format = workbook.add_format({
        'border': 1
    })
    
    # Set column widths
    worksheet.set_column('A:A', 10)  # Rank
    worksheet.set_column('B:B', 25)  # Racer
    worksheet.set_column('C:C', 10)  # Car #
    worksheet.set_column('D:D', 10)  # Points
    worksheet.set_column('E:E', 10)  # Best Time
    worksheet.set_column('F:F', 10)  # Avg Time
    worksheet.set_column('G:G', 10)  # Races
    worksheet.set_column('H:H', 10)  # Wins
    
    # Write headers
    worksheet.write('A1', 'Rank', header_format)
    worksheet.write('B1', 'Racer', header_format)
    worksheet.write('C1', 'Car #', header_format)
    worksheet.write('D1', 'Points', header_format)
    worksheet.write('E1', 'Best Time', header_format)
    worksheet.write('F1', 'Avg Time', header_format)
    worksheet.write('G1', 'Races', header_format)
    worksheet.write('H1', 'Wins', header_format)
    
    # Get standings for this event
    standings = Standing.query.filter_by(event_id=event_id).order_by(Standing.rank).all()
    
    # Write data
    for i, standing in enumerate(standings):
        row = i + 1
        worksheet.write(row, 0, standing.rank, cell_format)
        worksheet.write(row, 1, standing.racer.name, cell_format)
        worksheet.write(row, 2, standing.racer.car_number, cell_format)
        worksheet.write(row, 3, standing.total_points, cell_format)
        worksheet.write(row, 4, f"{standing.best_time:.3f}" if standing.best_time else "N/A", cell_format)
        worksheet.write(row, 5, f"{standing.average_time:.3f}" if standing.average_time else "N/A", cell_format)
        worksheet.write(row, 6, standing.race_count, cell_format)
        worksheet.write(row, 7, standing.wins, cell_format)
    
    # Close the workbook
    workbook.close()
    
    # Seek to the beginning of the BytesIO object
    output.seek(0)
    
    # Create a filename with the event name and date
    filename = f"{event.name.replace(' ', '_')}_Standings_{datetime.now().strftime('%Y%m%d')}.xlsx"
    
    # Return the Excel file as an attachment
    return send_file(
        output,
        as_attachment=True,
        download_name=filename,
        mimetype='application/vnd.openxmlformats-officedocument.spreadsheetml.sheet'
    )

@export_bp.route('/export/heat/<int:heat_id>')
@login_required
def export_heat_results(heat_id):
    """Export results for a specific heat as Excel file"""
    race = Race.query.get_or_404(heat_id)
    event = race.event
    
    # Create a BytesIO object to store the Excel file
    output = io.BytesIO()
    
    # Create a workbook and add a worksheet
    workbook = xlsxwriter.Workbook(output)
    worksheet = workbook.add_worksheet()
    
    # Add formats
    header_format = workbook.add_format({
        'bold': True,
        'bg_color': '#4B5563',
        'font_color': 'white',
        'border': 1
    })
    
    cell_format = workbook.add_format({
        'border': 1
    })
    
    # Set column widths
    worksheet.set_column('A:A', 10)  # Lane
    worksheet.set_column('B:B', 25)  # Racer
    worksheet.set_column('C:C', 10)  # Car #
    worksheet.set_column('D:D', 10)  # Time
    worksheet.set_column('E:E', 10)  # Position
    worksheet.set_column('F:F', 10)  # Points
    worksheet.set_column('G:G', 15)  # Status
    
    # Write headers
    worksheet.write('A1', 'Lane', header_format)
    worksheet.write('B1', 'Racer', header_format)
    worksheet.write('C1', 'Car #', header_format)
    worksheet.write('D1', 'Time', header_format)
    worksheet.write('E1', 'Position', header_format)
    worksheet.write('F1', 'Points', header_format)
    worksheet.write('G1', 'Status', header_format)
    
    # Get results for this race
    results = RaceResult.query.filter_by(race_id=race.id).order_by(RaceResult.lane_number).all()
    
    # Write race data
    for i, result in enumerate(results):
        row = i + 1
        
        worksheet.write(row, 0, result.lane_number, cell_format)
        worksheet.write(row, 1, result.racer.name, cell_format)
        worksheet.write(row, 2, result.racer.car_number, cell_format)
        
        worksheet.write(row, 3, f"{result.time:.3f}" if result.time else "N/A", cell_format)
        worksheet.write(row, 4, result.position if result.position else "N/A", cell_format)
        worksheet.write(row, 5, result.points if result.points else 0, cell_format)
        worksheet.write(row, 6, "Disqualified" if result.disqualified else 
                      ("Completed" if race.status == 'completed' else race.status.capitalize()), 
                      cell_format)
    
    # Close the workbook
    workbook.close()
    
    # Seek to the beginning of the BytesIO object
    output.seek(0)
    
    # Create a filename with the heat information and date
    filename = f"{event.name.replace(' ', '_')}_Heat{race.heat_number}_Results_{datetime.now().strftime('%Y%m%d')}.xlsx"
    
    # Return the Excel file as an attachment
    return send_file(
        output,
        as_attachment=True,
        download_name=filename,
        mimetype='application/vnd.openxmlformats-officedocument.spreadsheetml.sheet'
    )
