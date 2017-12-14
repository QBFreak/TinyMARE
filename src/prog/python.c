/* prog/python.c */

/* 2017-12-13 - Code for Python integration - QBFreak@qbfreak.net
 *  Python integration via @python, @pytr commands and python() function
 */

/* Python module, handles @python, @pytr commands. */

#include "externs.h"

void do_python(dbref player, char *python) {
    Py_Initialize();

    //const char* pythonScript = "result = multiplicand * multiplier\n";
    PyObject* pmain = PyImport_AddModule("__main__");
    PyObject* globalDictionary = PyModule_GetDict(pmain);
    PyObject* localDictionary = PyDict_New();

    // Redirect stdout
    PyObject* pyStdOut = PyFile_FromString("CONOUT$", "w+");
    PyObject* sys = PyImport_ImportModule("sys");
    PyObject_SetAttrString(sys, "stdout", pyStdOut);

    // PyDict_SetItemString(localDictionary, "multiplicand", PyInt_FromLong(2));
    // PyDict_SetItemString(localDictionary, "multiplier", PyInt_FromLong(5));
    PyRun_String(python, Py_file_input, globalDictionary, localDictionary);

    // long result = PyInt_AsLong(PyDict_GetItemString(localDictionary, "result"));

    //notify(player, "Result: %ld", result);

    // Display output
    FILE* pythonOutput = PyFile_AsFile(pyStdOut);
    rewind(pythonOutput);
    char line[1024];
    while(fgets(line, sizeof line, pythonOutput) != NULL) {
        // Strip trailing \n
        for (int i=0;i<1024;i++) {
            if (line[i] == 0 && i > 0 && line[i-1] == '\n')
                line[i-1] = 0;
        }
        notify(player, "%s", line);
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
        // Traceback object
        // if (pExcTraceback != NULL) {
        //     PyObject* pRepr = PyObject_Repr(pExcTraceback);
        //     log_error("Python exception: %s", PyString_AsString(pRepr));
        //     Py_DecRef(pRepr);
        //     Py_DecRef(pExcTraceback);
        // }
        /* Do something with tb */
        notify(player, "#-1 Python exception");
    }

    // Done!
    Py_Finalize();
}
