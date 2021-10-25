The `main` program produces all its log output on stderr.
On stdout, it produces a single line of json using the following format:

```json
{
    "timestamp": "", // String; Timestamp when execution started. ISO 8601 date and time format: `%Y-%m-%dT%H:%M:%SZ%z`
    "name": "",      // String; Impl name
    "N": 0,          // int; Size of vector A
    "M": 0,          // int; Size of vector B
    "numprocs": 0,   // int; Number or processes
    "runtime": 0,    // int; Runtime of the entire program (this is the largest value of the `runtimes` array)
    "runtimes": [],  // int[]; Runtime in microseconds per process
    "errors": [],    // double[]; Numerical error per process, only exists if validation was used
}
```
