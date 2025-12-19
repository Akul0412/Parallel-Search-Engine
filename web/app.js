const API_BASE = window.location.origin;

async function executeSearch(query, useParallel, mpiProcesses = 2, ompThreads = 4) {
    try {
        const response = await fetch(`${API_BASE}/api/search`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ 
                query, 
                parallel: useParallel,
                processes: mpiProcesses,
                threads: ompThreads
            })
        });
        
        if (!response.ok) {
            throw new Error(`Search failed: ${response.statusText}`);
        }
        
        const data = await response.json();
        if (data.error) {
            throw new Error(data.error);
        }
        
        return {
            results: data.results || [],
            time: data.time || 0
        };
    } catch (error) {
        console.error('Search error:', error);
        return {
            results: [],
            time: 0,
            error: error.message
        };
    }
}

function displayResults(panelId, timeId, results, executionTime) {
    const resultsDiv = document.getElementById(panelId);
    const timeDiv = document.getElementById(timeId);
    
    timeDiv.textContent = `${executionTime} ms`;
    
    if (results.length === 0) {
        resultsDiv.innerHTML = '<p class="placeholder">No results found</p>';
        return;
    }
    
    resultsDiv.innerHTML = results.map((result, index) => `
        <div class="result-item">
            <div class="doc-id">Document ID: ${result.doc_id}</div>
            <div class="score">Matches: ${result.match_count || result.score}</div>
        </div>
    `).join('');
}

function displaySpeedup(parallelTime, sequentialTime) {
    const speedup = parseFloat(sequentialTime) / parseFloat(parallelTime);
    const speedupContainer = document.getElementById('speedupContainer');
    const speedupValue = document.getElementById('speedupValue');
    
    speedupValue.textContent = `${speedup.toFixed(2)}x`;
    speedupContainer.style.display = 'block';
}

async function performSearch() {
    const query = document.getElementById('searchInput').value.trim();
    
    if (!query) {
        alert('Please enter a search query');
        return;
    }
    
    // Get configuration values
    const mpiProcesses = parseInt(document.getElementById('mpiProcesses').value) || 2;
    const ompThreads = parseInt(document.getElementById('ompThreads').value) || 4;
    
    document.getElementById('parallelResults').innerHTML = '<p class="loading">Searching...</p>';
    document.getElementById('sequentialResults').innerHTML = '<p class="loading">Searching...</p>';
    document.getElementById('speedupContainer').style.display = 'none';
    
    const [parallelResult, sequentialResult] = await Promise.all([
        executeSearch(query, true, mpiProcesses, ompThreads),
        executeSearch(query, false)
    ]);
    
    if (parallelResult.error) {
        document.getElementById('parallelResults').innerHTML = 
            `<p class="placeholder" style="color: red;">Error: ${parallelResult.error}</p>`;
    } else {
        displayResults('parallelResults', 'parallelTime', parallelResult.results, parallelResult.time);
    }
    
    if (sequentialResult.error) {
        document.getElementById('sequentialResults').innerHTML = 
            `<p class="placeholder" style="color: red;">Error: ${sequentialResult.error}</p>`;
    } else {
        displayResults('sequentialResults', 'sequentialTime', sequentialResult.results, sequentialResult.time);
    }
    
    if (!parallelResult.error && !sequentialResult.error && 
        parallelResult.time > 0 && sequentialResult.time > 0) {
        displaySpeedup(parallelResult.time, sequentialResult.time);
    } else {
        document.getElementById('speedupContainer').style.display = 'none';
    }
}

document.getElementById('searchButton').addEventListener('click', performSearch);

document.getElementById('searchInput').addEventListener('keypress', (e) => {
    if (e.key === 'Enter') {
        performSearch();
    }
});

// Update total workers display when config changes
function updateTotalWorkers() {
    const mpiProcesses = parseInt(document.getElementById('mpiProcesses').value) || 2;
    const ompThreads = parseInt(document.getElementById('ompThreads').value) || 4;
    document.getElementById('totalWorkers').textContent = mpiProcesses * ompThreads;
}

// Add event listeners for config controls
document.addEventListener('DOMContentLoaded', () => {
    document.getElementById('mpiProcesses').addEventListener('input', updateTotalWorkers);
    document.getElementById('ompThreads').addEventListener('input', updateTotalWorkers);
    updateTotalWorkers(); // Initial calculation
});

window.addEventListener('DOMContentLoaded', async () => {
    try {
        const response = await fetch(`${API_BASE}/api/stats`);
        const stats = await response.json();
        if (!stats.indexed) {
            console.warn('Index not built yet');
        }
    } catch (error) {
        console.warn('Could not connect to backend');
    }
});
