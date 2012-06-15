# Tutorial: most used modules
## Input
### Stdin

To have anything to be parsed it should be somehow passed in machine. Frozen have different ways in input data, most simple one is console.
[mod_machine_stdin][] used to get data from stdin. And because stdin module is passive, [mod_machine_query][] with [mod_machine_thread][] used to activly transfer
data from one machine shop to another. In this case destination is [mod_machine_stdout][] used. For more about outputs read next chapter.

[tutorial_in_stdin.conf][] 

### Fuse

Next way is [mod_machine_fuse][] machine. It create filesystem using fuse and pass all read and writes to user defined machines.
Sample configuration can look like so:

[tutorial_in_fuse.conf][]

This creates file system in /home/test folder, which contain file named "one" and every write request to this file will result in 
console output. Use "echo 'hello' >> /home/test/one" or similar to test it.

### Emitter

One more way to input is describe machine which send requests you want. One of such machines is [mod_machine_emitter][]. Example:
[tutorial_in_emitter.conf][]

### HTTP

Another way is different modules, for example go_http. It create simple web server and pass request to user's machines.
[tutorial_in_http.conf][]

### File

You could read data from file and use it as input data. Construction is same as for stdin.
[tutorial_io_file.conf][]

## Output

### Debugging

To debug request flow you can use [mod_machine_debug][] machine. It print line in stderr on request arrival, and optionally on request end.
Also it can show request content. Example:
[tutorial_out_debug.conf][]

### Stdout

This could be done by [mod_machine_stdout][]. It print buffer in console stdout. For stderr - use [mod_machine_stderr][].
[tutorial_io_file.conf][]

### File

Write data to file with [mod_machine_file][] machine. Example:
[tutorial_out_file.conf][]

## Processing

### Split

It is very common to process data line-by-line, especially in UNIX world, but for performance reasons frozen by default don't split
incoming data on lines. If you want lines you should use [mod_machine_split][] machine. By default it split input exactly by \n which mean
end of line. 

### Regexp

To match some input for pattern you can use [mod_machine_regexp][]. It add special marker to current request if data match defined pattern.

### Conditions

As we have some markers, where got to be some condition. [mod_machine_switch][] control request flow using conditions. Because flow could be
redirected in any other machine shop you could do anything with such request or enviroment, for example terminate request, return success or
error, write this to logger, trigger some action and so on.

## Combining all together

As we know some of processing and input\output machines we could combine it in something useful. For example, you want to know which users have /bin/false shell.
Configuration will look like following:
[tutorial_proc_shell.conf][]

[mod_machine_file][] open file for reading only. [mod_machine_split][] split it in lines.
[mod_machine_regexp][] matches /bin/false aganist input line and pass to [mod_machine_switch][]. It lookup for default marker value and if find - pass
to [mod_machine_stdout][]. Whole thing works like simple grep, a bit silly but read further. What if you want more regexps? No problem:
[tutorial_proc_shell2.conf][]

As regexp set same marker this construction works like OR. For AND use different markers and combine them in switch rule like so:

	...
	{ class => "data/regexp", regexp = "/bin",  marker = "key1"  }, 
	{ class => "data/regexp", regexp = "/home", marker = "key2"  }, 
	{ class => "request/switch", rules = {
	 {  
	     request = {
		     key1 = (uint_t)'1',
		     key2 = (uint_t)'1'
	     },
	...



Because [mod_machine_switch][] don't care what in your rule's request you could first check for such AND condition, if it not matches check
for only one marker, or for another, or for both. And for each rule you can supply different machine with any action. 

You can have any number of regexps. And even more: you can define one regexp for pre-matching and it's result will define which set of actions
(including another regexps) it will go through. You could write matching lines in one file, and simultaneously write non-matching to another file.
If you pick up [mod_machine_fuse][] for input and run frozen as daemon - this predefined complex pipe will process any data at any time. Grep can't do that. 

Prev tutorial: [Tutorial: execution][tutorial_execution]

[mod_machine_stdin]: /doxygen/group__mod__machine__stdin.html
[mod_machine_stdout]: /doxygen/group__mod__machine__stdout.html
[mod_machine_stderr]: /doxygen/group__mod__machine__stderr.html
[mod_machine_query]: /doxygen/group__mod__machine__query.html
[mod_machine_fuse]: /doxygen/group__mod__machine__fuse.html
[mod_machine_split]: /doxygen/group__mod__machine__split.html
[mod_machine_regexp]: /doxygen/group__mod__machine__regexp.html
[mod_machine_debug]: /doxygen/group__mod__machine__debug.html
[mod_machine_switch]: /doxygen/group__mod__machine__switch.html
[mod_machine_file]: /doxygen/group__mod__machine__file.html
[mod_machine_thread]: /doxygen/group__mod__machine__thread.html
[mod_machine_emitter]: /doxygen/group__mod__machine__emitter.html

[tutorial_out_file.conf]: https://github.com/x86-64/frozen/blob/master/examples/tutorial_out_file.conf
[tutorial_in_stdin.conf]: https://github.com/x86-64/frozen/blob/master/examples/tutorial_in_stdin.conf
[tutorial_in_fuse.conf]: https://github.com/x86-64/frozen/blob/master/examples/tutorial_in_fuse.conf
[tutorial_in_emitter.conf]: https://github.com/x86-64/frozen/blob/master/examples/tutorial_in_emitter.conf
[tutorial_in_http.conf]: https://github.com/x86-64/frozen/blob/master/examples/tutorial_in_http.conf
[tutorial_io_file.conf]: https://github.com/x86-64/frozen/blob/master/examples/tutorial_io_file.conf
[tutorial_out_debug.conf]: https://github.com/x86-64/frozen/blob/master/examples/tutorial_out_debug.conf
[tutorial_proc_shell.conf]: https://github.com/x86-64/frozen/blob/master/examples/tutorial_proc_shell.conf
[tutorial_proc_shell.conf]: https://github.com/x86-64/frozen/blob/master/examples/tutorial_proc_shell.conf
[tutorial_proc_shell2.conf]: https://github.com/x86-64/frozen/blob/master/examples/tutorial_proc_shell2.conf
[tutorial_proc_shell3.conf]: https://github.com/x86-64/frozen/blob/master/examples/tutorial_proc_shell3.conf

[tutorial_execution]: tutorial_execution.html

