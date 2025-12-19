# Parallel String Matching Search Engine

A parallel computing project

## Quick Start

### 1. Create Environment

```bash
python3 -m venv venv
source venv/bin/activate 
```

### 2. Install Dependencies

```bash
pip install -r requirements.txt
```

### 3. Compile Executables

```bash
make clean
make MPI=1
```

### 4. Run Server

```bash
python3 server.py
```

Then open `http://localhost:5000`

### 5. Use quick_test.sh for Local Testing

```bash

./quick_test.sh "computer"
./quick_test.sh "algorithm"
```


### Web Interface

1. Start server: `python3 server.py`
2. Open browser to `http://localhost:8080`
3. Configure MPI processes and OpenMP threads
4. Enter search query and compare serial vs parallel performance

### Command Line

```bash
# run this for serial execution
./build/string_match serial documents/wikitext_metadata.txt "pattern"

# run this for parallel execution
mpirun --hostfile hosts.txt -np 4 ./build/string_match mpi documents/wikitext_metadata.txt "pattern"
```

### Testing

```bash
# "default word 'the' " 
./quick_test.sh

# Add any word here 
./quick_test.sh "your word"
```


### Notes

1. Some inputs might not work as those words might not be present in any files. Try generic words such as 'happy', 'game',.. etc.
2. You can use the website for real-time experimentation using different core thread combos.
3. For the structure we have build folder - which has the executables, the src folder which has the actual code, the web folder which has the front end implementation and server.py which starts the website, document folder which has the test dats.