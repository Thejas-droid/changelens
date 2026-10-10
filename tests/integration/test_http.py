#!/usr/bin/env python3
"""End-to-end API regression tests using an isolated, trusted repository."""
import http.client
import json
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time

binary = str(Path(sys.argv[1] if len(sys.argv) > 1 else './build/testgraph-server').resolve())
token = 'test-only-token-that-is-at-least-32-characters'
with tempfile.TemporaryDirectory(prefix='testgraph-http-') as temp:
    root = Path(temp)
    (root / 'src').mkdir()
    (root / 'tests').mkdir()
    (root / 'src/payment.hpp').write_text('#pragma once\nint total();\n')
    (root / 'tests/payment_test.cpp').write_text('#include "../src/payment.hpp"\nTEST(Payment, Total) {}\n')
    (root / 'tests/other_test.cpp').write_text('TEST(Other, Works) {}\n')
    (root / 'testgraph.yml').write_text('project:\n  name: api-fixture\nfallback:\n  fullSuiteWhenChanged:\n    - "CMakeLists.txt"\n')
    # An external symlink must never become an analyzed source file.
    (root / 'outside.cpp').symlink_to('/etc/passwd')
    (root / 'nested/node_modules').mkdir(parents=True)
    (root / 'nested/node_modules/ignore.cpp').write_text('int ignored;')
    with socket.socket() as reserve:
        reserve.bind(('127.0.0.1', 0))
        port = reserve.getsockname()[1]
    env = {**os.environ, 'TESTGRAPH_HOST': '127.0.0.1', 'TESTGRAPH_PORT': str(port),
           'TESTGRAPH_REPOSITORY_PATH': str(root), 'TESTGRAPH_API_TOKEN': token}
    env.pop('TESTGRAPH_API_TOKEN_FILE', None)
    process = subprocess.Popen([binary], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    def request(method, path, body=None, authenticated=True):
        connection = http.client.HTTPConnection('127.0.0.1', port, timeout=10)
        headers = {'Content-Type': 'text/plain'}
        if authenticated:
            headers['Authorization'] = 'Bearer ' + token
        connection.request(method, path, body, headers)
        response = connection.getresponse()
        payload = json.loads(response.read())
        connection.close()
        return response.status, payload
    def raw_request(payload):
        with socket.create_connection(('127.0.0.1', port), timeout=10) as connection:
            connection.sendall(payload)
            connection.shutdown(socket.SHUT_WR)
            response = b''
            while True:
                try:
                    chunk = connection.recv(4096)
                except ConnectionResetError:
                    # Rejection may close a socket with unread oversized input.
                    assert response, "server reset before sending an HTTP response"
                    break
                if not chunk:
                    break
                response += chunk
            return int(response.split(b' ')[1])
    try:
        for _ in range(100):
            if process.poll() is not None:
                raise RuntimeError(process.stderr.read().decode())
            try:
                if request('GET', '/health/ready', authenticated=False)[0] == 200:
                    break
            except OSError:
                time.sleep(.05)
        else:
            raise RuntimeError('server did not become ready')
        assert request('GET', '/api/v1/workspace', authenticated=False)[0] == 401
        status, workspace = request('GET', '/api/v1/workspace')
        assert status == 200 and workspace['repository'] == 'api-fixture'
        assert len(workspace['tests']) == 2
        assert not any(n['path'] == 'outside.cpp' or 'node_modules' in n['path'] for n in workspace['nodes'])
        status, impact = request('POST', '/api/v1/impact', 'src/payment.hpp')
        assert status == 200 and impact['selectedTests'] == 1 and not impact['fallback']
        selected = [t for t in impact['tests'] if t['selected']]
        assert selected[0]['path'] == ['tests/payment_test.cpp', 'src/payment.hpp']
        assert request('POST', '/api/v1/impact', 'CMakeLists.txt')[1]['fallback']
        assert request('POST', '/api/v1/impact', 'unknown.hpp')[1]['fallback']
        for invalid in ['', '../outside.cpp', '/etc/passwd', 'src\\payment.hpp', 'a\x00b', '\n'.join(['a'] * 129)]:
            assert request('POST', '/api/v1/impact', invalid)[0] == 400, repr(invalid)
        # Correct JSON escaping for quoted paths and parser diagnostics.
        (root / 'src/quote"file.hpp').write_text('#include "missing.hpp"\n')
        status, workspace = request('GET', '/api/v1/workspace')
        assert status == 200 and any(n['path'] == 'src/quote"file.hpp' for n in workspace['nodes'])
        assert request('POST', '/webhooks/github', '{}', authenticated=False)[0] == 501
        assert raw_request(b'POST /api/v1/impact HTTP/1.1\r\nContent-Length: 1\r\nContent-Length: 2\r\n\r\nx') == 400
        assert raw_request(b'POST /api/v1/impact HTTP/1.1\r\nContent-Length: 10\r\n\r\nx') == 408
        assert raw_request(b'POST /api/v1/impact HTTP/1.1\r\nContent-Length: 1048577\r\n\r\n') == 413
        assert raw_request(b'POST /api/v1/impact HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n') == 400
        assert raw_request(b'GET /health/live HTTP/1.1\r\nX-Large: ' + b'a' * 33000 + b'\r\n\r\n') == 431
        # A disconnected client must not terminate the process through SIGPIPE.
        with socket.create_connection(('127.0.0.1', port)) as connection:
            connection.sendall(b'GET /health/live HTTP/1.1\r\n\r\n')
        assert request('GET', '/health/live', authenticated=False)[0] == 200
    finally:
        process.terminate()
        try:
            process.wait(timeout=8)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
        process.stderr.close()
    assert process.returncode == 0, process.returncode
    # Exposed binds must fail closed when no sufficiently strong token is supplied.
    rejected = subprocess.run([binary], env={**env, 'TESTGRAPH_HOST': '0.0.0.0', 'TESTGRAPH_API_TOKEN': ''}, capture_output=True, timeout=5)
    assert rejected.returncode != 0 and b'at least 32' in rejected.stderr
print('HTTP integration passed: real scans/impact, auth, path boundaries, JSON escaping, malformed/oversized requests, shutdown.')
