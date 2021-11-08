The `main` program produces all its log output on stderr.
On stdout, it produces a single line of json using the following format:

```json
{
    "timestamp": "",        // String; Timestamp when execution started. ISO 8601 date and time format: `%Y-%m-%dT%H:%M:%SZ%z`
    "name": "",             // String; Impl name
    "N": 0,                 // int; Size of vector A
    "M": 0,                 // int; Size of vector B
    "numprocs": 0,          // int; Number or processes
    "runtime": 0,           // int; Runtime of the entire program (this is the largest value of the `runtimes` array)
    "runtime_mpi": 0,       // int; Runtime of the MPI calls (for the process corresponding to `runtime`)
    "runtime_compute": 0,   // int; runtime - runtime_compute
    "runtimes": [],         // int[]; Runtime in microseconds per process
    "runtimes_mpi": [],     // int[];
    "runtimes_compute": [], // int[];
    "errors": [],           // double[]; Numerical error per process, only exists if validation was used
    "num_iterations": 1,    // int; Number of iterations
    "iteration": 0          // int; Current iteration [0, num_iterations) 
}
```
