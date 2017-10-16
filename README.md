gCSVedit
========

This is version 0.6.1 of gCSVedit.

gCSVedit is a simple CSV/TSV text editor.

See the [announcement](https://blogs.gnome.org/swilmet/2015/11/03/announcing-gcsvedit-a-simple-text-editor-to-edit-csv-files/).

Currently, the main features of gCSVedit are:
- Columns alignment.
- File loading and saving (the virtual spaces added for the alignment are
  removed when saving the file).
- Syntax highlighting.
- Draw spaces and tabs (only those present in the file).
- Header support: specify on which line the column titles are located, so that
  the header is not taken into account for the alignment.

Any kind of delimiter-separated values (DSV) files are supported, not just
comma-separated values (CSV) files. The application is called gCSVedit, because
“CSV” is commonly used to also refer to TSV (tab-separated values) or DSV in
general.

gCSVedit is a Free/_Libre_ software, it is licensed under the GPLv3+, see the
`COPYING` file for more information.

gCSVedit is [hosted on GitHub](https://github.com/swilmet/gCSVedit).

About versions
--------------

gCSVedit follows the GNOME versioning scheme. That is, the package version has
the form 'major.minor.micro' with an even minor version for stable releases,
and an odd minor version for unstable/development releases. For example the
0.4.x versions are stable and the 0.5.x versions are alpha/beta/rc versions.

Dependencies
------------

- GLib >= 2.44
- GTK+ 3 >= 3.22
- GtkSourceView 3 >= 3.24
- [Tepl](https://wiki.gnome.org/Projects/Tepl) >= 3.0

Installation
------------

If you want to install from a tarball, take the `*.tar.xz` tarball, not the
tarballs generated automatically by GitHub.

Simple install procedure:

```
$ ./configure
$ make
[ Become root if necessary ]
$ make install
```

See the `INSTALL` file for more detailed information.

From the Git repository, the `configure` script and the `INSTALL` file are not
yet generated, so you need to run `autogen.sh` instead, which takes the same
arguments as `configure`.

Developer documentation
-----------------------

See the `HACKING` file.
