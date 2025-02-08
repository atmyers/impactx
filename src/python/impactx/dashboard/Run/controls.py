import asyncio
import io

from wurlitzer import pipes

from ..simulation import run_simulation
from ..trame_setup import setup_server

server, state, ctrl = setup_server()


def execute_impactx_sim():
    buf = io.StringIO()

    with pipes(stdout=buf, stderr=buf):
        run_simulation()

    buf.seek(0)
    lines = [line.strip() for line in buf.getvalue().splitlines()]

    # Use $nextTick to ensure the terminal is fully rendered before printing
    async def print_lines():
        for line in lines:
            ctrl.terminal_print(line)
        ctrl.terminal_print("Simulation complete.")

    asyncio.create_task(print_lines())
