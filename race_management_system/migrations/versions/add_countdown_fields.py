"""Add countdown fields to Heat model

Revision ID: b148e5db0aac
Revises: a148e5db0aab
Create Date: 2025-04-14 09:08:00.000000

"""
from alembic import op
import sqlalchemy as sa


# revision identifiers, used by Alembic.
revision = 'b148e5db0aac'
down_revision = 'a148e5db0aab'
branch_labels = None
depends_on = None


def upgrade():
    # Add countdown fields to Heat table
    with op.batch_alter_table('heat', schema=None) as batch_op:
        batch_op.add_column(sa.Column('countdown_duration', sa.Integer(), nullable=True))
        batch_op.add_column(sa.Column('countdown_start_time', sa.DateTime(), nullable=True))
        batch_op.add_column(sa.Column('countdown_paused_at', sa.DateTime(), nullable=True))


def downgrade():
    # Remove countdown fields from Heat table
    with op.batch_alter_table('heat', schema=None) as batch_op:
        batch_op.drop_column('countdown_paused_at')
        batch_op.drop_column('countdown_start_time')
        batch_op.drop_column('countdown_duration')
