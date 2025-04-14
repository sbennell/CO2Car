# Race Scheduling Guide: Creating Rounds and Heats

This guide explains how to create a complete race schedule with rounds and heats for your CO2 car racing event.

## Overview

The race management system allows you to automatically generate a fair race schedule that:
- Creates multiple rounds (Qualifying, Semi-Finals, Finals)
- Arranges racers into heats based on the number of available lanes
- Rotates racers between rounds to ensure everyone races against different opponents
- Tracks results and calculates standings

## Step-by-Step Guide

### Step 1: Access the Schedule Page

1. Navigate to your event detail page
2. Click on the **"Schedule Races"** button in the top-right action buttons

### Step 2: Configure the Schedule Parameters

On the Schedule Races page, you'll need to set:

- **Number of Rounds**: Typically 3 rounds works well
  - Round 1: Qualifying
  - Round 2: Semi-Finals
  - Round 3: Finals

- **Number of Lanes**: This depends on your track setup
  - Most CO2 car tracks have 2 lanes
  - The system supports up to 8 lanes

### Step 3: Check In Racers

Before generating the schedule, ensure your racers are checked in:

- On the Schedule Races page, you'll see a list of all registered racers
- Use the **"Check In"** button for each participating racer
- Only checked-in racers will be included in the schedule
- If no racers are checked in, all racers will be included by default

### Step 4: Generate the Schedule

Click the **"Generate Schedule"** button to create your race schedule.

The system will automatically:
- Create the appropriate number of rounds
- Name them appropriately (Qualifying, Finals, etc.)
- Calculate how many heats are needed based on racer count
- Assign racers to lanes within each heat
- Use a round-robin algorithm to create fair matchups

### Step 5: Review the Schedule

After generation, you'll be redirected to the event detail page showing all scheduled races. You can:

- See all races with their round number, heat number, and status
- Click **"View"** on any race to see its details
- Click **"View Rounds"** to see a breakdown by round
- Click on individual rounds to see their heats
- Click on heats to see lane assignments

## How Race Generation Works

The schedule generation uses a sophisticated algorithm:

1. **Round Creation**: Creates the specified number of rounds
2. **Heat Calculation**: Determines how many heats are needed based on racer count and lane count
3. **Initial Assignment**: Randomly assigns racers to heats in the first round
4. **Round-Robin Rotation**: For subsequent rounds, rotates racers to create different matchups
5. **Lane Assignment**: Assigns racers to specific lanes within each heat

## Race Day Operations

Once your schedule is created:

1. **Starting Races**: 
   - Navigate to a specific race
   - Click the "Start Race" button when ready
   - The system will communicate with the track hardware

2. **Recording Results**:
   - Results are automatically recorded when using electronic timing
   - Can also be manually entered if needed

3. **Viewing Standings**:
   - Click "Standings" on the event page to see current rankings
   - Points are automatically calculated based on finishing positions

## Tips for Optimal Race Scheduling

1. **Even Numbers**: Try to have an even number of racers that divides evenly by lane count
   
2. **Multiple Opportunities**: Using 3+ rounds ensures every racer has multiple opportunities to race

3. **Check-In Deadline**: Set a check-in deadline so you know exactly how many racers to include

4. **Points System**: The default points system awards:
   - 1st Place: 10 points
   - 2nd Place: 8 points
   - 3rd Place: 6 points
   - 4th Place: 5 points
   - Additional places: Decreasing points

5. **Race Time**: Allow approximately 3-5 minutes per heat for setup, racing, and recording results

## Troubleshooting

If you need to modify your schedule:

1. **Regenerating Schedule**: You can regenerate the schedule at any time, which will delete all existing rounds and create new ones
   
2. **Adding Individual Races**: You can manually add races using the "Add Race" button on the event detail page

3. **Missing Racers**: If racers arrive late, check them in and regenerate the schedule if races haven't started

## Example Schedule

For an event with 8 racers, 2 lanes, and 3 rounds, the system would create:

**Round 1: Qualifying**
- Heat 1: Racer 1 vs Racer 2
- Heat 2: Racer 3 vs Racer 4
- Heat 3: Racer 5 vs Racer 6
- Heat 4: Racer 7 vs Racer 8

**Round 2: Semi-Finals**
- Heat 1: Racer 1 vs Racer 3
- Heat 2: Racer 2 vs Racer 5
- Heat 3: Racer 4 vs Racer 7
- Heat 4: Racer 6 vs Racer 8

**Round 3: Finals**
- Heat 1: Racer 1 vs Racer 5
- Heat 2: Racer 2 vs Racer 4
- Heat 3: Racer 3 vs Racer 6
- Heat 4: Racer 7 vs Racer 8 