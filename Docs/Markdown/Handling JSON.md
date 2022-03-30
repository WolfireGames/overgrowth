---
format: Markdown
...

# Introduction 

I'm (that is Micah) have needed JSON processing as a piece in a larger construction.  Given that it's a great file format and nice for humans to edit and we have an actual JavaScript layer -- so I encourage everybody to use it wherever it would be useful.



# C++ side 

I've included the jsoncpp library (pretty straightforward, not the greatest, but the best I could find without huge dependencies or c++11 requirements ).

The documentation is here: [external](http://open-source-parsers.github.io/jsoncpp-docs/doxygen/index.html)

Just 

    #include "json/jsonhelper.h"

and you're off to races.

# AngelScript side

**Note** I have higher priorities than good documentation at the moment, but in lieu of that I'm including code samples to help you get started till I have time to document my whole set of features.

JSON support is automatically in script instances, nothing new to include.

    // Parse an existing string
    string testString = "{ \"encoding\" : \"UTF-8\", \"plug-ins\" : [ \"python\", \"c++\", \"ruby\" ],\"indent\" : { \"length\" : 3, \"use_space\": true } }";

    JSON jsonRead;   
    if( jsonRead.parseString( testString ) ) {
       Print("Parse ok\n");
    }
    else {
       Print("Parse NOT ok\n");
    }

    // Output the JSON as strings
    Print("Human friendly:\n");
    Print( jsonRead.writeString(true) ); 

    Print("Machine friendly:\n");
    Print( jsonRead.writeString(false) ); 

    // Get some values out and print some information
    JSONValue val = jsonRead.getRoot()[ "encoding" ];
    Print( "Value is " + val.asString() + "\n" );
    Print( "Root type is " + jsonRead.getRoot().typeName() + "\n" );
    Print( "encoding type is " + val.typeName() + "\n" );

    // See if plug-ins is an array 

    if( jsonRead.getRoot()[ "plug-ins" ].isArray() ) {
        Print("plug-ins is an array\n");
    }
    else {
        Print("plug-ins is not an array\n");
    }

    // Now build up a JSON object
    JSON jsonCreate;

    // Create a new string 
    Print( "Create a new string \n" );
    jsonCreate.getRoot()["foo"] = JSONValue( "String" );
    
    // Create a new bool
    Print( "Create a new bool \n" );
    jsonCreate.getRoot()["bar"] = JSONValue( false );

    // Create a new object ('map')
    Print( "Create a new object\n" );
    jsonCreate.getRoot()["baz"] = JSONValue( JSONobjectValue );

    Print("Type of new object: " + jsonCreate.getRoot()["baz"].typeName() + "\n" );

    // Add some new stuff to the created object
    Print( "Access two layers deep\n" );
    jsonCreate.getRoot()["baz"]["first"] = JSONValue( 1 );
    jsonCreate.getRoot()["baz"]["second"] = JSONValue( 2 );
    jsonCreate.getRoot()["baz"]["third"] = JSONValue( 3 );

    // Create an isolated node
    JSONValue jsonArray( JSONarrayValue ); 

    jsonArray.append( JSONValue( "A" ) );
    jsonArray.append( JSONValue( "B" ) );
    jsonArray.append( JSONValue( "C" ) );

    Print( "The array has size: " + jsonArray.size() + "\n" );

    // Now graft it on the main JSON structure
    jsonCreate.getRoot()["boo"] = jsonArray;

    // See if there's an elephant in our creation
    if( jsonCreate.getRoot().isMember( "elephant" ) ) {
        Print("elephant is a member\n");
    }
    else {
        Print("elephant is not a member\n");
    }

    // See what members we do have 
    array<string> members = jsonCreate.getRoot().getMemberNames();

    Print("Members of the root object are: ");
    for( uint i = 0; i < members.length(); ++i ) {
        Print( members[i] + " " );
    }
    Print("\n");

    // See what we have wrought
    Print("Human friendly:\n");
    Print( jsonCreate.writeString(true) );

    // Remove some stuff 
    jsonCreate.getRoot().removeMember("bar");
    jsonCreate.getRoot()["boo"].removeIndex(2);

    // See the difference
    Print("After changes (machine friendly):\n");
    Print( jsonCreate.writeString(false) );

Which would give the output:

    Parse ok
    Human friendly:
    {
       "encoding" : "UTF-8",
       "indent" : {
          "length" : 3,
          "use_space" : true
       },
       "plug-ins" : [ "python", "c++", "ruby" ]
    }
    Machine friendly:
    {"encoding":"UTF-8","indent":{"length":3,"use_space":true},"plug-ins":["python","c++","ruby"]}
    Value is UTF-8
    Root type is object
    encoding type is string
    plug-ins is an array
    Create a new string 
    Create a new bool 
    Create a new object
    Type of new object: object
    Access two layers deep
    The array has size: 3
    elephant is not a member
    Members of the root object are: bar baz boo foo 
    Human friendly:
    {
       "bar" : true,
       "baz" : {
          "first" : 1,
          "second" : 2,
          "third" : 3
       },
       "boo" : [ "A", "B", "C" ],
       "foo" : "String"
    }
    After changes (machine friendly):
    {"baz":{"first":1,"second":2,"third":3},"boo":["A","B"],"foo":"String"}

