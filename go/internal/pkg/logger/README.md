# sandie-ndn/go/internal/pkg/logger

This package provides a wrapper over [Logrus](https://github.com/sirupsen/logrus), a structured logger for Go.

This logger package support log level configuration through environment variables. For log module "Foo", the initialization code first looks for "LOG\_Foo" and, if not found, looks for the generic "LOG" environment variable.
The value of this environment variable must be one of:

* **D**: DEBUG level
* **I**: INFO level (default)
* **W**: WARNING level
* **E**: ERROR level
* **F**: FATAL level
* **N**: PANIC level

To find log module names in this codebase, execute:

```console
user@cms:~$ git grep -E 'logger\.New'
```
