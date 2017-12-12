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

### @sql

`@sql SQL STATEMENT`

The `@sql` command executes a SQLite statement and returns the result to the
player's console as a space-delimited table.

```
@sql SELECT * FROM foo
1 foo
2 bar
```

### sql()

`sql(SQL STATEMENT)`
`sql(SQL STATEMENT, COL DELIM)`
`sql(SQL STATEMENT, COL DELIM, ROW DELIM)`

The `sql()` function executes a SQLite statement and returns the result. The
default format is a space-delimited table. The default column delimiter is a
space and the default row delimiter is a newline.

## Noise

Making use of Stegu's public domain Noise library, it is now possible to
generate noise in anywhere from one to four dimensions. This is immensely useful
for very large areas of automatically generated terrain.

The Perlin Noise functions of the library were left unimplemented as they
required double the parameters to use as the simpler noiseX() functions. If you
require the true Perlin Noise functions either drop me a message or implement
them yourself and submit a pull request. I simply left them out because they
appeared to be more difficult to use and I didn't see a scenario where higher
quality noise was required for a MUD.

### noise1()

`noise1(x)`

Returns a float value of noise for the 1-dimensional coordinate x.

### noise2()

`noise2(x, y)`

Returns a float value of noise for the 2-dimensional coordinates x,y.

### noise3()

`noise3(x, y, z)`

Returns a float value of noise for the 3-dimensional coordinates x,y,z.

### noise4()

`noise4(x, y, z, h)`

Returns a float value of noise for the 4-dimensional coordinates x,y,z,h.
