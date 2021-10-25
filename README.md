libalternatives
==============

A configurable alternative for update-alternatives.


Purpose
-------

`update-alternatives`, at its core, is a symlink management system. This
system is then mostly used as means for *administrators* of a system to
select a default application. For example, if we have `vim` or `emacs`
installed, the admin can then make one accessible as an `editor`
application.

The simple principle behind `update-alternatives` also is its main
drawback. Since it only manages symlinks, the preferred configuration
is only managed per system and cannot be altered per user. Furthermore,
if an application requires additional processing prior to execution via
the default handler, this is outside the domain of `update-alternatives`.

Another major drawback of `update-alternatives` is that it is a state
system and this state is modified or updated at *installation* time. If
the state of the running system is to be kept separate from the installation
state (eg. multiple snapshots of parts of the system), the symlinks'
state can become stale or simply wrong without a *possibility* of a
fallback mechanism.

Description
-----------

`libalterntive` provides an *alternative*, not a replacement, to this
symlink management system. Instead of symlinks, the preferred executable
is executed directly. Which executable is executed depends on the
available alternatives installed on the system and the system and/or
user configuration files. By default, the preferred alternative with the
highest priority is executed. If an alternative is selected by the user
or the system administrator, this alternative will be executed instead.
If the user or system selection is unavailable, the system will fall
back to the default, highest priority, installed alternative.

`alts` is a simple wrapper around two config files. First is a
config file provided by packages that describe the alternatives
provided. The second config file is either a system or a user
preferences file that overrides default alternative preference.

Packages need to ship their package preferences in the following format,

	/usr/share/libalternatives/<binary>/<pref>.conf

where

+ *binary* is the name of the alternative provided
+ *pref* is a numeric priority

The config file must be of the following format:

	binary=<absolute path to binary>
	man=<list of manpages>
	group=<list of binaries>
	options=KeepArgv0

where the *binary* entry is mandatory and *list* consists of a comma (,) separated items.
All other entries are optional

Typically, the *binary* can then be a symlink to
*/usr/bin/alts* that will **exec()** the configured binary
with largest pref number, forwarding all the arguments to the configured
binary.

Argv[0]
-------

In normal cases, argv[0] contains name of the executed target
by the *alts* binary. In the example below, this would be *vim*. If the
called process needs the name of for argv[0], set the option 
*options=KeepArgv0* in the config file.

Note: Only these two options for argv[0] are available. Some called processes expect other names which cannot be handled by libalternatives.

For example:

	binary=/usr/bin/vim
	man=vim.1
	options=KeepArgv0

*alts* is also the configuration utility that allows system or user
override of the default binary. See *alts(1)* for details.

Groups
------

At times, multiple alternatives should be controlled as a group.
Altering the preferred binary in one should alter the entire group. To
facilitate this, one should use the `group` entry in the package
preferences to list all the binaries that are to be altered together.

It is important that all the binaries have the same group entries in
their package preference files and have the same priorities. It is also
important that no higher priority alternative exists unless it also
forms the same sized group. The `alts` binary will warn you when reading
the state of one of the executables with a `group` entry.


Notes
-----

`libalternatives` is NOT aimed to be a replacement for
`update-alternatives`. It is meant as an alternative that tends to stay
out of the way and provides more flexibility and stability for users and
the package maintainers.

API
---


See Also
--------
  + alts(1)
