# SiriDB Module for ThingsDB

This module is written in Go.

## Building the module

To build the siridb module, make sure Go is installed and configured.

First go to the module path and run the following command to install the module dependencies:

```
go mod tidy
```

Next, you are ready to build the module:

```
go build
```

Copy the created binary file to the ThingsDB module path.

## Using the module

The SiriDB module must be configured before it can be used. If you have multiple SiriDB databases, then simply configure the SiriDB module more than once.

In this example we will name the module `SiriDB`. This name is arbitrary and can be anything you like. The example uses a database named `dbtest` with the
default username and password combination.

Run the following in the `@thingsdb` scope:

```
// The values MUST be change according to your situation, this is just an example
new_module('SiriDB', {
    username: 'iris',
    password: 'siri',
    database: 'dbtest',
    servers: [
        ["localhost", 9000]
    ]
});
```


