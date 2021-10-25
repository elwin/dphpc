```json
{
    "timestamp": "", // Timestamp; format TBD
    "name": "",      // String; Impl name
    "N": 0,          // int; Size of vector A
    "M": 0,          // int; Size of vector B
    "numprocs": 0,   // int; Number or processes
    "runtime": 0,    // int; Microseconds
    "runtimes": [],  // int[]; Runtime in microseconds per process
    "errors": [],    // float[]; Numerical error per process, only exists if validation was used
}
```

This will be written in a single line and can be concatenated to a json lines file.
