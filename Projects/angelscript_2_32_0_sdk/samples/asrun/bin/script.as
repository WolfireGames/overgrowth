/*

  Script must have 'int main()' or 'void main()' as the entry point.

  Some functions that are available:

   void           print(const string &in str);
   array<string> @getCommandLineArgs();
   void           createCoRoutine(coroutine @func, dictionary @args);
   void           yield();

  Some objects that are available:

   funcdef void coroutine(dictionary@)
   string
   array<T>
   dictionary
   file
   filesystem

*/

string g_str = getDefaultString();

enum E
{
  VALUE1 = 20,
  VALUE2 = 30
}

class Test
{
  int a;
  int b;
  string c;
  void method()
  {
    print("In Test::method()\n");
  }
}

int main()
{
  E val = E(100);
  array<string> @args = getCommandLineArgs();

  print("Received the following args : " + join(args, "|") + "\n");

  Test t;
  Test @ht = t;
  t.method();
  
  array<int> a;
  array<int> @ha;
  
  function();

  // Garbage collection is automatic
  // Set up a circular reference to prove this
  {
    Link @link = Link();
    @link.next = link;
  }
  
  // Use a co-routine to fill an array with objects
  array<Link@> links;
  createCoRoutine(fillArray, dictionary = {{'my array', @links}});
  for( int n = 0; n < 10; n++ )
  {
    print("The size of the array is currently " + links.length() + "\n");
    yield();
  }
  
  print("Press enter to exit\n");
  getInput();

  return 0;
}

void function()
{
  print("Currently in a different function\n");

  int n = 0;
  {
    int n = 1; // This will warn that it is hiding the above variable of the same name
    string s = "hello";
    print(s + "\n");
  }
  {
    int n = 2;
  }
}

// A co-routine
void fillArray(dictionary @dict)
{
  array<Link@> @links = cast<array<Link@>>(dict['my array']);
  for( int n = 0; n < 50; n++ )
  {
    links.insertLast(Link());
    if( n % 10 == 9 )
      yield();
  }
}

string getDefaultString()
{
  return "default";
}

class Link
{
  Link @next;
}

class PrintOnDestruct
{
  ~PrintOnDestruct()
  {
    print("I'm being destroyed now\n");
  }
}

// This object will only be destroyed after the 
// script has finished execution. 
PrintOnDestruct pod;
