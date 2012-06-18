# Tutorial: making NoSQL storage from any data

Frozen can create NoSQL storage on top of any data it can parse. By using special formats and chains of data we can construct
any complex data with indexing and all fancy stuff.

## Simple key-value

Let's start from something very simple, like key-value store with values embedded right into configuration.

	local db = {                                       // define db 
		key1 = "value1", key2 = "value2"
	}

	sub {
		$value1 = void_t:"", $value2 = void_t:"";  // temporary variables
		
		db.lookup(hashkey_t:"key1", $value1);      // get values
		db.lookup(hashkey_t:"key2", $value2);
		
		$value1 | fd_t:"stdout";                   // print them
		$value2 | fd_t:"stdout";
	}

As you can see here, we define "db" as hash with key1 and key2 in it. Next, we lookup this values and print them. Nothing special here, because
any high-level language can do that. 

## Key-value from string

Next one to try is to make data from some external data. First of all, we will try simple string.

	local db = 
		format ~ record_t : "\n"   >               // use record_t as format for data
		"value1\nvalue2\n"
	
	sub {
		$value1 = void_t:"", $value2 = void_t:"";
		
		db.lookup(0, $value1);
		db.lookup(7, $value2);
		
		$value1 | fd_t:"stdout";
		$value2 | fd_t:"stdout";
	}

Here we have string, with "record_t" data on top of it and this defined as "format". That means that any data that pass thought format
converted to template defined. In this case to "record_t". Record_t represent one line in this case, because it looks for "\n".
Notice that we use numerics to query our "database", instead of hashkey_t as in previous example. Also notice that this numeric represent offset 
in target string, not line number as could one think about it at first sight.

To remove this numeric offsets and query data by line number we need some help. Lets use "key ~" to store data offsets.

	local db = 
		key    ~ list_t : {               // use list_t as key mapper
			0, 7
		}                          >
		format ~ record_t : "\n"   >
		"value1\nvalue2\n"

	sub {
		$value1 = void_t:"", $value2 = void_t:"";
		
		db.lookup(0, $value1);
		db.lookup(1, $value2);
		
		$value1 | fd_t:"stdout";
		$value2 | fd_t:"stdout";
	}

As you can see we moved our offsets inside database definition, and now our client know only what it have two elements with id=0 and id=1.
But what if we have a lot of such lines and don't want to write offsets manually. We have to enumerate all items and add them to our list
with offsets.

	local db = 
		key    ~ list_t : { }      >         // empty list here, we will fill it later
		format ~ record_t : "\n"   >
		"value1\nvalue2\n"

	sub {
		db[!data data] | sub {       // enumerate all items in format ~ record_t
			db[!data slave].create(void_t:"", $key);   // add { void => $key } to list_t
		};
	}


	sub {
		$value1 = void_t:"", $value2 = void_t:"";
		
		db.lookup(0, $value1);
		db.lookup(1, $value2);
		
		$value1 | fd_t:"stdout";
		$value2 | fd_t:"stdout";
	}

Finally, we have useful configuration which take string and make key-value storage from it. We can query it, enumerate items, add or delete items.

## Key-value storage from string with nested data structure

Querying lines is not so useful, let's do it with more complex data. For example i have lines with data, but each line have fields of certain format. To make storage from it we need to define two formats:

	local db = 
		key    ~ list_t : { }                   >
		format ~ container_t : {                          // format for each line data
			netstring_t : {} > raw_t : {},
			raw_t : {}
		}                                       >
		format ~ record_t : "\n"                >         // format for lines
		"3:abc,hello abc\n3:def,hello def\n3:ghk,hello ghk\n"
		
	sub {
		db[!data data] | sub { db[!data slave].create(void_t:"", $key); };
	}
	
	sub {
		$value1 = void_t : "", $value2 = void_t : "";
		
		db.lookup(0, $value1);
		db.lookup(1, $value2);
		
		// note container_t to construct output.
		container_t:{ "value1 = ", $value1[1], "\n" } | fd_t : "stdout";
		container_t:{ "value2 = ", $value2[1], "\n" } | fd_t : "stdout";
	}

Output for this program would be:

	value1 = hello abc
	value2 = hello def


## Key-value from files

If we need to make storage from some file we just change string definition with file definition:

	local db = 
		format ~ record_t : "\n"   >
		file_t : "examples/article_complexdata_06.dat"

	sub {
		$value1 = void_t:"", $value2 = void_t:"";
		
		db.lookup(0, $value1);
		db.lookup(7, $value2);
		
		$value1 | fd_t:"stdout";
		$value2 | fd_t:"stdout";
	}

And "examples/article_complexdata_06.dat" is just:
	
	value1
	value2


## Customization

As you can see everything fits to everything, so even in complex cases we can pick any storage (string or file or database).
Good improvements for big datasets would be nice key mapper, for example judy array, or we can use
any other kv storage to keep offsets, for example, redis or embedded leveldb. All of this is done and works very smooth.

## Future plans

We need to make binding to other languages, not only perl. We need replications modules, key hashing and so on. With this modules we can use
any database or storage engine with whole set of features, despite of their support in target database.

