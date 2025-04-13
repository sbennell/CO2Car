"""Add standings and points_system

Revision ID: 8a5befbe0706
Revises: 8a5befbe0705
Create Date: 2025-04-14 08:15:00.000000

"""
from alembic import op
import sqlalchemy as sa


# revision identifiers, used by Alembic.
revision = '8a5befbe0706'
down_revision = '8a5befbe0705'
branch_labels = None
depends_on = None


def upgrade():
    # Add points_system column to event table
    op.add_column('event', sa.Column('points_system', sa.String(length=50), nullable=True, server_default='standard'))
    
    # Create standing table
    op.create_table('standing',
        sa.Column('id', sa.Integer(), nullable=False),
        sa.Column('event_id', sa.Integer(), nullable=True),
        sa.Column('racer_id', sa.Integer(), nullable=True),
        sa.Column('total_points', sa.Integer(), nullable=True, default=0),
        sa.Column('best_time', sa.Float(), nullable=True),
        sa.Column('average_time', sa.Float(), nullable=True),
        sa.Column('race_count', sa.Integer(), nullable=True, default=0),
        sa.Column('wins', sa.Integer(), nullable=True, default=0),
        sa.Column('rank', sa.Integer(), nullable=True),
        sa.Column('last_updated', sa.DateTime(), nullable=True),
        sa.ForeignKeyConstraint(['event_id'], ['event.id'], ),
        sa.ForeignKeyConstraint(['racer_id'], ['racer.id'], ),
        sa.PrimaryKeyConstraint('id')
    )


def downgrade():
    # Drop standing table
    op.drop_table('standing')
    
    # Remove points_system column from event table
    op.drop_column('event', 'points_system')
