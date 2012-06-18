# Tutorial: executing config

To actually execute some configuration you should type in your shell following:

	$ frozend -c configuration_file.conf

<b>frozend</b> here is frozen application which parse configration file and execute it. <b>frozend --help</b> for more options.

If you use [mod_machine_stdin][] in configuration, pass input as usual. Same for [mod_machine_stdout][], [mod_machine_stderr][].

	$ cat file | frozend -c configuration_file.conf
	$ cat file | frozend -c configuration_file.conf  >log_stdout
	$ cat file | frozend -c configuration_file.conf 2>log_stderr


m4 configuration file is same, change extension to use it.

	$ frozend -c configuration_file.m4


Prev tutorial: [Tutorial: configuration][tutorial_configuration]

Next tutorial: [Tutorial: most used modules][tutorial_common]

[mod_machine_stdin]: /doxygen/group__mod__machine__stdin.html
[mod_machine_stdout]: /doxygen/group__mod__machine__stdout.html
[mod_machine_stderr]: /doxygen/group__mod__machine__stderr.html
[tutorial_configuration]: tutorial_configuration.html
[tutorial_common]: tutorial_common.html

