from app import db, create_app
from sqlalchemy import text

def add_countdown_columns():
    app = create_app()
    with app.app_context():
        # Add columns directly using SQL
        try:
            db.session.execute(text("ALTER TABLE heat ADD COLUMN countdown_duration INTEGER"))
            db.session.execute(text("ALTER TABLE heat ADD COLUMN countdown_start_time DATETIME"))
            db.session.execute(text("ALTER TABLE heat ADD COLUMN countdown_paused_at DATETIME"))
            db.session.commit()
            print("Successfully added countdown columns to Heat table")
        except Exception as e:
            db.session.rollback()
            print(f"Error adding columns: {e}")

if __name__ == '__main__':
    add_countdown_columns()
