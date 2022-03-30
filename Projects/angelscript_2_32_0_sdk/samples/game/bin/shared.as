// The 'shared' keyword make it possible to send 
// objects of the type between different modules. 
shared class CMessage
{
  CMessage(const string &in t) { txt = t; }
  string txt;
}