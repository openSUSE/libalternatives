libalternatives
===============

A configurable alternatives for update-alternatives.


Purpose
-------

`update-alternatives`, at its core, is a symlink management system. This
system is then mostly used as means for *administrators* of a system to
select a default application. For example, if we have `vim` or `emacs`
installed, the admin can then make one accessible as an `editor`
application.

The simple principle behind `update-alternatives` also is its main
drawbacks. Since it only manages symlinks, the preferred configuration
is only managed per system and cannot be altered per user. Furthermore,
if an application requires additional processing prior via the default
handler, this is no the domain of `update-alternatives`.

Another major drawback of `update-alternatives` is that it a state
system and this state is modified or updated at installation time. If
the state of the running system is kept separate from the installation
state (eg. multiple snapshots of parts of the system), the symlinks
state can become stale or simply wrong.

Description
-----------

`alternatives` is a simple wrapper around two config files. First is a
config file provided by packages that describe the alternatives
provided. The second config file is either a system or a user
preferences file that overrides default alternative preference.

Packages need to ship their package preferences in,
    /usr/share/alternatives/*binary*/*pref*.conf
where
	- *binary* is the alternative provided
	- *pref* is a numeric priority

The config file must be of the following format:

	binary=<absolute path to binary>
	man=<manpage>

*binary* entry is mandatory.

Typically, the *binary* can then be a symlink to
*/usr/bin/alternatives_default* that will **exec()** the configured binary
with largest pref number forwarding all the arguments to the configured
binary.

For example:

	binary=/usr/bin/vim
	man=vim.1

To override this behaviour, see *alternatives(1)*

API
---


See Also
--------
  + alternatives(1)
