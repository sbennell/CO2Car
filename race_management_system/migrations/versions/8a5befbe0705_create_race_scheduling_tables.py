"""Create race scheduling tables

Revision ID: 8a5befbe0705
Revises: 8a5befbe0704
Create Date: 2025-04-14 07:53:27.600205

"""
from alembic import op
import sqlalchemy as sa


# revision identifiers, used by Alembic.
revision = '8a5befbe0705'
down_revision = '8a5befbe0704'
branch_labels = None
depends_on = None


def upgrade():
    # Create Round table
    op.create_table('round',
        sa.Column('id', sa.Integer(), nullable=False),
        sa.Column('event_id', sa.Integer(), nullable=True),
        sa.Column('number', sa.Integer(), nullable=True),
        sa.Column('name', sa.String(length=100), nullable=True),
        sa.Column('status', sa.String(length=20), nullable=True),
        sa.Column('created_at', sa.DateTime(), nullable=True),
        sa.ForeignKeyConstraint(['event_id'], ['event.id'], name='fk_round_event_id'),
        sa.PrimaryKeyConstraint('id')
    )
    
    # Create Heat table
    op.create_table('heat',
        sa.Column('id', sa.Integer(), nullable=False),
        sa.Column('round_id', sa.Integer(), nullable=True),
        sa.Column('number', sa.Integer(), nullable=True),
        sa.Column('status', sa.String(length=20), nullable=True),
        sa.Column('created_at', sa.DateTime(), nullable=True),
        sa.ForeignKeyConstraint(['round_id'], ['round.id'], name='fk_heat_round_id'),
        sa.PrimaryKeyConstraint('id')
    )
    
    # Create Lane table
    op.create_table('lane',
        sa.Column('id', sa.Integer(), nullable=False),
        sa.Column('heat_id', sa.Integer(), nullable=True),
        sa.Column('racer_id', sa.Integer(), nullable=True),
        sa.Column('lane_number', sa.Integer(), nullable=True),
        sa.Column('created_at', sa.DateTime(), nullable=True),
        sa.ForeignKeyConstraint(['heat_id'], ['heat.id'], name='fk_lane_heat_id'),
        sa.ForeignKeyConstraint(['racer_id'], ['racer.id'], name='fk_lane_racer_id'),
        sa.PrimaryKeyConstraint('id')
    )
    
    # Add columns to racer table
    with op.batch_alter_table('racer', schema=None) as batch_op:
        batch_op.add_column(sa.Column('checked_in', sa.Boolean(), nullable=True))
        batch_op.add_column(sa.Column('check_in_time', sa.DateTime(), nullable=True))
    
    # Update race_result table
    with op.batch_alter_table('race_result', schema=None) as batch_op:
        batch_op.add_column(sa.Column('lane_id', sa.Integer(), nullable=True))
        batch_op.add_column(sa.Column('lane_number', sa.Integer(), nullable=True))
        batch_op.add_column(sa.Column('notes', sa.Text(), nullable=True))
        batch_op.create_foreign_key('fk_race_result_lane_id', 'lane', ['lane_id'], ['id'])


def downgrade():
    # Remove foreign key and columns from race_result
    with op.batch_alter_table('race_result', schema=None) as batch_op:
        batch_op.drop_constraint('fk_race_result_lane_id', type_='foreignkey')
        batch_op.drop_column('notes')
        batch_op.drop_column('lane_number')
        batch_op.drop_column('lane_id')
    
    # Remove columns from racer
    with op.batch_alter_table('racer', schema=None) as batch_op:
        batch_op.drop_column('check_in_time')
        batch_op.drop_column('checked_in')
    
    # Drop Lane table
    op.drop_table('lane')
    
    # Drop Heat table
    op.drop_table('heat')
    
    # Drop Round table
    op.drop_table('round')
