# How it works?

To process some data, we need to construct processing pipeline, like in real industries. And like in real factory we need special blocks
called "machine". Machines connected to each other and form production line or "machine-shop". On picture below there is
examples of two such shops: left one consist of "machine 1,2,3", right one have "machine 4,5,6,7". Left one is simple -
it pass request sequentially from machine 1 to 2, and next to 3. Right one pass same request to machine 5,6,7 from machine 4 (it act as duplicator).

There is many kinds of machines, but almost all of them used to only transform or mark data. To do useful things we need to get some data or store it in our stores.
So processing is data-centric and this is all about data. We transfer data from user, modify it and send to another store, or back to user.

![Machine graph](dot_inline_dotgraph_1.png)

Almost every machine require some configuration parameters. You can supply them in configuration file. More about configuration parameters
here: [Tutorial: configuration][tutorial_configuration]

Next tutorial: [Tutorial: configuration][tutorial_configuration]

[tutorial_configuration]: tutorial_configuration.html
