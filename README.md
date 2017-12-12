# TinyMARE
My modifications to the TinyMARE engine

These are my modifications to the TinyMARE MUD engine.
You can find the original source at https://www.winds.org/pub/tinymare/.
The following changes have been made:

 - Added secondary SQLite3 database engine, accessible in-game via the `@sql` command and `sql()` function
 - Addition of Perlin Noise functions from https://github.com/stegu/perlin-noise

## SQLite3 database

The SQLite3 database is automatically connected at runtime, and is located at
`run/db/sdb`. It is accessible in-game to Wizards, and those granted the power
`SQL` (`@empower user=SQL:Y`) via the `@sql` command and the `sql()` function.
