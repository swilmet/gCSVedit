gCSVedit
========

This is version 0.8.0 of gCSVedit.

gCSVedit is a simple CSV/TSV text editor.

See the [announcement](https://swilmet.be/blog/2015/11/03/announcing-gcsvedit-a-simple-text-editor-to-edit-csv-files/).

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

Buy the application on the Microsoft Store
------------------------------------------

gCSVedit is available for purchase on the Microsoft Store:
[gCSVedit for Windows](https://www.microsoft.com/store/apps/9N7MP1CQ3M32).

About versions
--------------

gCSVedit follows the GNOME versioning scheme. That is, the package version has
the form 'major.minor.micro' with an even minor version for stable releases,
and an odd minor version for unstable/development releases. For example the
0.4.x versions are stable and the 0.5.x versions are alpha/beta/rc versions.

Dependencies
------------

- GLib >= 2.44
- GTK >= 3.22
- GtkSourceView >= 4.0
- [Tepl](https://wiki.gnome.org/Projects/Tepl) >= 4.99

Installation
------------

If you want to install from a tarball, take the `*.tar.xz` tarball, not the
tarballs generated automatically by GitHub.

Simple install procedure from a tarball:

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
