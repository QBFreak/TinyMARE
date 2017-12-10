# TinyMARE
My modifications to the TinyMARE engine

These are my modifications to the TinyMARE MUD engine.
You can find the original source at https://www.winds.org/pub/tinymare/.
The following changes have been made:

 - Added secondary SQLite3 database engine, accessible in-game via the `@sql` command and `sql()` function
 
The following changes are planned:

 - Addition of Perlin Noise functions from https://github.com/stegu/perlin-noise
 - Security for SQLite3 database, most likely simply restricting access to those that have appropriate powers
