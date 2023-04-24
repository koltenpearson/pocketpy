---
icon: code
label: 'Execute Python code'
order: 93
---

Once you have a `VM` instance, you can execute python code by calling `exec` method.

### `PyObject* exec(Str source, Str filename, CompileMode mode, PyObject* _module=nullptr)`

+ `source`, the python source code to be executed
+ `filename`, the filename of the source code. This is used for error reporting
+ `mode`, the compile mode. See below for details
+ `module`, the module to be executed. If `nullptr`, the code will be executed in the `__main__` module

`exec` handles possible exceptions and returns a `PyObject*`.
If the execution is not successful, e.g. a syntax error or a runtime exception,
the return value will be `nullptr`.

#### Compile mode

The `mode` parameter controls how the source code is compiled. There are 4 possible values:
+ `EXEC_MODE`, this is the default mode. Just do normal execution.
+ `EVAL_MODE`, this mode is used for evaluating a single expression. The `source` should be a single expression. It cannot contain any statements.
+ `REPL_MODE`, this mode is used for REPL. It is similar to `EXEC_MODE`, but generates `PRINT_EXPR` bytecode for global expressions.
+ `JSON_MODE`, this mode is used for JSON parsing. It is similar to `EVAL_MODE`, but uses a lexing rule designed for JSON. For example, `true` will be parsed as `True`.


### Fine-grained execution

In some cases, you may want to execute python code in a more fine-grained way.
These two methods are provided for this purpose:

+ `CodeObject_ compile(Str source, Str filename, CompileMode mode, bool unknown_global_scope)`
+ `PyObject* _exec(CodeObject_ co, PyObject* _module)`

`compile` compiles the source code into a `CodeObject_` instance.
`_exec` executes the `CodeObject_` instance.
It does not handle exceptions, so you may need to use `try..catch` manually.