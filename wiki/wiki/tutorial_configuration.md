# Tutorial: configuration
## Shop configuration

To describe shop you want to build you should write configuration file. Configuration file syntax is simple and looks like JSON with
some modifications.

	{ <machine 1 configuration> },
	{ <machine 2 configuration> },
	{ <machine 3 configuration> }

	{
	  <machine 4 configuration>
	  duplicate_to = {                             // this is configuration parameter name for imaginary duplicator machine
	    { <machine 5 configuration> },
	    { <machine 6 configuration> },
	    { <machine 7 configuration> },
	  }
	}

This example configuration describe shops showed on picture in [Tutorial: how it works?][tutorial_basics].

## Machine configuration

To configure single machine with required parameters you write all of them inside {} of machine configuration splitting by comma.

	{
	   name      = "some machine name",                     // single line comment
	   stringpar = "some string value",
	   intpar    = (uint_t)"12345",
	   otherpar  = (sometype_t)"sometype configuration string",
	   morepar   = (sometype_t){
		  value  = "more parameters",
		  length = (uint_t)"100",
		  nestedpar = (anothertype_t){
		       size = (uint_t)"100"
		  }
	   },
	   envpar    = (env_t)"somekey"
	}

Here we have every possible parameter that can be passed to machine. There is strings, ints, special types, nested types:

- Already known name parameter is used by core to maintain global names
- String parameter, can have "\t\r\n" inside string.
- Integer parameter, here we have [uint_t][], it is current platform maximum integer size (4 bytes for 32-bits, 8 for 64-bits). You can specify [uint8_t][], [int16_t][] and another variations.
- Some sometype_t data, which can be initialized from string. Examples: [format_t][], [hashkey_t][]
- Some complex sometype_t data, which can be initialized from hash. Example: [raw_t][].
- Environment data [env_t][]. It use current request key "somekey" to obtain data. Not every machine parameter can be used in such way. Machine must use this parameter during request and not in initialization stage.
- You can use "=", "=>", ":" as assignment.
- Double or single quoted strings, @@ or ## strings.
- C-style comments, both single and multi-line.

## Macros and m4

Because of simple nature of machines, writing complex systems become very hard. Too many machine need to shop together to achieve some
feature. So, we need some macros. And very powerful thing in this field is m4. It is macro processor with simple syntax, easy to learn and
easy to solve our problems. Refer to m4 manual to learn it.

It have includes, dynamic macro assignment, string manipulation and nice libraries.

Prev tutorial: [Tutorial: how it works?][tutorial_basics]

Next tutorial: [Tutorial: execution][tutorial_execution]

[uint_t]: /doxygen/uint__t_8h_source.html
[uint8_t]: /doxygen/uint8__t_8h_source.html
[int16_t]: /doxygen/uint16__t_8h_source.html
[format_t]: /doxygen/format__t_8h_source.html
[hashkey_t]: /doxygen/hashkey__t_8h_source.html
[raw_t]: /doxygen/raw__t_8h_source.html
[env_t]: /doxygen/env__t_8h_source.html
[tutorial_basics]: tutorial_basics.html
[tutorial_execution]: tutorial_execution.html

