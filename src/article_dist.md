Title: Building distributed data processing pipeline
Date: 21-02-2012

Then we try to process huge amount of data it nearly always results in building complex and hardly maintainable
systems with all sort of tools in it: starting with split + rsync files between computers and ending with hardcore scripts on
all possible languages. Recently some problems were solved, including appearance of ZeroMQ which really helps. But you still can not
jump on to problem and solve it quickly, it always require to spend some time to prepare, build blocks and only then solve actual problems.
Frozen try to fill this gap and provide these blocks. Lets see how we can build something distributed.

## Goal
Lets took something simple and move to more complex things. Assume we have huge file with data represented as lines in it. We want to
split it in several groups and for each group we have own special handler.

## Step 1: read data

To read data we use following config:
[article_dist_source.m4][]

We just read data from file, next we split it in lines and send using zeromq to workers pool. We do not bother with doing it fast right here
because we do not need to. If you have huge data, you possibly have huge cluster to process it, and very likely you have distributed fs on
it. Every cluster configuration have it's own advantages and disadvantages. Maybe you have several data storage servers and they are very good
at reading speed, but maybe you have common hardware with low read\write speed on hdd. In any case you should consider best approach for your
cluster and use it. We can start as many readers with as many files as we want, because zeromq allow us to join this flows to be merged into
one.

## Step 2: categorize data

At first, we create dummy worker, it only print our data and we ensure that everything is ok.
[article_dist_worker.m4][]

Next step is to decide - which data where to go. Lets to in straightforward for now with [mod_machine_regexp.][]
[article_dist_worker_reg.m4][]

Here we grep data with "dhcp" in it and send to another process. See both [mod_machine_regexp][], 
[mod_machine_switch][] for more complex examples.

## Step 3: save results

After processing we want to save results, lets save it to simple file.
[article_dist_destination.m4][]

## Testing

As you can notice, all zeromq adresses is local, so we definitely need to start it on local machine, but it will work on cluster
when you change addresses to correct one.

	$ frozend -c examples/article_dist_destination.m4 


	$ frozend -c examples/article_dist_worker_reg.m4


	$ frozend -c examples/article_dist_source.m4 

Then source finish reading, we could see in worker console grepped data and destination worker wrote data to file too.

## Possible improvements

That was easy, but not so useful. Here how we can improve things:

<ul>
<li> Problem: hardcoded addresses. Solutions:
<ul>
<li>As you can notice config file is m4 script, you can make include file with addresses, or join all files into one and use
ifdef and roles supplied from command line</li>
<li>You can pass zeromq socket to remote machine. And remote machine use this socket to return data, or send it to another stage</li>
</ul>
</li>
<li>
Problem: hardcoded categorization and complex config file. Solutions:
<ul>
<li>It is still m4 file - use define() to describe common categorization rule, and just put MY_RULE(`dhcp', `tcp://somehost:12345') as many
as you need.</li>
<li>You can pass parameters and rules for worker at same time with data. Don't know there it can be used, but you can.</li>
</ul>
</li>
<li>Problem: not flexible processing. Solutions: if you have some very complex processing requirements, write module by yourself -
it is just zeromq messages with clean data flowing around. Pick your favorite language and process as you like, return to frozen if need.</li>
</ul>

[mod_machine_switch]: /doxygen/group__mod__machine__switch.html
[mod_machine_regexp]: /doxygen/group__mod__machine__regexp.html

[article_dist_worker.m4]: https://github.com/x86-64/frozen/blob/master/examples/article_dist_worker.m4
[article_dist_worker_reg.m4]: https://github.com/x86-64/frozen/blob/master/examples/article_dist_worker_reg.m4
[article_dist_source.m4]: https://github.com/x86-64/frozen/blob/master/examples/article_dist_source.m4
[article_dist_destination.m4]: https://github.com/x86-64/frozen/blob/master/examples/article_dist_destination.m4


