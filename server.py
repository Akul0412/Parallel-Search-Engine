import subprocess
import re
import os
from flask import Flask, request, jsonify, send_from_directory
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

SEARCH_ENGINE = "./build/string_match"
METADATA_FILE = "documents/wikitext_metadata.txt"

def parse_results(output):
    """Extract results from string matching output."""
    results = []
    for line in output.split('\n'):
        match = re.search(r'Document ID: (\d+), Matches: (\d+)', line)
        if match:
            results.append({
                'doc_id': int(match.group(1)),
                'match_count': int(match.group(2)),
                'score': int(match.group(2))  # Use match count as score for display
            })
    return results

@app.route('/')
def index():
    return send_from_directory('web', 'index.html')

@app.route('/<path:path>')
def serve_static(path):
    return send_from_directory('web', path)

@app.route('/api/search', methods=['POST'])
def search():
    try:
        data = request.json
        query = data.get('query', '').strip()
        use_parallel = data.get('parallel', True)
        
        if not query:
            return jsonify({'error': 'Query required'}), 400
        
        mode = 'mpi' if use_parallel else 'serial'
        
        num_processes = data.get('processes', 2)
        num_threads = data.get('threads', 4)
        
        cmd = [SEARCH_ENGINE, mode, METADATA_FILE, query, '--benchmark']
        
        env = os.environ.copy()
        if mode == 'mpi':
            env['OMP_NUM_THREADS'] = str(num_threads)
            cmd = ['mpirun', '--hostfile', 'hosts.txt', '-np', str(num_processes)] + cmd
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30, env=env)
        else:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=10, env=env)
        
        if result.returncode != 0:
            return jsonify({'error': 'Search failed'}), 500
        
        time_match = re.search(r'Execution time: ([\d.]+) ms', result.stdout)
        if not time_match:
            return jsonify({'error': 'Invalid response from search engine'}), 500
        
        execution_time = float(time_match.group(1))
        
        return jsonify({
            'results': parse_results(result.stdout),
            'time': execution_time,
            'query': query
        })
    except Exception as e:
        return jsonify({'error': 'Search failed'}), 500

@app.route('/api/stats', methods=['GET'])
def stats():
    doc_count = 0
    if os.path.exists(METADATA_FILE):
        with open(METADATA_FILE, 'r') as f:
            doc_count = len([l for l in f if l.strip()])
    
    return jsonify({'indexed': True, 'document_count': doc_count})

if __name__ == '__main__':
    port = 8080
    app.run(debug=True, host='0.0.0.0', port=port)
