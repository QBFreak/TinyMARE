/* prog/python.c */

/* 2017-12-13 - Code for Python integration - QBFreak@qbfreak.net
 *  Python integration via @python, @pytr commands and python() function
 */

/* Python module, handles @python, @pytr commands. */

#include "externs.h"

char* exec_python(char *python, dbref player, dbref cause) {
    // Output buffer
    static char buff[8192];
    // Wipe the results of the last run
    for (int i=0;i<8192;i++)
        buff[i] = 0;
    // Index of buff[] as we fill it up
    int b = 0;

    Py_Initialize();

    // Set up the __main__ module
    PyObject* module_main = PyImport_AddModule("__main__");
    PyObject* dict_main = PyModule_GetDict(module_main);
    PyObject* dict_local = PyDict_New();

    // Import sys
    PyObject* module_sys = PyImport_ImportModule("sys");
    PyObject* dict_sys = PyModule_GetDict(module_sys);
    PyDict_SetItemString(dict_main, "sys", module_sys);

    // Our Python modules are in lib/python but cwd is run/, so ../lib/python/
    PyRun_SimpleString( "sys.path.append(\"../lib/python/\")\n" );

    // Import TinyMARE
    PyObject* module_tinymare = PyImport_ImportModule("TinyMARE");
    PyObject* dict_tinymare = PyModule_GetDict(module_tinymare);
    PyDict_SetItemString(dict_main, "TinyMARE", module_tinymare);

    // Redirect stdout
    PyObject* pyStdOut = PyFile_FromString("CONOUT$", "w+");
    PyObject_SetAttrString(module_sys, "stdout", pyStdOut);

    // actor = %# = player
    // thing = %! = cause
    PyDict_SetItemString(dict_local, "player", PyInt_FromLong((long) player));
    PyDict_SetItemString(dict_local, "cause", PyInt_FromLong((long) cause));

    // Run the Python
    PyRun_String(python, Py_file_input, dict_main, dict_local);

    // Buffer output
    FILE* pythonOutput = PyFile_AsFile(pyStdOut);
    rewind(pythonOutput);
    char line[1024];
    while(fgets(line, sizeof line, pythonOutput) != NULL) {
        // Strip trailing \n
        for (int i=0;i<1024;i++) {
            if (line[i] == 0)
                // Don't copy the string terminator
                break;
            buff[b] = line[i];
            b++;
            if (b > 8192) {
                // ACK! Buffer full! Terminate the string and return it
                buff[8191] = 0;
                return buff;
            }
        }
    }

    // Check for errors
    PyObject* ex = PyErr_Occurred();
    if (ex) {
        PyObject *pExcType , *pExcValue , *pExcTraceback ;
        PyErr_Fetch( &pExcType, &pExcValue, &pExcTraceback );
        if (pExcType != NULL) {
            PyObject* pRepr = PyObject_Repr(pExcType);
            log_error("Python exception: %s", PyString_AsString(pRepr));
            Py_DecRef(pRepr);
            Py_DecRef(pExcType);
        }
        if (pExcValue != NULL) {
            PyObject* pRepr = PyObject_Repr(pExcValue);
            log_error("Python exception: %s", PyString_AsString(pRepr));
            Py_DecRef(pRepr);
            Py_DecRef(pExcValue);
        }
        /* Do something with exception */
        char errExecption[21] = "#-1 Python exception";
        // Only copy the error, not the string terminator (size - 1)
        for (int i=0;i<20;i++) {
            buff[b] = errExecption[i];
            b++;
            if (b > 8192) {
                // ACK! Buffer full! Terminate the string and return it
                buff[8191] = 0;
                return buff;
            }
        }
    }

    // Done!
    Py_Finalize();

    // Remove the trailing newline, if there is one
    for (int i=0;i<8192;i++)
        if(buff[i] == 0 && i > 0 && buff[i-1] == '\n')
            buff[i-1] = 0;

    return buff;
}

/* @python command - Execute Python and display output to player */
void do_python(player, python, arg2, argv, pure, cause)
dbref player, cause;
char *python, *arg2, **argv, *pure;
{
    // TODO: Loop through the return of exec_python() and do a separate
    //          notify() for each line. This matters for things like puppets
    //          and @iterate
    notify(player, exec_python(python, player, cause));
}

/* @pytr command - Trigger an attribute to run as Python */
void do_pytr(player, object, arg2, argv, pure, cause)
dbref player, cause;
char *object, *arg2, **argv, *pure;
{
  dbref thing;
  ATTR *attr;
  char *s;
  int a;

  /* validate target */
  if(!(s=strchr(object, '/'))) {
    notify(player, "You must specify an attribute name.");
    return;
  }
  *s++='\0';
  if((thing=match_thing(player, object, MATCH_EVERYTHING)) == NOTHING)
    return;
  if(!controls(player, thing, POW_MODIFY)) {
    notify(player, "Permission denied.");
    return;
  }

  /* Validate attribute */
  if(!(attr=atr_str(thing, s))) {
    notify(player, "No match.");
    return;
  } if(!can_see_atr(player, thing, attr)) {
    notify(player, "Permission denied.");
    return;
  } if(attr->flags & (AF_HAVEN|AF_LOCK|AF_FUNC)) {
    notify(player, "Cannot trigger that attribute.");
    return;
  }

  /* Trigger attribute */
  for(a=0;a<10;a++)
    if(argv[a+1])
      strcpy(env[a], argv[a+1]);
    else
      *env[a]='\0';

  if(!Quiet(player))
    notify(player, "%s - Triggered.", db[thing].name);

  // TODO: Loop through the return of exec_python() and do a separate
  //          notify() for each line. This matters for things like puppets
  //          and @iterate
  char *contents=atr_get_obj(thing, attr->obj, attr->num);
  notify(player, exec_python(contents, player, cause));

}
