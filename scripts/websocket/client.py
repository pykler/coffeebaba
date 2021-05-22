import sys
import json
import time
import random
import asyncio
import websockets

g = {'i': 0}

async def hello(uri):
    async with websockets.connect(uri) as websocket:
        while True:
            g['i'] += 1
            request = {'id': g['i'], 'command': 'getData'}
            if random.randint(0,1):
                request.update({
                    'command': 'setControlParams',
                    'params': {'burner': random.randint(0,1)}
                })

            request = json.dumps(request)
            print(f'> {request}')
            await websocket.send(request)

            response = await websocket.recv()
            response = json.loads(response)
            print(f'< {response}')
            time.sleep(5)


if len(sys.argv) < 2:
    print(f'usage: {sys.argv[0]} IP_ADDRESS:PORT')
    exit(1)

ws_path = f'ws://{sys.argv[1]}/WebSocket'
print(f'connecting to {ws_path}')
asyncio.get_event_loop().run_until_complete(
    hello(ws_path))
