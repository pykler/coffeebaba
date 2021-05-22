import os, sys
import time
import json
import http
import argparse
import asyncio
import websockets

import random

WEBDIR = os.path.join(
    os.path.abspath(os.path.dirname(__file__)),
    '..', '..', 'lib', 'coffeebaba_web', 'src', 'html',
)

beanTemp_min = 21  # mock ambient temp
start = time.time()

async def process_request(path, request_headers):
    if path == "/index.html":
        with open(f'{WEBDIR}/index.html', 'rb') as f:
            return (
                http.HTTPStatus.OK,
                [('Content-Type', 'text/html')],
                f.read(),
            )
    if path == "/admin":
        return (
            http.HTTPStatus.OK,
            [('Content-Type', 'application/json')],
            json.dumps({
                'stats': {
                    'heap': 102400, # bytes
                    'sketch_size': 102400,
                    'sketch_free': 102400,
                    'uptime': (time.time() - start) / 60, # mints
                    'last_reboot': 'normal reboot',
                },
                'chip': {
                    'cpu_mhz': 10,
                    'id': 10,
                    'sdk': 'sdk_version',
                    'version': 'some_version_str',
                },
            }).encode('utf-8'),
        )

async def echo(websocket, path):
    # global vars
    burner = 0
    beanTemp = beanTemp_min
    time_last = time.time()
    beanTemp_delta = 2 # degress / second
    print(f'Request for {path} from {websocket.remote_address}')
    async for message in websocket:
        message = json.loads(message)
        time_delta, time_last = time.time() - time_last, time.time()
        print('artisan:', message)
        r_message = {'id': message['id']}
        if message['command'] == 'setControlParams':
            burner = message['params']['burner']
        temp_delta = beanTemp_delta * time_delta
        if burner == 0:
            temp_delta = -temp_delta
        beanTemp += temp_delta
        beanTemp = max(beanTemp, beanTemp_min)
        r_message['data'] = {
            'beanTemp': beanTemp,
            'burner': burner,
        }
        print('response:', r_message)
        await websocket.send(json.dumps(r_message))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', default='0.0.0.0')
    parser.add_argument('--port', type=int, default=8080)
    args = parser.parse_args()
    print(f'Listening on ws://{args.host}:{args.port} ...')
    asyncio.get_event_loop().run_until_complete(
        websockets.serve(
            echo, args.host, args.port, process_request=process_request,
        )
    )
    asyncio.get_event_loop().run_forever()

if __name__ == '__main__':
    main()
