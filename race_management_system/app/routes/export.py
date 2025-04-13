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

@export_bp.route('/export/event/<int:event_id>')
@login_required
def export_event_menu(event_id):
    """Display export options for an event"""
    event = Event.query.get_or_404(event_id)
    return render_template('export_menu.html', event=event)

@export_bp.route('/export/event/<int:event_id>/results')
@login_required
def export_event_results(event_id):
    """Export all race results for an event as Excel file"""
    event = Event.query.get_or_404(event_id)
    
    # Create a BytesIO object to store the Excel file
    output = io.BytesIO()
    
    # Create a workbook and add a worksheet
    workbook = xlsxwriter.Workbook(output)
    
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
    
    # Create a worksheet for event info
    info_sheet = workbook.add_worksheet("Event Info")
    info_sheet.set_column('A:A', 20)
    info_sheet.set_column('B:B', 30)
    
    # Write event information
    info_sheet.write('A1', 'Event Name', header_format)
    info_sheet.write('B1', event.name, cell_format)
    info_sheet.write('A2', 'Date', header_format)
    info_sheet.write('B2', event.date.strftime('%Y-%m-%d'), cell_format)
    info_sheet.write('A3', 'Location', header_format)
    info_sheet.write('B3', event.location or 'N/A', cell_format)
    info_sheet.write('A4', 'Status', header_format)
    info_sheet.write('B4', event.status.capitalize(), cell_format)
    info_sheet.write('A5', 'Lane Count', header_format)
    info_sheet.write('B5', event.lane_count, cell_format)
    info_sheet.write('A6', 'Points System', header_format)
    info_sheet.write('B6', event.points_system.capitalize(), cell_format)
    info_sheet.write('A7', 'Export Date', header_format)
    info_sheet.write('B7', datetime.now().strftime('%Y-%m-%d %H:%M:%S'), cell_format)
    
    # Create standings worksheet
    standings_sheet = workbook.add_worksheet("Standings")
    standings_sheet.set_column('A:A', 5)  # Rank
    standings_sheet.set_column('B:B', 25)  # Racer Name
    standings_sheet.set_column('C:C', 10)  # Car Number
    standings_sheet.set_column('D:D', 10)  # Points
    standings_sheet.set_column('E:E', 10)  # Best Time
    standings_sheet.set_column('F:F', 10)  # Avg Time
    standings_sheet.set_column('G:G', 10)  # Races
    standings_sheet.set_column('H:H', 10)  # Wins
    
    # Write standings headers
    standings_sheet.write('A1', 'Rank', header_format)
    standings_sheet.write('B1', 'Racer', header_format)
    standings_sheet.write('C1', 'Car #', header_format)
    standings_sheet.write('D1', 'Points', header_format)
    standings_sheet.write('E1', 'Best Time', header_format)
    standings_sheet.write('F1', 'Avg Time', header_format)
    standings_sheet.write('G1', 'Races', header_format)
    standings_sheet.write('H1', 'Wins', header_format)
    
    # Get standings data
    standings = Standing.query.filter_by(event_id=event_id).order_by(Standing.rank).all()
    
    # Write standings data
    for i, standing in enumerate(standings):
        row = i + 1
        standings_sheet.write(row, 0, standing.rank, cell_format)
        standings_sheet.write(row, 1, standing.racer.name, cell_format)
        standings_sheet.write(row, 2, standing.racer.car_number, cell_format)
        standings_sheet.write(row, 3, standing.total_points, cell_format)
        standings_sheet.write(row, 4, f"{standing.best_time:.3f}" if standing.best_time else "N/A", cell_format)
        standings_sheet.write(row, 5, f"{standing.average_time:.3f}" if standing.average_time else "N/A", cell_format)
        standings_sheet.write(row, 6, standing.race_count, cell_format)
        standings_sheet.write(row, 7, standing.wins, cell_format)
    
    # Create a worksheet for each round
    rounds = Round.query.filter_by(event_id=event_id).order_by(Round.number).all()
    
    for round in rounds:
        round_sheet = workbook.add_worksheet(f"Round {round.number}")
        round_sheet.set_column('A:A', 10)  # Heat
        round_sheet.set_column('B:B', 10)  # Lane
        round_sheet.set_column('C:C', 25)  # Racer
        round_sheet.set_column('D:D', 10)  # Car #
        round_sheet.set_column('E:E', 10)  # Time
        round_sheet.set_column('F:F', 10)  # Position
        round_sheet.set_column('G:G', 10)  # Points
        round_sheet.set_column('H:H', 15)  # Status
        
        # Write headers
        round_sheet.write('A1', 'Heat', header_format)
        round_sheet.write('B1', 'Lane', header_format)
        round_sheet.write('C1', 'Racer', header_format)
        round_sheet.write('D1', 'Car #', header_format)
        round_sheet.write('E1', 'Time', header_format)
        round_sheet.write('F1', 'Position', header_format)
        round_sheet.write('G1', 'Points', header_format)
        round_sheet.write('H1', 'Status', header_format)
        
        # Get heats for this round
        heats = Heat.query.filter_by(round_id=round.id).order_by(Heat.number).all()
        
        row = 1
        for heat in heats:
            # Get lanes for this heat
            lanes = Lane.query.filter_by(heat_id=heat.id).order_by(Lane.lane_number).all()
            
            for lane in lanes:
                result = lane.result
                
                round_sheet.write(row, 0, heat.number, cell_format)
                round_sheet.write(row, 1, lane.lane_number, cell_format)
                round_sheet.write(row, 2, lane.racer.name, cell_format)
                round_sheet.write(row, 3, lane.racer.car_number, cell_format)
                
                if result:
                    round_sheet.write(row, 4, f"{result.time:.3f}" if result.time else "N/A", cell_format)
                    round_sheet.write(row, 5, result.position if result.position else "N/A", cell_format)
                    round_sheet.write(row, 6, result.points if result.points else 0, cell_format)
                    round_sheet.write(row, 7, "Disqualified" if result.disqualified else "Completed", cell_format)
                else:
                    round_sheet.write(row, 4, "N/A", cell_format)
                    round_sheet.write(row, 5, "N/A", cell_format)
                    round_sheet.write(row, 6, 0, cell_format)
                    round_sheet.write(row, 7, "No Result", cell_format)
                
                row += 1
    
    # Create a worksheet for racers
    racers_sheet = workbook.add_worksheet("Racers")
    racers_sheet.set_column('A:A', 25)  # Name
    racers_sheet.set_column('B:B', 10)  # Car #
    racers_sheet.set_column('C:C', 15)  # Group
    racers_sheet.set_column('D:D', 15)  # Check-in Status
    
    # Write headers
    racers_sheet.write('A1', 'Name', header_format)
    racers_sheet.write('B1', 'Car #', header_format)
    racers_sheet.write('C1', 'Group', header_format)
    racers_sheet.write('D1', 'Check-in Status', header_format)
    
    # Get all racers who participated in this event
    racer_ids = set()
    for standing in standings:
        racer_ids.add(standing.racer_id)
    
    racers = Racer.query.filter(Racer.id.in_(racer_ids)).order_by(Racer.name).all()
    
    # Write racer data
    for i, racer in enumerate(racers):
        row = i + 1
        racers_sheet.write(row, 0, racer.name, cell_format)
        racers_sheet.write(row, 1, racer.car_number, cell_format)
        racers_sheet.write(row, 2, racer.group or "N/A", cell_format)
        racers_sheet.write(row, 3, "Checked In" if racer.checked_in else "Not Checked In", cell_format)
    
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
    worksheet.set_column('A:A', 5)  # Rank
    worksheet.set_column('B:B', 25)  # Racer Name
    worksheet.set_column('C:C', 10)  # Car Number
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
    
    # Get standings data
    standings = Standing.query.filter_by(event_id=event_id).order_by(Standing.rank).all()
    
    # Write standings data
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

@export_bp.route('/export/round/<int:round_id>')
@login_required
def export_round_results(round_id):
    """Export results for a specific round as Excel file"""
    round = Round.query.get_or_404(round_id)
    event = round.event
    
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
    worksheet.set_column('A:A', 10)  # Heat
    worksheet.set_column('B:B', 10)  # Lane
    worksheet.set_column('C:C', 25)  # Racer
    worksheet.set_column('D:D', 10)  # Car #
    worksheet.set_column('E:E', 10)  # Time
    worksheet.set_column('F:F', 10)  # Position
    worksheet.set_column('G:G', 10)  # Points
    worksheet.set_column('H:H', 15)  # Status
    
    # Write headers
    worksheet.write('A1', 'Heat', header_format)
    worksheet.write('B1', 'Lane', header_format)
    worksheet.write('C1', 'Racer', header_format)
    worksheet.write('D1', 'Car #', header_format)
    worksheet.write('E1', 'Time', header_format)
    worksheet.write('F1', 'Position', header_format)
    worksheet.write('G1', 'Points', header_format)
    worksheet.write('H1', 'Status', header_format)
    
    # Get heats for this round
    heats = Heat.query.filter_by(round_id=round.id).order_by(Heat.number).all()
    
    row = 1
    for heat in heats:
        # Get lanes for this heat
        lanes = Lane.query.filter_by(heat_id=heat.id).order_by(Lane.lane_number).all()
        
        for lane in lanes:
            result = lane.result
            
            worksheet.write(row, 0, heat.number, cell_format)
            worksheet.write(row, 1, lane.lane_number, cell_format)
            worksheet.write(row, 2, lane.racer.name, cell_format)
            worksheet.write(row, 3, lane.racer.car_number, cell_format)
            
            if result:
                worksheet.write(row, 4, f"{result.time:.3f}" if result.time else "N/A", cell_format)
                worksheet.write(row, 5, result.position if result.position else "N/A", cell_format)
                worksheet.write(row, 6, result.points if result.points else 0, cell_format)
                worksheet.write(row, 7, "Disqualified" if result.disqualified else "Completed", cell_format)
            else:
                worksheet.write(row, 4, "N/A", cell_format)
                worksheet.write(row, 5, "N/A", cell_format)
                worksheet.write(row, 6, 0, cell_format)
                worksheet.write(row, 7, "No Result", cell_format)
            
            row += 1
    
    # Close the workbook
    workbook.close()
    
    # Seek to the beginning of the BytesIO object
    output.seek(0)
    
    # Create a filename with the round name and date
    filename = f"{event.name.replace(' ', '_')}_{round.name.replace(' ', '_')}_Results_{datetime.now().strftime('%Y%m%d')}.xlsx"
    
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
    heat = Heat.query.get_or_404(heat_id)
    round = heat.round
    event = round.event
    
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
    
    # Get lanes for this heat
    lanes = Lane.query.filter_by(heat_id=heat.id).order_by(Lane.lane_number).all()
    
    # Write lane data
    for i, lane in enumerate(lanes):
        row = i + 1
        result = lane.result
        
        worksheet.write(row, 0, lane.lane_number, cell_format)
        worksheet.write(row, 1, lane.racer.name, cell_format)
        worksheet.write(row, 2, lane.racer.car_number, cell_format)
        
        if result:
            worksheet.write(row, 3, f"{result.time:.3f}" if result.time else "N/A", cell_format)
            worksheet.write(row, 4, result.position if result.position else "N/A", cell_format)
            worksheet.write(row, 5, result.points if result.points else 0, cell_format)
            worksheet.write(row, 6, "Disqualified" if result.disqualified else "Completed", cell_format)
        else:
            worksheet.write(row, 3, "N/A", cell_format)
            worksheet.write(row, 4, "N/A", cell_format)
            worksheet.write(row, 5, 0, cell_format)
            worksheet.write(row, 6, "No Result", cell_format)
    
    # Close the workbook
    workbook.close()
    
    # Seek to the beginning of the BytesIO object
    output.seek(0)
    
    # Create a filename with the heat information and date
    filename = f"{event.name.replace(' ', '_')}_{round.name.replace(' ', '_')}_Heat{heat.number}_Results_{datetime.now().strftime('%Y%m%d')}.xlsx"
    
    # Return the Excel file as an attachment
    return send_file(
        output,
        as_attachment=True,
        download_name=filename,
        mimetype='application/vnd.openxmlformats-officedocument.spreadsheetml.sheet'
    )
