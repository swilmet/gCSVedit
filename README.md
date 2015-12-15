gCSVedit
========

This is version 0.0.2 of gCSVedit.

gCSVedit is under development. The goal is to have a simple CSV text editor
that runs on GNU/Linux and Windows (maybe Mac OS X too).

See the [announcement](https://blogs.gnome.org/swilmet/2015/11/03/announcing-gcsvedit-a-simple-text-editor-to-edit-csv-files/).

Currently, the main features of gCSVedit are:
- Columns alignment
- File loading and saving (the virtual spaces added for the alignment are
  removed when saving the file).
- Syntax highlighting
- Draw spaces and tabs (only those present in the file).
- Header support: specify on which line the column titles are located, so that
  the header is not taken into account for the alignment.

Any kind of delimiter-separated values (DSV) files are supported, not just
comma-separated values (CSV) files. The application is called gCSVedit, because
“CSV” is commonly used to also refer to TSV (tab-separated values) or DSV in
general.

gCSVedit is a free/libre software (GPLv3+ license) and is [hosted on
GitHub](https://github.com/UCL-CATL/gcsvedit).

You are encouraged to [report bugs or feature
requests](https://github.com/UCL-CATL/gcsvedit/issues).

Note that currently gCSVedit depends on unstable versions of GtkSourceView and
GTK+. It can therefore be a bit difficult to install. We use
[Jhbuild](https://wiki.gnome.org/Projects/Jhbuild) for the development.

A stable version of gCSVedit will be released at the same time as GNOME 3.20,
in March 2016. gCSVedit is not yet meant for mass adoption, it's still under
development.

Downloads
---------

- [Tarballs](http://ucl-catl.github.io/tarballs/gcsvedit/)

Dependencies
------------

- GLib >= 2.44
- GTK+ >= 3.19.4
- GtkSourceView >= 3.19.0
- gettext

Note that you need autoconf-archive for building the sources from Git.

Installation
------------

Simple install procedure:

```
$ ./configure
$ make
[ Become root if necessary ]
$ make install
```

See the file `INSTALL` for more detailed information.

From the Git repository, the `configure` script and the `INSTALL` file are not
yet generated, so you need to run `autogen.sh` instead, which takes the same
arguments as `configure`.

Developer Documentation
-----------------------

See the file `HACKING`.
