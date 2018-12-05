# Sands of Time
>⏳ Sands of Time modifies time for different processes whenever you want

![Kung Fury (2015)](https://i.kym-cdn.com/photos/images/newsfeed/001/059/850/546.gif)


## Example
To run the example:
```bash
$ npm install
$ node example/index.js 
```
`example/log_date.js` just logs date every 5 seconds in the `/tmp/log_date.sands.log` file.

`example/index.js` runs `log_date` and after 20 seconds, doubles the speed of time for that process.


## How does it work?

Sands of time consists of three parts:
1. `src/lib.c` manipulates time for a single process.
2. `lib/server.js` spawns processes and sends commands to `lib.c`.
3. `lib/index.js` is a wrapper to use the module. 

Let's take a closer look on these files.

### src/lib.c
Its responsibility is to intercept system calls and send fake date and time to the intended process, instead of the real time of the OS. You have the ability of specifying fake time even when the process is running.


### lib/server.js
This file includes a class called `SandsServer` which implements some methods to run an HTTP server specified by the `address` and `port`. Also, this class implements three following APIs:

#### `POST /spawn`
Sample body:
```json
{
  "command": "node",
  "args": ["test.js"],
  "env": {},
  "filename": "/tmp/out.sands.log"
}
```
Spawns a new process specified by the following fields:
* `command`: The command to be run.
* `args`: An array of the args for the command.
* `env`:  Environment Variables.
* `filename`: The file which the outputs must write to.

Sample Result:
```json
{
  "id": "mniukPHpX",
  "sockaddr": "/tmp/sandmniukPHpX.sock"
}
```
* `id`: The id of the process which is spawned by `SandsServer`. It will be used by other APIs.
* `sockaddr`: The address of the socket for that process.

#### `POST /change/:id`
Sample body:
```json
{
  "alpha": 2,
  "seconds": 1539262907,
  "nanoseconds": 0
}
```
Change time for a process specified by `:id` part of the url. The formula is `αt + β` and the params are as follow:
* `alpha`: Speed of time.
* `seconds`: Seconds of `β` part of the formula.
* `nanoseconds`: Nanoseconds of `β` part of the formula.

In case of a successful call, the body of this API is always an empty object.

#### `POST /shift/:id`
Sample body:
```json
{
  "alpha": 1,
  "seconds": 120,
  "nanoseconds": 0
}
```
Shift time for a process specefied by `:id` part of the url. For example the above sample body will shift time to 2 minutes later for a process.
* `alpha`: Speed of time.
* `seconds`: Seconds of `β` part of the formula.
* `nanoseconds`: Nanoseconds of `β` part of the formula.

In case of a successful call, the body of this API is always an empty object.


### lib/index.js
This part of the library is a wrapper for starting an instance of `SandsServer` and calling above APIs through function calls.


## Issues
* Sands of time supports Linux and macOS only.
* JVM-based applications are not tested yet.
* Node.js applications do not support rewinding time. According to this issue and the rare usecases of rewinding time, we disable this feature in `lib/server.js`.