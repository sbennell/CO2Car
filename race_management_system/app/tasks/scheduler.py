import os
import time
import logging
import threading
import schedule
from datetime import datetime
from app.tasks.check_in_notifications import send_check_in_notifications

# Set up logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('scheduler')

def run_threaded(job_func):
    """Run a scheduled job in a separate thread"""
    job_thread = threading.Thread(target=job_func)
    job_thread.start()

def initialize_scheduler():
    """Initialize the scheduler with all scheduled tasks"""
    # Schedule check-in notifications to run every hour
    schedule.every(1).hours.do(run_threaded, send_check_in_notifications)
    
    logger.info("Scheduler initialized with the following jobs:")
    for job in schedule.get_jobs():
        logger.info(f"- {job}")

def run_scheduler():
    """Run the scheduler in the background"""
    initialize_scheduler()
    
    logger.info("Starting scheduler...")
    
    # Run the scheduler continuously
    while True:
        schedule.run_pending()
        time.sleep(60)  # Check for pending jobs every minute

if __name__ == "__main__":
    run_scheduler()
